// library.cpp
#define _CRT_SECURE_NO_WARNINGS
#include <bits/stdc++.h>
#include <optional>
using namespace std;

#ifdef _WIN32
#define CLEAR_CMD "cls"
#else
#define CLEAR_CMD "clear"
#endif
// ==================== STRUCTS ====================
struct Book {
    int bookID = 0;
    string title;
    string author;
    string isbn;
    bool isAvailable = true;

    string serialize() const {
        // pipe-delimited: id|title|author|isbn|isAvailable
        // replace any '\n' from strings (shouldn't normally happen)
        auto esc = [](const string& s) {
            string r = s;
            // remove newline chars to keep one-line format
            r.erase(remove(r.begin(), r.end(), '\n'), r.end());
            r.erase(remove(r.begin(), r.end(), '\r'), r.end());
            return r;
            };
        return to_string(bookID) + "|" + esc(title) + "|" + esc(author) + "|" + esc(isbn) + "|" + (isAvailable ? "1" : "0");
    }

    static optional<Book> deserialize(const string& line) {
        Book b;
        vector<string> f;
        string cur;
        for (char c : line) {
            if (c == '|') { f.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        f.push_back(cur);
        if (f.size() != 5) return {};
        try {
            b.bookID = stoi(f[0]);
            b.title = f[1];
            b.author = f[2];
            b.isbn = f[3];
            b.isAvailable = (f[4] == "1");
            return b;
        }
        catch (...) { return {}; }
    }
};

struct Loan {
    int loanID = 0;
    int bookID = 0;
    string username;
    time_t loanDate = 0;
    time_t dueDate = 0;
    time_t returnDate = 0;
    bool isReturned = false;
    double overdueAmount = 0.0;

    string serialize() const {
        // one-line: loanID|bookID|username|loanDate|dueDate|returnDate|isReturned|overdueAmount
        auto esc = [](const string& s) {
            string r = s;
            r.erase(remove(r.begin(), r.end(), '\n'), r.end());
            r.erase(remove(r.begin(), r.end(), '\r'), r.end());
            return r;
            };
        return to_string(loanID) + "|" + to_string(bookID) + "|" + esc(username) + "|" +
            to_string((long long)loanDate) + "|" + to_string((long long)dueDate) + "|" +
            to_string((long long)returnDate) + "|" + (isReturned ? "1" : "0") + "|" +
            to_string(overdueAmount);
    }

    static optional<Loan> deserialize(const string& line) {
        Loan L;
        vector<string> f;
        string cur;
        for (char c : line) {
            if (c == '|') { f.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        f.push_back(cur);
        if (f.size() != 8) return {};
        try {
            L.loanID = stoi(f[0]);
            L.bookID = stoi(f[1]);
            L.username = f[2];
            L.loanDate = (time_t)stoll(f[3]);
            L.dueDate = (time_t)stoll(f[4]);
            L.returnDate = (time_t)stoll(f[5]);
            L.isReturned = (f[6] == "1");
            L.overdueAmount = stod(f[7]);
            return L;
        }
        catch (...) { return {}; }
    }
};

struct User {
    string username;
    string password;
    int activeLoans = 0;

    string serialize() const {
        // username|password|activeLoans
        auto esc = [](const string& s) {
            string r = s;
            r.erase(remove(r.begin(), r.end(), '\n'), r.end());
            r.erase(remove(r.begin(), r.end(), '\r'), r.end());
            return r;
            };
        return esc(username) + "|" + esc(password) + "|" + to_string(activeLoans);
    }

    static optional<User> deserialize(const string& line) {
        User U;
        vector<string> f;
        string cur;
        for (char c : line) {
            if (c == '|') { f.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        f.push_back(cur);
        if (f.size() != 3) return {};
        try {
            U.username = f[0];
            U.password = f[1];
            U.activeLoans = stoi(f[2]);
            return U;
        }
        catch (...) { return {}; }
    }
};

// ==================== GLOBALS ====================
const int MAX_LOAN_LIMIT = 5;
const int LOAN_PERIOD_DAYS = 14;

vector<Book> books;
vector<Loan> loans;
vector<User> users;

int nextBookID = 1;
int nextLoanID = 1;
string currentUser = "";

// file names
const string BOOKS_FILE = "books.txt";
const string LOANS_FILE = "loans.txt";
const string USERS_FILE = "users.txt";
const string META_FILE = "meta.txt"; // store next IDs here

// ==================== UTIL ====================

void clearScreen() {
    system(CLEAR_CMD);
}

// Trim whitespace
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Read non-empty line
string inputLine(const string& prompt) {
    string s;
    while (true) {
        cout << prompt;
        getline(cin, s);
        s = trim(s);
        if (!s.empty()) return s;
        cout << "Input cannot be empty. Please try again.\n";
    }
}

// Read integer with validation
int inputInt(const string& prompt, int minVal = INT_MIN, int maxVal = INT_MAX) {
    string s;
    int value;
    while (true) {
        cout << prompt;
        getline(cin, s);
        s = trim(s);
        try {
            size_t idx;
            value = stoi(s, &idx);
            if (idx != s.size()) throw invalid_argument("extra");
            if (value < minVal || value > maxVal)
                throw out_of_range("range");
            return value;
        }
        catch (...) {
            cout << "Invalid number. Please enter a valid integer.\n";
        }
    }
}

void pressEnterToContinue() {
    cout << "\nPress Enter to continue...";
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int getUserActiveLoansCount(const string& username) {
    int cnt = 0;
    for (const auto& L : loans) {
        if (L.username == username && !L.isReturned) cnt++;
    }
    return cnt;
}

double calculateOverdueFee(int daysOverdue) {
    if (daysOverdue <= 0) return 0.0;
    if (daysOverdue == 1) return 5.00;
    if (daysOverdue == 2) return 6.00;
    if (daysOverdue == 3) return 7.00;
    return 10.00 * daysOverdue;
}

// ==================== FILE IO ====================
void saveMeta() {
    ofstream of(META_FILE);
    if (!of) return;
    of << nextBookID << " " << nextLoanID << "\n";
}
void loadMeta() {
    ifstream inf(META_FILE);
    if (!inf) return;
    if (!(inf >> nextBookID >> nextLoanID)) {
        nextBookID = 1; nextLoanID = 1;
    }
}

void saveBooks() {
    ofstream of(BOOKS_FILE);
    if (!of) { cerr << "Failed to open " << BOOKS_FILE << " for writing\n"; return; }
    for (const auto& b : books) of << b.serialize() << "\n";
}

void loadBooks() {
    books.clear();
    ifstream inf(BOOKS_FILE);
    if (!inf) return;
    string line;
    while (getline(inf, line)) {
        if (line.empty()) continue;
        auto ob = Book::deserialize(line);
        if (ob) books.push_back(*ob);
    }
    // ensure nextBookID > max existing id
    int maxID = 0;
    for (auto& b : books) if (b.bookID > maxID) maxID = b.bookID;
    if (nextBookID <= maxID) nextBookID = maxID + 1;
}

void saveLoans() {
    ofstream of(LOANS_FILE);
    if (!of) { cerr << "Failed to open " << LOANS_FILE << " for writing\n"; return; }
    for (const auto& l : loans) of << l.serialize() << "\n";
}

void loadLoans() {
    loans.clear();
    ifstream inf(LOANS_FILE);
    if (!inf) return;
    string line;
    while (getline(inf, line)) {
        if (line.empty()) continue;
        auto ol = Loan::deserialize(line);
        if (ol) loans.push_back(*ol);
    }
    int maxID = 0;
    for (auto& l : loans) if (l.loanID > maxID) maxID = l.loanID;
    if (nextLoanID <= maxID) nextLoanID = maxID + 1;
}

void saveUsers() {
    ofstream of(USERS_FILE);
    if (!of) { cerr << "Failed to open " << USERS_FILE << " for writing\n"; return; }
    for (const auto& u : users) of << u.serialize() << "\n";
}

void loadUsers() {
    users.clear();
    ifstream inf(USERS_FILE);
    if (!inf) {
        // if no users file, create default admin
        User admin;
        admin.username = "admin";
        admin.password = "admin123";
        admin.activeLoans = 0;
        users.push_back(admin);
        saveUsers();
        return;
    }
    string line;
    while (getline(inf, line)) {
        if (line.empty()) continue;
        auto ou = User::deserialize(line);
        if (ou) users.push_back(*ou);
    }
    if (users.empty()) {
        User admin;
        admin.username = "admin";
        admin.password = "admin123";
        admin.activeLoans = 0;
        users.push_back(admin);
        saveUsers();
    }
}

void persistAll() {
    saveBooks();
    saveLoans();
    saveUsers();
    saveMeta();
}

// ==================== AUTH ====================
optional<int> findUserIndex(const string& username) {
    for (size_t i = 0; i < users.size(); ++i)
        if (users[i].username == username) return (int)i;
    return {};
}

bool loginUser() {

    clearScreen();

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "========================================\n";
    cout << "    BKCL LOGIN SYSTEM\n";
    cout << "========================================\n";

    string username = inputLine("Username: ");
    string password = inputLine("Password: ");

    for (const auto& u : users) {
        if (u.username == username && u.password == password) {
            currentUser = username;
            cout << "\nLogin successful! Welcome, " << username << "!\n";
            pressEnterToContinue();
            return true;
        }
    }

    cout << "\nInvalid username or password!\n";
    pressEnterToContinue();
    return false;
}


void registerUser() {
    clearScreen();
    cout << "========================================\n";
    cout << "    USER REGISTRATION\n";
    cout << "========================================\n";

    string username = inputLine("Enter new username: ");

    if (findUserIndex(username)) {
        cout << "\nUsername already exists!\n";
        pressEnterToContinue();
        return;
    }

    string password = inputLine("Enter password: ");

    users.push_back({ username, password, 0 });
    saveUsers();

    cout << "\nRegistration successful! You can now login.\n";
    pressEnterToContinue();
}


// ==================== BOOK CATALOGUE ====================
void addBook() {
    clearScreen();
    cout << "========================================\n";
    cout << "    ADD NEW BOOK\n";
    cout << "========================================\n";

    Book b;
    b.bookID = nextBookID++;

    cout << "Book ID (auto): " << b.bookID << "\n";
    b.title = inputLine("Enter book title: ");
    b.author = inputLine("Enter author name: ");
    b.isbn = inputLine("Enter ISBN: ");
    b.isAvailable = true;

    books.push_back(b);
    saveBooks();
    saveMeta();

    cout << "\nBook added successfully!\n";
    pressEnterToContinue();
}


void viewBooks() {
    clearScreen();
    cout << "========================================\n";
    cout << "    BOOK CATALOGUE\n";
    cout << "========================================\n";
    if (books.empty()) {
        cout << "No books in the catalogue.\n";
    }
    else {
        cout << left << setw(5) << "ID" << setw(30) << "Title"
            << setw(20) << "Author" << setw(15) << "ISBN"
            << setw(10) << "Status" << "\n";
        cout << string(80, '-') << "\n";
        for (const auto& b : books) {
            string title = b.title;
            if ((int)title.size() > 28) title = title.substr(0, 28);
            string author = b.author;
            if ((int)author.size() > 18) author = author.substr(0, 18);
            cout << left << setw(5) << b.bookID
                << setw(30) << title
                << setw(20) << author
                << setw(15) << b.isbn
                << setw(10) << (b.isAvailable ? "Available" : "Loaned")
                << "\n";
        }
    }
    pressEnterToContinue();
}

void searchBook() {
    clearScreen();
    cout << "========================================\n";
    cout << "    SEARCH BOOK\n";
    cout << "========================================\n";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Enter search keyword (title or author): ";
    string kw;
    getline(cin, kw);
    if (kw.empty()) { cout << "No keyword entered.\n"; pressEnterToContinue(); return; }
    bool found = false;
    cout << "\nSearch Results:\n";
    cout << left << setw(5) << "ID" << setw(30) << "Title"
        << setw(20) << "Author" << setw(15) << "ISBN"
        << setw(10) << "Status" << "\n";
    cout << string(80, '-') << "\n";
    for (const auto& b : books) {
        if (b.title.find(kw) != string::npos || b.author.find(kw) != string::npos) {
            string t = b.title;
            if ((int)t.size() > 28) t = t.substr(0, 28);
            string a = b.author;
            if ((int)a.size() > 18) a = a.substr(0, 18);
            cout << left << setw(5) << b.bookID
                << setw(30) << t
                << setw(20) << a
                << setw(15) << b.isbn
                << setw(10) << (b.isAvailable ? "Available" : "Loaned")
                << "\n";
            found = true;
        }
    }
    if (!found) cout << "No books found matching the keyword.\n";
    pressEnterToContinue();
}

void editBook() {
    clearScreen();
    cout << "========================================\n";
    cout << "    EDIT BOOK INFORMATION\n";
    cout << "========================================\n";
    cout << "Enter Book ID to edit: ";
    int id;
    if (!(cin >> id)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid input\n"; pressEnterToContinue(); return; }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    int idx = -1;
    for (size_t i = 0; i < books.size(); ++i) if (books[i].bookID == id) { idx = (int)i; break; }
    if (idx == -1) { cout << "Book not found!\n"; pressEnterToContinue(); return; }
    cout << "\nCurrent Title: " << books[idx].title << "\nNew Title (or press Enter to keep): ";
    string s;
    getline(cin, s);
    if (!s.empty()) books[idx].title = s;
    cout << "Current Author: " << books[idx].author << "\nNew Author (or press Enter to keep): ";
    getline(cin, s);
    if (!s.empty()) books[idx].author = s;
    cout << "Current ISBN: " << books[idx].isbn << "\nNew ISBN (or press Enter to keep): ";
    getline(cin, s);
    if (!s.empty()) books[idx].isbn = s;
    saveBooks();
    cout << "\nBook information updated successfully!\n";
    pressEnterToContinue();
}

void deleteBook() {
    clearScreen();
    cout << "========================================\n";
    cout << "    DELETE BOOK\n";
    cout << "========================================\n";
    cout << "Enter Book ID to delete: ";
    int id;
    if (!(cin >> id)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid\n"; pressEnterToContinue(); return; }
    int idx = -1;
    for (size_t i = 0; i < books.size(); ++i) if (books[i].bookID == id) { idx = (int)i; break; }
    if (idx == -1) { cout << "Book not found!\n"; pressEnterToContinue(); return; }
    if (!books[idx].isAvailable) {
        cout << "Cannot delete book that is currently loaned!\n";
        pressEnterToContinue(); return;
    }
    books.erase(books.begin() + idx);
    saveBooks();
    cout << "Book deleted successfully!\n";
    pressEnterToContinue();
}

// ==================== LOANS ====================
void loanBook() {
    if (currentUser.empty()) { cout << "No user logged in\n"; pressEnterToContinue(); return; }
    clearScreen();
    cout << "========================================\n";
    cout << "    LOAN BOOK\n";
    cout << "========================================\n";
    int active = getUserActiveLoansCount(currentUser);
    cout << "Your active loans: " << active << "/" << MAX_LOAN_LIMIT << "\n";
    if (active >= MAX_LOAN_LIMIT) { cout << "You have reached the maximum loan limit!\n"; pressEnterToContinue(); return; }

    // check outstanding overdue amounts for this user
    for (const auto& l : loans) {
        if (l.username == currentUser && l.overdueAmount > 0.0) {
            cout << "You have outstanding overdue payments. Clear them before borrowing more.\n";
            pressEnterToContinue();
            return;
        }
    }

    cout << "Enter Book ID to loan: ";
    int id;
    if (!(cin >> id)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid\n"; pressEnterToContinue(); return; }
    int idx = -1;
    for (size_t i = 0; i < books.size(); ++i) if (books[i].bookID == id) { idx = (int)i; break; }
    if (idx == -1) { cout << "Book not found!\n"; pressEnterToContinue(); return; }
    if (!books[idx].isAvailable) { cout << "This book is currently loaned out!\n"; pressEnterToContinue(); return; }

    Loan L;
    L.loanID = nextLoanID++;
    L.bookID = id;
    L.username = currentUser;
    L.loanDate = time(nullptr);
    L.dueDate = L.loanDate + (time_t)LOAN_PERIOD_DAYS * 24 * 60 * 60;
    L.returnDate = 0;
    L.isReturned = false;
    L.overdueAmount = 0.0;
    loans.push_back(L);

    // mark book not available
    books[idx].isAvailable = false;

    // update user's activeLoans counter (persistent in user record too)
    auto optidx = findUserIndex(currentUser);
    if (optidx) { users[*optidx].activeLoans = getUserActiveLoansCount(currentUser); saveUsers(); }

    saveBooks();
    saveLoans();
    saveMeta();

    cout << "\nBook loaned successfully!\n";
    cout << "Due date: " << LOAN_PERIOD_DAYS << " days from now\n";
    pressEnterToContinue();
}

void returnBook() {
    if (currentUser.empty()) { cout << "No user logged in\n"; pressEnterToContinue(); return; }
    clearScreen();
    cout << "========================================\n";
    cout << "    RETURN BOOK\n";
    cout << "========================================\n";

    cout << "Your active loans:\n";
    cout << left << setw(8) << "Loan ID" << setw(30) << "Book Title" << setw(15) << "Due Date" << "\n";
    cout << string(60, '-') << "\n";

    bool hasLoans = false;
    for (const auto& l : loans) {
        if (l.username == currentUser && !l.isReturned) {
            string bookTitle = "Unknown";
            for (const auto& b : books)
                if (b.bookID == l.bookID)
                    bookTitle = b.title;

            char duebuff[20];
            tm dt{};

            // Safe version
            if (localtime_s(&dt, &l.dueDate) == 0) {
                strftime(duebuff, sizeof(duebuff), "%Y-%m-%d", &dt);
            }
            else {
                strcpy_s(duebuff, sizeof(duebuff), "N/A");
            }

            cout << left << setw(8) << l.loanID
                << setw(30) << bookTitle.substr(0, 28)
                << setw(15) << duebuff << "\n";

            hasLoans = true;
        }
    }

    if (!hasLoans) { cout << "You have no active loans.\n"; pressEnterToContinue(); return; }

    cout << "\nEnter Loan ID to return: ";
    int lid;
    if (!(cin >> lid)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid\n"; pressEnterToContinue(); return; }
    int idx = -1;
    for (size_t i = 0; i < loans.size(); ++i) if (loans[i].loanID == lid && loans[i].username == currentUser && !loans[i].isReturned) { idx = (int)i; break; }
    if (idx == -1) { cout << "Loan not found or already returned!\n"; pressEnterToContinue(); return; }

    time_t currentTime = time(nullptr);
    loans[idx].returnDate = currentTime;
    loans[idx].isReturned = true;

    int daysOverdue = (int)difftime(currentTime, loans[idx].dueDate) / (24 * 60 * 60);
    if (daysOverdue < 0) daysOverdue = 0;
    if (daysOverdue > 0) {
        loans[idx].overdueAmount = calculateOverdueFee(daysOverdue);
        cout << "\nBook is " << daysOverdue << " day(s) overdue!\n";
        cout << "Overdue fee: RM " << fixed << setprecision(2) << loans[idx].overdueAmount << "\n";
    }
    else {
        cout << "\nBook returned on time!\n";
    }

    // mark book available
    for (auto& b : books) if (b.bookID == loans[idx].bookID) { b.isAvailable = true; break; }

    // update user activeLoans
    auto optidx = findUserIndex(currentUser);
    if (optidx) { users[*optidx].activeLoans = getUserActiveLoansCount(currentUser); saveUsers(); }

    saveBooks();
    saveLoans();
    cout << "Book returned successfully!\n";
    pressEnterToContinue();
}

void viewOverduePayments() {
    if (currentUser.empty()) { cout << "No user logged in\n"; pressEnterToContinue(); return; }
    clearScreen();
    cout << "========================================\n";
    cout << "    OVERDUE PAYMENTS\n";
    cout << "========================================\n";

    cout << left << setw(8) << "Loan ID" << setw(30) << "Book Title" << setw(15) << "Amount (RM)" << "\n";
    cout << string(60, '-') << "\n";
    bool hasOverdues = false;
    double total = 0.0;
    for (const auto& l : loans) {
        if (l.username == currentUser && l.overdueAmount > 0.0) {
            string title = "Unknown";
            for (const auto& b : books) if (b.bookID == l.bookID) { title = b.title; break; }
            string t = title;
            if ((int)t.size() > 28) t = t.substr(0, 28);
            cout << left << setw(8) << l.loanID << setw(30) << t << setw(15) << fixed << setprecision(2) << l.overdueAmount << "\n";
            total += l.overdueAmount;
            hasOverdues = true;
        }
    }
    if (!hasOverdues) cout << "You have no overdue payments.\n";
    else {
        cout << string(60, '-') << "\n";
        cout << "Total Overdue: RM " << fixed << setprecision(2) << total << "\n";
    }
    pressEnterToContinue();
}

void payOverdue() {
    if (currentUser.empty()) { cout << "No user logged in\n"; pressEnterToContinue(); return; }
    clearScreen();
    cout << "========================================\n";
    cout << "    PAY OVERDUE FEES\n";
    cout << "========================================\n";

    bool has = false;
    for (const auto& l : loans) if (l.username == currentUser && l.overdueAmount > 0) { has = true; break; }
    if (!has) { cout << "You have no overdue payments.\n"; pressEnterToContinue(); return; }

    cout << "Enter Loan ID to pay: ";
    int lid;
    if (!(cin >> lid)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid\n"; pressEnterToContinue(); return; }
    int idx = -1;
    for (size_t i = 0; i < loans.size(); ++i) if (loans[i].loanID == lid && loans[i].username == currentUser && loans[i].overdueAmount > 0.0) { idx = (int)i; break; }
    if (idx == -1) { cout << "Loan not found or no overdue amount!\n"; pressEnterToContinue(); return; }
    cout << "Amount to pay: RM " << fixed << setprecision(2) << loans[idx].overdueAmount << "\n";
    cout << "Confirm payment? (1=Yes, 0=No): ";
    int conf; if (!(cin >> conf)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid\n"; pressEnterToContinue(); return; }
    if (conf == 1) {
        loans[idx].overdueAmount = 0.0;
        saveLoans();
        cout << "Payment successful!\n";
    }
    else cout << "Payment cancelled.\n";
    pressEnterToContinue();
}

// ==================== MENUS ====================
void bookCatalogueMenu() {
    int choice;
    do {
        clearScreen();
        cout << "========================================\n";
        cout << "    BOOK CATALOGUE MANAGEMENT\n";
        cout << "========================================\n";

        cout << "1. Add New Book\n";
        cout << "2. View All Books\n";
        cout << "3. Search Book\n";
        cout << "4. Edit Book Information\n";
        cout << "5. Delete Book\n";
        cout << "0. Back to Main Menu\n";

        choice = inputInt("Enter your choice: ", 0, 5);

        switch (choice) {
        case 1: addBook(); break;
        case 2: viewBooks(); break;
        case 3: searchBook(); break;
        case 4: editBook(); break;
        case 5: deleteBook(); break;
        }
    } while (choice != 0);
}


void mainMenu() {
    int choice;
    do {
        clearScreen();
        cout << "========================================\n";
        cout << "  BUKIT KATIL COMMUNITY LIBRARY (BKCL)\n";
        cout << "========================================\n";
        cout << "Logged in as: " << currentUser << "\n";
        cout << "Active Loans: " << getUserActiveLoansCount(currentUser)
            << "/" << MAX_LOAN_LIMIT << "\n\n";

        cout << "1. Book Catalogue Management\n";
        cout << "2. Loan a Book\n";
        cout << "3. Return a Book\n";
        cout << "4. View Overdue Payments\n";
        cout << "5. Pay Overdue Fees\n";
        cout << "0. Logout\n";

        choice = inputInt("Enter your choice: ", 0, 5);

        switch (choice) {
        case 1: bookCatalogueMenu(); break;
        case 2: loanBook(); break;
        case 3: returnBook(); break;
        case 4: viewOverduePayments(); break;
        case 5: payOverdue(); break;
        case 0:
            currentUser.clear();
            cout << "Logged out successfully!\n";
            pressEnterToContinue();
            break;
        }
    } while (!currentUser.empty());
}


// ==================== MAIN ====================
int main() {
    // Load everything
    loadMeta();
    loadBooks();
    loadLoans();
    loadUsers();

    // ensure user activeLoans counters are consistent with loans
    for (auto& u : users) {
        u.activeLoans = getUserActiveLoansCount(u.username);
    }
    saveUsers();

    int choice;
    do {
        clearScreen();
        cout << "========================================\n";
        cout << "  BUKIT KATIL COMMUNITY LIBRARY (BKCL)\n";
        cout << "     Library Book Management System\n";
        cout << "========================================\n";
        cout << "1. Login\n2. Register New User\n0. Exit\n";
        cout << "Enter your choice: ";
        if (!(cin >> choice)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); choice = -1; }
        switch (choice) {
        case 1:
            if (loginUser()) {
                mainMenu();
            }
            break;
        case 2: registerUser(); break;
        case 0:
            cout << "\nThank you for using BKCL System!\n";
            break;
        default:
            cout << "Invalid choice!\n";
            pressEnterToContinue();
        }
    } while (choice != 0);

    persistAll();
    return 0;
}


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
using namespace std;

// These standard library headers give the program the tools it needs:
// - fstream: reading the CSV file from disk
// - sstream: splitting and rebuilding date strings
// - vector: dynamic arrays such as the list of records
// - string: storing text values from the CSV
// - algorithm: max() and other helper routines for tree balancing
// - limits: safe handling of invalid user input
// The using namespace std; line keeps the code shorter and matches the style used in class exercises.

// A Record represents one row from the CSV file.
// Each field is kept as a string because the dataset is text-based, and the program converts
// only the values it actually needs (such as the date and cumulative number) into numeric types.
struct Record {
    string Direction;        // Direction of trade, such as imports or exports
    string Year;             // Year field from the CSV
    string Date;             // Date in dd/mm/yyyy format after normalization
    string Weekday;          // Day of the week from the file
    string Country;          // Country or region name
    string Commodity;        // Type of commodity being traded
    string Transport_Mode;   // Shipping method, road, air, etc.
    string Measure;          // Reporting measure
    string Value;            // Numeric value of the trade amount
    string Cumulative;       // Running cumulative value stored as text
};

// Forward declaration so the date-normalizing helper can be used before its actual definition.
string normalizeDate(const string& date);

// This function reads one raw CSV line and splits it into separate values.
// It handles commas that appear inside quotation marks, which is important because some fields
// may contain text such as "Air freight" or other quoted values.
vector<string> parseCSVLine(const string& line){
    vector<string> result;
    string cell;
    bool inQuotes=false;
    for(char c:line){
        if(c=='"'){
            inQuotes=!inQuotes;
        } else if(c==','&&!inQuotes){
            result.push_back(cell);
            cell.clear();
        } else {
            cell+=c;
        }
    }
    result.push_back(cell);
    return result;
}

// This function converts a CSV row string into a Record object.
// It checks that the row has at least 10 columns before accepting it, because the dataset
// is expected to contain ten important values, and malformed rows are ignored.
bool parseRecord(const string& line, Record& record) {
    vector<string> fields = parseCSVLine(line); // Break the raw text into columns

    if(fields.size() < 10) // If the row is shorter than expected, it cannot be valid
        return false;

    // Store each value in the correct member of the Record.
    record.Direction = fields[0];
    record.Year = fields[1];
    record.Date = normalizeDate(fields[2]);
    record.Weekday = fields[3];
    record.Country = fields[4];
    record.Commodity = fields[5];
    record.Transport_Mode = fields[6];
    record.Measure = fields[7];
    record.Value = fields[8];
    record.Cumulative = fields[9];

    return true; // The row was valid and has been loaded into memory
}

// This helper turns a date like 3/4/2021 into a sortable integer such as 20210403.
// That makes date comparisons much easier than comparing strings directly.
long long dateToNum(const string& date) {
    stringstream ss(date); // Split the date string using the '/' character as a delimiter
    string day, month, year;

    getline(ss, day, '/');
    getline(ss, month, '/');
    getline(ss, year, '/');

    if(day.length() == 1) day = "0" + day;   // Add leading zero to day when needed
    if(month.length() == 1) month = "0" + month; // Add leading zero to month when needed

    try {
        return stoll(year + month + day); // Combine YYYYMMDD into one integer for sorting
    } catch(...) {
        return 0; // If the date is malformed, return 0 instead of crashing
    }
}

// This function ensures dates are in a consistent format, such as 03/04/2021.
// The project wants the dates normalized so every date in the dataset follows the same pattern.
string normalizeDate(const string& date) {
    stringstream ss(date); // Turn the input into a stream so it can be split by '/'
    string day, month, year;

    getline(ss, day, '/');
    getline(ss, month, '/');
    getline(ss, year, '/');

    if(day.empty() || month.empty() || year.empty())
        return date; // If the input is incomplete, leave it unchanged

    if(day.length() == 1) day = "0" + day;     // Convert 3 -> 03
    if(month.length() == 1) month = "0" + month; // Convert 4 -> 04
    if(year.length() == 2) year = "20" + year;   // Convert 21 -> 2021

    return day + "/" + month + "/" + year; // Final normalized date, e.g. 03/04/2021
}

// This function tries to convert the cumulative text value into a long long integer.
// If the conversion fails, it returns 0 rather than throwing an exception.
long long parseCumulative(const string& value) {
    try {
        return stoll(value);
    } catch(...) {
        return 0;
    }
}

// The program loads all valid rows from the CSV into memory.
// Each valid row is stored as a Record, which means we can reuse the same dataset in many different
// structures (BST and hash table) without reading the file again.
vector<Record> loadAllRecords(const string& filename) {
    vector<Record> records; // The final list of parsed records
    ifstream file(filename); // Opens the CSV file for sequential reading

    if(!file.is_open()) { // If the file cannot be opened, inform the user and stop
        cout << "Error: Could not open file '" << filename << "'.\n";
        return records;
    }

    string line; // One line from the CSV file
    getline(file, line); // Skip the header row, because it contains column names not data

    while(getline(file, line)) { // Read every data row after the header
        if(line.empty()) continue; // Ignore blank lines if any are present

        Record record; // Temporary place to store one row
        if(parseRecord(line, record))
            records.push_back(record); // Keep only valid rows in the vector
    }

    file.close(); // Close the file when the reading is done
    return records; // Return the complete dataset
}

// This function repeatedly asks the user for a menu number until the input is valid.
// It clears the input stream after an invalid entry so the program can continue cleanly.
int readMenuChoice() {
    int value; // Stores the number entered by the user
    while(true) {
        if(cin >> value)
            return value; // Return as soon as a valid integer has been read

        cout << "Invalid input. Please enter a number.\n";
        cin.clear(); // Reset the stream error state
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Remove the bad input from the buffer
    }
}

/* ==================== BST BY DATE ==================== */

// This section builds an AVL tree ordered by date.
// An AVL tree is a self-balancing binary search tree, so insertion, search, and deletion remain efficient.
// The key idea is that each node stores one date and the corresponding cumulative value for that date.
// This structure makes "find the cumulative value for a given day" very fast, even with a large dataset.

struct DateNode {
    string date;          // The normalized date in dd/mm/yyyy format
    long long dateNum;    // Same date converted to an integer like YYYYMMDD for easier comparison
    long long cumulative; // The cumulative value stored for that date
    int height;           // AVL height factor for balancing
    DateNode* left;       // Left child, containing earlier dates
    DateNode* right;      // Right child, containing later dates

    DateNode(const string& d, long long c)
        : date(d), dateNum(dateToNum(d)), cumulative(c),
          height(1), left(nullptr), right(nullptr) {}
};

// Returns the height of a node, or 0 if the node is null.
// This is the core information AVL trees use to decide whether a subtree is unbalanced.
int getHeight(DateNode* node) {
    return node ? node->height : 0;
}

// A right rotation fixes a left-heavy subtree.
// This is one of the standard AVL rebalancing operations.
DateNode* rotateRight(DateNode* y) {
    DateNode* x = y->left;     // The left child becomes the new root
    y->left = x->right;        // Move x's right subtree to y's left
    x->right = y;              // Put y on x's right side

    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    return x; // Return the new subtree root
}

// A left rotation fixes a right-heavy subtree.
DateNode* rotateLeft(DateNode* x) {
    DateNode* y = x->right;    // The right child becomes the new root
    x->right = y->left;        // Move y's left subtree to x's right
    y->left = x;               // Put x on y's left side

    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    return y; // Return the new subtree root
}

// Insert a date into the AVL tree.
// If the same date already exists, no duplicate is created.
DateNode* insertDateNode(DateNode* node, const string& date, long long cumulative) {
    if(!node)
        return new DateNode(date, cumulative); // Empty tree: create the node

    long long dateNum = dateToNum(date); // Convert the date to a sortable numeric key

    if(dateNum < node->dateNum)
        node->left = insertDateNode(node->left, date, cumulative);
    else if(dateNum > node->dateNum)
        node->right = insertDateNode(node->right, date, cumulative);
    else
        return node; // Same date already exists, so ignore the duplicate

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getHeight(node->left) - getHeight(node->right);

    // Case 1: left-left imbalance
    if(balance > 1 && dateNum < node->left->dateNum)
        return rotateRight(node);

    // Case 2: right-right imbalance
    if(balance < -1 && dateNum > node->right->dateNum)
        return rotateLeft(node);

    // Case 3: left-right imbalance
    if(balance > 1 && dateNum > node->left->dateNum) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    // Case 4: right-left imbalance
    if(balance < -1 && dateNum < node->right->dateNum) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node; // Tree remains valid without needing a rotation
}

// Search for a date by converting it into a numeric date key and descending through the tree.
DateNode* searchDateNode(DateNode* node, long long dateNum) {
    if(!node || node->dateNum == dateNum)
        return node; // Found it, or the tree is empty

    if(dateNum < node->dateNum)
        return searchDateNode(node->left, dateNum);

    return searchDateNode(node->right, dateNum);
}

// The minimum node in a BST is the leftmost node in the subtree.
DateNode* findMinDateNode(DateNode* node) {
    while(node && node->left)
        node = node->left;

    return node;
}

// Delete a date from the AVL tree and rebalance it afterwards.
DateNode* deleteDateNode(DateNode* node, long long dateNum) {
    if(!node) return nullptr; // Nothing to delete

    if(dateNum < node->dateNum) {
        node->left = deleteDateNode(node->left, dateNum);
    } else if(dateNum > node->dateNum) {
        node->right = deleteDateNode(node->right, dateNum);
    } else {
        // The node has been found. Now handle deleting it in the BST way.
        if(!node->left || !node->right) {
            DateNode* temp = node->left ? node->left : node->right;
            delete node;
            return temp;
        }

        // For a two-child node, replace it with the smallest node in the right subtree.
        DateNode* temp = findMinDateNode(node->right);
        node->date = temp->date;
        node->dateNum = temp->dateNum;
        node->cumulative = temp->cumulative;
        node->right = deleteDateNode(node->right, temp->dateNum);
    }

    // After the deletion, update the height and check the balance factor.
    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getHeight(node->left) - getHeight(node->right);

    // Rebalance after deletion in the same way as insertion.
    if(balance > 1 && getHeight(node->left->left) >= getHeight(node->left->right))
        return rotateRight(node);

    if(balance > 1 && getHeight(node->left->left) < getHeight(node->left->right)) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if(balance < -1 && getHeight(node->right->right) >= getHeight(node->right->left))
        return rotateLeft(node);

    if(balance < -1 && getHeight(node->right->right) < getHeight(node->right->left)) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

// Print the tree in chronological order from oldest to newest date.
void printDateTree(DateNode* node) {
    if(!node) return;
    printDateTree(node->left);
    cout << node->date << " | " << node->cumulative << "\n";
    printDateTree(node->right);
}

// Remove all nodes in the tree to free the memory used by the AVL structure.
void deleteDateTree(DateNode* node) {
    if(!node) return;
    deleteDateTree(node->left);
    deleteDateTree(node->right);
    delete node;
}

/* ==================== BST BY CUMULATIVE ==================== */

// This second AVL tree stores values by cumulative amount instead of by date.
// The purpose is to support queries like: "what is the lowest cumulative value?" or "what is the highest?"
// Because multiple dates can share the same cumulative value, each node contains a vector of dates.
// This lets the tree store several dates under one cumulative key while still remaining balanced.

struct CumNode {
    long long cumulative; // The numeric cumulative value used as the BST key
    vector<string> dates; // Dates that share this cumulative total
    int height;           // AVL height
    CumNode* left;        // Left subtree contains smaller cumulative values
    CumNode* right;       // Right subtree contains larger cumulative values

    CumNode(long long c, const string& d)
        : cumulative(c), dates(1, d), height(1),
          left(nullptr), right(nullptr) {}
};

// The height helper is the same idea as before, but now for the cumulative tree.
int getHeight(CumNode* node) {
    return node ? node->height : 0;
}

// Standard AVL rotation used for the cumulative-value tree.
CumNode* rotateRight(CumNode* y) {
    CumNode* x = y->left;
    y->left = x->right;
    x->right = y;

    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    return x;
}

// Standard AVL rotation used for the cumulative-value tree.
CumNode* rotateLeft(CumNode* x) {
    CumNode* y = x->right;
    x->right = y->left;
    y->left = x;

    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    return y;
}

// Insert a cumulative value into the tree.
// If the same cumulative value already exists, we append the new date to that node's vector.
CumNode* insertCumNode(CumNode* node, long long cumulative, const string& date) {
    if(!node)
        return new CumNode(cumulative, date); // Empty tree: create a node

    if(cumulative == node->cumulative) {
        node->dates.push_back(date); // Multiple dates may have the same cumulative total
        return node;
    }

    if(cumulative < node->cumulative)
        node->left = insertCumNode(node->left, cumulative, date);
    else
        node->right = insertCumNode(node->right, cumulative, date);

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getHeight(node->left) - getHeight(node->right);

    if(balance > 1 && cumulative < node->left->cumulative)
        return rotateRight(node);

    if(balance < -1 && cumulative > node->right->cumulative)
        return rotateLeft(node);

    if(balance > 1 && cumulative > node->left->cumulative) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if(balance < -1 && cumulative < node->right->cumulative) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

// Print the cumulative tree in sorted order from smallest cumulative value to largest.
void printCumTree(CumNode* node) {
    if(!node) return;
    printCumTree(node->left);

    cout << node->cumulative << " -> ";
    for(size_t i = 0; i < node->dates.size(); ++i) {
        cout << node->dates[i];
        if(i + 1 < node->dates.size()) cout << ", ";
    }
    cout << "\n";

    printCumTree(node->right);
}

// Traverse to the leftmost node to find the minimum cumulative value.
void printMinCum(CumNode* node) {
    if (!node) {
        cout << "Tree is empty.\n";
        return;
    }
    while (node->left) node = node->left;
    cout << "MINIMUM Cumulative: " << node->cumulative << " -> Dates: ";
    for (size_t i = 0; i < node->dates.size(); ++i) 
        cout << node->dates[i] << (i + 1 < node->dates.size() ? ", " : "");
    cout << "\n";
}

// Traverse to the rightmost node to find the maximum cumulative value.
void printMaxCum(CumNode* node) {
    if (!node) {
        cout << "Tree is empty.\n";
        return;
    }
    while (node->right) node = node->right;
    cout << "MAXIMUM Cumulative: " << node->cumulative << " -> Dates: ";
    for (size_t i = 0; i < node->dates.size(); ++i) 
        cout << node->dates[i] << (i + 1 < node->dates.size() ? ", " : "");
    cout << "\n";
}

// Free memory for the cumulative tree.
void deleteCumTree(CumNode* node) {
    if(!node) return;
    deleteCumTree(node->left);
    deleteCumTree(node->right);
    delete node;
}

/* ==================== HASH TABLE ==================== */

// This section implements a hash table with separate chaining.
// A hash table is useful when we want fast lookup by date without keeping the data in sorted order.
// The date is converted into a bucket index using a hash function, and then the bucket stores a linked list of nodes.
// If two dates hash to the same bucket, both are stored in the same chain and searched one by one.
// This is different from the BSTs, which rely on comparisons and balanced tree rotations.

struct HashNode {
    Record data;
    HashNode* next;

    HashNode(const Record& record)
        : data(record), next(nullptr) {}
};

class HashTable {
private:
    int m;
    HashNode** table;

public:
    HashTable(int size) : m(size) {
        table = new HashNode*[m];
        for(int i = 0; i < m; ++i)
            table[i] = nullptr;
    }

    ~HashTable() {
        clear();
        delete[] table;
    }

    int hashFunction(const string& date) {
        int sum = 0;
        for(char c : date)
            sum += static_cast<unsigned char>(c);

        return sum % m;
    }

    // Insert only unique dates (keeps first record, matching BST behavior)
    void insert(const Record& record) {
        int index = hashFunction(record.Date);

        HashNode* current = table[index];
        while(current) {
            if(current->data.Date == record.Date) {
                return; // Date already present
            }
            current = current->next;
        }

        HashNode* node = new HashNode(record);
        node->next = table[index];
        table[index] = node;
    }

    HashNode* search(const string& date) {
        int index = hashFunction(date);
        HashNode* current = table[index];

        while(current) {
            if(current->data.Date == date)
                return current;

            current = current->next;
        }

        return nullptr;
    }

    bool remove(const string& date) {
        int index = hashFunction(date);
        HashNode* current = table[index];
        HashNode* previous = nullptr;
        bool removed = false;

        while(current) {
            HashNode* nextNode = current->next;

            if(current->data.Date == date) {
                if(!previous)
                    table[index] = nextNode;
                else
                    previous->next = nextNode;

                delete current;
                removed = true;
            } else {
                previous = current;
            }

            current = nextNode;
        }

        return removed;
    }

    bool update(const string& date, const string& newCumulative) {
        int index = hashFunction(date);
        HashNode* current = table[index];
        bool updated = false;

        while(current) {
            if(current->data.Date == date) {
                current->data.Cumulative = newCumulative;
                updated = true;
            }

            current = current->next;
        }

        return updated;
    }

    void displayTable() {
        cout << "\n========== HASH TABLE ==========\n";

        for(int i = 0; i < m; ++i) {
            cout << "Bucket [" << i << "] : ";

            HashNode* current = table[i];

            if(!current)
                cout << "EMPTY";

            while(current) {
                cout << "[" << current->data.Date << " | " << current->data.Cumulative << "]";

                if(current->next)
                    cout << " -> ";

                current = current->next;
            }

            cout << "\n";
        }

        cout << "================================\n";
    }

    void clear() {
        for(int i = 0; i < m; ++i) {
            HashNode* current = table[i];

            while(current) {
                HashNode* temp = current;
                current = current->next;
                delete temp;
            }

            table[i] = nullptr;
        }
    }
};

/* ==================== MENUS ==================== */

// These menu functions are the user interface of the program.
// They make the program interactive by showing choices like BST, hash table, and exit.
// Each menu prints options, and the program reads the user's selection and reacts to it.
// The actual work is done by the data structures below, while these functions only control navigation.

void showTopMenu() {
    cout << "\n=========== DATA STRUCTURE ===========\n";
    cout << "1. Load CSV into BST\n";
    cout << "2. Load CSV into Hash Table (Chaining)\n";
    cout << "3. Exit\n";
    cout << "======================================\n";
    cout << "Choice: ";
}

void showBSTTypeMenu() {
    cout << "\n=========== BST BUILD TYPE ===========\n";
    cout << "1. Load BST by Date\n";
    cout << "2. Load BST by Cumulative per Day\n";
    cout << "3. Back\n";
    cout << "======================================\n";
    cout << "Choice: ";
}

void showDateBSTMenu() {
    cout << "\n============= BST BY DATE ============\n";
    cout << "1. Search Cumulative by Date\n";
    cout << "2. Modify Cumulative by Date\n";
    cout << "3. Delete Record by Date\n";
    cout << "4. Display BST\n";
    cout << "5. Back\n";
    cout << "======================================\n";
    cout << "Choice: ";
}

void showCumBSTMenu() {
    cout << "\n========= BST BY CUMULATIVE ==========\n";
    cout << "1. Find MINIMUM Cumulative\n";
    cout << "2. Find MAXIMUM Cumulative\n";
    cout << "3. Display Full BST\n";
    cout << "4. Back\n";
    cout << "======================================\n";
    cout << "Choice: ";
}

void showHashMenu() {
    cout << "\n========== HASH TABLE MENU ==========\n";
    cout << "1. Search Cumulative by Date\n";
    cout << "2. Modify Cumulative by Date\n";
    cout << "3. Delete Record by Date\n";
    cout << "4. Display Hash Table\n";
    cout << "5. Back\n";
    cout << "=====================================\n";
    cout << "Choice: ";
}

/* ==================== MAIN ==================== */

// The main program starts here.
// It reads all CSV data once, stores it in memory, and then lets the user choose which data structure to use.
// This is why the same dataset can be reused for the BST versions and the hash table version without reopening the file.
// The loop keeps running until the user chooses to exit, so the program behaves like a small menu-driven database.

int main(){
    string filename="effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv";

    vector<Record> records=loadAllRecords(filename);

    if(records.empty()){
        cout<<"No records loaded.\n";
        return 1;
    }

    cout<<"Success! Loaded "<<records.size()<<" records\n";

    int mainChoice;

    // This is the main loop that keeps the menu system active.
    // The program keeps asking the user for an action until they choose to leave.
    // This loop is the heart of the interactive application and controls which data structure is used.
    do{
        showTopMenu();
        mainChoice=readMenuChoice();

        // If the user chooses option 1, they want to work with a BST.
        // This branch then shows a second menu to decide whether the tree is ordered by date or by cumulative value.
        if(mainChoice==1){
            int bstChoice;

            do{
                showBSTTypeMenu();
                bstChoice=readMenuChoice();

                // Option 1: build a date-based AVL tree.
                if(bstChoice==1){
                    DateNode* root=nullptr;

                    // Fill the tree by inserting each CSV date and its cumulative value.
                    for(const Record& record:records)
                        root=insertDateNode(root,record.Date,parseCumulative(record.Cumulative));

                    int choice;

                    do{
                        showDateBSTMenu();
                        choice=readMenuChoice();

                        // Search the cumulative value using a specific date.
                        if(choice==1){
                            string date;
                            cout<<"Enter date: ";
                            cin>>date;
                            date=normalizeDate(date);

                            DateNode* result=searchDateNode(root,dateToNum(date));

                            if(result)
                                cout<<"Cumulative: "<<result->cumulative<<"\n";
                            else
                                cout<<"Record not found.\n";
                        }
                        // Modify the cumulative value of an existing date.
                        else if(choice==2){
                            string date;
                            long long newCumulative;

                            cout<<"Enter date: ";
                            cin>>date;
                            date=normalizeDate(date);

                            DateNode* result=searchDateNode(root,dateToNum(date));

                            if(!result){
                                cout<<"Record not found.\n";
                            } else{
                                cout<<"Current Cumulative: "<<result->cumulative<<"\n";
                                cout<<"Enter new Cumulative: ";
                                cin>>newCumulative;

                                result->cumulative=newCumulative;
                                cout<<"Cumulative updated successfully.\n";
                            }
                        }
                        // Delete a date from the date-based BST.
                        else if(choice==3){
                            string date;

                            cout<<"Enter date to delete: ";
                            cin>>date;
                            date=normalizeDate(date);

                            if(searchDateNode(root,dateToNum(date))){
                                root=deleteDateNode(root,dateToNum(date));
                                cout<<"Record deleted successfully.\n";
                            } else{
                                cout<<"Record not found.\n";
                            }
                        }
                        // Display the date-sorted tree in order.
                        else if(choice==4){
                            cout<<"\nDate | Cumulative\n";
                            cout<<"------------------\n";
                            printDateTree(root);
                        }

                    }while(choice!=5);

                    deleteDateTree(root);
                }
                // Option 2: build a cumulative-value BST.
                else if(bstChoice==2){
                    CumNode* root=nullptr;

                    // Insert each cumulative value into the second tree.
                    // The same date may appear more than once in the sorted cumulative structure, so dates are grouped in a vector.
                    for(const Record& record:records){
                        long long cumulative=parseCumulative(record.Cumulative);
                        root=insertCumNode(root,cumulative,record.Date);
                    }

                    int choice;

                    do{
                        showCumBSTMenu();
                        choice=readMenuChoice();

                        // Find the smallest cumulative value in the tree.
                        if(choice==1){
                            cout<<"\nCumulative -> Dates\n";
                            cout<<"----------------------\n";
                            printMinCum(root);
                        }
                        // Find the largest cumulative value in the tree.
                        else if(choice==2){
                            cout<<"\nCumulative -> Dates\n";
                            cout<<"----------------------\n";
                            printMaxCum(root);
                        }
                        // Print the full cumulative BST in order.
                        else if(choice==3){
                            cout<<"\nCumulative -> Dates\n";
                            cout<<"----------------------\n";
                            printCumTree(root);
                        }

                    }while(choice!=4);

                    deleteCumTree(root);
                }

            }while(bstChoice!=3);
        }
        // If the user chooses option 2, they want to use a hash table instead of BSTs.
        else if(mainChoice==2){
            int m;

            do{
                cout<<"\nEnter number of buckets (odd number): ";
                m=readMenuChoice();

                if(m<=0||m%2==0)
                    cout<<"m must be a positive odd number.\n";

            }while(m<=0||m%2==0);

            // Create the hash table using the selected bucket count.
            HashTable hashTable(m);

            // Insert all records so the table is ready for search, update, delete, and display operations.
            for(const Record& record:records) hashTable.insert(record);

            int choice;

            do{
                showHashMenu();
                choice=readMenuChoice();

                // Search for a date and print its cumulative value.
                if(choice==1){
                    string date;

                    cout<<"Enter date: ";
                    cin>>date;
                    date=normalizeDate(date);

                    HashNode* result=hashTable.search(date);

                    if(result)
                        cout<<"Cumulative: "<<result->data.Cumulative<<"\n";
                    else
                        cout<<"Record not found.\n";
                }
                // Update the cumulative value for a given date.
                else if(choice==2){
                    string date;
                    string newCumulative;

                    cout<<"Enter date: ";
                    cin>>date;
                    date=normalizeDate(date);

                    HashNode* result=hashTable.search(date);

                    if(result){
                        cout<<"Current Cumulative: "<<result->data.Cumulative<<"\n";
                        cout<<"Enter new Cumulative: ";
                        cin>>newCumulative;

                        if(hashTable.update(date,newCumulative)){
                            cout<<"Cumulative updated successfully.\n";
                        }
                    }
                    else{
                        cout<<"Record not found.\n";
                    }
                }
                // Delete a date record from the hash table.
                else if(choice==3){
                    string date;

                    cout<<"Enter date to delete: ";
                    cin>>date;
                    date=normalizeDate(date);

                    if(hashTable.remove(date)) cout<<"Record deleted successfully.\n";
                    else cout<<"Record not found.\n";
                }
                // Display every bucket of the hash table.
                else if(choice==4){
                    hashTable.displayTable();
                }

            }while(choice!=5);
        }

    }while(mainChoice!=3);

    cout<<"Program terminated.\n";
    return 0;
}
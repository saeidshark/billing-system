#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <cctype>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace std;

static const string CUSTOMERS_FILE = "customers.csv";
static const string ITEMS_FILE = "items.csv";
static const string INVOICES_META_FILE = "invoices_meta.csv";
static const string INVOICES_FOLDER = "invoices";

string now_datetime() {
    time_t t = time(nullptr);
    tm* lt = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    return string(buf);
}

string now_date() {
    time_t t = time(nullptr);
    tm* lt = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return string(buf);
}

string trim(const string& s) {
    size_t i = 0;
    size_t j = s.size();
    while (i < j && isspace((unsigned char)s[i])) ++i;
    while (j > i && isspace((unsigned char)s[j-1])) --j;
    return s.substr(i, j - i);
}

string input_line(const string& prompt) {
    cout << prompt << flush;
    string s;
    getline(cin, s);
    return trim(s);
}

int input_int(const string& prompt, bool allowEmpty=false, int defaultVal=0) {
    while (true) {
        string s = input_line(prompt);
        if (s.empty()) {
            if (allowEmpty) return defaultVal;
            cout << "Please enter a number.\n";
            continue;
        }
        try {
            size_t pos = 0;
            int v = stoi(s, &pos);
            if (pos != s.size()) throw invalid_argument("extra");
            return v;
        } catch (...) {
            cout << "Invalid number. Try again.\n";
        }
    }
}

double input_double(const string& prompt, bool allowEmpty=false, double defaultVal=0.0) {
    while (true) {
        string s = input_line(prompt);
        if (s.empty()) {
            if (allowEmpty) return defaultVal;
            cout << "Please enter a number.\n";
            continue;
        }
        try {
            size_t pos = 0;
            double v = stod(s, &pos);
            if (pos != s.size()) throw invalid_argument("extra");
            return v;
        } catch (...) {
            cout << "Invalid number. Try again.\n";
        }
    }
}

int next_id_from_file(const string& file, int defaultStart) {
    ifstream in(file.c_str());
    if (!in) return defaultStart;
    string line;
    int maxId = defaultStart - 1;
    while (getline(in, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string idStr;
        if (getline(ss, idStr, ',')) {
            try {
                int id = stoi(idStr);
                if (id > maxId) maxId = id;
            } catch (...) {}
        }
    }
    return maxId + 1;
}

struct Customer {
    int id;
    string name;
    string address;
    string email;
    string phone;
    static Customer from_csv(const string& line) {
        Customer c;
        stringstream ss(line);
        string field;
        getline(ss, field, ','); c.id = stoi(field);
        getline(ss, c.name, ',');
        getline(ss, c.address, ',');
        getline(ss, c.email, ',');
        getline(ss, c.phone, ',');
        return c;
    }
    string to_csv() const {
        stringstream ss;
        ss << id << "," << name << "," << address << "," << email << "," << phone;
        return ss.str();
    }
};

struct Item {
    int id;
    string code;
    string description;
    double unitPrice;
    double taxPercent;
    static Item from_csv(const string& line) {
        Item it;
        stringstream ss(line);
        string field;
        getline(ss, field, ','); it.id = stoi(field);
        getline(ss, it.code, ',');
        getline(ss, it.description, ',');
        getline(ss, field, ','); it.unitPrice = stod(field);
        getline(ss, field, ','); it.taxPercent = stod(field);
        return it;
    }
    string to_csv() const {
        stringstream ss;
        ss << id << "," << code << "," << description << "," << fixed << setprecision(2) << unitPrice << "," << taxPercent;
        return ss.str();
    }
};

struct InvoiceLine {
    int itemId;
    string description;
    double unitPrice;
    double quantity;
    double taxPercent;
    double lineAmount() const { return unitPrice * quantity; }
    double taxAmount() const { return lineAmount() * (taxPercent / 100.0); }
};

struct Invoice {
    int id;
    int customerId;
    string date;
    string note;
    vector<InvoiceLine> lines;
    double discountPercent;
    double shipping;
    string to_txt(const Customer& c) const {
        stringstream ss;
        ss << left << setw(40) << "BILLING SYSTEM" << right << setw(30) << "INVOICE #" << id << "\n";
        ss << left << setw(50) << "Date: " << date << "\n";
        ss << left << setw(50) << "Customer: " << c.name << "\n";
        ss << left << setw(50) << "Address: " << c.address << "\n";
        ss << left << setw(50) << "Email: " << c.email << "\n";
        ss << left << setw(50) << "Phone: " << c.phone << "\n\n";
        ss << left << setw(6) << "No" << setw(8) << "Qty" << setw(10) << "Unit" << setw(40) << "Description" << setw(10) << "Tax%" << setw(14) << "LineTotal" << "\n";
        ss << string(90, '-') << "\n";
        double sub = 0;
        double tax = 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            const InvoiceLine& L = lines[i];
            ss << left << setw(6) << i + 1 << setw(8) << L.quantity << setw(10) << fixed << setprecision(2) << L.unitPrice << setw(40) << L.description << setw(10) << L.taxPercent << setw(14) << fixed << setprecision(2) << L.lineAmount() << "\n";
            sub += L.lineAmount();
            tax += L.taxAmount();
        }
        ss << string(90, '-') << "\n";
        ss << right << setw(70) << "Subtotal: " << setw(14) << fixed << setprecision(2) << sub << "\n";
        ss << right << setw(70) << "Tax: " << setw(14) << fixed << setprecision(2) << tax << "\n";
        double discount = (discountPercent / 100.0) * (sub + tax);
        ss << right << setw(70) << "Discount (" << discountPercent << "%): " << setw(14) << fixed << setprecision(2) << -discount << "\n";
        ss << right << setw(70) << "Shipping: " << setw(14) << fixed << setprecision(2) << shipping << "\n";
        double total = sub + tax - discount + shipping;
        ss << right << setw(70) << "TOTAL: " << setw(14) << fixed << setprecision(2) << total << "\n\n";
        ss << "Note: " << note << "\n";
        ss << "Generated: " << now_datetime() << "\n";
        return ss.str();
    }
    string meta_csv() const {
        stringstream ss;
        ss << id << "," << customerId << "," << date << "," << fixed << setprecision(2) << discountPercent << "," << fixed << setprecision(2) << shipping;
        return ss.str();
    }
};

struct BillingSystem {
    vector<Customer> customers;
    vector<Item> items;
    vector<Invoice> invoices;
    int nextCustomerId;
    int nextItemId;
    int nextInvoiceId;
    BillingSystem() {
        ensure_invoices_folder();
        nextCustomerId = next_id_from_file(CUSTOMERS_FILE, 1001);
        nextItemId = next_id_from_file(ITEMS_FILE, 5001);
        nextInvoiceId = next_id_from_file(INVOICES_META_FILE, 9001);
        load_all();
    }
    void ensure_invoices_folder() {
#ifdef _WIN32
        _mkdir(INVOICES_FOLDER.c_str());
#else
        mkdir(INVOICES_FOLDER.c_str(), 0755);
#endif
    }
    void load_all() {
        customers.clear();
        items.clear();
        invoices.clear();
        ifstream fc(CUSTOMERS_FILE.c_str());
        string line;
        while (getline(fc, line)) {
            if (line.empty()) continue;
            try { customers.push_back(Customer::from_csv(line)); } catch(...) {}
        }
        ifstream fi(ITEMS_FILE.c_str());
        while (getline(fi, line)) {
            if (line.empty()) continue;
            try { items.push_back(Item::from_csv(line)); } catch(...) {}
        }
        ifstream fm(INVOICES_META_FILE.c_str());
        while (getline(fm, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idStr, customerStr, dateStr, discStr, shipStr;
            getline(ss, idStr, ','); getline(ss, customerStr, ','); getline(ss, dateStr, ','); getline(ss, discStr, ','); getline(ss, shipStr, ',');
            try {
                Invoice inv;
                inv.id = stoi(idStr);
                inv.customerId = stoi(customerStr);
                inv.date = dateStr;
                inv.discountPercent = stod(discStr);
                inv.shipping = stod(shipStr);
                string invFile = INVOICES_FOLDER + "/invoice_" + to_string(inv.id) + ".txt";
                ifstream vin(invFile.c_str());
                if (vin) {
                    inv.note = "";
                    inv.lines.clear();
                    invoices.push_back(inv);
                }
            } catch(...) {}
        }
    }
    void save_customer(const Customer& c) {
        ofstream out(CUSTOMERS_FILE.c_str(), ios::app);
        out << c.to_csv() << "\n";
    }
    void rewrite_customers() {
        ofstream out(CUSTOMERS_FILE.c_str());
        for (auto& c : customers) out << c.to_csv() << "\n";
    }
    void save_item(const Item& it) {
        ofstream out(ITEMS_FILE.c_str(), ios::app);
        out << it.to_csv() << "\n";
    }
    void rewrite_items() {
        ofstream out(ITEMS_FILE.c_str());
        for (auto& it : items) out << it.to_csv() << "\n";
    }
    void save_invoice(const Invoice& inv, const Customer& c) {
        ensure_invoices_folder();
        string fname = INVOICES_FOLDER + "/invoice_" + to_string(inv.id) + ".txt";
        ofstream out(fname.c_str());
        out << inv.to_txt(c);
        ofstream meta(INVOICES_META_FILE.c_str(), ios::app);
        meta << inv.meta_csv() << "\n";
    }
    Customer* find_customer(int id) {
        for (auto& c : customers) if (c.id == id) return &c;
        return nullptr;
    }
    Item* find_item_by_id(int id) {
        for (auto& it : items) if (it.id == id) return &it;
        return nullptr;
    }
    Item* find_item_by_code(const string& code) {
        for (auto& it : items) if (it.code == code) return &it;
        return nullptr;
    }
    void add_customer() {
        Customer c;
        c.id = nextCustomerId++;
        c.name = input_line("Enter customer name: ");
        c.address = input_line("Enter address: ");
        c.email = input_line("Enter email: ");
        c.phone = input_line("Enter phone: ");
        customers.push_back(c);
        save_customer(c);
        cout << "Customer saved with ID " << c.id << "\n";
    }
    void list_customers() {
        cout << left << setw(6) << "ID" << setw(30) << "Name" << setw(30) << "Email" << setw(20) << "Phone" << "\n";
        cout << string(90, '-') << "\n";
        for (auto& c : customers) {
            cout << left << setw(6) << c.id << setw(30) << c.name << setw(30) << c.email << setw(20) << c.phone << "\n";
        }
    }
    void search_customers() {
        string q = input_line("Search by name or email: ");
        string lowerq = q;
        transform(lowerq.begin(), lowerq.end(), lowerq.begin(), ::tolower);
        cout << left << setw(6) << "ID" << setw(30) << "Name" << setw(30) << "Email" << setw(20) << "Phone" << "\n";
        cout << string(90, '-') << "\n";
        for (auto& c : customers) {
            string l = c.name + " " + c.email;
            string lower = l;
            transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find(lowerq) != string::npos) {
                cout << left << setw(6) << c.id << setw(30) << c.name << setw(30) << c.email << setw(20) << c.phone << "\n";
            }
        }
    }
    void add_item() {
        Item it;
        it.id = nextItemId++;
        it.code = input_line("Enter item code (SKU): ");
        it.description = input_line("Enter description: ");
        it.unitPrice = input_double("Enter unit price: ");
        it.taxPercent = input_double("Enter tax percent: ");
        items.push_back(it);
        save_item(it);
        cout << "Item saved with ID " << it.id << "\n";
    }
    void list_items() {
        cout << left << setw(6) << "ID" << setw(12) << "SKU" << setw(40) << "Description" << setw(12) << "Price" << setw(8) << "Tax" << "\n";
        cout << string(80, '-') << "\n";
        for (auto& it : items) {
            cout << left << setw(6) << it.id << setw(12) << it.code << setw(40) << it.description << setw(12) << fixed << setprecision(2) << it.unitPrice << setw(8) << it.taxPercent << "\n";
        }
    }
    void search_items() {
        string q = input_line("Search by SKU or description: ");
        string lowerq = q;
        transform(lowerq.begin(), lowerq.end(), lowerq.begin(), ::tolower);
        cout << left << setw(6) << "ID" << setw(12) << "SKU" << setw(40) << "Description" << setw(12) << "Price" << setw(8) << "Tax" << "\n";
        cout << string(80, '-') << "\n";
        for (auto& it : items) {
            string l = it.code + " " + it.description;
            string lower = l;
            transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find(lowerq) != string::npos) {
                cout << left << setw(6) << it.id << setw(12) << it.code << setw(40) << it.description << setw(12) << fixed << setprecision(2) << it.unitPrice << setw(8) << it.taxPercent << "\n";
            }
        }
    }
    void create_invoice() {
        if (customers.empty()) { cout << "No customers defined. Add customer first.\n"; return; }
        if (items.empty()) { cout << "No items defined. Add items first.\n"; return; }
        Invoice inv;
        inv.id = nextInvoiceId++;
        int cid = input_int("Enter customer ID: ");
        Customer* c = find_customer(cid);
        if (!c) { cout << "Customer not found.\n"; return; }
        inv.customerId = cid;
        inv.date = now_date();
        inv.note = "";
        inv.discountPercent = 0.0;
        inv.shipping = 0.0;
        while (true) {
            string yn = input_line("Add line? (y/n): ");
            if (yn.empty() || (yn[0] != 'y' && yn[0] != 'Y')) break;
            string key = input_line("Enter SKU or item ID: ");
            Item* it = nullptr;
            bool digits = !key.empty() && all_of(key.begin(), key.end(), ::isdigit);
            if (digits) it = find_item_by_id(stoi(key));
            if (!it) it = find_item_by_code(key);
            if (!it) { cout << "Item not found. Try again.\n"; continue; }
            InvoiceLine L;
            L.itemId = it->id;
            L.description = it->description;
            L.unitPrice = it->unitPrice;
            L.taxPercent = it->taxPercent;
            L.quantity = input_double("Enter quantity: ");
            inv.lines.push_back(L);
        }
        inv.discountPercent = input_double("Enter discount percent (0 for none): ", true, 0.0);
        inv.shipping = input_double("Enter shipping amount: ", true, 0.0);
        inv.note = input_line("Enter optional note: ");
        save_invoice(inv, *c);
        cout << "Invoice created and saved as " << INVOICES_FOLDER << "/invoice_" << inv.id << ".txt\n";
    }
    void list_invoices_meta() {
        ifstream meta(INVOICES_META_FILE.c_str());
        if (!meta) { cout << "No invoices found.\n"; return; }
        cout << left << setw(8) << "InvID" << setw(10) << "CustID" << setw(15) << "Date" << setw(12) << "Discount" << setw(12) << "Shipping" << "\n";
        cout << string(60, '-') << "\n";
        string line;
        while (getline(meta, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, cust, date, disc, ship;
            getline(ss, id, ','); getline(ss, cust, ','); getline(ss, date, ','); getline(ss, disc, ','); getline(ss, ship, ',');
            cout << left << setw(8) << id << setw(10) << cust << setw(15) << date << setw(12) << disc << setw(12) << ship << "\n";
        }
    }
    void view_invoice_file() {
        string s = input_line("Enter invoice ID to view: ");
        string fname = INVOICES_FOLDER + "/invoice_" + s + ".txt";
        ifstream in(fname.c_str());
        if (!in) { cout << "Invoice file not found.\n"; return; }
        string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        cout << content << "\n";
    }
    void export_report_sales() {
        string start = input_line("Enter start date (YYYY-MM-DD): ");
        string end = input_line("Enter end date (YYYY-MM-DD): ");
        double totalSales = 0;
        ifstream meta(INVOICES_META_FILE.c_str());
        if (!meta) { cout << "No invoices found.\n"; return; }
        string line;
        cout << left << setw(8) << "InvID" << setw(10) << "CustID" << setw(12) << "Date" << setw(12) << "Total" << "\n";
        cout << string(50, '-') << "\n";
        while (getline(meta, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, cust, date, disc, ship;
            getline(ss, id, ','); getline(ss, cust, ','); getline(ss, date, ','); getline(ss, disc, ','); getline(ss, ship, ',');
            if (date < start || date > end) continue;
            string fname = INVOICES_FOLDER + "/invoice_" + id + ".txt";
            ifstream invf(fname.c_str());
            if (!invf) continue;
            string l;
            while (getline(invf, l)) {
                if (l.find("TOTAL:") != string::npos) {
                    stringstream ss2(l);
                    string tmp;
                    vector<string> parts;
                    while (ss2 >> tmp) parts.push_back(tmp);
                    if (!parts.empty()) {
                        string num = parts.back();
                        double val = atof(num.c_str());
                        cout << left << setw(8) << id << setw(10) << cust << setw(12) << date << setw(12) << fixed << setprecision(2) << val << "\n";
                        totalSales += val;
                    }
                    break;
                }
            }
        }
        cout << string(50, '-') << "\n";
        cout << left << setw(30) << "TOTAL SALES: " << setw(12) << fixed << setprecision(2) << totalSales << "\n";
    }
    void remove_customer() {
        int id = input_int("Enter customer ID to remove: ");
        auto it = find_if(customers.begin(), customers.end(), [&](const Customer& c){ return c.id == id; });
        if (it == customers.end()) { cout << "Not found.\n"; return; }
        customers.erase(it);
        rewrite_customers();
        cout << "Customer removed.\n";
    }
    void remove_item() {
        int id = input_int("Enter item ID to remove: ");
        auto it = find_if(items.begin(), items.end(), [&](const Item& i){ return i.id == id; });
        if (it == items.end()) { cout << "Not found.\n"; return; }
        items.erase(it);
        rewrite_items();
        cout << "Item removed.\n";
    }
};

void pause() {
    cout << "\nPress ENTER to continue..." << flush;
    string s;
    getline(cin, s);
}

int main() {
    BillingSystem sys;
    while (true) {
        cout << string(60, '=') << "\n";
        cout << "Professional Billing System - Menu\n";
        cout << "1. Customers - Add\n";
        cout << "2. Customers - List\n";
        cout << "3. Customers - Search\n";
        cout << "4. Customers - Remove\n";
        cout << "5. Items - Add\n";
        cout << "6. Items - List\n";
        cout << "7. Items - Search\n";
        cout << "8. Items - Remove\n";
        cout << "9. Create Invoice\n";
        cout << "10. List Invoices (meta)\n";
        cout << "11. View Invoice File\n";
        cout << "12. Sales Report (by date range)\n";
        cout << "0. Exit\n";
        string opt;
        while (true) {
            opt = input_line("Choose option: ");
            if (opt.empty()) {
                cout << "Please enter a menu number.\n";
                continue;
            }
            break;
        }
        if (opt == "1") { sys.add_customer(); pause(); }
        else if (opt == "2") { sys.list_customers(); pause(); }
        else if (opt == "3") { sys.search_customers(); pause(); }
        else if (opt == "4") { sys.remove_customer(); pause(); }
        else if (opt == "5") { sys.add_item(); pause(); }
        else if (opt == "6") { sys.list_items(); pause(); }
        else if (opt == "7") { sys.search_items(); pause(); }
        else if (opt == "8") { sys.remove_item(); pause(); }
        else if (opt == "9") { sys.create_invoice(); pause(); }
        else if (opt == "10") { sys.list_invoices_meta(); pause(); }
        else if (opt == "11") { sys.view_invoice_file(); pause(); }
        else if (opt == "12") { sys.export_report_sales(); pause(); }
        else if (opt == "0") { cout << "Exiting. Goodbye.\n"; break; }
        else { cout << "Invalid option.\n"; pause(); }
    }
    return 0;
}

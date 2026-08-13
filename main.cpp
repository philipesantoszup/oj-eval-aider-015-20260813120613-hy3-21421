#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <sstream>

using namespace std;

const int NUM_BUCKETS = 16;

string bucket_filename(int bucket) {
    return "bucket_" + to_string(bucket) + ".dat";
}

unsigned int hash_index(const string& s) {
    unsigned int h = 0;
    for (char c : s) {
        h = h * 31 + (unsigned char)c;
    }
    return h;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;
    cin.ignore(); // ignore newline after n

    for (int i = 0; i < n; ++i) {
        string line;
        if (!getline(cin, line)) break;
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        if (cmd == "insert") {
            string index;
            int value;
            iss >> index >> value;
            int bucket = hash_index(index) % NUM_BUCKETS;
            ofstream out(bucket_filename(bucket), ios::app);
            out << "I " << index << " " << value << "\n";
        } else if (cmd == "delete") {
            string index;
            int value;
            iss >> index >> value;
            int bucket = hash_index(index) % NUM_BUCKETS;
            ofstream out(bucket_filename(bucket), ios::app);
            out << "D " << index << " " << value << "\n";
        } else if (cmd == "find") {
            string index;
            iss >> index;
            int bucket = hash_index(index) % NUM_BUCKETS;
            ifstream in(bucket_filename(bucket));
            set<int> values;
            string rec;
            while (getline(in, rec)) {
                if (rec.empty()) continue;
                char type;
                string rec_index;
                int val;
                istringstream riss(rec);
                riss >> type >> rec_index >> val;
                if (rec_index == index) {
                    if (type == 'I') {
                        values.insert(val);
                    } else if (type == 'D') {
                        values.erase(val);
                    }
                }
            }
            if (values.empty()) {
                cout << "null\n";
            } else {
                bool first = true;
                for (int v : values) {
                    if (!first) cout << " ";
                    cout << v;
                    first = false;
                }
                cout << "\n";
            }
        }
    }
    return 0;
}

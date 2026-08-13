#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <sstream>

using namespace std;

const int NUM_BUCKETS = 10000;
const char* DIR_FILE = "dir.dat";
const char* REC_FILE = "records.dat";

#pragma pack(push, 1)
struct Record {
    char index[64];
    int value;
    int next;
    int active; // 1 active, 0 deleted
};
#pragma pack(pop)

static_assert(sizeof(Record) == 76, "Record size mismatch");

unsigned int hash_index(const string& s) {
    unsigned int h = 0;
    for (char c : s) {
        h = h * 31 + (unsigned char)c;
    }
    return h;
}

fstream rec_file;
fstream dir_file;

void init_files() {
    dir_file.open(DIR_FILE, ios::in | ios::out | ios::binary);
    if (!dir_file) {
        fstream create(DIR_FILE, ios::out | ios::binary);
        int init = -1;
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            create.write(reinterpret_cast<const char*>(&init), sizeof(int));
        }
        create.close();
        dir_file.open(DIR_FILE, ios::in | ios::out | ios::binary);
    }
    rec_file.open(REC_FILE, ios::in | ios::out | ios::binary);
    if (!rec_file) {
        fstream create(REC_FILE, ios::out | ios::binary);
        create.close();
        rec_file.open(REC_FILE, ios::in | ios::out | ios::binary);
    }
}

int get_bucket_head(int bucket) {
    int head;
    dir_file.seekg(bucket * sizeof(int), ios::beg);
    dir_file.read(reinterpret_cast<char*>(&head), sizeof(int));
    return head;
}

void set_bucket_head(int bucket, int head) {
    dir_file.seekp(bucket * sizeof(int), ios::beg);
    dir_file.write(reinterpret_cast<const char*>(&head), sizeof(int));
    dir_file.flush();
}

int append_record(const Record& rec) {
    rec_file.seekp(0, ios::end);
    streampos pos = rec_file.tellp();
    int id = static_cast<int>(pos / sizeof(Record));
    rec_file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    rec_file.flush();
    return id;
}

Record read_record(int id) {
    Record rec;
    rec_file.seekg(id * sizeof(Record), ios::beg);
    rec_file.read(reinterpret_cast<char*>(&rec), sizeof(Record));
    return rec;
}

void write_record(int id, const Record& rec) {
    rec_file.seekp(id * sizeof(Record), ios::beg);
    rec_file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    rec_file.flush();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    init_files();

    int n;
    if (!(cin >> n)) return 0;
    cin.ignore();

    for (int i = 0; i < n; ++i) {
        string line;
        if (!getline(cin, line)) break;
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        if (cmd == "insert") {
            string index; int value;
            iss >> index >> value;
            int bucket = hash_index(index) % NUM_BUCKETS;
            int head = get_bucket_head(bucket);
            Record rec;
            memset(rec.index, 0, 64);
            strncpy(rec.index, index.c_str(), 64);
            rec.value = value;
            rec.next = head;
            rec.active = 1;
            int new_id = append_record(rec);
            set_bucket_head(bucket, new_id);
        } else if (cmd == "delete") {
            string index; int value;
            iss >> index >> value;
            int bucket = hash_index(index) % NUM_BUCKETS;
            int cur = get_bucket_head(bucket);
            while (cur != -1) {
                Record rec = read_record(cur);
                if (rec.active && strncmp(rec.index, index.c_str(), 64) == 0 && rec.value == value) {
                    rec.active = 0;
                    write_record(cur, rec);
                    break;
                }
                cur = rec.next;
            }
        } else if (cmd == "find") {
            string index;
            iss >> index;
            int bucket = hash_index(index) % NUM_BUCKETS;
            int cur = get_bucket_head(bucket);
            vector<int> values;
            while (cur != -1) {
                Record rec = read_record(cur);
                if (rec.active && strncmp(rec.index, index.c_str(), 64) == 0) {
                    values.push_back(rec.value);
                }
                cur = rec.next;
            }
            if (values.empty()) {
                cout << "null\n";
            } else {
                sort(values.begin(), values.end());
                for (size_t j = 0; j < values.size(); ++j) {
                    if (j) cout << ' ';
                    cout << values[j];
                }
                cout << '\n';
            }
        }
    }
    return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

#pragma GCC optimize("O3")

using namespace std;

const int NUM_BUCKETS = 100003;
const char* DIR_FILE = "dir.dat";
const char* REC_FILE = "records.dat";

static char dir_buf[262144];
static char rec_buf[262144];

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
    unsigned int h = 2166136261u;
    for (char c : s) {
        h ^= (unsigned char)c;
        h *= 16777619u;
    }
    return h;
}

bool index_equal(const Record& rec, const string& index) {
    if (index.size() >= 64) {
        return memcmp(rec.index, index.c_str(), 64) == 0;
    }
    return strncmp(rec.index, index.c_str(), index.size()) == 0 && rec.index[index.size()] == '\0';
}

fstream rec_file;
fstream dir_file;

vector<int> dir_heads(NUM_BUCKETS, -1);
int rec_count = 0;

void init_files() {
    dir_file.rdbuf()->pubsetbuf(dir_buf, sizeof(dir_buf));
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
    dir_file.read(reinterpret_cast<char*>(dir_heads.data()), NUM_BUCKETS * sizeof(int));
    dir_file.clear();

    rec_file.rdbuf()->pubsetbuf(rec_buf, sizeof(rec_buf));
    rec_file.open(REC_FILE, ios::in | ios::out | ios::binary);
    if (!rec_file) {
        fstream create(REC_FILE, ios::out | ios::binary);
        create.close();
        rec_file.open(REC_FILE, ios::in | ios::out | ios::binary);
    }
    rec_file.seekg(0, ios::end);
    streampos len = rec_file.tellg();
    rec_count = static_cast<int>(len / sizeof(Record));
    rec_file.seekg(0, ios::beg);
    rec_file.clear();
}

int get_bucket_head(int bucket) {
    return dir_heads[bucket];
}

void set_bucket_head(int bucket, int head) {
    dir_heads[bucket] = head;
    dir_file.seekp(bucket * sizeof(int), ios::beg);
    dir_file.write(reinterpret_cast<const char*>(&head), sizeof(int));
}

int append_record(const Record& rec) {
    int id = rec_count;
    rec_file.seekp(id * sizeof(Record), ios::beg);
    rec_file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    rec_count++;
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
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    init_files();

    int n;
    if (!(cin >> n)) return 0;

    for (int i = 0; i < n; ++i) {
        string cmd;
        if (!(cin >> cmd)) break;
        if (cmd == "insert") {
            string index; int value;
            cin >> index >> value;
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
            cin >> index >> value;
            int bucket = hash_index(index) % NUM_BUCKETS;
            int cur = get_bucket_head(bucket);
            int prev = -1;
            Record prev_rec;
            while (cur != -1) {
                Record rec = read_record(cur);
                if (rec.active && index_equal(rec, index) && rec.value == value) {
                    if (prev == -1) {
                        set_bucket_head(bucket, rec.next);
                    } else {
                        prev_rec.next = rec.next;
                        write_record(prev, prev_rec);
                    }
                    rec.active = 0;
                    rec.next = -1;
                    write_record(cur, rec);
                    break;
                }
                prev = cur;
                prev_rec = rec;
                cur = rec.next;
            }
        } else if (cmd == "find") {
            string index;
            cin >> index;
            int bucket = hash_index(index) % NUM_BUCKETS;
            int cur = get_bucket_head(bucket);
            vector<int> values;
            while (cur != -1) {
                Record rec = read_record(cur);
                if (rec.active && index_equal(rec, index)) {
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
    dir_file.close();
    rec_file.close();
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>

#pragma GCC optimize("O3")

using namespace std;

const int NUM_BUCKETS = 100003;
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

int rec_fd = -1;
int dir_fd = -1;

vector<int> dir_heads(NUM_BUCKETS, -1);
int rec_count = 0;

void init_files() {
    dir_fd = open(DIR_FILE, O_RDWR | O_CREAT, 0644);
    if (dir_fd < 0) {
        perror("open dir");
        exit(1);
    }
    struct stat st;
    fstat(dir_fd, &st);
    if (st.st_size >= (off_t)(NUM_BUCKETS * sizeof(int))) {
        pread(dir_fd, dir_heads.data(), NUM_BUCKETS * sizeof(int), 0);
    } else {
        fill(dir_heads.begin(), dir_heads.end(), -1);
        pwrite(dir_fd, dir_heads.data(), NUM_BUCKETS * sizeof(int), 0);
    }

    rec_fd = open(REC_FILE, O_RDWR | O_CREAT, 0644);
    if (rec_fd < 0) {
        perror("open rec");
        exit(1);
    }
    fstat(rec_fd, &st);
    rec_count = static_cast<int>(st.st_size / sizeof(Record));
}

int get_bucket_head(int bucket) {
    return dir_heads[bucket];
}

void set_bucket_head(int bucket, int head) {
    dir_heads[bucket] = head;
}

int append_record(const Record& rec) {
    int id = rec_count;
    pwrite(rec_fd, &rec, sizeof(Record), id * sizeof(Record));
    rec_count++;
    return id;
}

Record read_record(int id) {
    Record rec;
    pread(rec_fd, &rec, sizeof(Record), id * sizeof(Record));
    return rec;
}

void write_record(int id, const Record& rec) {
    pwrite(rec_fd, &rec, sizeof(Record), id * sizeof(Record));
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

    pwrite(dir_fd, dir_heads.data(), NUM_BUCKETS * sizeof(int), 0);
    close(rec_fd);
    close(dir_fd);
    return 0;
}

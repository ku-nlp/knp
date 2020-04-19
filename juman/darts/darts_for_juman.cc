#include <iostream>
#include <vector>
#include <string>
#include "config.h"
#include "darts_for_juman.h"
#include "darts.h"
#include "mmap.h"

std::vector<Darts::DoubleArray *> darts_files;
std::vector<Mmap *> darts_dbs;

#ifndef MAX_DIC_NUMBER
#define MAX_DIC_NUMBER 5 /* 同時に使える辞書の数の上限 (JUMAN) */
#endif

#define DAFILE "jumandic.da"
#define BINFILE "jumandic.bin"

static struct _dic_t {
  int used;
  int fd;
  off_t size;
  caddr_t addr;
} dicinfo[MAX_DIC_NUMBER];

inline const char *read_ptr(const char **ptr, size_t size) {
    const char *r = *ptr;
    *ptr += size;
    return r;
}

template <class T>
inline void read_static(const char **ptr, T& value) {
    const char *r = read_ptr(ptr, sizeof(T));
    memcpy(&value, r, sizeof(T));
}

/* DB get */
char *db_get(Mmap *db, const size_t index, char *rslt, char key_type, char deleted_bytes) {
    const char *ptr = db->begin();
    ptr += index;

    // number of entries
    unsigned int num;
    read_static<unsigned int>(&ptr, num);

    for (unsigned int j = 0; j < num; j++) {
        // length of this entry
        unsigned int buf_len;
        read_static<unsigned int>(&ptr, buf_len);

        *rslt++ = key_type + PAT_BUF_INFO_BASE;
        *rslt++ = deleted_bytes + PAT_BUF_INFO_BASE;
        memcpy(rslt, ptr, buf_len); // entry itself
        ptr += buf_len;
        rslt += buf_len;
        *rslt++ = '\n';
        *rslt = '\0';
    }
    return rslt;
}

void push_darts_file(char *basename) {
    Darts::DoubleArray *darts = new Darts::DoubleArray;
    std::string darts_filename = basename;
    darts_filename += DAFILE;

    if (darts->open(darts_filename.c_str()) != -1) {
        // std::cerr << "opened darts file: " << darts_filename << std::endl;
        darts_files.push_back(darts);
    }
    else {
        fprintf(stderr, ";; cannot open darts file: %s.\n", darts_filename.c_str());
        return;
    }

    std::string darts_dbname = basename;
    darts_dbname += BINFILE;
    Mmap *db = new Mmap;
    if (db->open(darts_dbname.c_str()))
        darts_dbs.push_back(db);
    else
        fprintf(stderr, ";; cannot open bin file: %s.\n", darts_dbname.c_str());
    return;
}

// search the double array with a given str
size_t da_search(int dic_no, char *str, char *rslt) {
    rslt += strlen(rslt);

    Darts::DoubleArray::result_pair_type result_pair[1024];
    size_t num = darts_files[dic_no]->commonPrefixSearch(str, result_pair, 1024);

    for (size_t i = 0; i < num; i++) { // hit num
        size_t start_id = result_pair[i].value;
        rslt = db_get(darts_dbs[dic_no], start_id, rslt, 1, 1);
    }

    return num;
}

// traverse the double array with a given str
int da_traverse(int dic_no, char *str, size_t *node_pos, size_t key_pos, size_t key_length, char key_type, char deleted_bytes, char *rslt) {
    rslt += strlen(rslt);
    deleted_bytes++;

    int value = darts_files[dic_no]->traverse(str, *node_pos, key_pos, key_length);
    if (value >= 0) {
        size_t start_id = value;
        rslt = db_get(darts_dbs[dic_no], start_id, rslt, key_type, deleted_bytes);
        return 1;
    }
    else {
        return value;
    }
}

void close_darts() {
    for (size_t i = 0; i < darts_files.size(); i++) {
        darts_files[i]->clear();
        delete darts_files[i];
        delete darts_dbs[i];
    }
}

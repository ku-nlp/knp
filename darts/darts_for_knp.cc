#include <iostream>
#include "config.h"
#include "darts_for_knp.h"
#include "darts.h"

int Rep2IDFileExist;
Darts::DoubleArray darts;

int init_darts(char *filename) {
    if (darts.open(filename) == -1) {
        Rep2IDFileExist = 0;
    }
    else {
        Rep2IDFileExist = 1;
    }
    return Rep2IDFileExist;
}

// search the double array with a given str
size_t search_ld(char *str, size_t *result_lengths, size_t *result_values) {
    if (Rep2IDFileExist == 0) {
        return 0;
    }

    Darts::DoubleArray::result_pair_type result_pair[1024];
    size_t num = darts.commonPrefixSearch(str, result_pair, 1024);

    for (size_t i = 0; i < num; i++) { // hit num
        result_lengths[i] = result_pair[i].length;
        result_values[i] = result_pair[i].value; // size: result_pair[i].value & 0xff
                                                 // real value: result_pair[i].value >> 8
    }

    return num;
}

// traverse the double array with a given str
size_t traverse_ld(char *str, size_t *node_pos, size_t key_pos, size_t key_length, size_t *result_lengths, size_t *result_values) {
    if (Rep2IDFileExist == 0) {
        return 0;
    }

    int value = darts.traverse(str, *node_pos, key_pos, key_length);
    if (value >= 0) {
        result_lengths[0] = key_length;
        result_values[0] = value;
        return 1;
    }
    else {
        return value;
    }
}

void close_darts() {
    if (Rep2IDFileExist) {
        darts.clear();
    }
}

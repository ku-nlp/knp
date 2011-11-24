#include "common.h"
#include "distsim.h"

namespace Dist {

Distsim::Distsim(const std::string &in_mi_dbname) {
    init(in_mi_dbname);
}

Distsim::Distsim(const char *in_mi_dbname_char) {
    init(in_mi_dbname_char);
}

void Distsim::init(const std::string &in_mi_dbname) {
    midb = new Dbm(in_mi_dbname); // keymap file or basename
}

void Distsim::init(const char *in_mi_dbname_char) {
    std::string in_mi_dbname = in_mi_dbname_char;
    midb = new Dbm(in_mi_dbname); // keymap file or basename
}

Distsim::~Distsim() {
    delete midb;
}

void Distsim::make_vector(std::string &word, std::vector<std::string> &vector) {
    std::string mi_str = midb->get(word.c_str());

    split_string(mi_str, "|", vector);

    // delete MI value
    for (std::vector<std::string>::iterator it = vector.begin(); it != vector.end(); it++) {
        string::size_type pos = (*it).find_last_of(";");
        (*it).erase(pos);
    }

//     for (std::vector<std::string>::iterator it = vector.begin(); it != vector.end(); it++) {
//         cout << *it << endl;
//     }
}

double Distsim::calc_sim(const char *word1_char, const char *word2_char) {
    std::string word1 = word1_char, word2 = word2_char;
    return calc_sim(word1, word2);
}

double Distsim::calc_sim(std::string &word1, std::string &word2) {
    std::vector<std::string> vector1, vector2;
    make_vector(word1, vector1);
    make_vector(word2, vector2);

    // word1 or word2 is not found in DB
    if (vector1.size() == 0 || vector2.size() == 0)
        return 0;

    sort(vector1.begin(), vector1.end());
    sort(vector2.begin(), vector2.end());

    std::vector<std::string> int_result(std::min(vector1.size(), vector2.size())), union_result(vector1.size() + vector2.size());

    // intersection
    std::vector<std::string>::iterator end_it = set_intersection(vector1.begin(), vector1.end(), vector2.begin(), vector2.end(), int_result.begin());
    int int_num = int(end_it - int_result.begin());
    // cout << "intersection has " << int_num << " elements." << endl;
    if (int_num <= INTERSECTION_NUM_THRESHOLD) {
        return 0;
    }

    // union
    end_it = set_union(vector1.begin(), vector1.end(), vector2.begin(), vector2.end(), union_result.begin());
    int union_num = int(end_it - union_result.begin());
    // cout << "union has " << union_num << " elements." << endl;

    double jaccard_sim = (double)int_num / union_num;
    double simpson_sim = (double)int_num / std::min(vector1.size(), vector2.size());
    double sim = (jaccard_sim + simpson_sim) / 2;
    return sim;
}

}

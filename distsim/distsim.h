#ifndef DISTSIM_H
#define DISTSIM_H

#include "common.h"
#include "dbm.h"

namespace Dist {

class Distsim {
    Dbm *midb;
  public:
    Distsim() {}
    Distsim(const std::string &in_mi_dbname);
    Distsim(const char *in_mi_dbname_char);
    Distsim(const std::string &in_mi_dbname, bool compressed);
    Distsim(const char *in_mi_dbname_char, bool compressed);
    ~Distsim();
    void init(const std::string &in_mi_dbname);
    void init(const char *in_mi_dbname_char);
    void init(const std::string &in_mi_dbname, bool compressed);
    void init(const char *in_mi_dbname_char, bool compressed);
    void make_vector(std::string &word, std::vector<std::string> &vector);
    double calc_sim(std::string &word1, std::string &word2);
    double calc_sim(const char *word1_char, const char *word2_char);
};

}

#endif

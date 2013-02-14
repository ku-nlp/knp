#ifndef DISTSIM_H
#define DISTSIM_H

#include "common.h"
#include "dbm.h"

namespace Dist {

#define PRE_CALCULATE_SIMILARITY_TH 0.15

class Distsim {
    Dbm *midb;
    Dbm *simdb;
    std::set<std::string> wordlist;
    bool wordlist_available;
  public:
    Distsim() {}
    Distsim(const std::string &in_mi_dbname, const std::string &in_sim_dbname, const std::string &in_wordlist_name);
    Distsim(const char *in_mi_dbname_char, const char *in_sim_dbname_char, const char *in_wordlist_name_char);
    Distsim(const std::string &in_mi_dbname, const std::string &in_sim_dbname, const std::string &in_wordlist_name, bool compressed);
    Distsim(const char *in_mi_dbname_char, const char *in_sim_dbname_char, const char *in_wordlist_name_char, bool compressed);
    ~Distsim();
    void init(const std::string &in_mi_dbname, const std::string &in_sim_dbname, const std::string &in_wordlist_name, bool compressed);
    void init(const char *in_mi_dbname_char, const char *in_sim_dbname_char, const char *in_wordlist_name_char, bool compressed);
    void make_vector(std::string &word, std::vector<std::string> &vector);
    double calc_sim(std::string &word1, std::string &word2);
    double calc_sim(const char *word1_char, const char *word2_char);
    double calc_sim_from_mi(std::string &word1, std::string &word2);
    double revise_similarity(double sim);
};

}

#endif

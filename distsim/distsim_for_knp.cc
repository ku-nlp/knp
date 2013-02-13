#include "config.h"
#include "distsim_for_knp.h"
#include "distsim.h"

char *DistSimFile = "mi.db";
char *DistSimDB = "sim.db";
char *DistSimWordList = "wordlist.txt";
Dist::Distsim distsim;

void init_distsim() {
#ifdef HAVE_ZLIB_H
    distsim.init(DistSimFile, DistSimDB, DistSimWordList, true); // value is compressed
#else
    distsim.init(DistSimFile, DistSimDB, DistSimWordList, false);
#endif
}

double calc_distsim(char *word1, char *word2) {
    return distsim.calc_sim(word1, word2);
}

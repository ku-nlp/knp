#include "distsim_for_knp.h"
#include "distsim.h"

char *DistSimFile = "all.mi";
Dist::Distsim distsim;

void init_distsim() {
    distsim.init(DistSimFile);
}

double calc_distsim(char *word1, char *word2) {
    return distsim.calc_sim(word1, word2);
}

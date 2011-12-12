#include "common.h"
#include "distsim.h"
#include "cmdline.h"

void option_proc(cmdline::parser &option, int argc, char **argv) {
    option.add<std::string>("midb", 'm', "MIDB name (keymap or basename)", false, "all.mi");
    option.add("compressed", 'z', "database is compressed");
    option.parse_check(argc, argv);
}

int main(int argc, char** argv) {
    cmdline::parser option;
    option_proc(option, argc, argv);

    Dist::Distsim distsim(option.get<std::string>("midb"), option.exist("compressed") ? true : false);

    std::ifstream is(argv[1]); // input stream

    std::string buffer;
    while (getline(is ? is : cin, buffer)) {
        if (buffer.length() == 0 || buffer.at(0) == '#') // empty line or comment line
            continue;

        std::vector<std::string> word_pair;
        split_string(buffer, " ", word_pair);
        if (word_pair.size() == 2) {
            double sim = distsim.calc_sim(word_pair[0], word_pair[1]);
            cout << "Simpson+Jaccard similarity between \"" << word_pair[0] << "\" and \"" << word_pair[1] << "\": " << sim << endl;
        }
    }

    return 0;
}

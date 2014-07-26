#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include "config.h"
#include "opal_for_knp.h"

#define OPAL_MODEL_EXT ".model"
#define OPAL_TMP_FILE "/tmp/ttt.txt"

// opal::option opt(argc, argv);
const opal::option opt;
std::map<std::string, opal::Model*> model_map;

int init_opal(char *dir) {
    DIR* dp = opendir(dir);
    if (dp != NULL) {
        struct dirent* d_ent;
        do {
            d_ent = readdir(dp);
            if (d_ent != NULL && d_ent->d_type != DT_DIR) {
                std::string target_id = d_ent->d_name;
                std::string::size_type pos = target_id.rfind(OPAL_MODEL_EXT);
                if (pos != std::string::npos) // extract ID from \d+.model
                    target_id.erase(pos, strlen(OPAL_MODEL_EXT));
                std::cerr << "open opal model for " << target_id << std::endl;
                model_map[target_id] = new opal::Model(opt);
                std::string filepath = dir;
                filepath += "/";
                filepath += d_ent->d_name;
                model_map[target_id]->load(filepath.c_str());
            }
        } while (d_ent != NULL);
        closedir(dp);
    }
    return 1;
}

int opal_classify(char *target_id, char *line, char *ret_label) {
    opal::Model* target_model;
    char *label;

    if (model_map.find(target_id) == model_map.end())
        return 0;
    target_model = model_map[target_id];

    // line -> ex_t -> pool
    FILE *tmp_file = fopen(OPAL_TMP_FILE, "w");
    fputs(line, tmp_file);
    fclose(tmp_file);

    opal::null_pool <opal::ex_t> pool;
    pool.read(OPAL_TMP_FILE, target_model->get_lm(), target_model->get_fm());
    // if (_opt.kernel == POLY) pool.setup (_fm);
    label = (char *)target_model->test_one_example(pool, 3); // opt.output
    strcpy(ret_label, label);

    // ex
    // opal::ex_t *ex;
    // ex->read(line, target_model->get_lm());
    // label = (char *)target_model->test_one_example(ex, 3); // opt.output

    return 1;
}

void close_opal() {
    for (std::map<std::string, opal::Model*>::iterator it = model_map.begin(); it != model_map.end(); it++)
        delete it->second;
}

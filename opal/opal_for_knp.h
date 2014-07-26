#define USE_POLYK 1
#define MAX_NUM_CLASSES 65536
#define USE_MULTICLASS 1

#include "pa.h"

#ifdef __cplusplus
extern "C" {
#endif
    extern int init_opal(char *wsd_dir);
    extern int opal_classify(char *target_rep_id, char *line, char *label);
    extern void close_opal();
#ifdef __cplusplus
}
#endif

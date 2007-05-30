#ifdef __cplusplus
extern "C" {
#endif
    extern int init_crf_for_NE();
    extern void crf_add(char *line);
    extern void clear_crf();
    extern void crf_parse();
    extern void get_crf_prob(int i, int j, double *prob);
#ifdef __cplusplus
}
#endif

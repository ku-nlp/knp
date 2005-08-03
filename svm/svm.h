#ifdef __cplusplus
extern "C" {
#endif
    extern int init_svm_for_anaphora();
    extern double svm_classify_for_anaphora(char *line, int pp);
    extern int init_svm_for_NE();
    extern double svm_classify_for_NE(char *line, int pp);
#ifdef __cplusplus
}
#endif

#define PAT_BUF_INFO_BASE 11

#ifdef __cplusplus
extern "C" {
#endif
    extern void push_darts_file(char *basename);
    extern size_t da_search(int dic_no, char *str, char *rslt);
    extern int da_traverse(int dic_no, char *str, size_t *node_pos, size_t key_pos, size_t key_length, char key_type, char deleted_bytes, char *rslt);
    extern void close_darts();
#ifdef __cplusplus
}
#endif

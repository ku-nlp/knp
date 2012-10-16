#ifdef __cplusplus
extern "C" {
#endif
    extern int init_darts(char *filename);
    extern size_t search_ld(char *str, size_t *result_lengths, size_t *result_values);
    extern size_t traverse_ld(char *str, size_t *node_pos, size_t key_pos, size_t key_length, size_t *result_lengths, size_t *result_values);
    extern void close_darts();
#ifdef __cplusplus
}
#endif

/*====================================================================

				EXTERN

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/

extern int		Thesaurus;
extern int		ParaThesaurus;

extern int		Revised_para_num;

extern char		*ErrorComment;
extern char 		PM_Memo[];

extern char  		cont_str[];

extern int		IPALExist;
extern int		BGHExist;
extern int		SMExist;
extern int		SM2CODEExist;
extern int		SMP2SMGExist;
DBM_FILE		sm_db;
DBM_FILE		sm2code_db;
DBM_FILE		smp2smg_db;

extern NamedEntity	NE_data[];

extern SENTENCE_DATA	sentence_data[];

extern int 		match_matrix[][BNST_MAX];
extern int 		path_matrix[][BNST_MAX];
extern int		restrict_matrix[][BNST_MAX];
extern int 		Dpnd_matrix[][BNST_MAX];
extern int 		Quote_matrix[][BNST_MAX];
extern int 		Mask_matrix[][BNST_MAX];

extern char		G_Feature[][64];

extern TOTAL_MGR	Op_Best_mgr;

extern int 		OptAnalysis;
extern int		OptDisc;
extern int		OptDemo;
extern int 		OptInput;
extern int 		OptExpress;
extern int 		OptDisplay;
extern int 		OptExpandP;
extern int 		OptInhibit;
extern int		OptCheck;
extern int		OptNE;
extern int		OptSVM;
extern int		OptLearn;
extern int		OptCaseFlag;
extern int		OptCFMode;
extern char		OptIgnoreChar;
extern char		*OptOptionalCase;
extern VerboseType	VerboseLevel;

extern CLASS    	Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE     	Type[TYPE_NO];
extern FORM     	Form[TYPE_NO][FORM_NO];
extern int 		CLASS_num;

extern HomoRule 	HomoRuleArray[];
extern int 		CurHomoRuleSize;

extern KoouRule		KoouRuleArray[];
extern int		CurKoouRuleSize;
extern DpndRule		DpndRuleArray[];
extern int		CurDpndRuleSize;

extern MrphRule 	NERuleArray[];
extern int 		CurNERuleSize;
extern MrphRule 	CNpreRuleArray[];
extern int 		CurCNpreRuleSize;
extern MrphRule 	CNRuleArray[];
extern int 		CurCNRuleSize;
extern MrphRule 	CNauxRuleArray[];
extern int 		CurCNauxRuleSize;

extern BnstRule		ContRuleArray[];
extern int 		ContRuleSize;

extern DicForRule	*DicForRuleVArray;
extern int		CurDicForRuleVSize;
extern DicForRule	*DicForRulePArray;
extern int		CurDicForRulePSize;

void			*EtcRuleArray;
int			CurEtcRuleSize;

extern int DicForRuleDBExist;

extern char 		*Case_name[];
extern char		*ETAG_name[];

/* 各種スコア, コスト */
extern int	SOTO_SCORE;
extern int	EX_PRINT_NUM;
extern int	PrintDeletedSM;
extern int 	EX_match_score[];
extern int	EX_match_exact;

/* 関数プロトタイプ */

/* bind_juman.c */
extern void CloseJuman();
extern FILE *JumanSentence(FILE *fp);

/* case_analysis.c */
extern void realloc_cmm();
extern void init_case_frame(CASE_FRAME *cf);
extern void init_case_analysis();
extern int pp_kstr_to_code(char *cp);
extern int pp_hstr_to_code(char *cp);
extern char *pp_code_to_kstr(int num);
extern char *pp_code_to_hstr(int num);
extern int MatchPP(int n, char *pp);
extern void call_case_analysis(SENTENCE_DATA *sp, DPND dpnd);
extern void record_case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, ELLIPSIS_MGR *em_ptr, int lastflag);
extern void decide_voice(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void copy_cpm(CF_PRED_MGR *dst, CF_PRED_MGR *src, int flag);
extern void copy_cf_with_alloc(CASE_FRAME *dst, CASE_FRAME *src);
extern char *make_print_string(BNST_DATA *bp);
extern void InitCPMcache();
extern void ClearCPMcache();
extern void fix_sm_person(SENTENCE_DATA *sp);
extern int find_best_cf(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, int closest, int decide);
extern void assign_gaga_slot(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void assign_ga_subject(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void fix_sm_place(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);

/* case_data.c */
extern void make_data_cframe(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void set_pred_voice(BNST_DATA *b_ptr);
extern void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr);
extern void _make_data_cframe_ex(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr);

/* case_ipal.c */
extern void init_cf();
extern void init_cf2(SENTENCE_DATA *sp);
extern void close_cf();
extern void set_pred_caseframe(SENTENCE_DATA *sp);
extern void clear_cf(int flag);
extern void init_mgr_cf(TOTAL_MGR *tmp);
extern void clear_mgr_cf(SENTENCE_DATA *sp);
extern void MakeInternalBnst(SENTENCE_DATA *sp);
extern int _make_ipal_cframe_pp(CASE_FRAME *c_ptr, unsigned char *cp, int num);
extern int check_examples(char *cp, char **ex_list, int ex_num);
extern int check_cf_case(CASE_FRAME *cfp, char *pp);

/* case_match.c */
extern int comp_sm(char *cpp, char *cpd, int start);
extern int _sm_match_score(char *cpp, char *cpd, int flag);
extern int _ex_match_score(char *cp1, char *cp2);
extern int case_frame_match(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int flag, int closest);
extern int cf_match_element(char *d, char *target, int flag);
extern int count_pat_element(CASE_FRAME *cfp, LIST *list2);
extern int cf_match_exactly(BNST_DATA *d, char **ex_list, int ex_num, int *pos);
extern int check_case(CASE_FRAME *cf, int c);

/* case_print.c */
extern void print_data_cframe(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr);
extern void print_good_crrspnds(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int ipal_num);
extern void print_case_result(SENTENCE_DATA *sp);
extern void print_crrspnd(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr);

/* configfile.c */
extern char *check_dict_filename(char *file, int flag);
extern char *check_rule_filename(char *file);
extern void read_rc(FILE *in);
extern void init_configfile();
extern void server_read_rc(FILE *fp);

/* context.c */
extern void InitAnaphoraList();
extern void RegisterPredicate(char *key, int voice, int cf_addr, int pp, char *word, int flag);
extern void ClearSentences(SENTENCE_DATA *sp);
extern void PreserveCPM(SENTENCE_DATA *sp_new, SENTENCE_DATA *sp);
extern SENTENCE_DATA *PreserveSentence(SENTENCE_DATA *sp);
extern void DiscourseAnalysis(SENTENCE_DATA *sp);
extern void RegisterLastClause(int Snum, char *key, int pp, char *word, int flag);

/* corpus.c */
extern int CorpusExampleDependencyCalculation(SENTENCE_DATA *sp, BNST_DATA *ptr1, 
					      char *case1, int h, CHECK_DATA *list, 
					      CORPUS_DATA *corpus);
extern int subordinate_level_check_special(char *cp, BNST_DATA *ptr2);
extern int corpus_clause_comp(BNST_DATA *ptr1, BNST_DATA *ptr2, int para_flag);
extern int corpus_clause_barrier_check(BNST_DATA *ptr1, BNST_DATA *ptr2);
extern int corpus_case_predicate_check(BNST_DATA *ptr1, BNST_DATA *ptr2);
extern int corpus_barrier_check(BNST_DATA *ptr1, BNST_DATA *ptr2);
extern int corpus_optional_case_comp(SENTENCE_DATA *sp, BNST_DATA *ptr1, char *case1, 
				     BNST_DATA *ptr2, CORPUS_DATA *corpus);
extern int init_clause();
extern int init_case_pred();
extern int init_optional_case();
extern void optional_case_evaluation(SENTENCE_DATA *sp);
extern void CheckChildCaseFrame(SENTENCE_DATA *sp);
extern void unsupervised_debug_print(SENTENCE_DATA *sp);
extern void close_clause();
extern void close_case_pred();
extern void close_optional_case();

/* db.c */
extern char *db_get(DBM_FILE db, char *buf);
extern DBM_FILE db_read_open(char *filename);

/* dpnd_analysis.c */
extern void dpnd_info_to_bnst(SENTENCE_DATA *sp, DPND *dp);
extern int compare_dpnd(SENTENCE_DATA *sp, TOTAL_MGR *new, TOTAL_MGR *best);
extern int after_decide_dpnd(SENTENCE_DATA *sp);
extern void calc_dpnd_matrix(SENTENCE_DATA *sp);
extern int relax_dpnd_matrix(SENTENCE_DATA *sp);
extern void para_postprocess(SENTENCE_DATA *sp);
extern int detect_dpnd_case_struct(SENTENCE_DATA *sp);
extern void when_no_dpnd_struct(SENTENCE_DATA *sp);
extern void memo_by_program(SENTENCE_DATA *sp);

/* feature.c */
extern char *check_feature(FEATURE *fp, char *fname);
extern void assign_cfeature(FEATURE **fpp, char *fname);
extern int feature_pattern_match(FEATURE_PATTERN *fr, FEATURE *fd, void *p1, void *p2);
extern void print_feature(FEATURE *fp, FILE *filep);
extern int comp_feature(char *data, char *pattern);
extern void print_feature2(FEATURE *fp, FILE *filep);
extern void print_some_feature(FEATURE *fp, FILE *filep);
extern int feature_AND_match(FEATURE *fp, FEATURE *fd, void *p1, void *p2);
extern void string2feature_pattern(FEATURE_PATTERN *f, char *cp);
extern void assign_feature(FEATURE **fpp1, FEATURE **fpp2, void *ptr);
extern void list2feature_pattern(FEATURE_PATTERN *f, CELL *cell);
extern void list2feature(CELL *cp, FEATURE **fpp);
extern void clear_feature(FEATURE **fpp);
extern void append_feature(FEATURE **fpp, FEATURE *afp);

/* koou.c */
int koou(SENTENCE_DATA *sp);

/* lib_bgh.c */
extern char *_get_bgh(char *cp);
extern int bgh_code_match(char *c1, char *c2);
extern void overflowed_function(char *str, int max, char *function);
extern void init_bgh();
extern void close_bgh();

/* lib_lib.c */
extern int str_part_eq(char *dat, char *pat);

/* lib_print.c */
extern void print_kakari(SENTENCE_DATA *sp);
extern void _print_bnst(BNST_DATA *ptr);
extern void print_matrix(SENTENCE_DATA *sp, int type, int L_B);
extern void print_result(SENTENCE_DATA *sp);
extern void print_bnst(BNST_DATA *ptr, char *cp);
extern void check_bnst(SENTENCE_DATA *sp);
extern void print_para_relation(SENTENCE_DATA *sp);
extern void assign_para_similarity_feature(SENTENCE_DATA *sp);
extern void prepare_all_entity(SENTENCE_DATA *sp);

/* lib_scase.c */
extern void get_scase_code(BNST_DATA *ptr);
extern void init_scase();
extern void close_scase();

/* lib_sm.c */
extern char *_get_ntt(char *cp);
extern char *sm2code(char *cp);
extern char *code2sm(char *cp);
extern float ntt_code_match(char *c1, char *c2, int flag);
extern int sm_time_match(char *c);
extern void init_ntt();
extern void close_ntt();
extern char *_smp2smg(char *cp);
extern char *smp2smg(char *cpd, int flag);
extern int sm_fix(BNST_DATA *bp, char *targets);
extern void merge_smp2smg(BNST_DATA *bp);
extern void assign_sm_aux_feature(BNST_DATA *bp);
extern int sm_match_check(char *pat, char *codes);
extern int assign_sm(BNST_DATA *bp, char *cp);
extern int sm_code_depth(char *cp);
extern int sm_all_match(char *c, char *target);
extern int sm_check_match_max(char *exd, char *exp, int expand, char *target);
extern int delete_matched_sm(char *sm, char *del);
extern int delete_specified_sm(char *sm, char *del);

/* main.c */
extern int one_sentence_analysis(SENTENCE_DATA *sp, FILE *input);
extern void usage();

/* noun.c */
extern void init_noun();
extern void close_noun();

/* para_dpnd.c */
extern int _check_para_d_struct(SENTENCE_DATA *sp, int str, int end,
				int extend_p, int limit, int *s_p);
extern void init_mask_matrix(SENTENCE_DATA *sp);
extern int check_dpnd_in_para(SENTENCE_DATA *sp);

/* para_analysis.c */
extern void para_recovery(SENTENCE_DATA *sp);
extern int check_para_key(SENTENCE_DATA *sp);
extern void detect_all_para_scope(SENTENCE_DATA *sp);
extern int detect_para_scope(SENTENCE_DATA *sp, int para_num, int restrict_p);

/* para_relation.c */
extern int detect_para_relation(SENTENCE_DATA *sp);

/* para_revision.c */
extern void revise_para_rel(SENTENCE_DATA *sp, int pre, int pos);
extern void revise_para_kakari(SENTENCE_DATA *sp, int num, int *array);

/* para_similarity.c */
extern int subordinate_level_check(char *cp, BNST_DATA *ptr2);
extern int levelcmp(char *cp1, char *cp2);
extern int subordinate_level_comp(BNST_DATA *ptr1, BNST_DATA *ptr2);
extern int subordinate_level_forbid(char *cp, BNST_DATA *ptr2);
extern void calc_match_matrix(SENTENCE_DATA *sp);

/* proper.c */
extern char *check_class(MRPH_DATA *mp);
extern int check_correspond_NE(MRPH_DATA *data, char *rule);
extern void clearNE();
extern void init_proper(SENTENCE_DATA *s);
extern void assign_ntt_dict(SENTENCE_DATA *sp, int i);
extern void NE_analysis(SENTENCE_DATA *sp);
extern void preserveNE(SENTENCE_DATA *sp);
extern void printNE();
extern void close_proper();
extern void assign_ne_rule(SENTENCE_DATA *sp);
extern void NEparaAnalysis(SENTENCE_DATA *sp);

/* quote.c */
extern int quote(SENTENCE_DATA *sp);

/* read_data.c */
extern int read_mrph(SENTENCE_DATA *sp, FILE *fp);
extern void change_mrph(MRPH_DATA *m_ptr, FEATURE *f);
extern void assign_mrph_feature(MrphRule *s_r_ptr, int r_size,
				MRPH_DATA *s_m_ptr, int m_length,
				int mode, int break_mode, int direction);
extern void assign_general_feature(SENTENCE_DATA *sp, int flag);
extern int make_bunsetsu(SENTENCE_DATA *sp);
extern int make_bunsetsu_pm(SENTENCE_DATA *sp);
extern void print_mrphs(SENTENCE_DATA *sp, int flag);
extern void assign_dpnd_rule(SENTENCE_DATA *sp);
extern void _assign_general_feature(void *data, int size, int flag);

/* read_rule.c */
extern int case2num(char *cp);
extern char *get_rulev(char *cp);
extern void read_homo_rule(char *file_name);
extern void read_general_rule(RuleVector *rule);
extern void read_dpnd_rule(char *file_name);
extern void read_koou_rule(char *file_name);
extern void read_mrph_rule(char *file_name, MrphRule *rp, int *count, int max);
extern void read_bnst_rule(char *file_name, BnstRule *rp, int *count, int max);

/* regexp.c */
extern void store_regexpmrphs(REGEXPMRPHS **mspp, CELL *cell);
extern void store_regexpbnsts(REGEXPBNSTS **bspp, CELL *cell);
extern int regexpmrph_match(REGEXPMRPH *ptr1, MRPH_DATA *ptr2);
extern int regexpmrphrule_match(MrphRule *r_ptr, MRPH_DATA *d_ptr,
				int bw_length, int fw_length);
extern int regexpbnstrule_match(BnstRule *r_ptr, BNST_DATA *d_ptr,
				int bw_length, int fw_length);
extern int _regexpbnst_match(REGEXPMRPHS *r_ptr, BNST_DATA *b_ptr);

/* thesaurus.c */
extern void init_thesaurus();
extern void close_thesaurus();
extern char *get_str_code(unsigned char *cp, int flag);
extern void get_bnst_code(BNST_DATA *ptr, int flag);
extern float CalcSimilarity(char *exd, char *exp, int expand);
extern float CalcWordsSimilarity(char *exd, char **exp, int num, int *pos);
extern float CalcSmWordsSimilarity(char *smd, char **exp, int num, int *pos, char *del, int expand);

/* tools.c */
extern void *malloc_data(size_t size, char *comment);
extern void *realloc_data(void *ptr, size_t size, char *comment);
extern void init_hash();
extern int hash(unsigned char *key, int keylen);
extern unsigned char *katakana2hiragana(unsigned char *cp);

/* tree_conv.c */
extern int make_dpnd_tree(SENTENCE_DATA *sp);
extern void init_bnst_tree_property(SENTENCE_DATA *sp);

extern char **GetDefinitionFromBunsetsu(BNST_DATA *bp);
extern int ParseSentence(SENTENCE_DATA *s, char *input);

/* unsupervised.c */
extern void CheckCandidates(SENTENCE_DATA *sp);

/* KNP 初期化 */
extern char *Knprule_Dirname;
extern char *Knpdict_Dirname;
extern RuleVector *RULE;
extern int CurrentRuleNum;
extern int RuleNumMax;
extern char *DICT[];

extern GeneralRuleType *GeneralRuleArray;
extern int GeneralRuleNum;
extern int GeneralRuleMax;

/* Server Client extention */
extern FILE *Infp;
extern FILE *Outfp;
extern int   OptMode;

/*====================================================================
				 END
====================================================================*/

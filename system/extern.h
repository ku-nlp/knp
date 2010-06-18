/*====================================================================

				EXTERN

                                               S.Kurohashi 93. 5.31

    $Id$

====================================================================*/

extern int		Thesaurus;
extern int		ParaThesaurus;
extern THESAURUS_FILE	THESAURUS[];

extern int		Revised_para_num;

extern char		*ErrorComment;
extern char 		PM_Memo[];

extern int		CFExist;
extern int		CFSimExist;
extern int		BGHExist;
extern int		SMExist;
extern int		SM2CODEExist;
extern int		SMP2SMGExist;
DBM_FILE		sm_db;
DBM_FILE		sm2code_db;
DBM_FILE		smp2smg_db;

extern SENTENCE_DATA	sentence_data[];
extern SENTENCE_DATA	current_sentence_data;

extern ENTITY_MGR       entity_manager;

extern int 		match_matrix[][BNST_MAX];
extern int 		path_matrix[][BNST_MAX];
extern int		restrict_matrix[][BNST_MAX];
extern int 		Dpnd_matrix[][BNST_MAX];
extern int 		Quote_matrix[][BNST_MAX];
extern int 		Mask_matrix[][BNST_MAX];
extern double 		Para_matrix[][BNST_MAX][BNST_MAX];

/* store Chinese dpnd between words */
extern CHI_DPND        Chi_dpnd_matrix[][BNST_MAX];
extern CHI_POS        Chi_pos_matrix[];
extern CHI_ROOT          Chi_root_prob_matrix[];
extern char            *Chi_word_type[];
extern char            *Chi_word_pos[];
extern int left_arg[];
extern int right_arg[];

/* store count for gigaword pa pair */
/* for cell (i,j), i is the position of argument, j is the position of predicate */
extern double           Chi_pa_matrix[][BNST_MAX];  
 
extern int              Chi_np_start_matrix[][BNST_MAX];
extern int              Chi_np_end_matrix[][BNST_MAX];

extern int              Chi_quote_start_matrix[][BNST_MAX];
extern int              Chi_quote_end_matrix[][BNST_MAX];

extern int              Chi_root; /* store the root of this sentence */

extern char		**Options;
extern int 		OptAnalysis;
extern int		OptCKY;
extern int		OptEllipsis;
extern int		OptGeneralCF;
extern int		OptCorefer;
extern int 		OptInput;
extern int 		OptExpress;
extern int 		OptDisplay;
extern int 		OptArticle;
extern int 		OptDisplayNE;
extern int 		OptExpandP;
extern int		OptProcessParen;
extern int		OptCheck;
extern int		OptUseCF;
extern int		OptUseNCF;
extern int		OptUseCPNCF;
extern int		OptMergeCFResult;
extern int		OptUseRN;
extern int		OptUseScase;
extern int		OptUseSmfix;
extern int		OptDiscPredMethod;
extern int		OptDiscNounMethod;
extern int		OptLearn;
extern int		OptCaseFlag;
extern int		OptDiscFlag;
extern int		OptCFMode;
extern char		OptIgnoreChar;
extern int		OptReadFeature;
extern int		OptAddSvmFeatureUtype;
extern int		OptAddSvmFeatureDiscourseDepth;
extern int		OptAddSvmFeatureObjectRecognition;
extern int		OptAddSvmFeatureReferedNum;
extern int		OptNoCandidateBehind;
extern int		OptCopula;
extern int		OptPostProcess;
extern int		OptRecoverPerson;
extern int		OptNE;
extern int              OptNECRF;
extern int              OptReadNE;
extern int		OptNEcache;
extern int		OptNEend;
extern int		OptNEdelete;
extern int		OptNEcase;
extern int		OptNEparent;
extern int		OptNElearn;
extern int		OptAnaphora;
extern int		OptAnaphoraBaseline;
extern int		OptParaFix;
extern int		OptParaNoFixFlag;
extern int		OptNbest;
extern int		OptBeam;

// option for Chinese
// 1 means use generative model, use chidpnd_prob.db chi_dis_comma_*.cb chidpnd_stru.db
// 0 means use collins model, use chidpnd.db chi_pa.db
extern int              OptChiGenerative;

// option for Chinese
// 1 means do pos-tagging with parsing together
// 0 means only do parsing
extern int             OptChiPos; 

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

extern BnstRule		ContRuleArray[];
extern int 		ContRuleSize;

void			*EtcRuleArray;
int			CurEtcRuleSize;

extern char 		*Case_name[];
extern char		*ETAG_name[];

extern char		*SVMFile[];
extern char		*SVMFileNE[];
extern char		*CRFFileNE;
extern char             *DBforNE;
extern char		*DTFile[];
extern char             *SynonymFile;

extern int	DiscAddedCases[];
extern int	LocationLimit[];
extern int	PrevSentenceLimit;
extern int	LocationOrder[][LOC_NUMBER];

extern SMLIST	smlist[];

/* 各種スコア, コスト */
extern int	SOTO_SCORE;
extern int	EX_PRINT_NUM;
extern int	PrintFrequency;
extern int	PrintDeletedSM;
extern int	PrintFeatures;
extern int	PrintEx;
extern int	PrintNum;
extern int 	EX_match_score[];
extern int	EX_match_exact;
extern float	AntecedentDecideThresholdPredGeneral;
extern float	AntecedentDecideThresholdForGa;
extern float	AntecedentDecideThresholdForNi;
extern float	AntecedentDecideThresholdForNoun;
extern float	CFSimThreshold;
extern float	SVM_FREQ_SD;
extern float	SVM_FREQ_SD_NO;
extern float	SVM_R_NUM_S_SD;
extern float	SVM_R_NUM_E_SD;

extern void *matched_ptr;

extern int Language;

extern int sen_num;

/* 関数プロトタイプ */

/* anaphora.c */
extern void assign_mrph_num(SENTENCE_DATA *sp);
extern void anaphora_analysis(SENTENCE_DATA *sp);

/* bnst_compare.c */
extern int subordinate_level_comp(BNST_DATA *ptr1, BNST_DATA *ptr2);
extern int subordinate_level_check(char *cp, FEATURE *f);
extern int levelcmp(char *cp1, char *cp2);
extern void calc_match_matrix(SENTENCE_DATA *sp);

/* case_analysis.c */
extern TOTAL_MGR Work_mgr;
extern void realloc_cmm();
extern void init_case_frame(CASE_FRAME *cf);
extern void init_case_analysis_cmm();
extern int pp_kstr_to_code(char *cp);
extern int pp_hstr_to_code(char *cp);
extern char *pp_code_to_kstr(int num);
extern char *pp_code_to_hstr(int num);
extern char *pp_code_to_kstr_in_context(CF_PRED_MGR *cpm_ptr, int num);
extern int MatchPP(int n, char *pp);
extern int MatchPPn(int n, int *list);
extern int CF_MatchPP(int c, CASE_FRAME *cf);
extern int call_case_analysis(SENTENCE_DATA *sp, DPND dpnd);
extern void assign_case_component_feature(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, int temp_assign_flag);
extern void record_case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, ELLIPSIS_MGR *em_ptr, int temp_assign_flag);
extern void record_all_case_analisys(SENTENCE_DATA *sp, int temp_assign_flag);
extern void decide_voice(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void copy_cpm(CF_PRED_MGR *dst, CF_PRED_MGR *src, int flag);
extern void copy_cf_with_alloc(CASE_FRAME *dst, CASE_FRAME *src);
extern char *make_print_string(TAG_DATA *bp, int flag);
extern void InitCPMcache();
extern void ClearCPMcache();
extern void after_case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void record_match_ex(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void record_closest_cc_match(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void verb_lexical_disambiguation_by_case_analysis(CF_PRED_MGR *cpm_ptr);
extern void noun_lexical_disambiguation_by_case_analysis(CF_PRED_MGR *cpm_ptr);
extern int get_dist_from_work_mgr(BNST_DATA *bp, BNST_DATA *hp);
extern int get_closest_case_component(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern double find_best_cf(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, int closest, int decide, CF_PRED_MGR *para_cpm_ptr);

/* case_data.c */
extern char *make_fukugoji_id(BNST_DATA *b_ptr);
extern char *make_fukugoji_case_string(TAG_DATA *b_ptr);
extern int make_data_cframe_child(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, TAG_DATA *child_ptr, int child_num, int closest_flag);
extern int make_data_cframe_rentai(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern int make_data_cframe(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void set_pred_voice(BNST_DATA *b_ptr);
extern TAG_DATA *_make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr, int flag);
extern void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr);
extern void _make_data_cframe_ex(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr);

/* case_ipal.c */
extern void init_cf();
extern void init_case_analysis_cpm(SENTENCE_DATA *sp);
extern void close_cf();
extern void set_caseframes(SENTENCE_DATA *sp);
extern void clear_cf(int flag);
extern void init_mgr_cf(TOTAL_MGR *tmp);
extern void clear_mgr_cf(SENTENCE_DATA *sp);
extern int _make_ipal_cframe_pp(CASE_FRAME *c_ptr, unsigned char *cp, int num, int flag);
extern int check_examples(char *cp, int cp_len, char **ex_list, int ex_num);
extern int check_cf_case(CASE_FRAME *cfp, char *pp);
extern char *make_pred_string(TAG_DATA *t_ptr, MRPH_DATA *m_ptr, char *orig_form, int use_rep_flag, int flag);
extern void assign_pred_feature_to_bp(SENTENCE_DATA *sp);
extern char *feature2case(TAG_DATA *tp);
extern float get_cfs_similarity(char *cf1, char *cf2);
extern double get_cf_probability(CASE_FRAME *cfd, CASE_FRAME *cfp);
extern double get_cf_probability_for_pred(CASE_FRAME *cfd, CASE_FRAME *cfp);
extern double get_case_function_probability_for_pred(int as1, CASE_FRAME *cfd,
						     int as2, CASE_FRAME *cfp, int flag);
extern double get_case_function_probability(int as1, CASE_FRAME *cfd,
					    int as2, CASE_FRAME *cfp, int flag);
extern double get_case_interpret_probability(char *scase, char *cfcase, int ellipsis_flag);
extern double get_general_probability(char *key1, char *key2);
extern double get_class_probability(char *key, int as2, CASE_FRAME *cfp);
extern double get_key_probability(TAG_DATA *tag_ptr);
extern double get_case_probability_from_str(char *case_str, CASE_FRAME *cfp, int aflag, CF_PRED_MGR *para_cpm_ptr);
extern double get_case_probability(int as2, CASE_FRAME *cfp, int aflag, CF_PRED_MGR *para_cpm_ptr);
extern double get_case_num_probability(CASE_FRAME *cfp, int num, CF_PRED_MGR *para_cpm_ptr);
extern double get_ex_probability(int as1, CASE_FRAME *cfd, TAG_DATA *dp,
				 int as2, CASE_FRAME *cfp, int sm_flag);
extern double get_ex_probability_with_para(int as1, CASE_FRAME *cfd,
					   int as2, CASE_FRAME *cfp);
extern double get_ex_ne_probability(char *cp, int as2, CASE_FRAME *cfp, int flag);
extern double get_chi_pa(BNST_DATA *ptr1, BNST_DATA *ptr2, int dist);
extern double get_np_modifying_probability(int as1, CASE_FRAME *cfd);
extern double calc_vp_modifying_probability(TAG_DATA *gp, CASE_FRAME *g_cf, TAG_DATA *dp, CASE_FRAME *d_cf);
extern double calc_vp_modifying_num_probability(TAG_DATA *t_ptr, CASE_FRAME *cfp, int num);
extern double calc_adv_modifying_probability(TAG_DATA *gp, CASE_FRAME *cfp, TAG_DATA *dp);
extern double calc_adv_modifying_num_probability(TAG_DATA *t_ptr, CASE_FRAME *cfp, int num);
extern double get_topic_generating_probability(int have_topic, TAG_DATA *g_ptr);
extern double get_para_exist_probability(char *para_key, double score, int flag, TAG_DATA *dp, TAG_DATA *gp);
extern double get_para_ex_probability(char *para_key, double score, TAG_DATA *dp, TAG_DATA *gp);
extern double get_noun_co_ex_probability(TAG_DATA *dp, TAG_DATA *gp);
extern double get_noun_co_num_probability(TAG_DATA *gp, int num, CKY *para_cky_ptr);
extern char *malloc_db_buf(int size);

/* case_match.c */
extern int comp_sm(char *cpp, char *cpd, int start);
extern int _sm_match_score(char *cpp, char *cpd, int flag);
extern int case_frame_match(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int flag, int closest, CF_PRED_MGR *para_cpm_ptr);
extern int cf_match_element(char *d, char *target, int flag);
extern int count_pat_element(CASE_FRAME *cfp, LIST *list2);
extern int count_assigned_adjacent_element(CASE_FRAME *cfp, LIST *list2);
extern int check_case(CASE_FRAME *cf, int c);
extern int cf_match_sm_thesaurus(TAG_DATA *tp, CASE_FRAME *cfp, int n);
extern float calc_similarity_word_cf(TAG_DATA *tp, CASE_FRAME *cfp, int n, int *pos);
extern float calc_similarity_word_cf_with_sm(TAG_DATA *tp, CASE_FRAME *cfp, int n, int *pos);
extern int dat_match_sm(int as1, CASE_FRAME *cfd, TAG_DATA *tp, char *sm);
extern int cf_match_exactly(char *word, int word_len, char **ex_list, int ex_num, int *pos);
extern float _calc_similarity_sm_cf(char *exd, int expand, char *unmatch_word, 
				    CASE_FRAME *cfp, int n, int *pos);
extern int sms_match(char *cpp, char *cpd, int expand);

/* case_print.c */
extern void print_data_cframe(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr);
extern void print_good_crrspnds(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int ipal_num);
extern void print_crrspnd(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr);
extern void print_pa_structure(SENTENCE_DATA *sp);

/* configfile.c */
extern char *check_dict_filename(char *file, int flag);
extern char *check_rule_filename(char *file);
extern DBM_FILE open_dict(int dic_num, char *dic_name, int *exist);
extern void init_configfile(char *opfile);
extern void server_read_rc(FILE *fp);

/* context.c */
extern void InitAnaphoraList();
extern void RegisterPredicate(char *key, int voice, int cf_addr, 
			      int pp, char *pp_str, char *word, int sent_n, int tag_n, int flag);
extern ELLIPSIS_COMPONENT *CheckEllipsisComponent(ELLIPSIS_COMPONENT *ccp, char *pp_str);
extern void ClearSentences(SENTENCE_DATA *sp);
extern void ClearSMList();
extern void PreserveCPM(SENTENCE_DATA *sp_new, SENTENCE_DATA *sp);
extern SENTENCE_DATA *PreserveSentence(SENTENCE_DATA *sp);
extern void DiscourseAnalysis(SENTENCE_DATA *sp);
extern void RegisterLastClause(int Snum, char *key, int pp, char *word, int flag);
extern char *loc_code_to_str(int loc);
extern int loc_name_to_code(char *loc);
extern int mark_location_classes(SENTENCE_DATA *sp, TAG_DATA *tp);

/* db.c */
extern char *db_get(DBM_FILE db, char *buf);
extern DBM_FILE db_read_open(char *filename);

/* dpnd_analysis.c */
extern void init_chi_dpnd_db();
extern void close_chi_dpnd_db();
extern void init_chi_type();
extern void free_chi_type();
extern void init_chi_pos();
extern void free_chi_pos();
extern void dpnd_info_to_bnst(SENTENCE_DATA *sp, DPND *dp);
extern void dpnd_info_to_tag(SENTENCE_DATA *sp, DPND *dp);
extern void dpnd_info_to_mrph(SENTENCE_DATA *sp);
extern int compare_dpnd(SENTENCE_DATA *sp, TOTAL_MGR *new_mgr, TOTAL_MGR *best_mgr);
extern int after_decide_dpnd(SENTENCE_DATA *sp);
extern void calc_dpnd_matrix(SENTENCE_DATA *sp);
extern void calc_chi_dpnd_matrix_forProbModel(SENTENCE_DATA *sp);
extern void calc_chi_dpnd_matrix_wpos(SENTENCE_DATA *sp);
extern void calc_chi_pos_matrix(SENTENCE_DATA *sp);
extern int relax_dpnd_matrix(SENTENCE_DATA *sp);
extern void tag_bnst_postprocess(SENTENCE_DATA *sp, int flag);
extern void undo_tag_bnst_postprocess(SENTENCE_DATA *sp);
extern void para_postprocess(SENTENCE_DATA *sp);
extern int detect_dpnd_case_struct(SENTENCE_DATA *sp);
extern void when_no_dpnd_struct(SENTENCE_DATA *sp);
extern void check_candidates(SENTENCE_DATA *sp);
extern void memo_by_program(SENTENCE_DATA *sp);
extern void calc_gigaword_pa_matrix(SENTENCE_DATA *sp);
extern double get_case_prob(SENTENCE_DATA *sp, int head, int left_arg_num, int right_arg_num);
extern double get_case_prob_wpos(SENTENCE_DATA *sp, int head, int left_arg_num, int right_arg_num, int pos_index_pre);
extern double get_lex_case_prob_wpos(SENTENCE_DATA *sp, int head, int left_arg_num, int right_arg_num, int pos_index_pre);

/* feature.c */
extern char *check_feature(FEATURE *fp, char *fname);
extern int check_category(FEATURE *fp, char *fname);
extern void assign_cfeature(FEATURE **fpp, char *fname, int temp_assign_flag);
extern int feature_pattern_match(FEATURE_PATTERN *fr, FEATURE *fd, void *p1, void *p2);
extern void print_one_feature(char *cp, FILE *filep);
extern void print_feature(FEATURE *fp, FILE *filep);
extern int comp_feature(char *data, char *pattern);
extern void print_feature2(FEATURE *fp, FILE *filep);
extern void print_some_feature(FEATURE *fp, FILE *filep);
extern int feature_AND_match(FEATURE *fp, FEATURE *fd, void *p1, void *p2);
extern void string2feature_pattern(FEATURE_PATTERN *f, char *cp);
extern void assign_feature(FEATURE **fpp1, FEATURE **fpp2, void *ptr, int offset, int length, int temp_assign_flag);
extern void list2feature_pattern(FEATURE_PATTERN *f, CELL *cell);
extern void list2feature(CELL *cp, FEATURE **fpp);
extern void clear_feature(FEATURE **fpp);
extern void append_feature(FEATURE **fpp, FEATURE *afp);
extern void delete_cfeature(FEATURE **fpp, char *type);
extern void delete_alt_feature(FEATURE **fpp);
extern void delete_cfeature_from_mrphs(MRPH_DATA *m_ptr, int length, char *type);
extern void copy_feature(FEATURE **dst_fpp, FEATURE *src_fp);
extern int check_str_type(unsigned char *ucp);
extern char* get_feature_for_chi (BNST_DATA *p_ptr);

/* koou.c */
int koou(SENTENCE_DATA *sp);

/* lib_bgh.c */
extern char *_get_bgh(char *cp, char *arg);
extern int bgh_code_match(char *c1, char *c2);
extern int bgh_code_match_for_case(char *cp1, char *cp2);
extern int bgh_match_check(char *pat, char *codes);
extern void init_bgh();
extern void close_bgh();

/* lib_dt.c */
extern float dt_classify(char *data, int pp);

/* lib_event.c */
extern float get_event_value(SENTENCE_DATA *sp1, TAG_DATA *p1, 
			     SENTENCE_DATA *sp2, TAG_DATA *p2);
extern float get_cf_event_value(CASE_FRAME *cf1, CASE_FRAME *cf2);

/* lib_lib.c */
extern int str_part_eq(char *dat, char *pat);

/* lib_print.c */
extern void print_kakari(SENTENCE_DATA *sp, int type);
extern void _print_bnst(TAG_DATA *ptr);
extern void print_matrix(SENTENCE_DATA *sp, int type, int L_B);
extern void print_result(SENTENCE_DATA *sp, int case_print_flag);
extern void print_bnst(BNST_DATA *ptr, char *cp);
extern void check_bnst(SENTENCE_DATA *sp);
extern void print_para_relation(SENTENCE_DATA *sp);
extern void assign_para_similarity_feature(SENTENCE_DATA *sp);
extern void prepare_all_entity(SENTENCE_DATA *sp);
extern void print_tree_for_chinese(SENTENCE_DATA *sp);
extern void do_postprocess(SENTENCE_DATA *sp);
extern void print_all_result(SENTENCE_DATA *sp);

/* lib_scase.c */
extern void get_scase_code(BNST_DATA *ptr);
extern void init_scase();
extern void close_scase();

/* lib_sm.c */
extern char *_get_ntt(char *cp, char *arg);
extern char *_process_code(char *code, char *arg);
extern int _code_match(char *code1,char *code2);
extern char *_code_flow(char *code);
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
extern int sm_match_check(char *pat, char *codes, int expand);
extern int assign_sm(BNST_DATA *bp, char *cp);
extern int sm_all_match(char *c, char *target);
extern int sm_check_match_max(char *exd, char *exp, int expand, char *target);
extern int delete_matched_sm(char *sm, char *del);
extern int delete_specified_sm(char *sm, char *del);
extern void fix_sm_person(SENTENCE_DATA *sp);
extern void fix_sm_place(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void assign_ga_subject(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void specify_sm_from_cf(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr);
extern void sm2feature(SENTENCE_DATA *sp);
extern char *check_noun_sm(char *key);

/* main.c */
extern void usage();

/* para_dpnd.c */
extern int _check_para_d_struct(SENTENCE_DATA *sp, int str, int end,
				int extend_p, int limit, int *s_p);
extern void init_mask_matrix(SENTENCE_DATA *sp);
extern void init_para_matrix(SENTENCE_DATA *sp);
extern int check_dpnd_in_para(SENTENCE_DATA *sp);

/* para_analysis.c */
extern int para_recovery(SENTENCE_DATA *sp);
extern int check_para_key(SENTENCE_DATA *sp);
extern void detect_all_para_scope(SENTENCE_DATA *sp);
extern int detect_para_scope(SENTENCE_DATA *sp, int para_num, int restrict_p);

/* para_relation.c */
extern int detect_para_relation(SENTENCE_DATA *sp);

/* para_revision.c */
extern void revise_para_rel(SENTENCE_DATA *sp, int pre, int pos);
extern void revise_para_kakari(SENTENCE_DATA *sp, int num, int *array);

/* proper.c */
extern void ne_para_analysis(SENTENCE_DATA *sp);
extern int ne_tagposition_to_code(char *cp);
extern char *ne_code_to_tagposition(int num);

/* quote.c */
extern int quote(SENTENCE_DATA *sp);
extern int process_input_paren(SENTENCE_DATA *sp, SENTENCE_DATA **paren_spp);
extern void prepare_paren_sentence(SENTENCE_DATA *sp, SENTENCE_DATA *paren_sp);

/* read_data.c */
extern int imi2feature(char *str, MRPH_DATA *m_ptr);
extern int read_mrph(SENTENCE_DATA *sp, FILE *fp);
extern void change_one_mrph_rep(MRPH_DATA *m_ptr, int modify_feature_flag, char suffix_char);
extern void change_mrph(MRPH_DATA *m_ptr, FEATURE *f);
extern void assign_general_feature(void *data, int size, int flag, int also_assign_flag, int temp_assign_flag);
extern int make_bunsetsu(SENTENCE_DATA *sp);
extern int make_bunsetsu_pm(SENTENCE_DATA *sp);
extern void print_mrphs_only(SENTENCE_DATA *sp);
extern void print_bnst_with_mrphs(SENTENCE_DATA *sp, int flag);
extern void assign_dpnd_rule(SENTENCE_DATA *sp);
extern int calc_bnst_length(SENTENCE_DATA *sp, BNST_DATA *b_ptr);
extern void make_tag_units(SENTENCE_DATA *sp);
extern void assign_feature_for_tag(SENTENCE_DATA *sp);
extern void assign_feature_alt_mrph(FEATURE **fpp, MRPH_DATA *m_ptr);
extern void delete_existing_features(MRPH_DATA *m_ptr);
extern void copy_mrph(MRPH_DATA *dst, MRPH_DATA *src);
extern char *get_mrph_rep(MRPH_DATA *m_ptr);
extern char *get_mrph_rep_from_f(MRPH_DATA *m_ptr, int flag);
extern int get_mrph_rep_length(char *rep_strt);
extern char *make_mrph_rn(MRPH_DATA *m_ptr);
extern char *get_bnst_head_canonical_rep(BNST_DATA *ptr, int compound_flag);
extern void assign_cc_feature_to_bp(SENTENCE_DATA *sp);
extern void assign_cc_feature_to_bnst(SENTENCE_DATA *sp);
extern void preprocess_mrph(SENTENCE_DATA *sp);

/* read_rule.c */
extern int case2num(char *cp);
extern char *get_rulev(char *cp);
extern void read_homo_rule(char *file_name);
extern void read_general_rule(RuleVector *rule);
extern void read_dpnd_rule(char *file_name);
extern void read_dpnd_rule_for_chinese(char *file_name);
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
extern int regexptagrule_match(BnstRule *r_ptr, TAG_DATA *d_ptr, 
			       int bw_length, int fw_length);
extern int _regexpbnst_match(REGEXPMRPHS *r_ptr, BNST_DATA *b_ptr);

/* thesaurus.c */
extern void init_thesaurus();
extern void close_thesaurus();
extern char *get_str_code(unsigned char *cp, int flag);
extern void get_bnst_code_all(BNST_DATA *ptr);
extern void get_bnst_code(BNST_DATA *ptr, int flag);
extern int code_depth(char *cp, int code_size);
extern float general_code_match(THESAURUS_FILE *th, char *c1, char *c2);
extern float calc_similarity(char *exd, char *exp, int expand);
extern float calc_words_similarity(char *exd, char **exp, int num, int *pos);
extern float calc_sm_words_similarity(char *smd, char **exp, int num, int *pos, char *del, int expand, char *unmatch_word);
extern void overflowed_function(char *str, int max, char *function);
extern char *get_most_similar_code(char *exd, char *exp);
extern char *get_str_code_with_len(char *cp, int len, int flag);

/* tools.c */
extern void *malloc_data(size_t size, char *comment);
extern void *realloc_data(void *ptr, size_t size, char *comment);
extern void init_hash();
extern int hash(unsigned char *key, int keylen);
extern unsigned char *katakana2hiragana(unsigned char *cp);
extern unsigned char *hiragana2katakana(unsigned char *cp);
extern char *strdup_with_check(const char *s);

/* tree_conv.c */
extern int make_dpnd_tree(SENTENCE_DATA *sp);
extern void init_bnst_tree_property(SENTENCE_DATA *sp);
extern BNST_DATA *t_add_node(BNST_DATA *parent, BNST_DATA *child, int pos);
extern MRPH_DATA *find_head_mrph_from_dpnd_bnst(BNST_DATA *dep_ptr, BNST_DATA *gov_ptr);

extern char **GetDefinitionFromBunsetsu(BNST_DATA *bp);
extern int ParseSentence(SENTENCE_DATA *s, char *input);

/* cky.c */
extern int cky(SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr);

/* base_phrase.c */
extern int base_phrase(SENTENCE_DATA *sp, int is_frag);
extern int fragment(SENTENCE_DATA *sp);

/* similarity.c */
extern void init_hownet();
extern float  similarity_chinese(char* str1, char* str2);

/* dic.c */
extern int check_auto_dic(MRPH_DATA *m_ptr, int m_length, char *value);

/* nv_mi.c */
extern int check_nv_mi_parent_and_children(TAG_DATA *v_ptr, int rank_threshold);

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

/* context.c for anaphra.c */
extern char *get_pred_id(char *cfid);
extern CFLIST *CheckCF(char *key);

/*====================================================================
				 END
====================================================================*/

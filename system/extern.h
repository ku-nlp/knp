/*====================================================================

				EXTERN

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/

extern SENTENCE_DATA 	sentence_data[256];

extern int		Thesaurus;

extern int		Process_type;
extern int		Revised_para_num;

extern char 		Comment[];
extern char		*ErrorComment;
extern char 		PM_Memo[];

extern char  		cont_str[];

extern int		IPALExist;
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
extern int 		OptInput;
extern int 		OptExpress;
extern int 		OptDisplay;
extern int 		OptExpandP;
extern int 		OptInhibit;
extern int		OptCheck;
extern int		OptNE;
extern int		OptLearn;
extern int		OptCaseFlag;
extern int		OptCFMode;
extern char		OptIgnoreChar;
extern char		*OptOptionalCase;
extern VerboseType	VerboseLevel;

extern enum OPTION1	Option1;
extern enum OPTION2	Option2;
extern enum OPTION3	Option3;

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

/* 関数プロトタイプ */
extern int read_mrph(SENTENCE_DATA *sp, FILE *fp);
extern char *get_bgh(char *cp);
extern char *db_get(DBM_FILE db, char *buf);

/* case_analysis.c */
extern void realloc_cmm();
extern void init_case_analysis();
extern int pp_kstr_to_code(char *cp);
extern int pp_hstr_to_code(char *cp);
extern char *pp_code_to_kstr(int num);
extern char *pp_code_to_hstr(int num);
extern void call_case_analysis(SENTENCE_DATA *sp, DPND dpnd);
extern void record_case_analysis(SENTENCE_DATA *sp);

/* case_ipal.c */
extern void init_cf();
extern void init_cf2(SENTENCE_DATA *sp);
extern void init_cf3(SENTENCE_DATA *sp);
extern void close_cf();
extern int check_examples(unsigned char *cp, unsigned char *list);
extern void set_pred_caseframe(SENTENCE_DATA *sp);
extern void clear_cf();

/* case_match.c */
extern int comp_sm(char *cpp, char *cpd, int start);
extern int _sm_match_score(char *cpp, char *cpd, int flag);
extern int _ex_match_score(char *cp1, char *cp2);
extern void case_frame_match(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int flag);

/* lib_sm.c */
extern char *sm2code(char *cp);
extern float ntt_code_match(char *c1, char *c2);

extern char **GetDefinitionFromBunsetsu(BNST_DATA *bp);
extern int ParseSentence(SENTENCE_DATA *s, char *input);

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

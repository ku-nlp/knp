/*====================================================================

				EXTERN

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
extern SENTENCE_DATA 	*sp;
extern SENTENCE_DATA 	sentence_data[256];

extern int		Thesaurus;

extern int		Process_type;
extern int		Para_num;
extern int		Para_M_num;
extern int		Revised_para_num;

extern char 		Comment[];
extern char		KNPSID[];
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

extern TOTAL_MGR	Best_mgr;
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
extern int		OptCFMode;
extern char		OptIgnoreChar;
extern char		*OptOptionalCase;

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

extern char *get_bgh(char *cp);
extern char *db_get(DBM_FILE db, char *buf);

/* KNP ½é´ü²½ */
extern char *Knprule_Dirname;
extern char *Knpdict_Dirname;
extern RuleVector *RULE;
extern int CurrentRuleNum;
extern int RuleNumMax;
extern char *DICT[];

extern GeneralRuleType *GeneralRuleArray;
extern int GeneralRuleNum;
extern int GeneralRuleMax;

/*====================================================================
				 END
====================================================================*/

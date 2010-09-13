/*====================================================================

		     KNP (Kurohashi-Nagao Parser)

    $Id$
====================================================================*/
#include "knp.h"

SENTENCE_DATA	current_sentence_data;
SENTENCE_DATA	sentence_data[SENTENCE_MAX];
SENTENCE_DATA	*paren_sentence_data;

MRPH_DATA 	mrph_data[MRPH_MAX];		/* 形態素データ */
BNST_DATA 	bnst_data[BNST_MAX];		/* 文節データ */
TAG_DATA 	tag_data[MRPH_MAX];		/* タグ単位データ */
PARA_DATA 	para_data[PARA_MAX]; 		/* 並列データ */
PARA_MANAGER	para_manager[PARA_MAX];		/* 並列管理データ */
TOTAL_MGR	Best_mgr;			/* 依存・格解析管理データ */
TOTAL_MGR	Op_Best_mgr;
ENTITY_MGR      entity_manager;                 /* ENTITY管理データ */

int 		Revised_para_num;			

char		*ErrorComment = NULL;		/* エラーコメント */
char		PM_Memo[DATA_LEN];		/* パターンマッチ結果 */

int		match_matrix[BNST_MAX][BNST_MAX];
int		path_matrix[BNST_MAX][BNST_MAX];
int		restrict_matrix[BNST_MAX][BNST_MAX];
int 		Dpnd_matrix[BNST_MAX][BNST_MAX]; /* 係り可能性 0, D, P, A */
int 		Quote_matrix[BNST_MAX][BNST_MAX];/* 括弧マスク 0, 1 */
int 		Mask_matrix[BNST_MAX][BNST_MAX]; /* 並列マスク
						    0:係り受け禁止
						    1:係り受けOK
						    2:並列のhead間,
						    3:並列のgapとhead間 */
double 		Para_matrix[PARA_MAX][BNST_MAX][BNST_MAX];
double          Chi_pa_matrix[BNST_MAX][BNST_MAX];  
int             Chi_np_start_matrix[BNST_MAX][BNST_MAX];
int             Chi_np_end_matrix[BNST_MAX][BNST_MAX];
int             Chi_quote_start_matrix[BNST_MAX][BNST_MAX];
int             Chi_quote_end_matrix[BNST_MAX][BNST_MAX];
CHI_DPND        Chi_dpnd_matrix[BNST_MAX][BNST_MAX];
CHI_POS        Chi_pos_matrix[BNST_MAX];
CHI_ROOT          Chi_root_prob_matrix[BNST_MAX];
int             Chi_root;
char            *Chi_word_type[CHI_POS_MAX];
char            *Chi_word_pos[CHI_POS_MAX];
int left_arg[CHI_ARG_NUM_MAX + 1];
int right_arg[CHI_ARG_NUM_MAX + 1];

char		**Options;
int 		OptAnalysis;
int		OptCKY;
int		OptEllipsis;
int             OptGeneralCF;
int		OptCorefer;
int 		OptInput;
int 		OptExpress;
int 		OptDisplay;
int 		OptDisplayNE;
int             OptArticle;
int		OptExpandP;
int		OptProcessParen;
int		OptCheck;
int             OptUseCF;
int             OptUseNCF;
int             OptUseCPNCF;
int             OptMergeCFResult;
int		OptUseRN;
int		OptDiscPredMethod;
int		OptDiscNounMethod;
int		OptLearn;
int		OptCaseFlag;
int		OptDiscFlag;
int		OptCFMode;
int		OptServerFlag;
char		OptIgnoreChar;
int		OptReadFeature;
int		OptAddSvmFeatureUtype;
int		OptAddSvmFeatureDiscourseDepth;
int		OptAddSvmFeatureObjectRecognition;
int		OptAddSvmFeatureReferedNum;
int		OptNoCandidateBehind;
int		OptCopula;
int		OptPostProcess;
int		OptRecoverPerson;
int		OptNE;
int             OptNECRF;
int             OptReadNE;
int		OptNEcache;
int		OptNEend;
int		OptNEdelete;
int		OptNEcase;
int		OptNEparent;
int		OptNElearn;
int		OptAnaphora;
int		OptAnaphoraBaseline;
int		OptTimeoutExit;
int		OptParaFix;
int		OptParaNoFixFlag;
int		OptNbest;
int		OptBeam;

// option for Chinese
// 1 means use generative model, use chidpnd_prob.db chi_dis_comma_*.cb chidpnd_stru.db
// 0 means use collins model, use chidpnd.db chi_pa.db
int             OptChiGenerative; 

// option for Chinese
// 1 means do pos-tagging with parsing together
// 0 means only do parsing
int             OptChiPos; 

int             PrintNum;
VerboseType	VerboseLevel = VERBOSE0;

/* sentence id, only for Chinese */
int             sen_num;

/* Chinese fragment tag */
int             is_frag;

/* DB file for Chinese dpnd rule */
DBM_FILE chi_dpnd_db;
int     CHIDpndExist;

/* Server Client Extention */
static int	sfd, fd;
int		OptMode = STAND_ALONE_MODE;
int		OptPort = DEFAULT_PORT;
char		OptHostname[SMALL_DATA_LEN];

int		Language;

FILE		*Infp;
FILE		*Outfp;

char *Case_name[] = {
    		"ガ格", "ヲ格", "ニ格", "デ格", "カラ格", 
		"ト格", "ヨリ格", "ヘ格", "マデ格", "ノ格",
		"ガ２", ""};

char 		*ProgName;
extern FILE	*Jumanrc_Fileptr;
extern CLASS    Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE     Type[TYPE_NO];
extern FORM     Form[TYPE_NO][FORM_NO];
int CLASS_num;

jmp_buf timeout;
int ParseTimeout = DEFAULT_PARSETIMEOUT;
char *Opt_knprc = NULL;

extern int	SOTO_THRESHOLD;
extern int	DISTANCE_STEP;
extern int	RENKAKU_STEP;
extern int	STRONG_V_COST;
extern int	ADJACENT_TOUTEN_COST;
extern int	LEVELA_COST;
extern int	TEIDAI_STEP;
extern int	EX_match_qua;
extern int	EX_match_unknown;
extern int	EX_match_sentence;
extern int	EX_match_tim;
extern int	EX_match_subject;

/*==================================================================*/
			     void usage()
/*==================================================================*/
{
    fprintf(stderr, "Usage: knp [-case|dpnd|bnst|ellipsis|demonstrative|anaphora]\n" 
#ifdef USE_SVM
	    "           [-ellipsis-svm|demonstrative-svm|anaphora-svm]\n" 
#endif
	    "           [-tree|bnsttree|sexp|tab|bnsttab]\n" 
	    "           [-normal|detail|debug]\n" 
	    "           [-expand]\n"
	    "           [-C host:port] [-S|F] [-N port]\n"
	    "           [-timeout second] [-r rcfile]\n");
    exit(1);    
}

/*==================================================================*/
	       void option_proc(int argc, char **argv)
/*==================================================================*/
{
    int i, count = 0;

    /* 引数処理 */

    Language = JAPANESE;
    OptAnalysis = OPT_CASE;
    OptCKY = TRUE;
    OptEllipsis = 0;
    OptGeneralCF = 0;
    OptCorefer = 4;
    OptInput = OPT_RAW;
    OptExpress = OPT_TREE;
    OptDisplay = OPT_NORMAL;
    OptDisplayNE = OPT_NORMAL;
    OptArticle = FALSE;
    OptExpandP = FALSE;
    OptCFMode = EXAMPLE;
    OptProcessParen = FALSE;
    OptCheck = FALSE;
    OptUseCF = TRUE;
    OptUseNCF = TRUE;
    OptUseCPNCF = TRUE;
    OptMergeCFResult = TRUE;
    OptUseRN = USE_RN;
    OptUseScase = TRUE;
    OptUseSmfix = TRUE;
    OptDiscPredMethod = OPT_NORMAL;
    OptDiscNounMethod = OPT_NORMAL;
    OptLearn = FALSE;
    OptCaseFlag = OPT_CASE_USE_REP_CF | OPT_CASE_USE_CREP_CF | OPT_CASE_USE_CN_CF | OPT_CASE_USE_PROBABILITY | OPT_CASE_ADD_SOTO_WORDS | OPT_CASE_GENERALIZE_AGENT | OPT_CASE_FIX_CF_SEARCH;
    OptDiscFlag = 0;
    OptServerFlag = 0;
    OptIgnoreChar = '\0';
    OptReadFeature = 0;
    OptAddSvmFeatureUtype = 0;
    OptAddSvmFeatureDiscourseDepth = 0;
    OptAddSvmFeatureObjectRecognition = 0;
    OptAddSvmFeatureReferedNum = 0;
    OptNoCandidateBehind = 0;
    OptCopula = 0;
    OptPostProcess = 0;
    OptRecoverPerson = 0;
    OptNE = 0;
    OptNECRF = 0;
    OptReadNE = 0;
    OptNEcache = 0;
    OptNEend = 0;
    OptNEdelete = 0;
    OptNEcase = 0;
    OptNElearn = 0;
    OptNEparent = 0;
    OptAnaphora = 0;
    OptAnaphoraBaseline = 0;
    OptTimeoutExit = 0;
    OptParaFix = TRUE;
    OptParaNoFixFlag = 0;
    OptNbest = 0;
    OptBeam = 0;
    OptChiGenerative = 0;
    OptChiPos = 0;

    /* オプションの保存 */
    Options = (char **)malloc_data(sizeof(char *) * argc, "option_proc");
    for (i = 1; i < argc; i++) {
	if (**(argv + i) == '-') {
	    *(Options + count++) = strdup(*(argv + i) + 1);
	}
    }
    *(Options + count) = NULL;

    while ((--argc > 0) && ((*++argv)[0] == '-')) {
	if (str_eq(argv[0], "-case"))         OptAnalysis = OPT_CASE;
	else if (str_eq(argv[0], "-case2"))   OptAnalysis = OPT_CASE2;
	else if (str_eq(argv[0], "-cfsm"))    OptCFMode   = SEMANTIC_MARKER;
	else if (str_eq(argv[0], "-tree"))    OptExpress  = OPT_TREE;
	else if (str_eq(argv[0], "-bptree"))  OptExpress  = OPT_TREE;
	else if (str_eq(argv[0], "-treef"))   OptExpress  = OPT_TREEF;
	else if (str_eq(argv[0], "-sexp"))    OptExpress  = OPT_SEXP;
	else if (str_eq(argv[0], "-tab"))     OptExpress  = OPT_TAB;
	else if (str_eq(argv[0], "-tag"))     OptExpress  = OPT_TAB;
	else if (str_eq(argv[0], "-tagtab"))  OptExpress  = OPT_TAB;
	else if (str_eq(argv[0], "-bptab"))   OptExpress  = OPT_TAB;
	else if (str_eq(argv[0], "-notag"))   OptExpress  = OPT_NOTAG;
	else if (str_eq(argv[0], "-notagtab")) OptExpress = OPT_NOTAG;
	else if (str_eq(argv[0], "-bnsttab")) OptExpress  = OPT_NOTAG;
	else if (str_eq(argv[0], "-bnsttree")) OptExpress = OPT_BNSTTREE;
	else if (str_eq(argv[0], "-mrphtab")) OptExpress  = OPT_MRPH;
	else if (str_eq(argv[0], "-mrphtree")) OptExpress = OPT_MRPHTREE;
	else if (str_eq(argv[0], "-pa"))      OptExpress  = OPT_PA;
	else if (str_eq(argv[0], "-table"))   OptExpress  = OPT_TABLE;
	else if (str_eq(argv[0], "-entity"))  OptDisplay  = OPT_ENTITY;
	else if (str_eq(argv[0], "-article")) OptArticle  = TRUE;
	else if (str_eq(argv[0], "-normal"))  OptDisplay  = OPT_NORMAL;
	else if (str_eq(argv[0], "-detail"))  OptDisplay  = OPT_DETAIL;
	else if (str_eq(argv[0], "-debug"))   OptDisplay  = OPT_DEBUG;
	else if (str_eq(argv[0], "-nbest"))   OptNbest    = TRUE;
	else if (str_eq(argv[0], "-expand"))  OptExpandP  = TRUE;
	else if (str_eq(argv[0], "-S"))       OptMode     = SERVER_MODE;
	else if (str_eq(argv[0], "-no-use-cf")) OptUseCF   = FALSE;
	else if (str_eq(argv[0], "-no-use-ncf")) OptUseNCF   = FALSE;
	else if (str_eq(argv[0], "-process-paren")) OptProcessParen = TRUE;
	else if (str_eq(argv[0], "-dpnd")) {
	    OptAnalysis = OPT_DPND;
	    OptUseCF = FALSE;
	    OptUseNCF = FALSE;
	}
	else if (str_eq(argv[0], "-dpnd-use-ncf")) {
	    OptAnalysis = OPT_DPND;
	    OptUseCF = FALSE;
	}
	else if (str_eq(argv[0], "-filter")) {
	    OptAnalysis = OPT_FILTER;
	    OptUseCF = FALSE;
	    OptUseNCF = FALSE;
	}
	else if (str_eq(argv[0], "-bnst")) {
	    OptAnalysis = OPT_BNST;
	    OptUseCF = FALSE;
	    OptUseNCF = FALSE;
	}
	else if (str_eq(argv[0], "-assignf")) {
	    OptAnalysis = OPT_AssignF;
	    OptUseCF = FALSE;
	    OptUseNCF = FALSE;
	}
	else if (str_eq(argv[0], "-check")) {
	    OptCheck = TRUE;
	}
	else if (str_eq(argv[0], "-probcase")) {
	    OptAnalysis = OPT_CASE;
	    OptCaseFlag |= OPT_CASE_USE_PROBABILITY;
	    SOTO_THRESHOLD = 0;
	}
	else if (str_eq(argv[0], "-no-probcase")) {
	    OptCaseFlag &= ~OPT_CASE_USE_PROBABILITY;
	    SOTO_THRESHOLD = DEFAULT_SOTO_THRESHOLD;
	    OptCKY = FALSE;
	}
	else if (str_eq(argv[0], "-probcase-use-ncf")) {
	    OptCaseFlag |= OPT_CASE_USE_NCF;
	}
	else if (str_eq(argv[0], "-cf-ne")) {
	    OptGeneralCF |= OPT_CF_NE;
	}
	else if (str_eq(argv[0], "-cf-category")) {
	    OptGeneralCF |= OPT_CF_CATEGORY;
	}
	else if (str_eq(argv[0], "-no-cf-ne")) {
	    OptGeneralCF &= ~OPT_CF_NE;
	}
	else if (str_eq(argv[0], "-no-cf-category")) {
	    OptGeneralCF &= ~OPT_CF_CATEGORY;
	}
	else if (str_eq(argv[0], "-no-parafix")) {
	    OptParaFix = FALSE;
	}
	else if (str_eq(argv[0], "-no-parafix-generate-all")) {
	    OptParaFix = FALSE;
	    OptParaNoFixFlag |= OPT_PARA_MULTIPLY_ALL_EX;
	}
	else if (str_eq(argv[0], "-no-parafix-generate-sim")) {
	    OptParaFix = FALSE;
	    OptParaNoFixFlag |= OPT_PARA_GENERATE_SIMILARITY;
	    OptParaNoFixFlag |= OPT_PARA_MULTIPLY_ALL_EX;
	}
	else if (str_eq(argv[0], "-no-parafix-synchronize")) {
	    OptParaFix = FALSE;
	    OptParaNoFixFlag |= OPT_PARA_SYNCHRONIZE;
	}
	else if (str_eq(argv[0], "-no-generalize-agent")) {
	    OptCaseFlag &= ~OPT_CASE_GENERALIZE_AGENT;
	}
	else if (str_eq(argv[0], "-generate-from-eos")) {
	    OptCaseFlag |= OPT_CASE_GENERATE_EOS;
	}
	else if (str_eq(argv[0], "-chi-generative")) {
	     OptChiGenerative = 1;
	}
	else if (str_eq(argv[0], "-chi-pos")) {
	     OptChiPos = 1;
	}
	else if (str_eq(argv[0], "-cky")) {
	    OptCKY = TRUE;
	}
	else if (str_eq(argv[0], "-no-cky")) {
	    OptCKY = FALSE;
	}
	else if (str_eq(argv[0], "-beam")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptBeam = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-language")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (!strcasecmp(argv[0], "chinese")) {
		Language = CHINESE;
		OptAnalysis = OPT_DPND;
		OptCKY = TRUE;
	    }
	    else if (!strcasecmp(argv[0], "japaense")) {
		Language = JAPANESE;
	    }
	    else {
		usage();
	    }
	}
	else if (str_eq(argv[0], "-ellipsis")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	}
	else if (str_eq(argv[0], "-demonstrative")) {
	    OptEllipsis |= OPT_DEMO;
	}
	else if (str_eq(argv[0], "-anaphora-detail")) {
	    OptAnaphora |= (OPT_ANAPHORA | OPT_PRINT_ENTITY);
	    OptEllipsis |= (OPT_ELLIPSIS | OPT_COREFER);
	    OptGeneralCF |= (OPT_CF_NE | OPT_CF_CATEGORY | OPT_CF_CLASS);
	}
	else if (str_eq(argv[0], "-anaphora-train")) {
	    OptAnaphora |= (OPT_ANAPHORA | OPT_PRINT_ENTITY | OPT_TRAIN);
	    OptEllipsis |= (OPT_ELLIPSIS | OPT_COREFER);
	    OptGeneralCF |= (OPT_CF_NE | OPT_CF_CATEGORY | OPT_CF_CLASS);
	}
	else if (str_eq(argv[0], "-anaphora-prob")) {
	    OptAnaphora |= (OPT_ANAPHORA | OPT_PRINT_ENTITY | OPT_ANAPHORA_PROB);
	    OptEllipsis |= (OPT_ELLIPSIS | OPT_COREFER);
	    OptGeneralCF |= (OPT_CF_NE | OPT_CF_CATEGORY);
	}
	else if (str_eq(argv[0], "-anaphora-normal") || str_eq(argv[0], "-anaphora")) {
	    OptAnaphora |= OPT_ANAPHORA;
	    OptEllipsis |= (OPT_ELLIPSIS | OPT_COREFER);
	    OptGeneralCF |= (OPT_CF_NE | OPT_CF_CATEGORY | OPT_CF_CLASS);
	}
	else if (str_eq(argv[0], "-anaphora-copula")) {
	    OptAnaphora |= (OPT_ANAPHORA | OPT_ANAPHORA_COPULA);
	    OptEllipsis |= (OPT_ELLIPSIS | OPT_COREFER);
	}	
	else if (str_eq(argv[0], "-read-ne")) {
	    OptReadNE = 1;    
	}
#ifdef USE_SVM
	else if (str_eq(argv[0], "-ellipsis-svm")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-ellipsis-svm-only")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscPredMethod = OPT_SVM;
	    OptDiscFlag |= OPT_DISC_CLASS_ONLY;
	}
	else if (str_eq(argv[0], "-ellipsis-svm-best")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscFlag |= OPT_DISC_BEST;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-ellipsis-svm-best-only")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscFlag |= OPT_DISC_BEST;
	    OptDiscFlag |= OPT_DISC_CLASS_ONLY;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-ellipsis-svm-flat")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscFlag |= OPT_DISC_FLAT;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-ellipsis-svm-flat-only")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscFlag |= OPT_DISC_FLAT;
	    OptDiscFlag |= OPT_DISC_CLASS_ONLY;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-demonstrative-svm")) {
	    OptEllipsis |= OPT_DEMO;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm-best")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_BEST;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm-best-only")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_BEST;
	    OptDiscFlag |= OPT_DISC_CLASS_ONLY;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm-flat")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_FLAT;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm-flat-only")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_FLAT;
	    OptDiscFlag |= OPT_DISC_CLASS_ONLY;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm-twin")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_TWIN_CAND;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-anaphora-svm-ranking")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_RANKING;
	    OptDiscPredMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-relation-noun-svm")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptDiscNounMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-relation-noun-svm-best")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptDiscFlag |= OPT_DISC_BEST;
	    OptDiscNounMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-relation-noun-svm-ranking")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptDiscFlag |= OPT_DISC_RANKING;
	    OptDiscNounMethod = OPT_SVM;
	}
	else if (str_eq(argv[0], "-print-svm-features")) {
	    PrintFeatures = 1;
	}
	else if (str_eq(argv[0], "-ne")) {
	    OptNE = 1;
	}
	else if (str_eq(argv[0], "-ne-debug")) {
	    OptDisplayNE  = OPT_DEBUG;
	    OptNE = 1;
	}
	else if (str_eq(argv[0], "-ne-cache")) { /* キャッシュを使用しない */
	    OptNE = 1;
	    OptNEcache = 1;
	}
	else if (str_eq(argv[0], "-ne-end")) { /* 末尾文字を使用しない */
	    OptNE = 1;
	    OptNEend = 1;
	}
	else if (str_eq(argv[0], "-ne-parent")) { /* 親の情報を用いない */
	    OptNE = 1;
	    OptNEparent = 1;
	}
	else if (str_eq(argv[0], "-ne-case")) { /* 格解析結果も使用する */
	    OptNE = 1;
	    OptNEcase = 1;
	    OptAnalysis = OPT_CASE;
	}
 	else if (str_eq(argv[0], "-ne-learn")) { /* NEの学習用featureを出力する */
	    OptNE = 1;
	    OptNElearn = 1;
	}
#else
#ifdef USE_CRF
	else if (str_eq(argv[0], "-ne-debug")) {
	    OptDisplayNE  = OPT_DEBUG;
	    OptNECRF = 1;
	    OptNE = 1;
	}
	else if (str_eq(argv[0], "-ne-parent")) { /* 親の情報を用いない */
	    OptNE = 1;
	    OptNECRF = 1;
	    OptNEparent = 1;
	}
	else if (str_eq(argv[0], "-ne-cache")) { /* キャッシュを使用しない */
	    OptNE = 1;
	    OptNECRF = 1;
	    OptNEcache = 1;
	}
 	else if (str_eq(argv[0], "-ne-learn")) { /* NEの学習用featureを出力する */
	    OptNE = 1;
	    OptNECRF = 1;
	    OptNElearn = 1;
	}
#endif
#endif
#ifdef USE_CRF
	else if (str_eq(argv[0], "-ne-crf")) {
	    OptNE = 1;
	    OptNECRF = 1;
	}
#endif
	else if (str_eq(argv[0], "-ellipsis-dt")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscPredMethod = OPT_DT;
	}
	else if (str_eq(argv[0], "-ellipsis-dt-only")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscPredMethod = OPT_DT;
	    OptDiscFlag |= OPT_DISC_CLASS_ONLY;
	}
	else if (str_eq(argv[0], "-demonstrative-dt")) {
	    OptEllipsis |= OPT_DEMO;
	    OptDiscPredMethod = OPT_DT;
	}
	else if (str_eq(argv[0], "-anaphora-dt")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscPredMethod = OPT_DT;
	}
	else if (str_eq(argv[0], "-relation-noun")) {
	    OptEllipsis |= OPT_REL_NOUN;
	}
	else if (str_eq(argv[0], "-relation-noun-only")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptEllipsis &= ~OPT_ELLIPSIS; 
	}	
	else if (str_eq(argv[0], "-relation-comp-noun")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptUseCPNCF = TRUE;
	}
	else if (str_eq(argv[0], "-relation-no-comp-noun")) {
	    OptUseCPNCF = FALSE;
	}
	else if (str_eq(argv[0], "-corefer")) { /* 係り受け判定のオプション */
	    OptEllipsis |= OPT_COREFER;
	    OptCorefer = 4; /* 文節間の修飾を考慮しない */
	}
	else if (str_eq(argv[0], "-corefer1")) { /* 係り受け判定のオプション */
	    OptEllipsis |= OPT_COREFER;
	    OptUseNCF = TRUE;
	    OptCorefer = 1; /* 名詞格フレームを用いる */
	}
	else if (str_eq(argv[0], "-corefer2")) { /* 係り受け判定のオプション */
	    OptEllipsis |= OPT_COREFER;
	    OptCorefer = 2; /* 主辞と同様に扱う */
	}
	else if (str_eq(argv[0], "-corefer3")) { /* 係り受け判定のオプション */
	    OptEllipsis |= OPT_COREFER;
	    OptCorefer = 3; /* 主辞以外は修飾されないと考える */
	}
	else if (str_eq(argv[0], "-corefer4")) { /* 係り受け判定のオプション */
	    OptEllipsis |= OPT_COREFER;
	    OptCorefer = 4; /* 文節間の修飾を考慮しない */
	}
	else if (str_eq(argv[0], "-corefer5")) { /* 係り受け判定のオプション */
	    OptEllipsis |= OPT_COREFER;
	    OptCorefer = 5; /* 修飾を考慮しない */
	}
	else if (str_eq(argv[0], "-relation-noun-best")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptDiscFlag |= OPT_DISC_BEST;
	}
	else if (str_eq(argv[0], "-relation-noun-dt")) {
	    OptEllipsis |= OPT_REL_NOUN;
	    OptDiscNounMethod = OPT_DT;
	}
	else if (str_eq(argv[0], "-anaphora-baseline")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_RANKING;
	    OptAnaphoraBaseline = OPT_BASELINE_NORMAL;
	}
	else if (str_eq(argv[0], "-anaphora-baseline-cook")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptEllipsis |= OPT_DEMO;
	    OptDiscFlag |= OPT_DISC_RANKING;
	    OptAnaphoraBaseline = OPT_BASELINE_COOK;
	}
	else if (str_eq(argv[0], "-learn")) {
	    OptLearn = TRUE;
	    OptDiscFlag |= OPT_DISC_OR_CF;
	    PrintFeatures = 1;
	}
	else if (str_eq(argv[0], "-no-wo-to")) {
	    OptDiscFlag |= OPT_DISC_NO_WO_TO;
	}
	else if (str_eq(argv[0], "-no-candidate-behind")) {
	    OptNoCandidateBehind = 1;
	}
	else if (str_eq(argv[0], "-i")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptIgnoreChar = *argv[0];
	}
	else if (str_eq(argv[0], "-use-ex-all")) {
	    OptCaseFlag |= OPT_CASE_USE_EX_ALL;
	}
	else if (str_eq(argv[0], "-print-ex-all")) {
	    EX_PRINT_NUM = -1;
	}
	else if (str_eq(argv[0], "-print-deleted-sm")) {
	    PrintDeletedSM = 1;
	}
	else if (str_eq(argv[0], "-print-frequency")) {
	    PrintFrequency = 1;
	}
	else if (str_eq(argv[0], "-print-ex")) {
	    PrintEx = 1;
	}
	else if (str_eq(argv[0], "-print-num")) {
	    PrintNum = 1;
	}
	else if (str_eq(argv[0], "-N")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptPort = atol(argv[0]);
	}
	else if (str_eq(argv[0], "-C")) {
	    OptMode = CLIENT_MODE;
	    argv++; argc--;
	    if (argc < 1) usage();
	    strcpy(OptHostname, argv[0]);
	}
	/* daemonにしない場合 (cygwin用) */
	else if (str_eq(argv[0], "-F")) {
	    OptMode = SERVER_MODE;
	    OptServerFlag = OPT_SERV_FORE;
	}
	else if (str_eq(argv[0], "-timeout")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    ParseTimeout = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-timeout-exit")) {
	    OptTimeoutExit = 1;
	}
	else if (str_eq(argv[0], "-scode")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (!strcasecmp(argv[0], "ntt")) {
		Thesaurus = USE_NTT;
	    }
	    else if (!strcasecmp(argv[0], "bgh")) {
		Thesaurus = USE_BGH;
	    }
	    else {
		usage();
	    }
	}
	else if (str_eq(argv[0], "-para-scode")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (!strcasecmp(argv[0], "ntt")) {
		ParaThesaurus = USE_NTT;
	    }
	    else if (!strcasecmp(argv[0], "bgh")) {
		ParaThesaurus = USE_BGH;
	    }
	    else if (!strcasecmp(argv[0], "none")) {
		ParaThesaurus = USE_NONE;
	    }
	    else {
		usage();
	    }
	}
	else if (str_eq(argv[0], "-no-use-rn-cf")) {
	    OptCaseFlag &= ~OPT_CASE_USE_REP_CF;
	    OptUseRN = 0;
	}
	else if (str_eq(argv[0], "-no-use-rn")) {
	    OptUseRN = 0;
	}
	else if (str_eq(argv[0], "-no-use-crn-cf")) { /* KNPが生成する主辞代表表記を使わない格フレーム */
	    OptCaseFlag &= ~OPT_CASE_USE_CREP_CF;
	}
	else if (str_eq(argv[0], "-no-use-cn-cf")) {  /* KNPが生成する複合名詞(主辞’)代表表記を使わない格フレーム */
	    OptCaseFlag &= ~OPT_CASE_USE_CREP_CF;
	    OptCaseFlag &= ~OPT_CASE_USE_CN_CF;
	}
	else if (str_eq(argv[0], "-no-fix-cf-search")) {
	    OptCaseFlag &= ~OPT_CASE_FIX_CF_SEARCH;
	}
	else if (str_eq(argv[0], "-no-scase")) {
	    OptUseScase = FALSE;
	}
	else if (str_eq(argv[0], "-no-smfix")) {
	    OptUseSmfix = FALSE;
	}
	else if (str_eq(argv[0], "-r")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    Opt_knprc = argv[0];
	}
	else if (str_eq(argv[0], "-verbose")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    VerboseLevel = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-v")) {
	    fprintf(stderr, "%s %s\n", PACKAGE_NAME, VERSION);
	    exit(0);
	}
	/* 格解析用オプション */
	else if (str_eq(argv[0], "-assign-ga-subj")) {
	    OptCaseFlag |= OPT_CASE_ASSIGN_GA_SUBJ;
	}
	else if (str_eq(argv[0], "-no")) {
	    OptCaseFlag |= OPT_CASE_NO;
	}
	else if (str_eq(argv[0], "-use-cf-included-soto-words")) {
	    OptCaseFlag &= ~OPT_CASE_ADD_SOTO_WORDS;
	}
	else if (str_eq(argv[0], "-disc-or-cf")) {
	    OptDiscFlag |= OPT_DISC_OR_CF;
	}
	else if (str_eq(argv[0], "-ellipsis-or-cf")) {
	    OptEllipsis |= OPT_ELLIPSIS;
	    OptDiscFlag |= OPT_DISC_OR_CF;
	}
	/* 以下コスト調整用 */
	else if (str_eq(argv[0], "-noun-th")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    AntecedentDecideThresholdForNoun = atof(argv[0]);
	}
	else if (str_eq(argv[0], "-cffix-th")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    CFSimThreshold = atof(argv[0]);
	}
	else if (str_eq(argv[0], "-sototh")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    SOTO_THRESHOLD = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-dcost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    DISTANCE_STEP = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-rcost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    RENKAKU_STEP = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-svcost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    STRONG_V_COST = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-atcost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    ADJACENT_TOUTEN_COST = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-lacost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    LEVELA_COST = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-tscost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    TEIDAI_STEP = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-quacost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    EX_match_qua = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-unknowncost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    EX_match_unknown = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-sentencecost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    EX_match_sentence = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-timecost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    EX_match_tim = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-sotocost")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    SOTO_SCORE = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-score-agent")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    EX_match_subject = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-read-feature")) {
	    OptReadFeature |= OPT_ELLIPSIS;
	    OptReadFeature |= OPT_REL_NOUN;
	    OptReadFeature |= OPT_COREFER;
	    OptEllipsis &= ~OPT_COREFER;
	}
	else if (str_eq(argv[0], "-read-feature-corefer")) {
	    OptReadFeature |= OPT_COREFER;
	    OptEllipsis &= ~OPT_COREFER;
	}
	else if (str_eq(argv[0], "-read-feature-ellipsis")) {
	    OptReadFeature |= OPT_ELLIPSIS;
	    OptEllipsis &= ~OPT_COREFER;
	}
	else if (str_eq(argv[0], "-read-feature-noun")) {
	    OptReadFeature |= OPT_REL_NOUN;
	    OptEllipsis &= ~OPT_COREFER;
	}
	else if (str_eq(argv[0], "-add-svmfeature-utype")) {
	    OptAddSvmFeatureUtype = 1;
	}
	else if (str_eq(argv[0], "-add-svmfeature-discourse-depth")) {
	    OptAddSvmFeatureDiscourseDepth = 1;
	}
	else if (str_eq(argv[0], "-add-svmfeature-object-recognition")) {
	    OptAddSvmFeatureObjectRecognition = 1;
	}
	else if (str_eq(argv[0], "-add-svmfeature-referred-num")) {
	    OptAddSvmFeatureReferedNum = 1;
	}
	else if (str_eq(argv[0], "-copula")) {
	    OptCopula = 1;
	}
	else if (str_eq(argv[0], "-postprocess")) {
	    OptPostProcess = 1;
	}
	else if (str_eq(argv[0], "-recover-person")) {
	    OptRecoverPerson = 1;
	}
	else if (str_eq(argv[0], "-def-sentence")) { /* used in rules */
	    ;
	}
	else if (str_eq(argv[0], "-use-smallest-phrase")) { /* used in rules */
	    ;
	}
	else if (str_eq(argv[0], "-disable-emoticon-recognition")) { /* used in rules */
	    ;
	}
	else {
	    usage();
	}
    }
    if (argc != 0) {
	usage();
    }

    /* 文脈解析のときは必ず格解析を行う (CASE2)
       解析済みデータのときは read_mrph() で CASE2 にしている
       ただし、共参照解析のみを行う場合は除く */
    if (OptEllipsis && (OptEllipsis != OPT_COREFER)) {
	if (OptAnalysis != OPT_CASE && OptAnalysis != OPT_CASE2) {
	    OptAnalysis = OPT_CASE2;
	}
    }
}

/*==================================================================*/
			void init_juman(void)
/*==================================================================*/
{
    int i;

    /* rcfile をさがす順
       1. -r で指定されたファイル
       2. $HOME/.knprc
       3. KNP_RC_DEFAULT (compile時)
       → rcfileがなければエラー
    */

    grammar(NULL);				/* 文法辞書 */
    katuyou(NULL);				/* 活用辞書 */

    for (i = 1; Class[i][0].id; i++);
    CLASS_num = i;
}

/*==================================================================*/
			void read_rules(void)
/*==================================================================*/
{
    int i;

    for (i = 0; i < CurrentRuleNum; i++) {
	/* 同形異義語ルール */
	if ((RULE+i)->type == HomoRuleType) {
	    read_homo_rule((RULE+i)->file);
	}
	/* 形態素ルール or 文節ルール */
	else if ((RULE+i)->type == MorphRuleType || 
		 (RULE+i)->type == PreProcessMorphRuleType || 
		 (RULE+i)->type == NeMorphRuleType || 
		 (RULE+i)->type == TagRuleType || 
		 (RULE+i)->type == BnstRuleType || 
		 (RULE+i)->type == AfterDpndBnstRuleType || 
		 (RULE+i)->type == AfterDpndTagRuleType || 
		 (RULE+i)->type == PostProcessTagRuleType) {
	    read_general_rule(RULE+i);
	}
	/* 係り受けルール */
	else if ((RULE+i)->type == DpndRuleType) {
	    if (Language == CHINESE) {
		read_dpnd_rule_for_chinese((RULE+i)->file);
	    }
	    else {
		read_dpnd_rule((RULE+i)->file);
	    }
	}
	/* 呼応表現ルール */
	else if ((RULE+i)->type == KoouRuleType) {
	    read_koou_rule((RULE+i)->file);
	}
	/* 文脈処理のルール */
	else if ((RULE+i)->type == ContextRuleType) {
	    read_bnst_rule((RULE+i)->file, ContRuleArray, &ContRuleSize, ContRule_MAX);
	}
    }
}

/*==================================================================*/
			   void close_all()
/*==================================================================*/
{
    close_cf();
    close_noun_cf();
    close_thesaurus();
    close_scase();
    close_auto_dic();
    close_nv_mi();

    if (Language == CHINESE) {
	close_chi_dpnd_db();
	free_chi_type();
	free_chi_pos();
    }

    if (OptEllipsis) {
	close_event();
    }

#ifdef USE_SVM
    if (OptNE && !OptNECRF) {
	close_db_for_NE();
    }
#endif
    if (OptEllipsis & OPT_COREFER)
	close_Synonym_db();

#ifdef DB3DEBUG
    db_teardown();
#endif
}

/*==================================================================*/
		static void timeout_function(int sig)
/*==================================================================*/
{
    longjmp(timeout, 1);
}

/*==================================================================*/
		      void set_timeout_signal()
/*==================================================================*/
{
#ifndef _WIN32
    sigset_t set;
    if (-1 == sigfillset(&set)) {
	perror("sigfullset:"); 
	exit(1);
    }
    if (-1 == sigprocmask(SIG_UNBLOCK, &set, 0)) {
	perror("sigprocmask:"); 
	exit(1);
    }

    signal(SIGALRM, timeout_function);
#endif
}

/*==================================================================*/
			   void init_all()
/*==================================================================*/
{
    int i;

    /* 初期化 */

#ifdef DB3DEBUG
    db_setup();
#endif
    init_hash();
    if (OptReadNE || OptNE) {
	init_tagposition();
    }
    init_configfile(Opt_knprc);	/* 各種ファイル設定初期化 */

    if (!OptNECRF && !DBforNE) OptNE = 0;
    if (!OptReadNE && OptNE) {
	init_ne_cache();
	if (!OptNECRF) init_db_for_NE(); /* NE用 */
    }

    if (OptEllipsis & OPT_COREFER) {
	init_Synonym_db();
	/* init_entity_cache(); */
    }

    if (Language == CHINESE) {
	init_hownet();
	init_chi_dpnd_db();
	init_chi_type();
	init_chi_pos();
    }

    init_juman();	/* JUMAN関係 */
    if (OptUseCF) {
	init_cf();	/* 格フレームオープン */
    }
    if (OptUseNCF) {
	init_noun_cf();	/* 名詞格フレームオープン */
    }
    init_thesaurus();	/* シソーラスオープン */
    init_scase();	/* 表層格辞書オープン */
    init_auto_dic();	/* 自動獲得辞書オープン */
    init_nv_mi();	/* 名詞動詞相互情報量DBオープン */

    if (OptEllipsis) {
#ifdef USE_SVM
	if (OptDiscPredMethod == OPT_SVM || OptDiscNounMethod == OPT_SVM) {
	    init_svm_for_anaphora();
	}
#endif
	if (OptDiscPredMethod == OPT_DT || OptDiscNounMethod == OPT_DT) {
	    init_dt();
	}
	init_event();
    }
#ifdef USE_SVM
    if (OptNE && !OptNElearn && !OptNECRF)
	init_svm_for_NE();
#endif
#ifdef USE_CRF
    if (OptNE && !OptNElearn && OptNECRF)
	init_crf_for_NE();
#endif
    /* 形態素, 文節情報の初期化 */
    memset(mrph_data, 0, sizeof(MRPH_DATA)*MRPH_MAX);
    memset(bnst_data, 0, sizeof(BNST_DATA)*BNST_MAX);
    memset(tag_data, 0, sizeof(MRPH_DATA)*TAG_MAX);

    current_sentence_data.mrph_data = mrph_data;
    current_sentence_data.bnst_data = bnst_data;
    current_sentence_data.tag_data = tag_data;
    current_sentence_data.para_data = para_data;
    current_sentence_data.para_manager = para_manager;
    current_sentence_data.Sen_num = 0;	/* これだけは増えていく */
    current_sentence_data.Mrph_num = 0;
    current_sentence_data.Bnst_num = 0;
    current_sentence_data.New_Bnst_num = 0;
    current_sentence_data.Tag_num = 0;
    current_sentence_data.Best_mgr = &Best_mgr;
    current_sentence_data.KNPSID = NULL;
    current_sentence_data.Comment = NULL;

    for (i = 0; i < BNST_MAX; i++) {
	 current_sentence_data.bnst_data[i].f = NULL;
    }

    if (OptEllipsis) {
	InitContextHash();
	entity_manager.num = 0;
    }

    set_timeout_signal();
}

/*==================================================================*/
	     void clear_mrph_features(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    if (OptEllipsis) {
	/* 中身は保存しておくので */
	for (i = 0; i < sp->Mrph_num + sp->New_Mrph_num; i++) {
	    (sp->mrph_data+i)->f = NULL;
	}
    }
    else {
	for (i = 0; i < sp->Mrph_num; i++) {
	    clear_feature(&(sp->mrph_data[i].f));
	}
	/* New_Mrphはもともとpointer */
	for (i = sp->Mrph_num; i < sp->Mrph_num + sp->New_Mrph_num; i++) {
	    (sp->mrph_data+i)->f = NULL;
	}
    }
}

/*==================================================================*/
	     void clear_bnst_features(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    if (OptEllipsis) {
	/* 中身は保存しておくので */
	for (i = 0; i < sp->Bnst_num + sp->Max_New_Bnst_num; i++) {
	    (sp->bnst_data+i)->f = NULL;
	}
    }
    else {
	for (i = 0; i < sp->Bnst_num; i++) {
	    clear_feature(&(sp->bnst_data[i].f));
	    if (Language == CHINESE) {
		sp->bnst_data[i].is_para = -1;
	    }
	}
	/* New_Bnstはもともとpointer */
	for (i = sp->Bnst_num; i < sp->Bnst_num + sp->Max_New_Bnst_num; i++) {
	    (sp->bnst_data+i)->f = NULL;
	}
    }
}

/*==================================================================*/
	      void clear_bp_features(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    if (OptEllipsis) {
	/* 中身は保存しておくので */
	for (i = 0; i < sp->Tag_num + sp->New_Tag_num; i++) {
	    (sp->tag_data+i)->f = NULL;
	}
    }
    else {
	for (i = 0; i < sp->Tag_num; i++) {
	    clear_feature(&(sp->tag_data[i].f));
	}
	/* New_Tagはもともとpointer */
	for (i = sp->Tag_num; i < sp->Tag_num + sp->New_Tag_num; i++) {
	    (sp->tag_data+i)->f = NULL;
	}
    }
}

/*==================================================================*/
	      void clear_all_features(SENTENCE_DATA *sp)
/*==================================================================*/
{
    clear_mrph_features(sp);
    clear_bnst_features(sp);
    clear_bp_features(sp);
}

/*==================================================================*/
      int one_sentence_analysis(SENTENCE_DATA *sp, int eos_flag)
/*==================================================================*/
{
    int flag, i;
    int relation_error, d_struct_error;

    /* 形態素列の前処理だけの場合 */
    if (OptAnalysis == OPT_FILTER) return TRUE;

    /* 形態素へのFEATURE付与 */

    assign_cfeature(&(sp->mrph_data[0].f), "文頭", FALSE);
    assign_cfeature(&(sp->mrph_data[sp->Mrph_num-1].f), "文末", FALSE);
    assign_general_feature(sp->mrph_data, sp->Mrph_num, MorphRuleType, FALSE, FALSE);

    /* 正規化代表表記を形態素に付与 
       ※代表表記変更後に行う */
    assign_canonical_rep_to_mrph(sp);

    /* 固有表現認識を行う */
    if (OptReadNE) {
	read_ne(sp);
    }
    else if (OptNE && !OptNEcase && OptNEparent) {
	ne_analysis(sp);
    }
    
    /* 形態素を文節にまとめる */
    if (OptInput == OPT_RAW) {
	if (make_bunsetsu(sp) == FALSE) {
	    clear_bnst_features(sp);
	    clear_bp_features(sp);
	    sp->available = 0;
	    sp->Bnst_num = 0;
	    sp->Tag_num = 0;
	    ErrorComment = strdup("Cannot make bunsetsu");
	    return TRUE;
	}
    }
    else {
	if (make_bunsetsu_pm(sp) == FALSE) {
	    clear_bnst_features(sp);
	    clear_bp_features(sp);
	    sp->available = 0;
	    sp->Bnst_num = 0;
	    sp->Tag_num = 0;
	    ErrorComment = strdup("Cannot make bunsetsu");
	    return TRUE;
	}
    }

    /* 文節化だけの場合 */

    if (OptAnalysis == OPT_BNST) return TRUE;

    /* 文節への意味情報付与 */

    for (i = 0; i < sp->Bnst_num; i++) {
	decide_head_ptr(sp->bnst_data + i);
	make_Jiritu_Go(sp, sp->bnst_data + i);
	get_bnst_code_all(sp->bnst_data + i);
    }

    /* 文節へのFEATURE付与 */

    assign_cfeature(&(sp->bnst_data[0].f), "文頭", FALSE);
    if (sp->Bnst_num > 0)
	assign_cfeature(&(sp->bnst_data[sp->Bnst_num - 1].f), "文末", FALSE);
    else
	assign_cfeature(&(sp->bnst_data[0].f), "文末", FALSE);
    assign_general_feature(sp->bnst_data, sp->Bnst_num, BnstRuleType, FALSE, FALSE);

    /* サ変動詞以外の動詞の意味素を引くのは意味がない
       ルール適用前には、featureがないためにチェックできない
       ※ ルール適用後に意味素を引かないのは:
           => 意味素はルールで使うかもしれないので、ルール適用前に与えておく */
    for (i = 0; i < sp->Bnst_num; i++) {
	if (!check_feature((sp->bnst_data+i)->f, "体言") && 
	    !check_feature((sp->bnst_data+i)->f, "サ変")) {
	    (sp->bnst_data+i)->SM_code[0] = '\0';
	    delete_cfeature(&((sp->bnst_data+i)->f), "SM");
	}
	if (Language == CHINESE) {
	    copy_feature(&(sp->bnst_data[i].f), sp->bnst_data[i].mrph_ptr->f);
	}
    }

    /* タグ単位作成 (-notag時もscaseを引くために行う) */
    if (OptInput == OPT_RAW || 
	(OptInput & OPT_INPUT_BNST)) {
	make_tag_units(sp);
    }
    else {
	make_tag_units_pm(sp);
    }

    assign_cc_feature_to_bp(sp);   /* 正規化代表表記を基本句に付与 */
    assign_cc_feature_to_bnst(sp); /* 正規化代表表記を文節に付与 */

    /* 固有表現認識結果をタグに付与 */
    if (OptReadNE || OptNE && !OptNEcase && !OptNElearn && OptNEparent) {
	assign_ne_feature_tag(sp);
    }

    /* 入力した正解情報をチェック */
    if (OptReadFeature) {
	check_annotation(sp);
    }

    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG)
	print_bnst_with_mrphs(sp, 0, eos_flag);

    fix_sm_person(sp);

    /* FEATURE付与だけの場合 */

    if (OptAnalysis == OPT_AssignF) return TRUE;

    assign_dpnd_rule(sp);			/* 係り受け規則 */

    assign_pred_feature_to_bp(sp); /* 用言代表表記を基本句に付与 */

    /* 格フレーム取得 */
    if ((OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2 || OptUseNCF) &&
	/* 訓練時以外ですべての関係タグを読み込む場合は格フレームは読み込まない */
	(!OptAnaphora || (OptAnaphora & OPT_TRAIN) ||
	 !(OptReadFeature & OPT_ELLIPSIS) || !(OptReadFeature & OPT_REL_NOUN))) {
	set_caseframes(sp);
    }

    /*この時点の文節情報を表示 */
    if (OptDisplay == OPT_DEBUG)
	check_bnst(sp);

    /**************/
    /* 本格的解析 */
    /**************/

    if ((Language == CHINESE && !OptChiGenerative) ||
	Language != CHINESE) {
	calc_dpnd_matrix(sp);
    }
    else if (Language == CHINESE && OptChiGenerative) {
      if (OptChiPos == 0) {
	calc_chi_dpnd_matrix_forProbModel(sp);
      }
      else {
	calc_chi_pos_matrix(sp);
	calc_chi_dpnd_matrix_wpos(sp);
      }
    }

    /* 依存可能性計算 */
    if (OptDisplay == OPT_DEBUG) print_matrix(sp, PRINT_DPND, 0);

    if (Language == CHINESE && !OptChiGenerative) {
	calc_gigaword_pa_matrix(sp);			/* get count of gigaword pa for Chinese */
    }

    /* 呼応表現の処理 */

    if (koou(sp) == TRUE && OptDisplay == OPT_DEBUG)
	print_matrix(sp, PRINT_DPND, 0);

    /* fragment for Chinese */
    if (Language == CHINESE) {
	if (!OptChiPos && fragment(sp) == TRUE) {
	    if (OptDisplay == OPT_DEBUG) {
		print_matrix(sp, PRINT_DPND, 0);
	    }
	    is_frag = 1;
	}
    }

    /* 鍵括弧の処理 */

    if ((flag = quote(sp)) == TRUE && OptDisplay == OPT_DEBUG)
	print_matrix(sp, PRINT_QUOTE, 0);

    if (flag == CONTINUE) return FALSE;

    /* base phrase for Chinese */
    if (Language == CHINESE && !OptChiPos) {
	base_phrase(sp, is_frag);
	print_matrix(sp, PRINT_DPND, 0);
    }

    /* 係り受け関係がない場合の弛緩 */
	
    if (Language != CHINESE && relax_dpnd_matrix(sp) == TRUE && OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Relaxation ... \n");
	print_matrix(sp, PRINT_DPND, 0);
    }

    if (OptInput & OPT_PARSED) {
	if (OptCheck == TRUE) {
	    call_count_dpnd_candidates(sp, &(sp->Best_mgr->dpnd));
	}
	dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd));

	sp->Para_num = 0;
	sp->Para_M_num = 0;
	if (check_para_key(sp)) {
	    calc_match_matrix(sp);		/* 文節間類似度計算 */
	    detect_all_para_scope(sp);    	/* 並列構造推定 */
	    assign_para_similarity_feature(sp);
	    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG) {
		print_matrix(sp, PRINT_PARA, 0);
	    }
	}

	para_recovery(sp);
	para_postprocess(sp);
	assign_para_similarity_feature(sp);
	after_decide_dpnd(sp, eos_flag);
	if (OptCheck == TRUE) {
	    check_candidates(sp);
	}
	goto PARSED;
    }

    /****************/
    /* 並列構造解析 */
    /****************/

    init_mask_matrix(sp);
    sp->Para_num = 0;
    sp->Para_M_num = 0;
    relation_error = 0;
    d_struct_error = 0;
    Revised_para_num = -1;

    if ((flag = check_para_key(sp)) > 0) {
	init_para_matrix(sp);
	calc_match_matrix(sp);		/* 文節間類似度計算 */
	detect_all_para_scope(sp);    	/* 並列構造推定 */
	do {
	    assign_para_similarity_feature(sp);
	    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG) {
		print_matrix(sp, PRINT_PARA, 0);
		/*
		  print_matrix2ps(sp, PRINT_PARA, 0);
		  exit(0);
		*/
	    }
	    /* 並列構造間の重なり解析 */
	    if (detect_para_relation(sp) == FALSE) {
		relation_error++;
		continue;
	    }
	    if (OptDisplay == OPT_DEBUG) print_para_relation(sp);
	    /* 並列構造内の依存構造チェック */
	    if (check_dpnd_in_para(sp) == FALSE) {
		d_struct_error++;
		continue;
	    }
	    if (OptDisplay == OPT_DEBUG) print_matrix(sp, PRINT_MASK, 0);
	    goto ParaOK;		/* 並列構造解析成功 */
	} while (relation_error <= 3 &&
		 d_struct_error <= 3 &&
		 detect_para_scope(sp, Revised_para_num, TRUE) == TRUE);
	ErrorComment = strdup("Cannot detect consistent CS scopes");
	init_mask_matrix(sp);
    }
    else if (flag == CONTINUE)
	return FALSE;

 ParaOK:
    /********************/
    /* 依存・格構造解析 */
    /********************/
    para_postprocess(sp);	/* 各conjunctのheadを提題の係り先に */

    if (OptCKY) {
	/* CKY */
	if (cky(sp, sp->Best_mgr, eos_flag) == FALSE) {
	    if (Language == CHINESE) {
		printf("sentence %d cannot be parsed\n",sen_num);
		return FALSE;
	    }
	    else {
		sp->available = 0;
		ErrorComment = strdup("Cannot detect dependency structure");
		when_no_dpnd_struct(sp);
	    }
	}
	else if (OptCheck == TRUE) {
	    check_candidates(sp);
	}
    }
    else {
#ifndef _WIN32
	alarm(ParseTimeout);
#endif

	/* 依存・格構造解析の呼び出し */
	if (detect_dpnd_case_struct(sp, eos_flag) == FALSE) {
	    sp->available = 0;
	    ErrorComment = strdup("Cannot detect dependency structure");
	    when_no_dpnd_struct(sp);	/* 係り受け構造が求まらない場合
					   すべて文節が隣に係ると扱う */
	}
	else if (OptCheck == TRUE) {
	    check_candidates(sp);
	}
#ifndef _WIN32
	alarm(0);
#endif
    }

PARSED:
    /* 係り受け情報を bnst 構造体に記憶 */
    dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd));
    para_recovery(sp);

    if (!(OptExpress & OPT_NOTAG)) {
	dpnd_info_to_tag(sp, &(sp->Best_mgr->dpnd));
	if (OptExpress & OPT_MRPH) {
	    dpnd_info_to_mrph(sp);
	}
    }

    if (OptNE && (!OptNEparent || OptNEcase)) {
	/* 固有表現認識に必要なfeatureを与える */
	for_ne_analysis(sp);   
	/* 格解析後に固有表現認識を行う */
	ne_analysis(sp);
	if (!OptNElearn) assign_ne_feature_tag(sp);
    }

    /* 構造決定後のルール適用 */
    assign_general_feature(sp->bnst_data, sp->Bnst_num, AfterDpndBnstRuleType, FALSE, FALSE);
    assign_general_feature(sp->tag_data, sp->Tag_num, AfterDpndTagRuleType, FALSE, FALSE);

    /* 照応解析に必要なFEATUREの付与 */
    if (OptEllipsis) assign_anaphor_feature(sp);

    /* 文節情報の表示 */
    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG) {
	check_bnst(sp);
    }

    memo_by_program(sp);	/* メモへの書き込み */

    /* 後処理 */
    if (!OptEllipsis && OptPostProcess) {
	do_postprocess(sp);
    }

    /* 格解析結果をfeatureへ */
    if (sp->available && (OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2) && !OptReadFeature) {
	record_all_case_analisys(sp, FALSE);
    }

    return TRUE;
}

/*==================================================================*/
	void init_for_one_sentence_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 格フレームの初期化 */
    if (OptAnalysis == OPT_CASE || 
	OptAnalysis == OPT_CASE2 ||
	OptUseNCF) {
	clear_cf(0);
    }

    /* 初期化 */
    if (sp->KNPSID) {
	free(sp->KNPSID);
	sp->KNPSID = NULL;
    }
    if (sp->Comment) {
	free(sp->Comment);
	sp->Comment = NULL;
    }

    /* FEATURE の初期化 */
    clear_all_features(sp);

    sp->available = 1;
}

/*==================================================================*/
	int one_line_analysis(SENTENCE_DATA *sp, FILE *input)
/*==================================================================*/
{
    int i, flag, paren_num = 0;

    /* initialization */
    init_for_one_sentence_analysis(sp);

    /* get sentence id for Chinese */
    if (Language == CHINESE) {
	sen_num++;
	is_frag = 0;
    }

    sp->Sen_num++;

    /* 形態素の読み込み */
    if ((flag = read_mrph(sp, input)) == EOF) return EOF;
    if (flag == FALSE) { /* EOSしかない空の文 */
	clear_all_features(sp);
	sp->available = 0;
	sp->Mrph_num = 0;
	sp->Bnst_num = 0;
	sp->Tag_num = 0;
	ErrorComment = strdup("Cannot make mrph");
    }
    else { /* 形態素読み込み成功 */
	/* 形態素列の前処理 */
	preprocess_mrph(sp);

	/* 括弧を処理して文として分割する場合 */
	if (OptProcessParen) {
	    paren_num = process_input_paren(sp, &paren_sentence_data);
	}

	/* 一文構文・格解析 */
	if ((flag = one_sentence_analysis(sp, paren_num ? 0 : 1)) == FALSE) {
	    sp->Sen_num--; /* 解析失敗時には文の数を増やさない */
	    return FALSE;
	}
    }

    /************/
    /* 文脈解析 */
    /************/
	       
    if (OptEllipsis) {
	assign_mrph_num(sp);
	make_dpnd_tree(sp);
	if (sp->Sen_num > SENTENCE_MAX) {
	    fprintf(stderr, "Sentence buffer overflowed!\n");
	    exit(1);
	}
	PreserveSentence(sp); /* 文情報を"sentence_data + sp->Sen_num - 1"に保存 */
	if (OptEllipsis & OPT_COREFER) corefer_analysis(sp); /* 共参照解析 */
	if (OptAnaphora) anaphora_analysis(sp);
	if (OptEllipsis != OPT_COREFER && !OptAnaphora) DiscourseAnalysis(sp);

	if (!OptArticle && OptPostProcess) { /* 後処理 (構文解析のときは、one_sentence_analysis()で行う) */
	    do_postprocess(sp);
	}
    }

    /* entity 情報の feature の作成 */
    if (OptDisplay == OPT_ENTITY) {
	prepare_all_entity(sp);
    }

    /* 入力した正解情報をクリア */
    if (OptReadFeature) {
	for (i = 0; i < sp->Tag_num; i++) {
	    if ((sp->tag_data + i)->c_cpm_ptr) {
		free((sp->tag_data + i)->c_cpm_ptr);
	    }
	}
    }
	
    /* 固有表現認識のためのキャッシュ作成 */
    if (OptNE) {
	make_ne_cache(sp);
    }

    /************/
    /* 結果表示 */
    /************/
    print_all_result(sp, paren_num ? 0 : 1); /* 括弧含む文: EOP(0); 通常文: EOS(1) */

    /* 括弧文の解析 */
    if (paren_num) {
	for (i = 0; i < paren_num; i++) {
	    /* initialization */
	    init_for_one_sentence_analysis(sp);
	    prepare_paren_sentence(sp, paren_sentence_data + i); /* spに設定 */
	    free((paren_sentence_data + i)->mrph_data);

	    if ((flag = one_sentence_analysis(sp, i != paren_num - 1 ? 0 : 1)) == FALSE) {
		continue; /* 解析失敗 */
	    }
	    print_all_result(sp, i != paren_num - 1 ? 0 : 1); /* last -> EOS(1); otherwise -> EOP(0) */
	}

	free(paren_sentence_data);
    }

    return flag;
}

/*==================================================================*/
			   void knp_main()
/*==================================================================*/
{
    int i, success = 1, flag;
    FILE *Jumanfp;
    SENTENCE_DATA *sp_new;

    SENTENCE_DATA *sp = &current_sentence_data;

    /* 格解析の準備 */
    init_case_analysis_cpm(sp);
    init_case_analysis_cmm();

    /* 意味クラスを用いた照応解析を行う場合クラスごとの出現確率を読み込む */    
    if (OptAnaphora && (OptGeneralCF & OPT_CF_CLASS)) init_class_prob();

    /* ルール読み込み
       Server Mode において、読み込むルールの変更がありえるので、ここで行う */
    read_rules();

    if (OptExpress == OPT_TABLE) fprintf(Outfp, "%%%% title=KNP解析結果\n");

    while ( 1 ) {

	/* Server Mode の場合 前回の出力が成功してない場合は 
	   ERROR とはく Server/Client モードの場合は,出力の同期をこれで行う */
	if (!success && OptMode == SERVER_MODE) {
	    fprintf(Outfp, "EOS ERROR\n");
	    fflush(Outfp);
	}

	/********************/
	/* 前の解析の後始末 */
	/********************/

	/* タイムアウト時 */

	if (setjmp(timeout)) {
	    /* timeoutした文をstderrに出力 */
	    fprintf(stderr, ";; Parse timeout.\n;; %s (", sp->KNPSID);
	    for (i = 0; i < sp->Mrph_num; i++)
		fprintf(stderr, "%s", sp->mrph_data[i].Goi2);
	    fprintf(stderr, ")\n");

	    ErrorComment = strdup("Parse timeout");
	    sp->available = 0;
	    when_no_dpnd_struct(sp);
	    dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd));
	    if (!(OptExpress & OPT_NOTAG)) {
		dpnd_info_to_tag(sp, &(sp->Best_mgr->dpnd)); 
		if (OptExpress & OPT_MRPH) {
		    dpnd_info_to_mrph(sp);
		}
	    }
	    if (!OptEllipsis) {
		if (OptPostProcess) { /* 後処理 */
		    do_postprocess(sp);
		}
		print_result(sp, 1, 1);
	    }
	    else {
		PreserveCPM(PreserveSentence(sp), sp);
	    }
	    fflush(Outfp);

	    /* OptTimeoutExit == 1 または格・省略解析のときは終わる */
	    if (OptTimeoutExit || 
		(OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2)) {
		exit(100);
	    }

	    set_timeout_signal();
	    continue;
	}

	/**************/
	/* メイン解析 */
	/**************/

	success = 0;

	if ((flag = one_line_analysis(sp, Infp)) == EOF) break;
	if (flag == FALSE) {
	    continue;
	}

	success = 1;	/* OK 成功 */
    }

    if (OptArticle && OptEllipsis) {
	for (i = 0; i < sp->Sen_num - 1; i++) {
	    if (OptPostProcess) { /* 後処理 */
		do_postprocess(sentence_data+i);
	    }
	    print_result(sentence_data+i, 1, 1);
	}
    }
}

#ifndef _WIN32
/* シグナル処理 */
static void sig_child()
{
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0) {}; 
    signal(SIGCHLD, sig_child); 
}

static void sig_term()
{
    shutdown(sfd,2);
    shutdown(fd, 2);
    exit(0);
}

/*==================================================================*/
			  void server_mode()
/*==================================================================*/
{
    /* サーバモード */

    int i;
    struct sockaddr_in sin;
    FILE *pidfile;
    struct passwd *ent_pw;

    if (OptServerFlag != OPT_SERV_FORE) {
	/* parent */
	if ((i = fork()) > 0) {
	    return;
	}
	else if (i == -1) {
	    fprintf(stderr, ";; unable to fork new process\n");
	    return;
	}
	/* child */
    }

    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sig_term);
    signal(SIGINT,  sig_term);
    signal(SIGQUIT, sig_term);
    signal(SIGCHLD, sig_child);
  
    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr,";; socket error\n");
	exit(1);
    }
  
    memset(&sin, 0, sizeof(sin));
    sin.sin_port        = htons(OptPort);
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
  
    /* bind */  
    if (bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	fprintf(stderr, ";; bind error\n");
	close(sfd);
	exit(1);
    }
  
    /* listen */  
    if (listen(sfd, SOMAXCONN) < 0) {
	fprintf(stderr, ";; listen error\n");
	close(sfd);
	exit(1);
    }

    /* make pid file */
    umask(022);
    pidfile = fopen(KNP_PIDFILE, "w");
    if (!pidfile) {
	fputs(";; can't write pidfile: " KNP_PIDFILE "\n", stderr);
    }
    else {
	fprintf(pidfile, "%d\n", getpid());
	fclose(pidfile);
    }
    umask(0);

    /* change uid and gid for security */
    ent_pw = getpwnam(KNP_SERVER_USER);
    if(ent_pw){
	gid_t dummy;
	struct group *gp;
	/* remove all supplementary groups */
	setgroups(0, &dummy);
	if ((gp = getgrgid(ent_pw->pw_gid)))
	    setgid(gp->gr_gid); 
	/* finally drop root */
	setuid(ent_pw->pw_uid);
    }

    /* accept loop */
    while (1) {
	int pid;

	if ((fd = accept(sfd, NULL, NULL)) < 0) {
	    if (errno == EINTR) 
		continue;
	    fprintf(stderr, ";; accept error\n");
	    close(sfd);
	    exit(1);
	}
    
	if ((pid = fork()) < 0) {
	    fprintf(stderr, ";; fork error\n");
	    sleep(1);
	    continue;
	}

	/* 子供 */
	if (pid == 0) {
	    char buf[1024];

	    /* ? */
	    chdir("/tmp");

	    close(sfd);
	    Infp  = fdopen(fd, "r");
	    Outfp = fdopen(fd, "w");

	    /* 挨拶 */
	    fprintf(Outfp, "200 Running KNP Server\n");
	    fflush(Outfp);

	    /* オプション解析 */
	    while (fgets(buf, sizeof(buf), Infp)) {

		/* QUIT */
		if (strncasecmp(buf, "QUIT", 4) == 0) {
		    fprintf(Outfp, "200 OK Quit\n");
		    fflush(Outfp);
		    exit(0);
		}

		if (strncasecmp(buf, "RC", 2) == 0) {
		    server_read_rc(Infp);
		    fprintf(Outfp, "200 OK\n");
		    fflush(Outfp);
		    continue;
		}

		/* RUN */
		/* Option 解析は strstr なんかでかなりいいかげん 
		   つまり間違ったオプションはエラーにならない */
		if (strncasecmp(buf, "RUN", 3) == 0) {
		    char *p;

		    if (strstr(buf, "-case"))   OptAnalysis = OPT_CASE;
		    if (strstr(buf, "-case2"))  OptAnalysis = OPT_CASE2;
		    if (strstr(buf, "-dpnd"))   OptAnalysis = OPT_DPND;
		    if (strstr(buf, "-bnst"))   OptAnalysis = OPT_BNST;
		    if (strstr(buf, "-ellipsis")) OptEllipsis |= OPT_ELLIPSIS;
		    if (strstr(buf, "-tree"))   OptExpress = OPT_TREE;
		    if (strstr(buf, "-sexp"))   OptExpress = OPT_SEXP;
		    if (strstr(buf, "-tab"))    OptExpress = OPT_TAB;
		    if (strstr(buf, "-normal")) OptDisplay = OPT_NORMAL;
		    if (strstr(buf, "-detail")) OptDisplay = OPT_DETAIL;
		    if (strstr(buf, "-debug"))  OptDisplay = OPT_DEBUG;
		    if (strstr(buf, "-expand")) OptExpandP = TRUE;
		    if ((p = strstr(buf, "-i")) != NULL) {
			p += 3;
			while(*p != '\0' && (*p == ' ' || *p == '\t')) p++;
			if (*p != '\0') OptIgnoreChar = *p;
		    } 
		    fprintf(Outfp, "200 OK option=[Analysis=%d Express=%d"
			    " Display=%d IgnoreChar=%c]\n",
			    OptAnalysis, OptExpress, OptDisplay, OptIgnoreChar);
		    fflush(Outfp);
		    break;
		} else {
		    fprintf(Outfp, "500 What?\n");
		    fflush(Outfp);
		}
	    }

	    /* 解析 */
	    knp_main();

	    /* 後処理 */
	    shutdown(fd, 2);
	    fclose(Infp);
	    fclose(Outfp);
	    close(fd);
	    exit(0); /* これしないと大変なことになるかも */
	}

	/* 親 */
	close(fd);
    }
}

/* 文字列を送って、ステータスコードを返す */  
static int send_string(FILE *fi, FILE *fo, char *str)
{
    int len, result = 0;
    char buf[1024];
    
    if (str != NULL){
	fwrite(str, sizeof(char), strlen(str), fo);
	fflush(fo);
    }

    while (fgets(buf, sizeof(buf)-1, fi) != NULL){
	len = strlen(buf);
	if (len >= 3 && buf[3] == ' ') {
	    buf[3] = '\0';
	    result = atoi(&buf[0]);
	    break;
	}
    }

    return result;
} 

/*==================================================================*/
			  void client_mode()
/*==================================================================*/
{
    /* クライアントモード (TCP/IPで接続するだけ) */

    struct sockaddr_in sin;
    struct hostent *hp;
    int fd;
    FILE *fi, *fo;
    char *p;
    char buf[1024*8];
    char option[1024];
    int  port = DEFAULT_PORT;
    int  strnum = 0;

    /* host:port という形の場合 */
    if ((p = strchr(OptHostname, ':')) != NULL) {
	*p++ = '\0';
	port = atoi(p);
    }

    /* つなげる準備 */
    if ((hp = gethostbyname(OptHostname)) == NULL) {
	fprintf(stderr, ";; host unkown\n");
	exit(1);
    }
  
    while ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
	fprintf(stderr, ";; socket error\n");
	exit(1);
    }
  
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(port);
    sin.sin_addr = *((struct in_addr * )hp->h_addr);

    if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	fprintf(stderr, ";; connect error\n");
	exit(1);
    }

    /* Server 用との通信ハンドルを作成 */
    if ((fi = fdopen(fd, "r")) == NULL || (fo = fdopen(fd, "w")) == NULL) {
	close(fd);
	fprintf(stderr, ";; fd error\n");
	exit(1);
    }

    /* 挨拶 */
    if (send_string(fi, fo, NULL) != 200) {
	fprintf(stderr, ";; greet error\n");
	exit(1);
    }

    /* オプション解析 (いいかげん) */
    option[0] = '\0';
    switch (OptAnalysis) {
    case OPT_CASE: strcat(option, " -case"); break;
    case OPT_DPND: strcat(option, " -dpnd"); break;
    case OPT_BNST: strcat(option, " -bnst"); break;
    }

    switch (OptExpress) {
    case OPT_TREE: strcat(option, " -tree"); break;
    case OPT_SEXP: strcat(option, " -sexp"); break;
    case OPT_TAB:  strcat(option, " -tab");  break;
    }

    switch (OptDisplay) {
    case OPT_NORMAL: strcat(option, " -normal"); break;
    case OPT_DETAIL: strcat(option, " -detail"); break;
    case OPT_DEBUG:  strcat(option, " -debug");  break;
    }
    
    if (OptExpandP) strcat(option, " -expand");
    if (!OptIgnoreChar) {
	sprintf(buf, " -i %c", OptIgnoreChar);
	strcat(option, buf);
    }

    /* これから動作 */
    sprintf(buf, "RUN%s\n", option);
    if (send_string(fi, fo, buf) != 200) {
	fprintf(stderr, ";; argument error OK? [%s]\n", option);
	close(fd);
	exit(1);
    }

    /* LOOP */
    strnum = 0;
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
	if (strncmp(buf, "EOS", 3) == 0) {
	    if (strnum != 0) {
		fwrite(buf, sizeof(char), strlen(buf), fo);
		fflush(fo);
		strnum = 0;
		while (fgets(buf, sizeof(buf), fi) != NULL) {
		    fwrite(buf, sizeof(char), strlen(buf), stdout);
		    fflush(stdout);
		    if (strncmp(buf, "EOS", 3) == 0)  break;
		}
	    }
	} else {
	    fwrite(buf, sizeof(char), strlen(buf), fo);
	    fflush(fo);
	    strnum++;
	}
    }

    /* 終了処理 */
    fprintf(fo,"\n%c\nQUIT\n", EOf);
    fclose(fo);
    fclose(fi);
    close(fd);
    exit(0);
}
#endif

/*==================================================================*/
		   int main(int argc, char **argv)
/*==================================================================*/
{
    option_proc(argc, argv);

    Infp  = stdin;
    Outfp = stdout;

    /* モードによって処理を分岐 */
    if (OptMode == STAND_ALONE_MODE) {
	init_all();
	knp_main();
	close_all();
    }
#ifndef _WIN32
    else if (OptMode == SERVER_MODE) {
	init_all();
	server_mode();
	close_all();
    }
    else if (OptMode == CLIENT_MODE) {
	client_mode();
    }
#endif

    exit(0);
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			      CONSTANTS

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/

#include "dbm.h"

/*====================================================================
				MACRO
====================================================================*/

#define Max(x,y) (x < y ? y : x)
#define Min(x,y) (x < y ? x : y)

#define str_eq(c1, c2) ( ! strcmp(c1, c2) )
#define L_Jiritu_M(ptr)   (ptr->jiritu_ptr + ptr->jiritu_num - 1)

#ifdef _WIN32
#define fprintf sjis_fprintf
#endif 

/*====================================================================
				LENGTH
====================================================================*/
#define	MRPH_MAX	200
#define	BNST_MAX	64
#define	BNST_LENGTH_MAX	256
#define	TAG_MAX		200
#define	PARA_MAX	32
#define PARA_PART_MAX	32
#define WORD_LEN_MAX	128
#define SENTENCE_MAX	512
#define PRINT_WIDTH	100
#define PARENT_MAX	20
#define BROTHER_MAX	20
#define TEIDAI_TYPES	5
#define HOMO_MAX	30
#define HOMO_MRPH_MAX	10

#define BGH_CODE_SIZE	11
#define SM_CODE_SIZE	12
#define SCASE_CODE_SIZE	11

#define	HomoRule_MAX	128
#define BonusRule_MAX	16
#define KoouRule_MAX	124
#define DpndRule_MAX	2625
#define DpndRule_G_MAX	2800
#define ContRule_MAX	256
#define DicForRule_MAX	1024
#define NERule_MAX	512
#define CNRule_MAX	512
#define EtcRule_MAX	1024
#define GeneralRule_MAX	1024

#define IsMrphRule	1
#define IsBnstRule	2
#define IsMrph2Rule	3

#ifndef SMALL
#define ALL_CASE_FRAME_MAX 	1024
#else
#define ALL_CASE_FRAME_MAX 	0
#endif
#define CF_ELEMENT_MAX 		20
#define PP_ELEMENT_MAX		10
#define SM_ELEMENT_MAX		256
#define EX_ELEMENT_MAX		256
#define MAX_MATCH_MAX 		10

#define CMM_MAX 	5				/* ڇŬӊŕŬ|Šߴ */
#define CPM_MAX 	64				/* ʸƢݒجߴ */
#define TM_MAX 		5				/* ڇŬЍ¸ٽ¤ߴ */

#ifndef IMI_MAX
	#define IMI_MAX	129	/* defined in "juman.h" */	
#endif

#define DATA_LEN	5120
#define SMALL_DATA_LEN	128
#define SMALL_DATA_LEN2	256
#define ALLOCATION_STEP	1024
#define DEFAULT_PARSETIMEOUT	180

#define	TBLSIZE	1024
#define	NSEED	32	/* ͰߴɽĎܯΠc2 Ď沾複ĊıĬĐĊĩĊĤc */
#define NSIZE	256

#define	BYTES4CHAR	2	/* euc-jp */

#define TREE_WIDTH_MAX  100     /* Chinese parse tree width */
#define SMALL_PROB      0.0000001 /* A small prob for Chinese relaxation */
#define TIME_PROB       50    /* time to enlarge dpnd prob for Chinese */
#define CHI_WORD_LEN_MAX 30   /* maximum Chinese word length */
#define CHI_POS_LEN_MAX  3    /* maximum Chinese pos length */

/*====================================================================
			    SIMILARITY
====================================================================*/

#define ENTITY_NUM 2220
#define ENTITY_MAX 300
#define HOWNET_NUM 56813
#define HOWNET_WORD_MAX 100
#define HOWNET_POS_MAX 10
#define HOWNET_DEF_MAX 300
#define HOWNET_TRAN_MAX 20
#define HOWNET_CONCEPT_MAX 20
#define LEVEL_NUM 20
#define LEVEL_MAX 5
#define LEVEL_PAR 1.6
#define DEF_NUM 20
#define DEF_MAX 100
#define PARA_1 0
#define PARA_2 0.4
#define PARA_3 0.4
#define PARA_4 0.2

/*====================================================================
				DEFINE
====================================================================*/
#define	JAPANESE	1
#define	CHINESE		2
#define	ENGLISH		3

#define OPT_CASE	1
#define OPT_CASE2	6
#define OPT_DPND	2
#define OPT_BNST	3
#define OPT_AssignF	4
#define OPT_ELLIPSIS	1
#define OPT_DEMO	2
#define OPT_REL_NOUN	4
#define OPT_COREFER	8
#define OPT_RAW		0
#define OPT_PARSED	1
#define OPT_INPUT_BNST	2
#define OPT_TREE	1
#define OPT_TREEF	5
#define OPT_SEXP	8
#define OPT_TAB		0
#define OPT_NOTAG	2
#define OPT_TABLE      16
#define OPT_PA	       32
#define OPT_NOTAGTREE	3
#define OPT_NORMAL	1
#define OPT_DETAIL	2
#define OPT_DEBUG	3
#define OPT_ENTITY	4
#define OPT_NBEST	5
#define OPT_SVM		2
#define OPT_DT		3
#define	OPT_SERV_FORE	1

#define	OPT_CASE_ASSIGN_GA_SUBJ	2
#define	OPT_CASE_NO	4
#define	OPT_CASE_USE_EX_ALL	8
#define	OPT_CASE_USE_PROBABILITY	16
#define	OPT_CASE_USE_REP_CF	32

#define	OPT_DISC_OR_CF	1
#define	OPT_DISC_BEST	2
#define	OPT_DISC_CLASS_ONLY	4
#define	OPT_DISC_FLAT	8
#define	OPT_DISC_TWIN_CAND	16
#define	OPT_DISC_RANKING	48
#define	OPT_DISC_NO_WO_TO	64

#define	OPT_BASELINE_NORMAL	1
#define	OPT_BASELINE_COOK	2

#define	IS_BNST_DATA	1
#define	IS_TAG_DATA	2

#define	PP_NUMBER	44
#define LOC_NUMBER	21
#define UTYPE_NUMBER	12
#define NE_MODEL_NUMBER	33

typedef enum {VERBOSE0, VERBOSE1, VERBOSE2, 
	      VERBOSE3, VERBOSE4, VERBOSE5} VerboseType;

#define PARA_KEY_O          0
#define PARA_KEY_N          1	/* ؀Ďʂγ */
#define PARA_KEY_P          2	/* ͑؀Ďʂγ */
#define PARA_KEY_A          4	/* ؀ī͑؀īʬīĩĊĤʂγ */
#define PARA_KEY_I          3	/* GAPĎĢīʂγ ii */

#define PRINT_PARA	0
#define PRINT_DPND	1
#define PRINT_MASK	2
#define PRINT_QUOTE	3
#define PRINT_RSTR	4
#define PRINT_RSTD	5
#define PRINT_RSTQ	6

#define SEMANTIC_MARKER	1
#define EXAMPLE		2

#define VOICE_SHIEKI 	1
#define VOICE_UKEMI 	2
#define VOICE_SHIEKI_UKEMI 	4
#define VOICE_MORAU 	8
#define VOICE_HOSHII 	16
#define VOICE_UNKNOWN 	32

#define FRAME_ACTIVE		1
#define FRAME_PASSIVE_I		2
#define FRAME_PASSIVE_1		3
#define FRAME_PASSIVE_2		4
#define FRAME_CAUSATIVE_WO_NI	5
#define FRAME_CAUSATIVE_WO	6
#define FRAME_CAUSATIVE_NI	7
#define FRAME_CAUSATIVE_PASSIVE	8

#define FRAME_POSSIBLE		9
#define FRAME_POLITE		10
#define FRAME_SPONTANE		11

#define CF_CAUSATIVE_WO		1
#define CF_CAUSATIVE_NI		2
#define CF_PASSIVE_1		4
#define CF_PASSIVE_2		8
#define CF_PASSIVE_I		16
#define CF_POSSIBLE		32
#define CF_POLITE		64
#define CF_SPONTANE		128

#define UNASSIGNED	-1
#define NIL_ASSIGNED	-2

#define	NIL_ASSINED_SCORE	-20
#define	FREQ0_ASSINED_SCORE	-13.815511 /* log(0.0000010) */
#define	UNKNOWN_CASE_SCORE	-11.512925 /* log(0.0000100) */
#define	UNKNOWN_CF_SCORE	-11.512925 /* log(0.0000100) */
#define	UNKNOWN_RENYOU_SCORE	-16.118096 /* log(0.0000001) */

#define	CASE_MATCH_FAILURE_SCORE	-2
#define	CASE_MATCH_FAILURE_PROB		-1001

#define END_M		-10

#define CONTINUE	-1
#define GUARD		'\n'

#define TYPE_KATAKANA	1
#define TYPE_HIRAGANA	2
#define TYPE_KANJI	3
#define TYPE_SUUJI	4
#define TYPE_EIGO	5
#define TYPE_KIGOU	6

#define SM_NO_EXPAND_NE	1
#define SM_EXPAND_NE	2
#define SM_CHECK_FULL	3
#define	SM_EXPAND_NE_DATA	4

#define RLOOP_MRM	0
#define RLOOP_RMM	1

#define RLOOP_BREAK_NONE	0
#define RLOOP_BREAK_NORMAL	1
#define RLOOP_BREAK_JUMP	2

#define LtoR		1
#define RtoL		-1

#define	CF_DECIDE_THRESHOLD	7
#define	DEFAULT_SOTO_THRESHOLD	8

/*====================================================================
				  ?
====================================================================*/

#define PARA_NIL 	0
#define PARA_NORMAL 	1	/* <P> */
#define PARA_INCOMP 	2	/* <I> */

#define REL_NOT		0 /* ݅ĊĪĊķ */
#define REL_BIT 	1 /* ޯķ݅Ċī */
#define REL_PRE 	2 /* pć݅Ċī */
#define REL_POS 	3 /* إć݅Ċī */
#define REL_PAR 	4 /* ݅ʣ 	*/
#define REL_REV 	5 /* pɴĎݤ5 */
#define REL_IN1 	6 /* ԞĞĬīp	*/
#define REL_IN2 	7 /* ԞĞĬīإ	*/
#define REL_BAD 	8 /* حĪ 	*/

/*====================================================================
		       Client/Server  ưڮŢ|ŉ
====================================================================*/

#define STAND_ALONE_MODE 0
#define SERVER_MODE      1
#define CLIENT_MODE      2

#define DEFAULT_PORT     31000
#define EOf 0x0b

#define KNP_SERVER_USER "nobody"
#define KNP_PIDFILE     "/var/run/knp.pid"

/*====================================================================
			       FEATURE
====================================================================*/

#define RF_MAX	16

/* FEATUREٽ¤ */
typedef struct _FEATURE *FEATUREptr;
typedef struct _FEATURE {
    char	*cp;
    FEATUREptr	next;
} FEATURE;

/* FEATUREőſ|ų */
typedef struct {
    FEATURE 	*fp[RF_MAX];
} FEATURE_PATTERN;

/*====================================================================
			     տ۷5լɽؽ
====================================================================*/

#define NOT_FLG '^'
#define MAT_FLG '\0'
#define AST_FLG '*'
#define QST_FLG '?'
#define NOT_STR "^"
#define AST_STR "*"
#define QST_STR "?"
#define FW_MATCHING 0
#define BW_MATCHING 1
#define ALL_MATCHING 0
#define PART_MATCHING 1
#define SHORT_MATCHING 0
#define LONG_MATCHING 1

#define RM_HINSHI_MAX 64
#define RM_BUNRUI_MAX 64
#define RM_KATA_MAX 64
#define RM_KEI_MAX  64
#define RM_GOI_MAX  64

/* ׁGőſ|ų */
typedef struct {
    char type_flag;	/* '?' or '^' or NULL */
    char ast_flag;	/* '*' or NULL */
    char Hinshi_not;
    int Hinshi[RM_HINSHI_MAX];
    char Bunrui_not;
    int Bunrui[RM_BUNRUI_MAX];
    char Kata_not;
    int Katuyou_Kata[RM_KATA_MAX];
    char Kei_not;
    char *Katuyou_Kei[RM_KEI_MAX];
    char Goi_not;
    char *Goi[RM_GOI_MAX];
    FEATURE_PATTERN f_pattern;
} REGEXPMRPH;

/* ׁGγőſ|ų */
typedef struct {
    REGEXPMRPH 	*mrph;
    char 	mrphsize;
} REGEXPMRPHS;

/* ʸ!őſ|ų */
typedef struct {
    char 	type_flag;	/* '?' or '^' or NULL */
    char 	ast_flag;	/* '*' or NULL */
    REGEXPMRPHS	*mrphs;
    FEATURE_PATTERN f_pattern;
} REGEXPBNST;

/* ʸ!γőſ|ų */
typedef struct {
    REGEXPBNST	*bnst;
    char	bnstsize;
} REGEXPBNSTS;

/*====================================================================
				 լ§
====================================================================*/

#define LOOP_BREAK	0
#define LOOP_ALL	1

/* ƱׁЛՁجլ§ */
typedef struct {
    REGEXPMRPHS	*pre_pattern;
    REGEXPMRPHS *pattern;
    FEATURE	*f;
} HomoRule;

/* ׁGγլ§ */
typedef struct {
    REGEXPMRPHS	*pre_pattern;
    REGEXPMRPHS	*self_pattern;
    REGEXPMRPHS	*post_pattern;
    FEATURE	*f;
} MrphRule;

/* ʸ!γլ§ */
typedef struct {
    REGEXPBNSTS	*pre_pattern;
    REGEXPBNSTS	*self_pattern;
    REGEXPBNSTS	*post_pattern;
    FEATURE	*f;
} BnstRule;

/* ׸Īܵıլ§ */
typedef struct {
    FEATURE_PATTERN dependant;
    FEATURE_PATTERN governor[DpndRule_G_MAX];
    char	    dpnd_type[DpndRule_G_MAX];
    FEATURE_PATTERN barrier;
    int 	    preference;
    int		    decide;	/* ЬЕċרĪĹīīĉĦī */
    double          prob_LtoR[DpndRule_G_MAX];  /* LtoR probability, only for Chinese */
    double          prob_RtoL[DpndRule_G_MAX];  /* RtoL probability, only for Chinese */
    char            dep_word[CHI_WORD_LEN_MAX];                  /* dependant word, only for Chinese */
    char            gov_word[DpndRule_G_MAX][CHI_WORD_LEN_MAX];  /* governor word, only for Chinese */
    int             count[DpndRule_G_MAX];      /* occurrence of word_pos pair */
    char            dpnd_relation[DpndRule_G_MAX][5];                /* dependant relation */
} DpndRule;

/* Ŝ|ŊŹլ§ */
typedef struct {
    REGEXPMRPHS *pattern;
    int		type;		/* ʂγĎſŤŗ */
} BonusRule;

/* ؆Ѿլ§ */
typedef struct {
    REGEXPMRPHS 	*start_pattern;
    REGEXPMRPHS 	*end_pattern;
    REGEXPMRPHS 	*uke_pattern;
    char		dpnd_type;
} KoouRule;

#define QUOTE_MAX 40

typedef struct {
    int in_num[QUOTE_MAX];
    int out_num[QUOTE_MAX];
} QUOTE_DATA;

typedef struct {
    char *key;
    FEATUREptr f;
} DicForRule;

/* ׁGγլ§, ʸ!γլ§ĎݸĞĪĲзĦĿġĎٽ¤ */
typedef struct {
    void	*RuleArray;
    int		CurRuleSize;
    int		type;
    int		mode;
    int		breakmode;
    int		direction;
} GeneralRuleType;

/* KNP Ďū|ūŕšŤūۘĪ͑ (.knprc) */
#define		DEF_JUMAN_GRAM_FILE	"JUMANʸˡŇţŬůňŪ"

#define		DEF_KNP_FILE		"KNPū|ūŕšŤū"
#define		DEF_KNP_DIR		"KNPū|ūŇţŬůňŪ"
#define		DEF_KNP_DICT_DIR	"KNPܭݱŇţŬůňŪ"
#define		DEF_KNP_DICT_FILE	"KNPܭݱŕšŤū"

#define		DEF_THESAURUS		"KNPŷŽ|ũŹ"
#define		DEF_CASE_THESAURUS	"KNPӊҲŷŽ|ũŹ"
#define		DEF_PARA_THESAURUS	"KNPʂγҲŷŽ|ũŹ"

#define		DEF_DISC_CASES		"KNPފάҲӊ"
#define		DEF_DISC_ORDER		"KNPފάҲõڷȏЏ"

#define		DEF_SVM_MODEL_FILE	"SVMŢŇūŕšŤū"
#define		DEF_DT_MODEL_FILE	"רĪ̚ŕšŤū"

#define		DEF_SVM_FREQ_SD		"SVMɑřɸݠʐڹ"
#define		DEF_SVM_FREQ_SD_NO	"SVMɑřɸݠʐڹŎӊ"

#define		DEF_SVM_REFERRED_NUM_SURFACE_SD		"SVMɽX۲ވҳߴɸݠʐڹ"
#define		DEF_SVM_REFERRED_NUM_ELLIPSIS_SD	"SVMފά۲ވҳߴɸݠʐڹ"

#define		DEF_DISC_LOC_ORDER	"KNPފάҲõڷݧݸ"
#define		DEF_DISC_SEN_NUM	"KNPފάҲõڷʸߴ"

#define		DEF_ANTECEDENT_DECIDE_TH	"KNPފάҲõڷ遼͢

#define         DEF_NE_MODEL_DIR        "NEŢŇūŕšŤūŇţŬůňŪ"
#define         DEF_SYNONYM_FILE        "ƱՁɽؽŕšŤū"

typedef struct _RuleVector {
    char	*file;
    int		type;
    int		mode;
    int		breakmode;
    int		direction;
} RuleVector;

#define RuleIncrementStep 10

/* Ɖğپğʽˡ */
#define MorphRuleType 1
#define BnstRuleType 2
#define HomoRuleType 3
#define DpndRuleType 4
#define KoouRuleType 5
#define NeMorphRuleType 6
#define NePhrasePreRuleType 7
#define NePhraseRuleType 8
#define NePhraseAuxRuleType 9
#define ContextRuleType 10
#define TagRuleType 11
#define AfterDpndBnstRuleType 12
#define AfterDpndTagRuleType 13
#define PostProcessTagRuleType 14
#define CaseFrameRuleType 15

/* ܭݱĎڇ§ߴ */
#define DICT_MAX	32

/* ܭݱĎĪՁ */
#define	BGH_DB		1
#define	SM_DB		2
#define	SM2CODE_DB	3
#define	SMP2SMG_DB	4
#define	SCASE_DB	5
#define CF_INDEX_DB	6
#define CF_DATA		7
#define	PROPER_DB	8
#define	PROPERC_DB	9
#define	PROPERCASE_DB	10
#define	CODE2SM_DB	12
#define	EVENT_DB	13
#define CF_NOUN_INDEX_DB	14
#define CF_NOUN_DATA		15
#define CF_SIM_DB	16
#define CF_CASE_DB	17
#define CF_EX_DB	18
#define CASE_DB		19
#define CFP_DB		20
#define RENYOU_DB	21
#define ADVERB_DB	22
#define PARA_DB		23
#define NOUN_CO_DB	24
#define CHI_CASE_DB	25
#define CHI_CASE_NOMINAL_DB	26
#define HOWNET_DEF_DB   27
#define HOWNET_ANTONYM_DB       28
#define HOWNET_CATEGORY_DB      29
#define HOWNET_SEM_DEF_DB      30
#define AUTO_DIC_DB	31

/* ŷŽ|ũŹĎڇ§ߴ */
#define THESAURUS_MAX	3


/*====================================================================
			      ԰˜Ň|ſ
====================================================================*/

/* ׁGŇ|ſ */
typedef struct {
    char 	Goi[WORD_LEN_MAX+1];	/* ض׿ */
    char 	Yomi[WORD_LEN_MAX+1];
    char 	Goi2[WORD_LEN_MAX+1];
    int  	Hinshi;
    int 	Bunrui;
    int 	Katuyou_Kata;
    int  	Katuyou_Kei;
    char	Imi[IMI_MAX];
    FEATUREptr	f;
    char 	*SM;				/* ĉ҃ */
} MRPH_DATA;

typedef struct cf_def *CF_ptr;
typedef struct cpm_def *CPM_ptr;
/* ʸ!Ň|ſ */
typedef struct tnode_b *Treeptr_B;
typedef struct tnode_b {
    int		type;
    /* Ȗ٦ */
    int 	num;
    /* ׁGŇ|ſ */
    int		mrph_num;
    int		preserve_mrph_num;
    MRPH_DATA 	*mrph_ptr, *head_ptr;
    /* Е̣ްʳ */
    char 	BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE+1];
    int		BGH_num;
    char 	SM_code[SM_ELEMENT_MAX*SM_CODE_SIZE+1];
    int         SM_num;
    /* ӊҲŇ|ſ */
    int 	voice;
    int 	cf_num;		/* ĽĎ͑؀ċĹīӊŕŬ|ŠĎߴ */
    CF_ptr 	cf_ptr;		/* ӊŕŬ|ŠԉͽǛγ(Case_frame_array)
				   ćĎĽĎ͑؀ĎӊŕŬ|ŠĎЌÖ */
    CPM_ptr     cpm_ptr;	/* ӊҲĎ׫ҌĎʝ۽ */
    int		pred_num;
    /* feature */
    FEATUREptr	f;
    /* ̚ٽ¤ŝŤųſ */
    Treeptr_B 	parent;
    Treeptr_B 	child[PARA_PART_MAX];
    struct tnode_b *pred_b_ptr;
    /* treeɽܨ͑ */
    int  	length;
    int 	space;
    /* ׸Īܵıްʳ (ݨͽĬӎĪإųŔ|) */
    int		dpnd_head;	/* ׸Ī(Ďʸ!Ȗ٦ */
    char 	dpnd_type;	/* ׸ĪĎſŤŗ : D, P, I, A */
    int		dpnd_dflt;	/* defaultĎ׸Ī(ʸ!Ȗ٦ */
    /* ɽXӊŇ|ſ */
    char 	SCASE_code[SCASE_CODE_SIZE];	/* ɽXӊ */
    /* ʂγٽ¤ */
    int 	para_num;	/* ѾĹīʂγٽ¤Ň|ſȖ٦ */
    char   	para_key_type;  /* ̾|ݒ|i featureīĩųŔ| */
    char	para_top_p;	/* TRUE -> PARA */
    char	para_type;	/* 0, 1:<P>, 2:<I> */
    				/* ĳĎ2ĄďPARAŎ|ŉĲƳƾĹīĿġĎĢĎ
				   dpnd_typeĊĉĈďȹ̯ċЛĊī */
    char	to_para_p;	/* ųŔ| */
    int 	sp_level;	/* ʂγٽ¤ċĹīŐŪŢ */

    char 	Jiritu_Go[BNST_LENGTH_MAX];
    DpndRule	*dpnd_rule;

    struct tnode_t *tag_ptr;
    int		tag_num;
} BNST_DATA;

/* ʂγٽ¤Ň|ſ */
typedef struct node_para_manager *Para_M_ptr;
typedef struct tnode_p *Treeptr_P;
typedef struct tnode_p {
    char 	para_char;
    int  	type;
    int  	max_num;
    int         key_pos, iend_pos, jend_pos, max_path[BNST_MAX];
    FEATURE_PATTERN f_pattern;	/* ˶ȸʸ!Ď޲ׯ */
    float	max_score;	/* Π۷-Ďڇ§Í */
    float	pure_score;	/* ˶ȸɽؽĎŜ|ŊŹĲݼĤĿÍ,֯ʂγĎ԰ݠ */
    char        status;
    Para_M_ptr  manager_ptr;
} PARA_DATA;

typedef struct node_para_manager {
    int 	para_num;
    int 	para_data_num[PARA_PART_MAX];
    int 	part_num;
    int 	start[PARA_PART_MAX];
    int 	end[PARA_PART_MAX];
    Para_M_ptr  parent;
    Para_M_ptr  child[PARA_PART_MAX];
    int 	child_num;
    BNST_DATA	*bnst_ptr;
    char 	status;
} PARA_MANAGER;

typedef struct _check {
    int num;
    int def;
    int pos[BNST_MAX];
} CHECK_DATA;

/* ʸæĎӆʸ!Ď׸Ī(ĊĉĎխϿ */
typedef struct {
    int  	head[BNST_MAX];	/* ׸Ī( */
    char  	type[BNST_MAX];	/* ׸ĪſŤŗ */
    int   	dflt[BNST_MAX];	/* ׸ĪĎշΥ */
    int 	mask[BNST_MAX];	/* ȳزڹ޲ׯ */
    int 	pos;		/* ؽڟĎݨͽЌÖ */
    CHECK_DATA	check[BNST_MAX];
    FEATURE	*f[BNST_MAX];	/* feature */
} DPND;

typedef struct thesaurus {
    char	*path;
    char	*name;
    int		*format;
    int		code_size;
    int		exist;
    DBM_FILE	db;
} THESAURUS_FILE;

/*====================================================================
				ӊҲ
====================================================================*/

typedef struct tnode_t {
    int		type;
    /* Ȗ٦ */
    int 	num;
    /* ׁGŇ|ſ */
    int		mrph_num;
    int		preserve_mrph_num;
    MRPH_DATA 	*mrph_ptr, *head_ptr;
    /* Е̣ްʳ */
    char 	BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE+1];
    int		BGH_num;
    char 	SM_code[SM_ELEMENT_MAX*SM_CODE_SIZE+1];
    int         SM_num;
    /* ӊҲŇ|ſ */
    int 	voice;
    int 	cf_num;
    CF_ptr 	cf_ptr;
    CPM_ptr     cpm_ptr;
    int		pred_num;
    /* feature */
    FEATUREptr	f;
    /* ̚ٽ¤ŝŤųſ */
    struct tnode_t	*parent;
    struct tnode_t	*child[PARA_PART_MAX];
    struct tnode_t	*pred_b_ptr;
    /* treeɽܨ͑ */
    int  	length;
    int 	space;
    /* ׸Īܵıްʳ */
    int		dpnd_head;
    char 	dpnd_type;
    int		dpnd_dflt;	/* ĤĩĊĤ? */
    /* ɽXӊŇ|ſ */
    char 	SCASE_code[SCASE_CODE_SIZE];
    /* ʂγٽ¤ */
    int 	para_num;
    char   	para_key_type;
    char	para_top_p;
    char	para_type;
    char	to_para_p;
    /* ʸ!ĈĎԘ׸ */
    int 	bnum;	/* ʸ!֨ĪĈЬ×ĹīĈĭĎȖ٦ */
    int		inum;
    BNST_DATA	*b_ptr;	/* ݪ°Ĺīʸ! */
    /* ׁGŇ|ſ */
    int		settou_num, jiritu_num, fuzoku_num;
    MRPH_DATA 	*settou_ptr, *jiritu_ptr, *fuzoku_ptr;
    int 	e_cf_num;
    /* 5ҲĎԘ׸Ň|ſ */
    CPM_ptr	c_cpm_ptr;
    /* ӊҲċĪıīʂγӊ͗G */
    struct tnode_t	*next;
} TAG_DATA;

#define CASE_MAX_NUM	20
#define CASE_TYPE_NUM	50

#define	USE_NONE -1
#define USE_BGH	1
#define	USE_NTT	2
#define	STOREtoCF	4
#define	USE_BGH_WITH_STORE	5
#define	USE_NTT_WITH_STORE	6
#define	USE_SUFFIX_SM	8
#define	USE_PREFIX_SM	16
#define USE_RN	32
#define USE_BGH_WITH_RN	33
#define USE_NTT_WITH_RN	34

#define	CF_PRED 1
#define	CF_NOUN	2

#define	CF_NORMAL	0
#define	CF_SUM		1	/* OR ĎӊŕŬ|Š */
#define	CF_GA_SEMI_SUBJECT	2
#define	CF_CHANGE	4

#define	CF_UNDECIDED	0
#define	CF_CAND_DECIDED	1
#define	CF_DECIDED	2

#define MATCH_SUBJECT	-1
#define MATCH_NONE	-2

typedef struct {
    char *kaku_keishiki;	/* ӊׁܰ */
    char *meishiku;		/* ֧̾۬ */
    char *imisosei;		/* Е̣G- */
} CF_CASE_SLOT;

typedef struct {
    char *yomi;
    char *hyoki;
    char *feature;
    char pred_type[3];
    int voice;
    int etcflag;
    int casenum;
    CF_CASE_SLOT cs[CASE_MAX_NUM];
    int	samecase[CF_ELEMENT_MAX][2];
    unsigned char *DATA;
} CF_FRAME;

/* ӊŕŬ|Šٽ¤
	{ ƾΏʸċķĆڮĩĬī
	{ ĞĿdӊŕŬ|ŠܭݱĎӆŨųňŪċĢڮĩĬī
		(VAĬīWĊĉĎެ٧ďܵ߈,º׉ĊĉċĽĬľĬЬĄ)
 */
typedef struct cf_def {
    int		type;
    int         type_flag;                              /* ȽĪ۬ćĢīīĉĦī */
    int 	element_num;				/* ӊ͗Gߴ */
    int 	oblig[CF_ELEMENT_MAX]; 			/* ɬߜӊīĉĦī */
    int 	adjacent[CF_ELEMENT_MAX];		/* ľpӊīĉĦī */
    int 	pp[CF_ELEMENT_MAX][PP_ELEMENT_MAX]; 	/* ӊݵ۬ */
    int 	sp[CF_ELEMENT_MAX];		 	/* ɽXӊ (ƾΏ¦) */
    char	*pp_str[CF_ELEMENT_MAX];
    char	*sm[CF_ELEMENT_MAX]; 			/* Е̣Ş|ū */
    char	*sm_delete[CF_ELEMENT_MAX];		/* ۈ͑֘۟Е̣Ş|ū */
    int		sm_delete_size[CF_ELEMENT_MAX];
    int		sm_delete_num[CF_ELEMENT_MAX];
    char	*sm_specify[CF_ELEMENT_MAX];		/* )؂Е̣Ş|ū */
    int		sm_specify_size[CF_ELEMENT_MAX];
    int		sm_specify_num[CF_ELEMENT_MAX];
    char 	*ex[CF_ELEMENT_MAX];			/* ͑Σ */
    char	**ex_list[CF_ELEMENT_MAX];
    int		*ex_freq[CF_ELEMENT_MAX];
    int		ex_size[CF_ELEMENT_MAX];
    int		ex_num[CF_ELEMENT_MAX];
    int		freq[CF_ELEMENT_MAX];
    char	*semantics[CF_ELEMENT_MAX];
    int 	voice;					/* ŴũŤŹ */
    int 	cf_address;				/* ӊŕŬ|ŠĎŢŉŬŹ */
    int 	cf_size;				/* ӊŕŬ|ŠĎŵŤź */
    char 	cf_id[SMALL_DATA_LEN];			/* ӊŕŬ|ŠĎID */
    char	pred_type[3];				/* ͑؀ſŤŗ (ư, ׁ, Ƚ) */
    char 	*entry;					/* ͑؀Ďɽխ */
    char 	imi[SMALL_DATA_LEN];
    int		etcflag;				/* ӊŕŬ|ŠĬ OR īĉĦī */
    char	*feature;
    int		weight[CF_ELEMENT_MAX];
    int		samecase[CF_ELEMENT_MAX][2];
    TAG_DATA	*pred_b_ptr;
    float	cf_similarity;
} CASE_FRAME;

/* ʸæĎӊ͗GĈӊŕŬ|ŠĎŹŭŃňĈĎѾɕıխϿ */
typedef struct {
    int  	flag[CF_ELEMENT_MAX];
    double	score[CF_ELEMENT_MAX];
    int		pos[CF_ELEMENT_MAX];
} LIST;

/* ʸĈӊŕŬ|ŠĎѾɕı׫ҌĎխϿ */
typedef struct {
    CASE_FRAME 	*cf_ptr;			/* ӊŕŬ|ŠĘĎŝŤųſ */
    double 	score;				/* ŹųŢ */
    double	pure_score[MAX_MATCH_MAX];	/* 5լҽĹīpĎŹųŢ */
    double	sufficiency;			/* ӊŕŬ|ŠĎˤĞĪİĢĤ */
    int 	result_num;			/* խұĹīѾԘ׸ߴ */
    LIST	result_lists_p[MAX_MATCH_MAX]; 	/* ŹųŢڇ§ĎѾԘ׸
						   (ƱŀĎެ٧ďʣߴ) */
    LIST	result_lists_d[MAX_MATCH_MAX];

    struct cpm_def	*cpm;
} CF_MATCH_MGR;

/* ʸĈ(͑؀ċĹīʣߴĎ҄ǽĊ)ӊŕŬ|ŠĎѾɕı׫ҌĎխϿ */
typedef struct cpm_def {
    CASE_FRAME 	cf;				/* ƾΏʸĎӊٽ¤ */
    TAG_DATA	*pred_b_ptr;			/* ƾΏʸĎ͑؀ʸ! */
    TAG_DATA	*elem_b_ptr[CF_ELEMENT_MAX];	/* ƾΏʸĎӊ͗Gʸ! */
    struct sentence	*elem_s_ptr[CF_ELEMENT_MAX];	/* ĉĎʸĎ͗GćĢīī (ފά͑) */
    int 	elem_b_num[CF_ELEMENT_MAX];	/* ƾΏʸĎӊ͗Gʸ!(ϢӊĎ׸Ī(ď-1,¾ďےĎݧȖ,ފάď-2,ވѾď-3) */
    double 	score;				/* ŹųŢڇ§Í(=cmm[0].score) */
    int 	result_num;			/* խұĹīӊŕŬ|Šߴ */
    int		tie_num;
    CF_MATCH_MGR cmm[CMM_MAX];			/* ŹųŢڇ§ĎӊŕŬ|ŠĈĎ
						   ѾɕıĲխϿ
						   (ƱŀĎެ٧ďʣߴ) */
    int		decided;
} CF_PRED_MGR;

/* ЬʸĎҲ׫ҌĎtխϿ */
typedef struct {
    DPND 	dpnd;		/* Ѝ¸ٽ¤ */
    int		pssb;		/* Ѝ¸ٽ¤Ď҄ǽ-ĎҿȖ̜ī */
    int		dflt;		/* i */
    double 	score;		/* ŹųŢ */
    int 	pred_num;	/* ʸæĎ͑؀ߴ */
    CF_PRED_MGR cpm[CPM_MAX];	/* ʸæĎӆ͑؀ĎӊҲ׫Ҍ */
    int		ID;		/* DPND Ď ID */
} TOTAL_MGR;

/*====================================================================
			       ʸ̮ݨͽ
====================================================================*/

typedef struct sentence {
    int 		Sen_num;	/* ʸȖ٦ 1A */
    int			available;
    int			Mrph_num;
    int			Bnst_num;
    int			New_Bnst_num;
    int			Max_New_Bnst_num;
    int			Tag_num;
    int			New_Tag_num;
    int			Para_M_num;	/* ʂγԉͽŞō|Ÿţߴ */
    int			Para_num;	/* ʂγٽ¤ߴ */
    MRPH_DATA		*mrph_data;
    BNST_DATA	 	*bnst_data;
    TAG_DATA	 	*tag_data;
    PARA_DATA		*para_data;
    PARA_MANAGER	*para_manager;
    CF_PRED_MGR		*cpm;
    CASE_FRAME		*cf;
    TOTAL_MGR		*Best_mgr;
    char		*KNPSID;
    char		*Comment;
    double		score;
} SENTENCE_DATA;

#define	CREL	1	/* ӊԘ׸ */
#define	EREL	2	/* ފάԘ׸ */

typedef struct case_component {
    char	*word;
    char	*pp_str;
    int		sent_num;
    int		tag_num;
    int		count;
    int		flag;
    struct case_component *next;
} CASE_COMPONENT;

/* ͑؀Ĉӊ͗GĎHĎٽ¤ */
typedef struct predicate_anaphora_list {
    char	*key;		/* ͑؀ */
    int		voice;
    int		cf_addr;
    CASE_COMPONENT *cc[CASE_MAX_NUM];	/* ӊ͗GĎŪŹň */
    struct predicate_anaphora_list *next;
} PALIST;

/* ͑؀ĈӊŕŬ|ŠIDĎٽ¤ */
typedef struct cf_list {
    char	*key;		/* ͑؀ */
    char	**cfid;		/* ӊŕŬ|ŠID */
    int		cfid_num;
    int		cfid_max;
    struct cf_list *next;
} CFLIST;

#define	ELLIPSIS_TAG_UNSPECIFIED_PEOPLE	-2	/* ɔƃĪ:ߍ */
#define	ELLIPSIS_TAG_I_WE		-3	/* 1ߍގ */
#define	ELLIPSIS_TAG_UNSPECIFIED_CASE	-4	/* ɔƃĪ:޵ַ */
#define	ELLIPSIS_TAG_PRE_SENTENCE	-5	/* pʸ */
#define	ELLIPSIS_TAG_POST_SENTENCE	-6	/* إʸ */
#define	ELLIPSIS_TAG_EXCEPTION		-7	/* ޝӰ */

typedef struct ellipsis_component {
    SENTENCE_DATA	*s;
    char		*pp_str;		/* Ŏӊ͑ */
    int			bnst;
    float		score;
    int			dist;			/* շΥ */
    struct ellipsis_component *next;
} ELLIPSIS_COMPONENT;

typedef struct ellipsis_cmm_list {
    CF_MATCH_MGR	cmm;
    CF_PRED_MGR		cpm;
    int			element_num;		/* ƾΏ¦ */
} ELLIPSIS_CMM;

typedef struct ellipsis_list {
    CF_PRED_MGR		*cpm;
    float		score;
    float		pure_score;
    ELLIPSIS_COMPONENT  cc[CASE_TYPE_NUM];	/* ފάӊ͗GĎŪŹň */
    FEATUREptr		f;
    int			result_num;
    ELLIPSIS_CMM	ecmm[CMM_MAX];
} ELLIPSIS_MGR;

typedef struct ellipsis_features {
    int		class;
    float	similarity;
    float	event1;
    float	event2;
    int		pos;
    int		frequency;
    int		discourse_depth;
    int		refered_num_surface;
    int		refered_num_ellipsis;

    int		c_pp;
    int		c_distance;
    int		c_dist_bnst;
    int		c_fs_flag;
    int		c_location;
    int		c_topic_flag;
    int		c_no_topic_flag;
    int		c_in_cnoun_flag;
    int		c_subject_flag;
    int		c_dep_mc_flag;
    int		c_n_modify_flag;
    char	c_dep_p_level[3];
    int		c_prev_p_flag;
    int		c_get_over_p_flag;
    int		c_sm_none_flag;
    int		c_extra_tag;

    int		p_pp;
    int		p_voice;
    int		p_type;
    int		p_sahen_flag;
    int		p_cf_subject_flag;
    int		p_cf_sentence_flag;
    int		p_n_modify_flag;
    char	p_dep_p_level[3];

    int		c_ac;
    int		match_sm_flag;
    int		match_case;
    int		match_verb;

    int		utype;
    int		objectrecognition;
} E_FEATURES;

typedef struct ellipsis_svm_features {
    float	similarity;
#ifdef DISC_USE_EVENT
    float	event1;
    float	event2;
#endif
#ifndef DISC_DONT_USE_FREQ
    float	frequency;
#endif
    float	discourse_depth_inverse;
    float	refered_num_surface;
    float	refered_num_ellipsis;

    int		c_pp[PP_NUMBER];
#ifdef DISC_USE_DIST
    int		c_distance;
    int		c_dist_bnst;
#else
    int		c_location[LOC_NUMBER];
#endif
    int		c_distance;
//    int		c_dist_bnst;
    int		c_fs_flag;
    int		c_topic_flag;
    int		c_no_topic_flag;
    int		c_in_cnoun_flag;
    int		c_subject_flag;
    int		c_n_modify_flag;
    int		c_dep_mc_flag;
    int		c_dep_p_level[6];
    int		c_prev_p_flag;
    int		c_get_over_p_flag;
    int		c_sm_none_flag;
    int		c_extra_tag[3];

    int		p_pp[3];
    int		p_voice[3];
    int		p_type[3];
    int		p_sahen_flag;
    int		p_cf_subject_flag;
    int		p_cf_sentence_flag;
    int		p_n_modify_flag;

    int		match_case;
    int		match_verb;

    int 	utype[UTYPE_NUMBER];
    int		objectrecognition;
} E_SVM_FEATURES;

typedef struct ellipsis_twin_cand_svm_features {
    float	c1_similarity;
    float	c2_similarity;

    int		c1_pp[PP_NUMBER];
    int		c1_location[LOC_NUMBER];
    int		c1_fs_flag;
    int		c1_topic_flag;
    int		c1_no_topic_flag;
    int		c1_in_cnoun_flag;
    int		c1_subject_flag;
    int		c1_n_modify_flag;
    int		c1_dep_mc_flag;
    int		c1_dep_p_level[6];
    int		c1_prev_p_flag;
    int		c1_get_over_p_flag;
    int		c1_sm_none_flag;
    int		c1_extra_tag[3];

    int		c2_pp[PP_NUMBER];
    int		c2_location[LOC_NUMBER];
    int		c2_fs_flag;
    int		c2_topic_flag;
    int		c2_no_topic_flag;
    int		c2_in_cnoun_flag;
    int		c2_subject_flag;
    int		c2_n_modify_flag;
    int		c2_dep_mc_flag;
    int		c2_dep_p_level[6];
    int		c2_prev_p_flag;
    int		c2_get_over_p_flag;
    int		c2_sm_none_flag;
    int		c2_extra_tag[3];

    int		p_pp[3];
    int		p_voice[3];
    int		p_type[3];
    int		p_sahen_flag;
    int		p_cf_subject_flag;
    int		p_cf_sentence_flag;
    int		p_n_modify_flag;
} E_TWIN_CAND_SVM_FEATURES;

typedef struct ellipsis_candidate {
    E_FEATURES	*ef;
    SENTENCE_DATA	*s;
    TAG_DATA	*tp;
    char	*tag;
} E_CANDIDATE;

/* ̾۬ĈЕ̣GĎٽ¤ */
typedef struct sm_list {
    char	*key;		/* ̾۬ */
    char	*sm;		/* Е̣G */
    struct sm_list *next;
} SMLIST;

/* ̾۬Ďȯ۲ވҳߴĎٽ¤ */
typedef struct entity_list {
    char	*key;
    int		surface_num;
    int		ellipsis_num;
    struct entity_list *next;
} ENTITY_LIST;

/*====================================================================
                               END
====================================================================*/

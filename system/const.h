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

/*====================================================================
				LENGTH
====================================================================*/
#define	MRPH_MAX	200
#define	BNST_MAX	200 /* 日本語の場合は64ぐらいで十分 */
#define	BNST_LENGTH_MAX	256
#define	TAG_MAX		200
#define	PAREN_MAX	100
#define	PARA_MAX	32
#define PARA_PART_MAX	32
#define WORD_LEN_MAX	128
#define REPNAME_LEN_MAX	256
#define CF_ID_LEN_MAX	280
#define SENTENCE_MAX	512
#define PRINT_WIDTH	100
#define PARENT_MAX	20
#define BROTHER_MAX	20
#define TEIDAI_TYPES	5
#define HOMO_MAX	30
#define HOMO_MRPH_MAX	10
#define PP_STRING_MAX   16 /* 最大となるのは"無格従属:-1"など */
#define FUKUGOJI_START  9  /* PP_STR_TO_CODEで複合辞が始まる番号 */
#define FUKUGOJI_END    37 /* PP_STR_TO_CODEで複合辞が終わる番号 */

#define BGH_CODE_SIZE	11
#define SM_CODE_SIZE	12
#define SCASE_CODE_SIZE	11

#define	HomoRule_MAX	128
#define BonusRule_MAX	16
#define KoouRule_MAX	124
#define DpndRule_MAX	128
#define DpndRule_G_MAX	35
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
#define CF_ELEMENT_MAX 		24    /* 基本的には20までだがmerge_emで必要 */
#define PP_ELEMENT_MAX		10
#define SM_ELEMENT_MAX		256
#define EX_ELEMENT_MAX		256
#define MAX_MATCH_MAX 		10

#define CF_ALIGNMENT_MAX	5

#define CMM_MAX 	5				/* 最適格フレーム数 */
#define CPM_MAX 	64				/* 文内述語数 */
#define TM_MAX 		5				/* 最適依存構造数 */

#define CLASS_NUM               2000  /* 単語クラスの数 */
#define MENTION_MAX             8     /* 1つの基本句が持つ照応詞数(ゼロ照応含む) */
#define ENTITY_MAX              4096  /* ENTITYの数 */
#define MENTIONED_MAX           256   /* 1つのENTITYが言及される回数 */

#ifndef IMI_MAX
	#define IMI_MAX	1024	/* defined in "juman.h" */	
#endif

#define DATA_LEN	5120
#define SMALL_DATA_LEN	128
#define SMALL_DATA_LEN2	256
#define ALLOCATION_STEP	1024
#define DEFAULT_PARSETIMEOUT	180

#define	TBLSIZE	1024
#define	NSEED	32	/* 乱数表の種類。2 の羃乗でなければならない。 */
#define NSIZE	256

#ifndef BYTES4CHAR
#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
#define	BYTES4CHAR	2	/* EUC-JP or SHIFT-JIS */
#else
#define	BYTES4CHAR	3	/* UTF-8 (usually) */
#endif
#endif

#define TREE_WIDTH_MAX  100     /* Chinese parse tree width */
#define CHI_WORD_LEN_MAX 30   /* maximum Chinese word length */
#define CHI_POS_LEN_MAX  3    /* maximum Chinese pos length */
#define CHI_DPND_TYPE_MAX 10 /* maximum dpnd type of Chinese word pair */
#define CHI_DPND_TYPE_LEN_MAX 10 /* maximum length of dpnd type */
#define CHI_DPND_RULE_LEN_MAX 50 /* maximum length of dpnd rule */
#define CHI_CKY_MAX 10 /* number of cky reserved for Chinese for each word pair */
#define DOUBLE_MIN  0.0000000000000001
#define CHI_TYPE_LEN_MAX 8
#define CHI_POS_MAX  33
#define CHI_ARG_NUM_MAX  30
#define CHI_POS_NBEST 2
#define CHI_DET_PENALTY 0.3

/*====================================================================
			    SIMILARITY
====================================================================*/

#define HOWNET_TRAN_MAX 100
#define HOWNET_CONCEPT_MAX 50

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
#define OPT_FILTER	5
#define OPT_ELLIPSIS	1
#define OPT_DEMO	2
#define OPT_REL_NOUN	4
#define OPT_COREFER	8
#define OPT_CASE_ANALYSIS 16
#define OPT_AUTHOR 32
#define OPT_COREFER_AUTO 64
#define OPT_AUTHOR_AUTO 128
#define OPT_ALL_CASE 256
#define OPT_RAW		0
#define OPT_PARSED	1
#define OPT_INPUT_BNST	2
#define OPT_TREE	1
#define OPT_TREEF      65
#define OPT_SEXP	8
#define OPT_TAB		0 /* TAG */
#define OPT_NOTAG	2 /* BNST */
#define OPT_MRPH	4
#define OPT_TABLE      16
#define OPT_PA	       32
#define OPT_BNSTTREE	3 /* NOTAGTREE */
#define OPT_MRPHTREE	5
#define OPT_NORMAL	1
#define OPT_DETAIL	2
#define OPT_DEBUG	3
#define OPT_ENTITY	4
#define OPT_SIMPLE	5
#define OPT_PARA_DETAIL	6
#define OPT_SVM		2
#define OPT_DT		3
#define	OPT_SERV_FORE	1
#define	OPT_CF_NE	1
#define	OPT_CF_CATEGORY	2
#define	OPT_CF_CLASS	4
#define OPT_ANAPHORA        1
#define OPT_PRINT_ENTITY    2
#define OPT_ANAPHORA_COPULA 4
#define OPT_ANAPHORA_PROB   8
#define OPT_TRAIN           16
#define OPT_UNNAMED_ENTITY  32
#define OPT_GS              64
#define OPT_NO_AUTHOR_ENTITY         128
#define OPT_NO_READER_ENTITY         256
#define OPT_AUTHOR_SCORE   512
#define OPT_READER_SCORE   1024
#define OPT_AUTHOR_ESTIMATE 2048
#define OPT_READER_ESTIMATE 4096
#define OPT_AUTHOR_AFTER    8192
#define OPT_ONLY_ENTITY    16384
#define OPT_NO_PSEUDO    	32768
#define OPT_EACH_SENTENCE   65536 	
#define OPT_ITERATIVE   131072 	
#define OPT_PRUNING   262144 	

#define	OPT_CASE_ASSIGN_GA_SUBJ	2
#define	OPT_CASE_NO	4
#define	OPT_CASE_USE_EX_ALL	8
#define	OPT_CASE_USE_PROBABILITY	16
#define	OPT_CASE_USE_REP_CF	32
#define	OPT_CASE_ADD_SOTO_WORDS	64
#define	OPT_CASE_GENERALIZE_AGENT	128
#define	OPT_CASE_GENERATE_EOS	256
#define	OPT_CASE_USE_CN_CF	512
#define	OPT_CASE_USE_CREP_CF	1024 /* 常にOPT_CASE_USE_REP_CF */
#define OPT_CASE_FIX_CF_SEARCH	2048
#define OPT_CASE_USE_NCF	4096
#define	OPT_CASE_CF_CACHE	8192
#define	OPT_CASE_CF_ON_MEMORY	16384
#define	OPT_CASE_CLEAR_CF	32768
#define	OPT_CASE_FALLBACK_TO_DPND	65536
#define	OPT_CASE_USE_CV_CF	131072 	
#define	OPT_CASE_CFCASE_FORMAT_DENOMINATOR	262144
#define	OPT_CASE_ANALYZE_DEVERBATIVE_NOUN	524288
#define	OPT_CASE_CF_USE_ID	1048576
#define	OPT_CASE_PRINT_SCORE	2097152
#define	OPT_CASE_PRINT_OBLIG	4194304
#define	OPT_CASE_PRINT_ALL_SLOT	8388608
#define	OPT_CASE_POSTPROCESS_PA	16777216

#define	OPT_DISC_OR_CF	1
#define	OPT_DISC_BEST	2
#define	OPT_DISC_CLASS_ONLY	4
#define	OPT_DISC_FLAT	8
#define	OPT_DISC_TWIN_CAND	16
#define	OPT_DISC_RANKING	48
#define	OPT_DISC_NO_WO_TO	64

#define	OPT_BASELINE_NORMAL	1
#define	OPT_BASELINE_COOK	2

#define OPT_PARA_MULTIPLY_ALL_EX	1
#define OPT_PARA_GENERATE_SIMILARITY	2
#define OPT_PARA_SYNCHRONIZE		4
#define OPT_PARA_MULTIPLY_AVE_EX	8

#define	IS_BNST_DATA	1
#define	IS_TAG_DATA	2
#define	IS_MRPH_DATA	4

#define	PP_NUMBER	44
#define LOC_NUMBER	21
#define UTYPE_NUMBER	12
#define NE_MODEL_NUMBER	33

typedef enum {VERBOSE0, VERBOSE1, VERBOSE2, 
	      VERBOSE3, VERBOSE4, VERBOSE5} VerboseType;

#define PARA_KEY_O          0
#define PARA_KEY_N          1	/* 体言の並列 */
#define PARA_KEY_P          2	/* 用言の並列 */
#define PARA_KEY_A          4	/* 体言か用言か分からない並列 */
#define PARA_KEY_I          3	/* GAPのある並列 ？？ */

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
#define FREQ0_ASSINED_SCORE     -13.815511 /* log(0.0000010) */
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
#define TYPE_KANJI	4
#define TYPE_SUUJI	8
#define TYPE_EIGO	16
#define TYPE_KIGOU	32
#define TYPE_PUNC	64

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

#define REL_NOT		0 /* 重なりなし */
#define REL_BIT 	1 /* 少し重なる */
#define REL_PRE 	2 /* 前で重なる */
#define REL_POS 	3 /* 後で重なる */
#define REL_PAR 	4 /* 重複 	*/
#define REL_REV 	5 /* 前部の修正 */
#define REL_IN1 	6 /* 含まれる前	*/
#define REL_IN2 	7 /* 含まれる後	*/
#define REL_BAD 	8 /* 誤り 	*/

/*====================================================================
		       Client/Server  動作モード
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

/* FEATURE構造体 */
typedef struct _FEATURE *FEATUREptr;
typedef struct _FEATURE {
    char	*cp;
    FEATUREptr	next;
} FEATURE;

/* FEATUREパターン */
typedef struct {
    FEATURE 	*fp[RF_MAX];
} FEATURE_PATTERN;

/*====================================================================
			     疑似正規表現
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

/* 形態素パターン */
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

/* 形態素列パターン */
typedef struct {
    REGEXPMRPH 	*mrph;
    char 	mrphsize;
} REGEXPMRPHS;

/* 文節パターン */
typedef struct {
    char 	type_flag;	/* '?' or '^' or NULL */
    char 	ast_flag;	/* '*' or NULL */
    REGEXPMRPHS	*mrphs;
    FEATURE_PATTERN f_pattern;
} REGEXPBNST;

/* 文節列パターン */
typedef struct {
    REGEXPBNST	*bnst;
    char	bnstsize;
} REGEXPBNSTS;

/*====================================================================
				 規則
====================================================================*/

#define LOOP_BREAK	0
#define LOOP_ALL	1

/* 同形異義語規則 */
typedef struct {
    REGEXPMRPHS	*pre_pattern;
    REGEXPMRPHS *pattern;
    FEATURE	*f;
} HomoRule;

/* 形態素列規則 */
typedef struct {
    REGEXPMRPHS	*pre_pattern;
    REGEXPMRPHS	*self_pattern;
    REGEXPMRPHS	*post_pattern;
    FEATURE	*f;
} MrphRule;

/* 文節列規則 */
typedef struct {
    REGEXPBNSTS	*pre_pattern;
    REGEXPBNSTS	*self_pattern;
    REGEXPBNSTS	*post_pattern;
    FEATURE	*f;
} BnstRule;

/* 係り受け規則 */
typedef struct {
    FEATURE_PATTERN dependant;
    FEATURE_PATTERN governor[DpndRule_G_MAX];
    char	    dpnd_type[DpndRule_G_MAX];
    FEATURE_PATTERN barrier;
    int 	    preference;
    int		    decide;	/* 一意に決定するかどうか */
} DpndRule;

/* ボーナス規則 */
typedef struct {
    REGEXPMRPHS *pattern;
    int		type;		/* 並列のタイプ */
} BonusRule;

/* 呼応規則 */
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

/* 形態素列規則, 文節列規則の集まりを扱うための構造体 */
typedef struct {
    void	*RuleArray;
    int		CurRuleSize;
    int		type;
    int		mode;
    int		breakmode;
    int		direction;
} GeneralRuleType;

/* KNP のルールファイル指定用 (.knprc) */
#define		DEF_JUMAN_GRAM_FILE	"JUMAN文法ディレクトリ"

#define		DEF_KNP_FILE		"KNPルールファイル"
#define		DEF_KNP_DIR		"KNPルールディレクトリ"
#define		DEF_KNP_DICT_DIR	"KNP辞書ディレクトリ"
#define		DEF_KNP_DICT_FILE	"KNP辞書ファイル"

#define		DEF_THESAURUS		"KNPシソーラス"
#define		DEF_CASE_THESAURUS	"KNP格解析シソーラス"
#define		DEF_PARA_THESAURUS	"KNP並列解析シソーラス"

#define		DEF_AUTO_DIC_FEATURES	"KNP自動獲得辞書適用属性"

#define		DEF_DISC_CASES		"KNP省略解析格"
#define		DEF_DISC_ORDER		"KNP省略解析探索範囲"

#define		DEF_SVM_MODEL_FILE	"SVMモデルファイル"
#define		DEF_DT_MODEL_FILE	"決定木ファイル"

#define		DEF_SVM_FREQ_SD		"SVM頻度標準偏差"
#define		DEF_SVM_FREQ_SD_NO	"SVM頻度標準偏差ノ格"

#define		DEF_SVM_REFERRED_NUM_SURFACE_SD		"SVM表層参照回数標準偏差"
#define		DEF_SVM_REFERRED_NUM_ELLIPSIS_SD	"SVM省略参照回数標準偏差"

#define		DEF_DISC_LOC_ORDER	"KNP省略解析探索順序"
#define		DEF_DISC_SEN_NUM	"KNP省略解析探索文数"

#define		DEF_ANTECEDENT_DECIDE_TH	"KNP省略解析探索閾値"

#define         DEF_NE_MODEL_DIR        "NEモデルファイルディレクトリ" /* SVM */
#define         DEF_NE_MODEL_FILE       "NEモデルファイル" /* CRF */
#define         DEF_SYNONYM_FILE        "同義表現ファイル"
#define         DEF_DISTSIM_FILE        "分布類似度ファイル"

typedef struct _RuleVector {
    char	*file;
    int		type;
    int		mode;
    int		breakmode;
    int		direction;
} RuleVector;

#define RuleIncrementStep 10

/* 読み込み方法 */
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
#define PreProcessMorphRuleType 16

/* 辞書の最大数 */
#define DICT_MAX	39

/* 辞書の定義 */
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
#define CHI_DPND_DB     25
#define AUTO_DIC_DB	26
#define HOWNET_DEF_DB   27
#define HOWNET_TRAN_DB  28
#define HOWNET_ANTONYM_DB       29
#define HOWNET_CATEGORY_DB      30
#define HOWNET_SEM_DEF_DB       31
#define CHI_PA_DB       32
#define CHI_DPND_PROB_DB        33
#define CHI_DIS_COMMA_DB        34
#define CHI_CASE_DB           35
#define CHI_POS_DB           36
#define NV_MI_DB	37
#define MRPH2ID_DB	38
#define SOTO_TXT	39

/* シソーラスの最大数 */
#define THESAURUS_MAX	3

/* 指定できる自動獲得辞書属性の最大数 */
#define	AUTO_DIC_FEATURES_MAX	10


/*====================================================================
			      基本データ
====================================================================*/

typedef struct tnode_b *Treeptr_B;

/* 形態素データ */
typedef struct {
    int		type;	/* タイプ (形態素, 基本句, 文節) */
    int		num;	/* 番号 (0〜) */
    /* 木構造ポインタ */
    Treeptr_B 	parent;
    Treeptr_B 	child[PARA_PART_MAX];
    /* tree表示用 */
    int  	length;
    int 	space;
    /* 係り受け情報 */
    int		dpnd_head;
    char 	dpnd_type;
    int		dpnd_dflt;	/* いらない? */
    /* 並列構造 (詳細はBNST_DATAを参照のこと) */
    int 	para_num;
    char   	para_key_type;
    char	para_top_p;
    char	para_type;
    char	to_para_p;
    /* MRPH_DATA, BNST_DATA, TAG_DATAとの共通部分はここまで */

    /* 基本句との関係 */
    int 	tnum;	/* 基本句区切りと一致するときの番号 */
    int		inum;	/* 基本句末の形態素:0, その1つ前の形態素: 1, ... */
    int		out_head_flag; /* 基本句内において、基本句の外に出る形態素かどうか */
    /* 形態素情報 */
    char 	Goi[WORD_LEN_MAX+1];	/* 原型 */
    char 	Yomi[WORD_LEN_MAX+1];
    char 	Goi2[WORD_LEN_MAX+1];
    int  	Hinshi;
    int 	Bunrui;
    int 	Katuyou_Kata;
    int  	Katuyou_Kei;
    char	Imi[IMI_MAX];
    FEATUREptr	f;
    int         Num;         /* 文先頭からこの形態素の終りまでの文字数 */
    char 	*SM;				/* 追加 */
    char        Pos[CHI_POS_LEN_MAX+1];    /* pos-tag for Chinese word */
    char        Type[CHI_TYPE_LEN_MAX+1];    /* type for Chinese word */
} MRPH_DATA;

typedef struct cf_def *CF_ptr;
typedef struct cpm_def *CPM_ptr;
/* 文節データ */
typedef struct tnode_b {
    int		type;	/* タイプ (形態素, 基本句, 文節) */
    int 	num;	/* 番号 (0〜) */
    /* 木構造ポインタ */
    Treeptr_B 	parent;
    Treeptr_B 	child[PARA_PART_MAX];
    /* tree表示用 */
    int  	length;
    int 	space;
    /* 係り受け情報 (処理が確定後コピー) */
    int		dpnd_head;	/* 係り先の文節番号 */
    char 	dpnd_type;	/* 係りのタイプ : D, P, I, A */
    int		dpnd_dflt;	/* defaultの係り先文節番号 */
    /* 並列構造 */
    int 	para_num;	/* 対応する並列構造データ番号 */
    char   	para_key_type;  /* 名|述|？ featureからコピー */
    char	para_top_p;	/* TRUE -> PARA */
    char	para_type;	/* 0, 1:<P>, 2:<I> */
    				/* この2つはPARAノードを導入するためのもの
				   dpnd_typeなどとは微妙に異なる */
    char	to_para_p;	/* コピー */
    /* MRPH_DATA, BNST_DATA, TAG_DATAとの共通部分はここまで */

    int 	sp_level;	/* 並列構造に対するバリア */
    /* 形態素データ */
    int		mrph_num;
    int		preserve_mrph_num;
    MRPH_DATA 	*mrph_ptr, *head_ptr;
    /* 意味情報 */
    char 	BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE+1];
    int		BGH_num;
    char 	SM_code[SM_ELEMENT_MAX*SM_CODE_SIZE+1];
    int         SM_num;
    /* 格解析データ */
    int 	voice;
    int 	cf_num;		/* その用言に対する格フレームの数 */
    CF_ptr 	cf_ptr;		/* 格フレーム管理配列(Case_frame_array)
				   でのその用言の格フレームの位置 */
    CPM_ptr     cpm_ptr;	/* 格解析の結果の保持 */
    int		pred_num;
    /* feature */
    FEATUREptr	f;
    /* 用言文節へのポインタ */
    struct tnode_b *pred_b_ptr;
    /* tag for Chinese coordination */
    int         is_para;
    /* 表層格データ */
    char 	SCASE_code[SCASE_CODE_SIZE];	/* 表層格 */
    /* BNST_DATA, TAG_DATAとの共通部分はここまで */

    char 	Jiritu_Go[BNST_LENGTH_MAX];
    DpndRule	*dpnd_rule;

    struct tnode_t *tag_ptr;
    int		tag_num;
} BNST_DATA;

/* 並列構造データ */
typedef struct node_para_manager *Para_M_ptr;
typedef struct tnode_p *Treeptr_P;
typedef struct tnode_p {
    char 	para_char;
    int  	type;
    int  	max_num;
    int         key_pos, iend_pos, jend_pos, max_path[BNST_MAX];
    FEATURE_PATTERN f_pattern;	/* 末尾文節の条件 */
    float	max_score;	/* 類似性の最大値 */
    float	pure_score;	/* 末尾表現のボーナスを除いた値,強並列の基準 */
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

/* 文中の各文節の係り先などの記録 */
typedef struct {
    int  	head[BNST_MAX];	/* 係り先 */
    char  	type[BNST_MAX];	/* 係りタイプ */
    int   	dflt[BNST_MAX];	/* 係りの距離 */
    int 	mask[BNST_MAX];	/* 非交差条件 */
    int 	pos;		/* 現在の処理位置 */
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

/* Chinese dpnd rule */
typedef struct {
    char  	direction[CHI_DPND_TYPE_MAX]; /* store different directions for each type */
    double      prob_LtoR[CHI_DPND_TYPE_MAX]; /* store different probability for each type */
    double      prob_RtoL[CHI_DPND_TYPE_MAX];
    double      prob_pos_LtoR; /* store different probability of pos pairs for each type */
    double      prob_pos_RtoL;
    char        type[CHI_DPND_TYPE_MAX][CHI_DPND_TYPE_LEN_MAX]; /* store different dpnd type */
    double      occur_pos; /* store occur time of different dpnd type */
    double      prob_dis_comma_LtoR[CHI_DPND_TYPE_MAX];
    double      prob_dis_comma_RtoL[CHI_DPND_TYPE_MAX];

  // variables storing probability for model with pos-tagging
    double      lex_prob_LtoR[CHI_DPND_TYPE_MAX];
    double      lex_prob_RtoL[CHI_DPND_TYPE_MAX];
    double      lex_prob_dis_comma_LtoR[CHI_DPND_TYPE_MAX];
    double      lex_prob_dis_comma_RtoL[CHI_DPND_TYPE_MAX];

  int       left_pos_index[CHI_DPND_TYPE_MAX];
  int       right_pos_index[CHI_DPND_TYPE_MAX];

    int         count; /* number of dpnd type */
} CHI_DPND;

/* Chinese pos rule */
typedef struct {
  char        *pos[CHI_POS_MAX];
  double       prob[CHI_POS_MAX];
  double       prob_pos_index[CHI_POS_MAX];
  int          pos_index[CHI_POS_MAX];
  int          pos_max;
} CHI_POS;

/* Chinese root rule */
typedef struct {
  double       prob[CHI_POS_MAX];
  int          pos_index[CHI_POS_MAX];
} CHI_ROOT;

/*====================================================================
			       照応解析
====================================================================*/

/* ENTITYへの言及(ゼロを含む) */
typedef struct mention {
    int 		sent_num;
    int                 tag_num;
    char                cpp_string[PP_STRING_MAX]; /* 用言の格要素としての格 */   
    char                spp_string[PP_STRING_MAX]; /* 格構造における表層格 */
    char                type; /* 'S', '=', 'N', 'C', 'O', 'D' */
    double              salience_score; /* どのくらい先行詞になりやすいか */    
	double              static_salience_score;/* salience_scoreの元になる値。*/
	/*各mentionのstatic_salience_score*(SALIENCE_DACAY_RATE**sentece_distance)の和をとると salience_scoreになる */
    struct tnode_t      *tag_ptr;
    struct entity       *entity;
    struct mention      *explicit_mention; /* 直接の格要素へのポインタ(flag=Cの場合のみ) */
} MENTION;

/* 基本句ごとにMENTIONを管理する構造体 */
typedef struct mention_manager {
    /* 0番目には必ず自分自身を入れる */
    /* 1番目以降は関係する格要素を入れていく */
    int                 num;
    char 	        cf_id[CF_ID_LEN_MAX]; /* 格構造出力用 */
    CF_ptr 	        cf_ptr;
    MENTION             mention[MENTION_MAX];        
} MENTION_MGR;

/* ENTITY */
typedef struct entity {
    int                 num;
    int                 output_num; /* {A,B}=CとなっているときにAとBの出力numを統一する */
    int                 mentioned_num;  /* 言及されている回数 */
	int                 link_entity;
	int                 first_appearance;
    double              salience_score; /* どのくらい先行詞になりやすいか */
    double              salience_mem; /* 正解を読み込んだ場合のsalienceの変化の記録 */
    int                 tmp_salience_flag; /* 同一文中に出現したノ格 */
	int                 hypothetical_flag;
	int                 real_entity;//hypothetical entityに実体がある場合(一人称->私Entity)
	int                 hypothetical_entity;//対応するhypothetical_entity(私Entity->一人称)
	int                 skip_flag;/*1ならそのentityは格に割り当てない*/
	char                hypothetical_name[WORD_LEN_MAX+1];
    MENTION             *mention[MENTIONED_MAX];
	char                named_entity[WORD_LEN_MAX];
    char                name[WORD_LEN_MAX+1]; /* ENTITY名 */
	int                 corefer_id; //自動解析結果を読み込むため
	int                 rep_sen_num;
	int                 rep_tag_num;
} ENTITY;

/* 文章全体に出現したENTITYを管理する構造体 */
typedef struct entity_manager {
    int                 num;
    ENTITY              entity[ENTITY_MAX];        
} ENTITY_MGR;

/*====================================================================
				格解析
====================================================================*/

typedef struct tnode_t {
    int		type;	/* タイプ (形態素, 基本句, 文節) */
    int 	num;	/* 番号 (0〜) */
    /* 木構造ポインタ */
    struct tnode_t	*parent;
    struct tnode_t	*child[PARA_PART_MAX];
    /* tree表示用 */
    int  	length;
    int 	space;
    /* 係り受け情報 */
    int		dpnd_head;
    char 	dpnd_type;
    int		dpnd_dflt;	/* いらない? */
    /* 並列構造 (詳細はBNST_DATAを参照のこと) */
    int 	para_num;
    char   	para_key_type;
    char	para_top_p;
    char	para_type;
    char	to_para_p;
    /* MRPH_DATA, BNST_DATA, TAG_DATAとの共通部分はここまで */

    int 	sp_level;	/* 並列構造に対するバリア */
    /* 形態素データ */
    int		mrph_num;
    int		preserve_mrph_num;
    MRPH_DATA 	*mrph_ptr, *head_ptr;
    /* 意味情報 */
    char 	BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE+1];
    int		BGH_num;
    char 	SM_code[SM_ELEMENT_MAX*SM_CODE_SIZE+1];
    int         SM_num;
    /* 格解析データ */
    int 	voice;
    int 	cf_num;
    CF_ptr 	cf_ptr;
    CPM_ptr     cpm_ptr;
    int		pred_num;
    /* feature */
    FEATUREptr	f;
    /* 用言文節へのポインタ */
    struct tnode_t	*pred_b_ptr;
    /* tag for Chinese coordination */
    int         is_para;
    /* 表層格データ */
    char 	SCASE_code[SCASE_CODE_SIZE];
    /* BNST_DATA, TAG_DATAとの共通部分はここまで */

    /* 文節との関係 */
    int 	bnum;	/* 文節区切りと一致するときの番号 */
    int		inum;	/* 文節末の基本句:0, その1つ前の基本句: 1, ... */
    BNST_DATA	*b_ptr;	/* 所属する文節 */
    /* 形態素データ */
    int		settou_num, jiritu_num, fuzoku_num;
    MRPH_DATA 	*settou_ptr, *jiritu_ptr, *fuzoku_ptr;
    int 	e_cf_num;
    /* 正解の関係データ */
    CPM_ptr	c_cpm_ptr;
    /* 格解析における並列格要素 */
    struct tnode_t	*next;
    /* MENTIONの管理 */
    MENTION_MGR mention_mgr;
    /* 照応解析 */
    struct tcf_def *tcf_ptr; /* 入力基本句の表層的な格構造 */
    struct ctm_def *ctm_ptr; /* 基本句の格・省略解析結果の記録 */
	double score_diff; /*省略解析結果の信頼度*/
	double ga_score_diff; /*ガ格の省略解析結果の信頼度*/
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
#define USE_DISTSIM	64

#define	CF_PRED 1
#define	CF_NOUN	2

#define	CF_NORMAL	0
#define	CF_SUM		1	/* OR の格フレーム */
#define	CF_GA_SEMI_SUBJECT	2
#define	CF_CHANGE	4

#define	CF_UNDECIDED	0
#define	CF_CAND_DECIDED	1
#define	CF_DECIDED	2

#define MATCH_SUBJECT	-1
#define MATCH_NONE	-2

typedef struct {
    char *kaku_keishiki;	/* 格形式 */
    char *meishiku;		/* 名詞句 */
    char *imisosei;		/* 意味素性 */
} CF_CASE_SLOT;

typedef struct {
    char *cf_id;
    int aligned_case[CF_ELEMENT_MAX][2];
} CF_ALIGNMENT;

typedef struct cf_frame_def {
    unsigned int address;
    char *yomi;
    char *hyoki;
    char *feature;
    char pred_type[BYTES4CHAR + 1];
    int voice;
    int etcflag;
    int casenum;
    CF_CASE_SLOT cs[CASE_MAX_NUM];
    int	samecase[CF_ELEMENT_MAX][2];
    CF_ALIGNMENT cf_align[CF_ALIGNMENT_MAX];
    unsigned char *DATA;
    struct cf_frame_def *next;
} CF_FRAME;

/* 格フレーム構造体
	○ 入力文に対して作られる
	○ また，格フレーム辞書の各エントリにも作られる
		(「〜れる」などの場合は受身,尊敬などにそれぞれ一つ)
 */
typedef struct cf_def {
    int		type;                                   /* 格フレームの種類 */
    int         type_flag;                              /* 用言・名詞両方の格フレームを調べる場合 */
    int 	element_num;				/* 格要素数 */
    int 	oblig[CF_ELEMENT_MAX]; 			/* 必須格かどうか */
    int 	adjacent[CF_ELEMENT_MAX];		/* 直前格かどうか */
    int 	pp[CF_ELEMENT_MAX][PP_ELEMENT_MAX]; 	/* 格助詞 */
    int 	sp[CF_ELEMENT_MAX];		 	/* 表層格 (入力側) */
    char	*pp_str[CF_ELEMENT_MAX];
    char	*sm[CF_ELEMENT_MAX]; 			/* 意味マーカ */
    char	*sm_delete[CF_ELEMENT_MAX];		/* 使用禁止意味マーカ */
    int		sm_delete_size[CF_ELEMENT_MAX];
    int		sm_delete_num[CF_ELEMENT_MAX];
    char	*sm_specify[CF_ELEMENT_MAX];		/* 制限意味マーカ */
    int		sm_specify_size[CF_ELEMENT_MAX];
    int		sm_specify_num[CF_ELEMENT_MAX];
    char 	*ex[CF_ELEMENT_MAX];			/* 用例 */
    char	**ex_list[CF_ELEMENT_MAX];
    int		*ex_freq[CF_ELEMENT_MAX];
    int		ex_size[CF_ELEMENT_MAX];
    int		ex_num[CF_ELEMENT_MAX];
    int		freq[CF_ELEMENT_MAX];
    char	*semantics[CF_ELEMENT_MAX];
    char	**gex_list[CF_ELEMENT_MAX];              /* 用例中に記された汎化素性を格納 */
    double	*gex_freq[CF_ELEMENT_MAX];
    int		gex_size[CF_ELEMENT_MAX];
    int		gex_num[CF_ELEMENT_MAX];
    int 	voice;					/* ヴォイス */
    unsigned int	cf_address;				/* 格フレームのアドレス */
    int 	cf_size;				/* 格フレームのサイズ */
    char 	cf_id[CF_ID_LEN_MAX];			/* 格フレームのID (代表表記+タイプ+番号) */
    char	pred_type[BYTES4CHAR + 1];		/* 用言タイプ (動, 形, 判) */
    char 	*entry;					/* 用言の表記 */
    char 	imi[SMALL_DATA_LEN];
    int		etcflag;				/* 格フレームが OR かどうか */
    char	*feature;
    int		weight[CF_ELEMENT_MAX];
    int		samecase[CF_ELEMENT_MAX][2];
    CF_ALIGNMENT cf_align[CF_ALIGNMENT_MAX];
    TAG_DATA	*pred_b_ptr;
    float	cf_similarity;
} CASE_FRAME;

/* 文中の格要素と格フレームのスロットとの対応付け記録 */
typedef struct {
    /* ある格がどの入力格要素に対応しているかの番号 */
    int  	flag[CF_ELEMENT_MAX];
    /* そのスコア */
    double	score[CF_ELEMENT_MAX];
    /* 用例の何番目とマッチしたか(主体マッチの場合はMATCH_SUBJECT(-1)) */
    int		pos[CF_ELEMENT_MAX];
} LIST;

/* 文と格フレームの対応付け結果の記録 */
typedef struct {
    CASE_FRAME 	*cf_ptr;			/* 格フレームへのポインタ */
    double 	score;				/* スコア */
    double	pure_score[MAX_MATCH_MAX];	/* 正規化する前のスコア */
    double	sufficiency;			/* 格フレームの埋まりぐあい */
    int 	result_num;			/* 記憶する対応関係数 */
    /* パターン(格フレーム)から見た割り当て状況 */
    LIST	result_lists_p[MAX_MATCH_MAX]; 	/* スコア最大の対応関係
						   (同点の場合は複数) */
    /* データ(入力)から見た割り当て状況 */
    LIST	result_lists_d[MAX_MATCH_MAX];
    /* result_lists_d[0].flag[10] == 4 ならば result_lists_p[0].flag[4] == 10 */

    struct cpm_def	*cpm;
} CF_MATCH_MGR;

/* 文と(用言に対する複数の可能な)格フレームの対応付け結果の記録 */
typedef struct cpm_def {
    CASE_FRAME 	cf;				/* 入力文の格構造 */
    TAG_DATA	*pred_b_ptr;			/* 入力文の用言文節 */
    TAG_DATA	*elem_b_ptr[CF_ELEMENT_MAX];	/* 入力文の格要素文節 */
    TAG_DATA	*para_b_ptr[CF_ELEMENT_MAX];	/* 入力文の格要素文節の並列要素 */
    struct sentence	*elem_s_ptr[CF_ELEMENT_MAX];	/* どの文の要素であるか (省略用) */
    int 	elem_b_num[CF_ELEMENT_MAX];	/* 入力文の格要素文節(連格の係り先は-1,他は子の順番,省略は-2,照応は-3) */
    double 	score;				/* スコア最大値(=cmm[0].score) */
    int 	result_num;			/* 記憶する格フレーム数 */
    int		tie_num;
    CF_MATCH_MGR cmm[CMM_MAX];			/* スコア最大の格フレームとの
						   対応付けを記録
						   (同点の場合は複数) */
    int		decided;
} CF_PRED_MGR;

/* 一文の解析結果の全記録 */
typedef struct {
    DPND 	dpnd;		/* 依存構造 */
    int		pssb;		/* 依存構造の可能性の何番目か */
    int		dflt;		/* ？ */
    double 	score;		/* スコア */
    int 	pred_num;	/* 文中の用言数 */
    CF_PRED_MGR cpm[CPM_MAX];	/* 文中の各用言の格解析結果 */
    int		ID;		/* DPND の ID */
} TOTAL_MGR;

typedef struct _CKY *CKYptr;
typedef struct _CKY {
    int		i;
    int		j;
    char	cp;
    double	score;		/* score at this point */
    double	para_score;	/* coordination score */
    double      chicase_score;
    double      chicase_lex_score;
    int		para_flag;	/* coordination flag */
    char	dpnd_type;	/* type of dependency (D or P) */
    int		direction;	/* direction of dependency */
    int         index;          /* index of dpnd rule for Chinese */
    BNST_DATA	*b_ptr;
    int 	scase_check[SCASE_CODE_SIZE];
    int		un_count;
    CF_PRED_MGR *cpm_ptr;	/* case components */
    CKYptr	left;		/* pointer to the left child */
    CKYptr	right;		/* pointer to the right child */
    CKYptr	next;		/* pointer to the next CKY data at this point */

  int        left_pos_index;  /* pos index for Chinese */
  int        right_pos_index;  /* pos index for Chinese */
} CKY;

typedef struct _cfcm_def {
    char	*key;
    int		pos;
    int		num;
    struct _cfcm_def	*next;
} CASE_FRAME_CACHE_MGR;

/*====================================================================
			       文脈処理
====================================================================*/

typedef struct sentence {
    int 		Sen_num;	/* 文番号 1〜 */
    int			available;
    int			Mrph_num;
    int			New_Mrph_num;
    int			Bnst_num;
    int			New_Bnst_num;
    int			Max_New_Bnst_num;
    int			Tag_num;
    int			New_Tag_num;
    int			Para_M_num;	/* 並列管理マネージャ数 */
    int			Para_num;	/* 並列構造数 */
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

#define	CREL	1	/* 格関係 */
#define	EREL	2	/* 省略関係 */

typedef struct case_component {
    char	*word;
    char	*pp_str;
    int		sent_num;
    int		tag_num;
    int		count;
    int		flag;
    struct case_component *next;
} CASE_COMPONENT;

/* 用言と格要素の組の構造体 */
typedef struct predicate_anaphora_list {
    char	*key;		/* 用言 */
    int		voice;
    int		cf_addr;
    CASE_COMPONENT *cc[CASE_MAX_NUM];	/* 格要素のリスト */
    struct predicate_anaphora_list *next;
} PALIST;

/* 用言と格フレームIDの構造体 */
typedef struct cf_list {
    char	*key;		/* 用言 */
    char	**cfid;		/* 格フレームID */
    int		cfid_num;
    int		cfid_max;
    struct cf_list *next;
} CFLIST;

#define	ELLIPSIS_TAG_UNSPECIFIED_PEOPLE	-2	/* 不特定:人 */
#define	ELLIPSIS_TAG_I_WE		-3	/* 1人称 */
#define	ELLIPSIS_TAG_UNSPECIFIED_CASE	-4	/* 不特定:状況 */
#define	ELLIPSIS_TAG_PRE_SENTENCE	-5	/* 前文 */
#define	ELLIPSIS_TAG_POST_SENTENCE	-6	/* 後文 */
#define	ELLIPSIS_TAG_EXCEPTION		-7	/* 対象外 */

typedef struct ellipsis_component {
    SENTENCE_DATA	*s;
    char		*pp_str;		/* ノ格用 */
    int			bnst;
    float		score;
    int			dist;			/* 距離 */
    struct ellipsis_component *next;
} ELLIPSIS_COMPONENT;

typedef struct ellipsis_cmm_list {
    CF_MATCH_MGR	cmm;
    CF_PRED_MGR		cpm;
    int			element_num;		/* 入力側 */
} ELLIPSIS_CMM;

typedef struct ellipsis_list {
    float		score;                  /* 正規化されたスコア */
    float		pure_score;             /* 正規化前のスコア */
    ELLIPSIS_COMPONENT  cc[CASE_TYPE_NUM];	/* 省略格要素のリスト */
    FEATUREptr		f;
    int			result_num;
    ELLIPSIS_CMM	ecmm[CMM_MAX];
} ELLIPSIS_MGR;

typedef struct ellipsis_features {
    int		ellipsis_class;
    float	similarity;
    float	event1;
    float	event2;
    int		pos;
    int		frequency;
    int		discourse_depth;
    float	refered_num_surface;
    float	refered_num_ellipsis;

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

/* 名詞と意味素の構造体 */
typedef struct sm_list {
    char	*key;		/* 名詞 */
    char	*sm;		/* 意味素 */
    struct sm_list *next;
} SMLIST;

/* 名詞の被参照回数の構造体 */
typedef struct entity_list {
    char	*key;
    double	surface_num;
    double	ellipsis_num;
    struct entity_list *next;
} ENTITY_LIST;

/*====================================================================
			 ENTITY BASE 文脈処理
====================================================================*/

#define FUNCTIONAL_TAG_MAX 5
/* 入力基本句の格構造 */
typedef struct tcf_def {
    CASE_FRAME 	    cf;				 /* 入力文の格構造 */
	CASE_FRAME 	    cf_with_functional_tag[FUNCTIONAL_TAG_MAX]; /* 入力文の格構造(1以降には機能的基本句の格構造が入る) */
	int             cf_num; 
    TAG_DATA	    *pred_b_ptr;		 /* 入力文の用言文節 */
    TAG_DATA	    *elem_b_ptr[CF_ELEMENT_MAX]; /* 入力文の格要素文節 */
	int         map_tcf_elem_to_cf[CF_ELEMENT_MAX];/*何番目のcfと対応するか(0は本動詞)*/
	int         map_tcf_elem_to_cf_elem[CF_ELEMENT_MAX];/*各格構造では何番目の格要素と対応するか*/
    int 	    elem_b_num[CF_ELEMENT_MAX];	 /* 入力文の格要素文節(連格の係り先は-1,他は子の順番) */
} TAG_CASE_FRAME;

/* CF_TAG_MGR中のomit_feature用の定数 */
#define ELLIPSIS_CASE_NUM 4

#define NO_ASSIGNMENT     0 /* ある格スロットが対応付けられない確率 */
#define EX_PMI            1 /* 語PMI */	       
#define CEX_PMI           2 /* カテゴリPMI */	       
#define NEX_PMI           3 /* 固有表現PMI */	       
#define CLS_PMI           4 /* クラスPMI */
#define WA_IN_THE_SENT    5 /* 先行詞候補が同一文に出現かつ格助詞「は」を伴う */
#define MAX_PMI           6 /*PMIで最大のもの*/
//#define SALIENCE_CHECK    6 /* SALIENCEが一定以上あるかどうか */
#define NE_PERSON         7 /* 先行詞候補が人名 */
#define NE_CHECK     8 /* 先行詞候補がNE*/
#define ASSIGNED          9 /* 埋まったかどうか */
#define OVERT_APPEARANCE  10 /* 文章全体の出現回数 */
#define BEFORE_OVERT_APPEARANCE  11 /* 前の文での出現回数 */
#define AFTER_OVERT_APPEARANCE  12 /* 後の文での出現回数 */
#define SAME_PRED         13 /*同じ動詞+格での出現*/
#define ASSIGNMENT       14 /*ある格スロットが対応付けられる確率*/
#define AUTHOR_SCORE 15
#define READER_SCORE 16
#define EX_CASE_PROB            17 /* 語が格スロットに入る確率 */	       
#define EX_PROB            18 /* 語が出現する確率 */	       
#define CEX_CASE_PROB           19 /* カテゴリPMI */	       
#define CEX_PROB           20 /* カテゴリPMI */	       
#define NEX_CASE_PROB           21 /* 固有表現PMI */	       
#define NEX_PROB           22 /* 固有表現PMI */	       
#define CLS_CASE_PROB           23 /* クラスPMI */
#define CLS_PROB           24 /* クラスPMI */
#define ALT_CT_PMI 25
#define ALT_CT_CASE_PROB 26
#define ALT_CT_PROB 27
#define CF_GA_FILLED_RATIO 28
#define EX_ASSIGNMENT 29 /*用例の割合からの埋まりやすさ*/
#define OLD_TOPIC 30 /*「は」で出現したが以降で別の「は」が出現*/
#define ABILITY 31
#define UNNAMED_CASE_PROB 32
#define VOICE_S 33
#define CLOSEST_APPEARANCE_S (VOICE_S+VOICE_NUM)
#define CLOSEST_APPEARANCE_NUM 4 
#define PRED_DPND_TYPE_S (CLOSEST_APPEARANCE_NUM+CLOSEST_APPEARANCE_S)
#define PRED_DPND_TYPE_NUM 3
#define VERB_SITUATION_S (PRED_DPND_TYPE_S+PRED_DPND_TYPE_NUM)
#define VERB_SITUATION_NUM 2
#define NE_FEATURE_S (VERB_SITUATION_S+VERB_SITUATION_NUM)
#define NE_FEATURE_NUM NE_NUM
#define LOCATION_S       (NE_FEATURE_S+NE_FEATURE_NUM)  /* 位置カテゴリ素性の開始位置 */
#define LOCATION_NUM    (135) /* 位置カテゴリ素性の種類 + simple_loc_num */
#define SIMPLE_LOCATION_S (LOCATION_S+LOCATION_NUM*3) 
#define SIMPLE_LOCATION_NUM 15
#define UNNAMED_NUM_S  (SIMPLE_LOCATION_S+SIMPLE_LOCATION_NUM*3)
#define YOBIKAKE_S (UNNAMED_NUM_S+UNNAMED_ENTITY_NUM*ELLIPSIS_CASE_NUM)
#define MODALITY_S (YOBIKAKE_S+UNNAMED_ENTITY_NUM)
#define MODALITY_F_NUM (MODALITY_NUM*2)
#define KEIGO_S (MODALITY_S+MODALITY_F_NUM)
#define KEIGO_F_NUM  (KEIGO_NUM*2)
#define TENSE_S (KEIGO_S+KEIGO_F_NUM)
#define TENSE_NUM 4 
#define CONTEXT_S (TENSE_S+TENSE_NUM)
#define CONTEXT_FEATURE_NUM 10
#define EACH_FEARUTE_NUM        (CONTEXT_S+CONTEXT_FEATURE_NUM)

#define NE_NUM 8

#define REAL_S        (EACH_FEARUTE_NUM) /*普通の素性*/
#define FILLED_S        (EACH_FEARUTE_NUM*2) /*省略でない部分*/
#define UNNAMED_S        (EACH_FEARUTE_NUM*3) /*UNNAMED_ENTITYの素性*/
#define ADJ_FEATURE_S (EACH_FEARUTE_NUM*(3+UNNAMED_ENTITY_NUM))
#define O_FEATURE_NUM (2*(EACH_FEARUTE_NUM*(3+UNNAMED_ENTITY_NUM)))



#define UNNAMED_ENTITY_NUM 5
#define MODALITY_NUM 11
#define VOICE_NUM 5
#define KEIGO_NUM 3
#define SENTENCE_CATEGORY_NUM 4 //共通、1文目、2、3文目、それ以降

/* 基本句の格・省略解析結果の記録 */
typedef struct ctm_def {
    double      score;                           /* 対応付けのスコア */
	double      score_def;/*トレーニング時の重みでのスコア*/           
	double      case_analysis_score; /*擬似的な格解析スコア*/
    CASE_FRAME 	*cf_ptr;			 /* 格フレームへのポインタ */

    /* 格フレームの要素のうち対応付けがついたもの
       (=cf_element_numに含まれる)のみTRUEとなる */
    int         filled_element[CF_ELEMENT_MAX];  

    /* ENTITYのうち対応付けがついたもの
       (=entity_numに含まれる)のみTRUEとなる */
    int         filled_entity[ENTITY_MAX];

    /* 対応付けられなかった入力文格要素
       入力文格要素のうち対応が付いていないものの番号を保存 */
    int         non_match_element[CF_ELEMENT_MAX]; 

    /* 以下は基本句の格構造の情報 */
    int         result_num;                      /* 対応付けられた要素数 */
    int         case_result_num;                 /* 格解析で対応付けられた要素数 */
	int         annotated_result_num;            /* 正解コーパスにあって対応付けられた要素数 */
    int         cf_element_num[CF_ELEMENT_MAX];  /* 格フレームの格要素への対応 */
    int         tcf_element_num[CF_ELEMENT_MAX]; /* 入力文の格要素への対応 */
    int         tcf_element_num_functional[CF_ELEMENT_MAX]; /* 入力文の格要素への対応(機能的基本句) */
    TAG_DATA    *elem_b_ptr[CF_ELEMENT_MAX];     /* 関連付けられた基本句 */       
    int         entity_num[CF_ELEMENT_MAX];      /* 関連付けられたENTITY */
    char        type[CF_ELEMENT_MAX];            /* 'S', 'N', 'C', 'O', 'D' */
	int         ga_entity;                       /*省略の場合のガ格が何で埋まったか*/
	int         case_analysis_ga_entity;         /*格解析の場合のガ格が何で埋まったか*/
    /* 機械学習用のfeature */
    double overt_arguments_score;                          /* 格解析スコア */
    double all_arguments_score;                          /* 省略解析後の格解析スコア */
    double omit_feature[ELLIPSIS_CASE_NUM][O_FEATURE_NUM]; /* 省略対応付け評価のための素性 */

} CF_TAG_MGR;

/*====================================================================
                               END
====================================================================*/

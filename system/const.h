/*====================================================================

			      CONSTANTS

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/

/*====================================================================
				MACRO
====================================================================*/

#define debug(s, d)    fprintf(stderr, "%s %d\n", s, d)
#define str_eq(c1, c2) ( ! strcmp(c1, c2) )
#define sizeof_char(cp) (sizeof(cp) / sizeof(char *))
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
#define	PARA_MAX	32
#define PARA_PART_MAX	32
#define WORD_LEN_MAX	128
#define SENTENCE_MAX	400
#define PRINT_WIDTH	100
#define PARENT_MAX	20
#define BROTHER_MAX	20
#define TEIDAI_TYPES	5
#define HOMO_MAX	30
#define HOMO_MRPH_MAX	10

#define BGH_CODE_SIZE	10
#define SM_CODE_SIZE	12
#define SCASE_CODE_SIZE	11

#define	HomoRule_MAX	128
#define BonusRule_MAX	16
#define KoouRule_MAX	124
#define DpndRule_MAX	124
#define DpndRule_G_MAX	16
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
#define PP_ELEMENT_MAX		5
#define SM_ELEMENT_MAX		256
#define EX_ELEMENT_MAX		256
#define MAX_MATCH_MAX 		10

#define CMM_MAX 	5				/* 最適格フレーム数 */
#define CPM_MAX 	32				/* 文内述語数 */
#define TM_MAX 		5				/* 最適依存構造数 */

#ifndef IMI_MAX
	#define IMI_MAX	129	/* defined in "juman.h" */	
#endif

#define DATA_LEN	5120
#define ALLOCATION_STEP	1024
#define DEFAULT_PARSETIMEOUT	180

#define	TBLSIZE	1024

/*====================================================================
				DEFINE
====================================================================*/
#define OPT_CASE	1
#define OPT_CASE2	6
#define OPT_DPND	2
#define OPT_BNST	3
#define OPT_AssignF	4
#define OPT_DISC	5
#define OPT_RAW		1
#define OPT_PARSED	2
#define OPT_TREE	1
#define OPT_TREEF	2
#define OPT_SEXP	3
#define OPT_TAB		4
#define OPT_NORMAL	1
#define OPT_DETAIL	2
#define OPT_DEBUG	3
#define OPT_NESM	2
#define OPT_NE		3
#define OPT_NE_SIMPLE	4
#define OPT_JUMAN	2
#define OPT_SVM		2

#define OPT_INHIBIT_CLAUSE		0x0001
#define OPT_INHIBIT_C_CLAUSE		0x0002
#define OPT_INHIBIT_OPTIONAL_CASE	0x0010
#define OPT_INHIBIT_CASE_PREDICATE	0x0100
#define OPT_INHIBIT_BARRIER		0x1000

#define	OPT_CASE_SOTO	1
#define	OPT_CASE_GAGA	2

typedef enum {VERBOSE0, VERBOSE1, VERBOSE2, 
	      VERBOSE3, VERBOSE4, VERBOSE5} VerboseType;

#define PARA_KEY_O          0
#define PARA_KEY_N          1	/* 体言の並列 */
#define PARA_KEY_P          2	/* 用言の並列 */
#define PARA_KEY_A          4	/* 体言か用言か分からない並列 */
#define PARA_KEY_I          3	/* GAPのある並列 ？？ */

#define BNST_RULE_INDP	1
#define BNST_RULE_SUFX	2
#define BNST_RULE_SKIP	3

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
#define VOICE_MORAU 	3
#define VOICE_UNKNOWN 	4

#define FRAME_ACTIVE		1
#define FRAME_PASSIVE_I		2
#define FRAME_PASSIVE_1		3
#define FRAME_PASSIVE_2		4
#define FRAME_CAUSATIVE_WO_NI	5
#define FRAME_CAUSATIVE_WO	6
#define FRAME_CAUSATIVE_NI	7

#define FRAME_POSSIBLE		8
#define FRAME_POLITE		9
#define FRAME_SPONTANE		10

#define UNASSIGNED	-1
#define NIL_ASSIGNED	-2

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
#define MAT_FLG NULL
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

/* KNP のルールファイル指定用 (.jumanrc) */
#ifndef DEF_GRAM_FILE
#define         DEF_GRAM_FILE           "文法ファイル"
#endif

#define		DEF_KNP_FILE		"KNPルールファイル"
#define		DEF_KNP_DIR		"KNPルールディレクトリ"
#define		DEF_KNP_DICT_DIR	"KNP辞書ディレクトリ"
#define		DEF_KNP_DICT_FILE	"KNP辞書ファイル"

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

/* 辞書の最大数 */
#define DICT_MAX	12

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
#define	NOUN_DB		11
#define	CODE2SM_DB	12

/*====================================================================
			     固有名詞解析
====================================================================*/

struct _pos_s {
    int Location;
    int Person;
    int Organization;
    int Artifact;
    int Others;
    char Type[9];
    int Count;
};

typedef struct {
    struct _pos_s XB;
    struct _pos_s AX;
    struct _pos_s AnoX;
    struct _pos_s XnoB;
    struct _pos_s self;
    struct _pos_s selfSM;
    struct _pos_s Case;
} NamedEntity;

/*====================================================================
			      基本データ
====================================================================*/

/* 形態素データ */
typedef struct {
    char 	Goi[WORD_LEN_MAX];	/* 原型 */
    char 	Yomi[WORD_LEN_MAX];
    char 	Goi2[WORD_LEN_MAX];
    int  	Hinshi;
    int 	Bunrui;
    int 	Katuyou_Kata;
    int  	Katuyou_Kei;
    char	Imi[IMI_MAX];
    char	type;
    FEATUREptr	f;
    char 	*SM;				/* 追加 */
    NamedEntity	NE;				/* 追加 */
    NamedEntity	eNE;				/* 追加 */
    struct _pos_s	Case[23];		/* 追加 */
} MRPH_DATA;

typedef struct cf_def *CF_ptr;
typedef struct cpm_def *CPM_ptr;
/* 文節データ */
typedef struct tnode_b *Treeptr_B;
typedef struct tnode_b {
  /* 番号 */
    int 	num;
  /* 形態素データ */
    int		mrph_num,   settou_num,  jiritu_num,  fuzoku_num;
    MRPH_DATA 	*mrph_ptr,  *settou_ptr, *jiritu_ptr, *fuzoku_ptr;
  /* 自立語データ */
    char 	Jiritu_Go[WORD_LEN_MAX];
  /* 並列構造 */
    int 	para_num;	/* 対応する並列構造データ番号 */
    char   	para_key_type;  /* 名|述|？ featureからコピー */
    char	para_top_p;	/* TRUE -> PARA */
    char	para_type;	/* 0, 1:<P>, 2:<I> */
    				/* この2つはPARAノードを導入するためのもの
				   dpnd_typeなどとは微妙に異なる */
    char	to_para_p;	/* コピー */
    int 	sp_level;	/* 並列構造に対するバリア */
  /* 意味情報 */
    char 	BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE+1];
    int		BGH_num;
    char 	SM_code[SM_ELEMENT_MAX*SM_CODE_SIZE+1];
    int         SM_num;
  /* 用言データ */
    char 	SCASE_code[SCASE_CODE_SIZE];	/* 表層格 */
    int 	voice;
    int 	cf_num;		/* その用言に対する格フレームの数 */
    CF_ptr 	cf_ptr;		/* 格フレーム管理配列(Case_frame_array)
				   でのその用言の格フレームの位置 */
    CPM_ptr     cpm_ptr;	/* 格解析の結果の保持 */
  /* 係り受け情報 (処理が確定後コピー) */
    int		dpnd_head;	/* 係り先の文節番号 */
    char 	dpnd_type;	/* 係りのタイプ : D, P, I, A */
    int		dpnd_dflt;	/* defaultの係り先文節番号 */
    char 	dpnd_int[32];	/* 文節係り先内部の位置，今は未処理 */
    char 	dpnd_ext[32];	/* 他の文節係り先可能性，今は未処理 */
  /* ETC. */
    int  	length;
    int 	space;
  /* 木構造ポインタ */
    Treeptr_B 	parent;
    Treeptr_B 	child[PARA_PART_MAX];

    FEATUREptr	f;
    DpndRule	*dpnd_rule;

    int		internal_num;
    int		internal_max;
    struct tnode_b *internal;
    struct tnode_b *pred_b_ptr;
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

typedef struct _optionalcase {
    int flag;
    int weight;
    char *type;
    char *data;
    char *candidatesdata;
} CORPUS_DATA;

/* 文中の各文節の係り先などの記録 */
typedef struct {
    int  	head[BNST_MAX];	/* 係り先 */
    char  	type[BNST_MAX];	/* 係りタイプ */
    int   	dflt[BNST_MAX];	/* 係りの距離 */
    int 	mask[BNST_MAX];	/* 非交差条件 */
    int 	pos;		/* 現在の処理位置 */
    int         flag;           /* テンポラリフラグ */
    char        *comment;       /* テンポラリ */
    struct _optionalcase op[BNST_MAX];	/* 任意格使用 */
    CHECK_DATA	check[BNST_MAX];
    FEATURE	*f[BNST_MAX];	/* feature */
} DPND;

/*====================================================================
				格解析
====================================================================*/
#define IPAL_FIELD_NUM	72
#define CASE_MAX_NUM	20

#define	USE_NONE 0
#define USE_BGH	1
#define	USE_NTT	2
#define	STOREtoCF	4
#define	USE_BGH_WITH_STORE	5
#define	USE_NTT_WITH_STORE	6

#define	CF_NORMAL	0
#define	CF_SUM		1	/* OR の格フレーム */
#define	CF_GA_SEMI_SUBJECT	2

#define	CF_UNDECIDED	0
#define	CF_CAND_DECIDED	1
#define	CF_DECIDED	2

#define MATCH_SUBJECT	-1
#define MATCH_NONE	-2

typedef struct {
    int id;				/* ID */
    int yomi;				/* 読み */
    int hyouki;				/* 表記 */
    int imi;				/* 意味 */
    int jyutugoso;			/* 述語素 */
    int kaku_keishiki[CASE_MAX_NUM];	/* 格形式 */
    int imisosei[CASE_MAX_NUM];		/* 意味素性 */
    int meishiku[CASE_MAX_NUM];		/* 名詞句 */
    int sase;				/* 態１ */
    int rare;				/* 態２ */
    int tyoku_noudou1;			/* 態３ */
    int tyoku_ukemi1;			/* 態４ */
    int tyoku_noudou2;			/* 態５ */
    int tyoku_ukemi2;			/* 態６ */
    int voice;				/* 態７ */
} IPAL_FRAME_INDEX;

typedef struct {
    int id;			/* ID */
    int yomi;			/* 読み */
    int hyouki;			/* 表記 */
    int imi;			/* 意味 */
    int jyutugoso;		/* 述語素 */
    int kaku_keishiki[CASE_MAX_NUM];	/* 格形式 */
    int imisosei[CASE_MAX_NUM];		/* 意味素性 */
    int meishiku[CASE_MAX_NUM];		/* 名詞句 */
    int sase;			/* 態１ */
    int rare;			/* 態２ */
    int tyoku_noudou1;		/* 態３ */
    int tyoku_ukemi1;		/* 態４ */
    int tyoku_noudou2;		/* 態５ */
    int tyoku_ukemi2;		/* 態６ */
    int voice;			/* 態７ */
    unsigned char *DATA;
} IPAL_FRAME;

/* 格フレーム構造体
	○ 入力文に対して作られる
	○ また，格フレーム辞書の各エントリにも作られる
		(「〜れる」などの場合は受身,尊敬などにそれぞれ一つ)
 */
typedef struct cf_def {
    int 	element_num;				/* 格要素数 */
    int 	oblig[CF_ELEMENT_MAX]; 			/* 必須格かどうか */
    int 	adjacent[CF_ELEMENT_MAX];		/* 直前格かどうか */
    int 	pp[CF_ELEMENT_MAX][PP_ELEMENT_MAX]; 	/* 格助詞 */
    char	*sm[CF_ELEMENT_MAX]; 			/* 意味マーカ */
    char	*sm_delete[CF_ELEMENT_MAX];		/* 使用禁止意味マーカ */
    int		sm_delete_size[CF_ELEMENT_MAX];
    int		sm_delete_num[CF_ELEMENT_MAX];
    char 	*ex[CF_ELEMENT_MAX];			/* 用例 (BGH) */
    char	*ex2[CF_ELEMENT_MAX];			/* 用例 (NTT) */
    char	**ex_list[CF_ELEMENT_MAX];
    int		ex_size[CF_ELEMENT_MAX];
    int		ex_num[CF_ELEMENT_MAX];
    char	*examples[CF_ELEMENT_MAX];
    char	*semantics[CF_ELEMENT_MAX];
    int 	voice;					/* ヴォイス */
    int 	ipal_address;				/* IPALのアドレス */
    int 	ipal_size;				/* IPALのサイズ */
    char 	ipal_id[128];				/* IPALのID */
    char 	*entry;					/* 用言の表記 */
    char 	imi[128];
    char	concatenated_flag;			/* 表記を前隣の文節と結合しているか */
    int		etcflag;				/* 格フレームが OR かどうか */
    int		weight[CF_ELEMENT_MAX];
    BNST_DATA	*pred_b_ptr;
} CASE_FRAME;

/* 文中の格要素と格フレームのスロットとの対応付け記録 */
typedef struct {
    int  	flag[CF_ELEMENT_MAX];
    int		score[CF_ELEMENT_MAX];
    int		pos[CF_ELEMENT_MAX];
} LIST;

/* 文と格フレームの対応付け結果の記録 */
typedef struct {
    CASE_FRAME 	*cf_ptr;			/* 格フレームへのポインタ */
    float 	score;				/* スコア */
    int		pure_score[MAX_MATCH_MAX];	/* 正規化する前のスコア */
    float	sufficiency;			/* 格フレームの埋まりぐあい */
    int 	result_num;			/* 記憶する対応関係数 */
    LIST	result_lists_p[MAX_MATCH_MAX]; 	/* スコア最大の対応関係
						   (同点の場合は複数) */
    LIST	result_lists_d[MAX_MATCH_MAX];

    struct cpm_def	*cpm;
} CF_MATCH_MGR;

/* 文と(用言に対する複数の可能な)格フレームの対応付け結果の記録 */
typedef struct cpm_def {
    CASE_FRAME 	cf;				/* 入力文の格構造 */
    BNST_DATA	*pred_b_ptr;			/* 入力文の用言文節 */
    BNST_DATA	*elem_b_ptr[CF_ELEMENT_MAX];	/* 入力文の格要素文節 */
    int 	elem_b_num[CF_ELEMENT_MAX];	/* 入力文の格要素文節(連格の係り先は-1,他は子の順番) */
    int 	score;				/* スコア最大値(=cmm[0].score) */
    int 	default_score;			/* 外の関係スコア (ルールで与えられている外の関係) */
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
    int 	score;		/* スコア */
    int 	pred_num;	/* 文中の用言数 */
    CF_PRED_MGR cpm[CPM_MAX];	/* 文中の各用言の格解析結果 */
    int		ID;		/* DPND の ID */
} TOTAL_MGR;

#define	OPTIONAL_CASE_SCORE	2

/*====================================================================
		      固有名詞解析 - 文脈処理へ
====================================================================*/

/* 保持しておくためのデータ */

typedef struct _MRPH_P {
    MRPH_DATA data;
    struct _MRPH_P *next;
} MRPH_P;

typedef struct _PreservedNamedEntity {
    MRPH_P *mrph;
    int Type;
    struct _PreservedNamedEntity *next;
} PreservedNamedEntity;

/*====================================================================
			       文脈処理
====================================================================*/

typedef struct sentence {
    int 		Sen_num;	/* 文番号 1〜 */
    int			Mrph_num;
    int			Bnst_num;
    int			New_Bnst_num;
    int			Para_M_num;	/* 並列管理マネージャ数 */
    int			Para_num;	/* 並列構造数 */
    MRPH_DATA		*mrph_data;
    BNST_DATA	 	*bnst_data;
    PARA_DATA		*para_data;
    PARA_MANAGER	*para_manager;
    CF_PRED_MGR		*cpm;
    CASE_FRAME		*cf;
    TOTAL_MGR		*Best_mgr;
    char		*KNPSID;
    char		*Comment;
} SENTENCE_DATA;

typedef struct anaphora_list {
    char	*key;
    int		count;
    struct anaphora_list *next;
} ALIST;

#define	CREL	1	/* 格関係 */
#define	EREL	2	/* 省略関係 */

typedef struct case_component {
    char	*word;
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

typedef struct ellipsis_component {
    SENTENCE_DATA	*s;
    int			bnst;
    float		score;
} ELLIPSIS_COMPONENT;

typedef struct ellipsis_cmm_list {
    CF_MATCH_MGR	cmm;
    CF_PRED_MGR		cpm;
    int			element_num;			/* 入力側 */
} ELLIPSIS_CMM;

typedef struct ellipsis_list {
    CF_PRED_MGR		*cpm;
    float		score;
    ELLIPSIS_COMPONENT cc[CASE_MAX_NUM];	/* 省略格要素のリスト */
    FEATUREptr		f;
    int			result_num;
    ELLIPSIS_CMM	ecmm[CMM_MAX];
} ELLIPSIS_MGR;

/*====================================================================
                               END
====================================================================*/

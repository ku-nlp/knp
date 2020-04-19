/*
==============================================================================
	juman.h
		1990/12/06/Thu	Yutaka MYOKI(Nagao Lab., KUEE)
		1990/01/09/Wed	Last Modified
                                          >>> 94/02 changed by T.Nakamura <<< 
==============================================================================
*/

/*
------------------------------------------------------------------------------
	inclusion of header files
------------------------------------------------------------------------------
*/

#ifdef HAVE_STDIO_H
#include	<stdio.h>
#endif

#include	<ctype.h>

#ifdef HAVE_STRING_H
#include	<string.h>
#endif

#ifdef HAVE_REGEX_H
#include        <regex.h>
#endif

#include	<stdarg.h>

#ifdef HAVE_LIMITS_H
#include	<limits.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include	<sys/types.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include	<sys/file.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif

#include	<time.h>

/* added by T.Nakamura */
#ifdef HAVE_STDLIB_H
#include        <stdlib.h>
#endif

#ifdef HAVE_FCNTL_H
#include 	<fcntl.h>
#endif

#include	"juman_pat.h"

/*
 * MS Windows の場合は SJIS入出力になるよう変更
 * Added by Taku Kudoh (taku@pine.kuee.kyoto-u.ac.jp)
 * Thu Oct 29 03:42:45 JST 1998
 */
#ifdef _WIN32
extern char *toStringEUC(char *str);
extern char *toStringSJIS(char *str);

#include        <stdarg.h>

#ifdef HAVE_WINDOWS_H
#include        <windows.h>

#if ! defined __CYGWIN__
typedef char *	caddr_t;
#endif
#endif

/* あとで 競合するらしいので undef (tricky?) */
#undef          TRUE  
#undef          FALSE
#endif

#define EOf		0x0b	/* クライアントサーバ間通信でのEOF */

#define NDBM_KEY_MAX 	256
#define NDBM_CON_MAX 	1024

/*
------------------------------------------------------------------------------
	global definition of macros
------------------------------------------------------------------------------
*/

#define 	FALSE		((int)(0))
#define 	TRUE		(!(FALSE))
#define		BOOL		int

#define		U_CHAR		unsigned char
#define		U_INT		unsigned int

/* #define		SEEK_SET	0		* use in "fseek()"         */
/* #define		SEEK_CUR	1		* use in "fseek()"         */
/* #define		SEEK_END	2		* use in "fseek()"         */

#define		EOA		((char *)(-1))	/* end of arguments         */

#ifndef 	FILENAME_MAX			/* maximum of filename      */
#define		FILENAME_MAX	1024		
#endif
#ifndef		BUFSIZE				/* (char) size of buffer    */
#define 	BUFSIZE		1025
#endif

#define 	NAME_MAX_	1024		/* maximum of various names */
#define 	MIDASI_MAX	129		/* maximum length of MIDASI */
#define 	WORD_CHAR_NUM_MAX	16	/* maximum number of chars of WORD in DIC */
#define 	YOMI_MAX	129		/* maximum length of YOMI   */
#define 	IMI_MAX		1024		/* maximum length of IMI    */
#define 	SUUSI_MIDASI_MAX	108 /* maximum length of MIDASI */
#define 	SUUSI_YOMI_MAX	108		/* maximum length of YOMI   */


#define		ENV_JUMANPATH	"JUMANPATH"

#define		GRAMMARFILE	"JUMAN.grammar"
#define 	CLASSIFY_NO	128

#define		KATUYOUFILE	"JUMAN.katuyou"
#define		TYPE_NO		128
#define		FORM_NO		128

#define		CONNECTFILE	"JUMAN.connect"
#define		KANKEIFILE	"JUMAN.kankei"

/* added by kuro */
#define		OPTIONFILE	"JUMAN.option"


#define 	S_POSTFIX	".dic"
				/* file postfix for s-expression dic-file */
#define 	I_POSTFIX	".int"
				/* file postfix for intermediate dic-file */
#define		I_FILE_ID	"this is INT-file for makehd.\n"

#define 	CONS		0
#define 	ATOM		1
#define 	NIL		((CELL *)(NULL))

#define 	COMMENTCHAR	';'
#define 	BPARENTHESIS	'('
#define 	BPARENTHESIS2	'<'
#define 	BPARENTHESIS3	'['
#define 	EPARENTHESIS	')'
#define 	EPARENTHESIS2	'>'
#define 	EPARENTHESIS3	']'
#define 	SCANATOM	"%[^(;) \n\t]"
#define 	NILSYMBOL	"NIL"

#define		CELLALLOCSTEP	1024
#define		BLOCKSIZE	16384

#define		Consp(x)	(!Null(x) && (_Tag(x) == CONS))
#define		Atomp(x)	(!Null(x) && (_Tag(x) == ATOM))
#define		_Tag(cell)	(((CELL *)(cell))->tag)
#define		_Car(cell)	(((CELL *)(cell))->value.cons.car)
#define		_Cdr(cell)	(((CELL *)(cell))->value.cons.cdr)
#define		Null(cell)	((cell) == NIL)
#define		new_cell()	(cons(NIL, NIL))
#define		Eq(x, y)	(x == y)
#define		_Atom(cell)	(((CELL *)(cell))->value.atom)


#define		EUC(c)		((c) | '\x80')
#define 	endchar(str)	(str[strlen(str)-1])

#define		M		30		/* order of B-tree         */
#define		MM		(M*2) 		/* order*2	           */

#define		PTRNIL		(-1L)		/* instead of NULL pointer */

#define		KANJI_CODE	128*128

#define 	BASIC_FORM 	"基本形"

#define		TABLEFILE	"jumandic.tab"
#define		MATRIXFILE	"jumandic.mat"
#define		DICFILE		"jumandic.dat"
#define		PATFILE		"jumandic.pat"

#define		calc_index(x)	((((int)(*(x)&0x7f))<<7)|((int)(*((x)+1))&0x7f))

enum		_ExitCode 	{ NormalExit, 
				       SystemError, OpenError, AllocateError, 
				       GramError, DicError, ConnError,  
				       ConfigError, ProgramError,
				       SyntaxError, UnknownId, OtherError };

/* added by T.Utsuro for weight of rensetu matrix */
#define         DEFAULT_C_WEIGHT  10

/* added by S.Kurohashi for mrph weight default values */
#define		MRPH_DEFAULT_WEIGHT	10

/* .jumanrc default values */

#define         CLASS_COST_DEFAULT   10            
#define         RENSETSU_WEIGHT_DEFAULT  100
#define         KEITAISO_WEIGHT_DEFAULT  1
#define         COST_WIDTH_DEFAULT      20
#define         UNDEF_WORD_DEFAULT  10000

#define         RENGO_ID		127	/* "999" yamaji */
#define         RENGO_DEFAULT_WEIGHT	0.5

/* for juman_lib.c */
#define		LENMAX			50000
#define         BUFFER_BLOCK_SIZE       1000

#define         Op_B		     	0
#define         Op_M		     	1
#define         Op_P		     	2
#define         Op_BB		     	3
#define         Op_PP		     	4
#define         Op_G		     	5
#define         Op_F		     	0
#define         Op_E		     	1
#define         Op_C		     	2
#define         Op_EE		     	3
#define         Op_E2		     	4
#define         Op_NODEBUG	     	0
#define         Op_DEBUG	     	1
#define         Op_DEBUG2	     	2

#define		MAX_PATHES		500
#define		MAX_PATHES_WK		5000

#define		MAX_LATTICES	16000	

#define		CONNECT_MATRIX_MAX	1000

#define		KUUHAKU            	0x20
#define		HANKAKU            	0x80
#define		PRIOD            	0xa1a5
#define		CHOON            	0xa1bc
#define		KIGOU            	0xa3b0
#define		SUJI           	        0xa3c0
#define		ALPH            	0xa4a0
#define		HIRAGANA                0xa5a0
#define		KATAKANA                0xa6a0
#define		GR                      0xb0a0
#define		KANJI                   0xffff

#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
#define	BYTES4CHAR	2	/* EUC-JP or SHIFT-JIS */
#else
#define	BYTES4CHAR	3	/* UTF-8 (usually) */
#endif

#define WIN_AZURE_DICFILE_DEFAULT ".\\dic\\"
#define WIN_AZURE_AUTODICFILE_DEFAULT ".\\autodic\\"
#define WIN_AZURE_WIKIPEDIADICFILE_DEFAULT ".\\wikipediadic\\"

#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp stricmp
#endif
#ifndef strncasecmp
#define strncasecmp strnicmp
#endif

#ifndef VERSION
#define VERSION "7.0"
#endif

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "juman"
#endif

#endif

/*
------------------------------------------------------------------------------
	global type definition of structures
------------------------------------------------------------------------------
*/

/* <car> 部と <cdr> 部へのポインタで表現されたセル */
typedef		struct		_BIN {
     void		*car;			/* address of <car> */
     void		*cdr;			/* address of <cdr> */
} BIN;

/* <BIN> または 文字列 を表現する完全な構造 */
typedef		struct		_CELL {
     int		tag;			/* tag of <cell> */
                                                /*   0: cons     */
                                                /*   1: atom     */
     union {
	  BIN		cons;
	  U_CHAR	*atom;
     } value;
} CELL;

/* "malloc" の回数を減少させるため，一定のメモリ領域を確保するテーブル */
typedef		struct		_CELLTABLE {
     void		*pre;
     void		*next;
     int		max;
     int		n;
     CELL		*cell;
} CELLTABLE;

/* changed by T.Nakamura and S.Kurohashi 
	構造体 MRPH がすべての情報を持ち， 
	構造体 MORPHEME はなくなった */
typedef         struct          _MRPH {
     U_CHAR             midasi[MIDASI_MAX];
     U_CHAR             midasi2[MIDASI_MAX];
     U_CHAR             yomi[YOMI_MAX];
     U_CHAR             imis[IMI_MAX];
     CELL		*imi;

     char               hinsi;
     char               bunrui;
     char               katuyou1;
     char               katuyou2;

     U_CHAR             weight;
     int                con_tbl;
     int                length;
} MRPH;

/* 形態品詞の分類・細分類 */
typedef		struct		_CLASS {
     U_CHAR	*id;
     int        cost;     /*品詞コスト by k.n*/
     int	kt;
} CLASS;

/* 活用型 */
typedef		struct		_TYPE {
     U_CHAR	*name;
} TYPE;

/* 活用形 */
typedef		struct		_FORM {
     U_CHAR	*name;
     U_CHAR	*gobi;
     U_CHAR	*gobi_yomi;	/* カ変動詞来 などの読みのため */
} FORM;

/* 辞書登録オプション */
typedef		struct		_DICOPT {
     int	toroku;
} DICOPT;

/* stat() ライブラリ関数で使用 */
typedef		struct stat	 STAT;

/* 連接表 */
typedef         struct          _RENSETU_PAIR {
     int   i_pos;
     int   j_pos;
     int   hinsi;
     int   bunrui;
     int   type;
     int   form;
     U_CHAR  *goi;
} RENSETU_PAIR;

typedef struct _process_buffer {
    int mrph_p;
    int start;
    int end;
    int score;
    int path[MAX_PATHES];     /* 前の PROCESS_BUFFER の情報 */
    int connect;	      /* FALSE なら接続禁止(連語の途中への割込禁止) */
} PROCESS_BUFFER;

typedef struct _chk_connect_wk {
  int pre_p;     /* PROCESS_BUFFER のインデックス */
  int score;     /* それまでのスコア */
} CHK_CONNECT_WK;

typedef struct _connect_cost {
    short p_no;     /* PROCESS_BUFFER のインデックス */
    short pos;
    int cost;     /* コスト */
    char opt;
} CONNECT_COST;

#define MAX_NODE_POS_NUM 10
typedef struct _char_node {
    struct _char_node *next;
    char chr[7];
    char type;
    size_t da_node_pos[MAX_NODE_POS_NUM];
    char node_type[MAX_NODE_POS_NUM];
    char deleted_bytes[MAX_NODE_POS_NUM];
    char *p_buffer[MAX_NODE_POS_NUM];
    size_t da_node_pos_num;
} CHAR_NODE;

/*
------------------------------------------------------------------------------
 additional type definition  written by K. Yanagi  >>>changed by T.Nakamura<<<
------------------------------------------------------------------------------
*/

typedef struct _DIC_FILES {
  int number;
  int now;
  FILE *dic[MAX_DIC_NUMBER];
  pat_node tree_top[MAX_DIC_NUMBER];
} DIC_FILES;

typedef struct _cost_omomi  {
    int rensetsu;
    int keitaiso;
    int cost_haba;
  } COST_OMOMI;            /*k.n*/
/*
------------------------------------------------------------------------------
	prototype definition of functions
------------------------------------------------------------------------------
*/

/* iotool.c */
FILE	*my_fopen(char *filename, char *mode);
FILE	*pathfopen(char *filename, char *mode, 
		   char *path, char *filename_path);
FILE	*my_pathfopen(char *filename, char *mode, 
		      char *path, char *filename_path);
int	my_feof(FILE *fp);
void	append_postfix(char *filename, char *affix);
void	change_postfix(char *filename, char *affix1, char *affix2);
void	getpath(char *cur_path, char *juman_path);
void	*my_alloc(int n);
void	*my_realloc(void *ptr, int n);
void	my_exit(int exit_code);
void	error(int errno, char *msg, ...);
char	lower(char c);
char	upper(char c);
int 	my_strlen(U_CHAR *s);
void	my_strcpy(U_CHAR *s1, U_CHAR *s2);
int	my_strcmp(U_CHAR *s1, U_CHAR *s2);
int 	compare_top_str(U_CHAR *s1, U_CHAR *s2);
int 	compare_top_str1(U_CHAR *s1, U_CHAR *s2);
int 	compare_top_str2(U_CHAR *s1, U_CHAR *s2);
int 	compare_end_str(U_CHAR *s1, U_CHAR *s2);

void	ls(FILE *fp, char *p, char *f);
void	print_current_time(FILE *fp);
void	print_execute_time(FILE *fp, int dt, float dp);

/* lisp.c */
int	s_feof(FILE *fp);
int	s_feof_comment(FILE *fp);
CELL	*make_cell(void);
/* CELLTABLE	*make_cell_table(CELLTABLE *pre, int size);*/
CELL	*tmp_atom(U_CHAR *atom);
CELL	*cons(void *car, void *cdr);
CELL	*car(CELL *cell);
CELL	*cdr(CELL *cell);
int	equal(void *x, void *y);
int	length(CELL *list);
int	ifnextchar(FILE *fp, int c);
int	comment(FILE *fp);
CELL	*s_read(FILE *fp);
CELL	*s_read_atom(FILE *fp);
CELL	*s_read_car(FILE *fp);
CELL	*s_read_cdr(FILE *fp);
CELL	*assoc(CELL *item, CELL *alist);
CELL	*s_print(FILE *fp, CELL *cell);
CELL	*_s_print_(FILE *fp, CELL *cell);
CELL	*_s_print_cdr(FILE *fp, CELL *cell);
void	*lisp_alloc(int n);
void 	lisp_alloc_push(void);
void 	lisp_alloc_pop(void);

/* grammar.c */
void	error_in_grammar(int n, int line_no);
void	initialize_class(void);
#define	print_class(fp) 	print_class_(fp, 0, 8, "*")
void 	print_class_(FILE *fp, int tab1, int tab2, char *flag);
void	read_class(FILE *fp);
void	grammar(FILE *fp_out);

/* katuyou.c */
static void	initialize_type_form(void);
void 	print_type_form(FILE *fp);
void	read_type_form(FILE *fp);
void	katuyou(FILE *fp);

/* connect.c */
void connect_table(FILE *fp_out);
void read_table(FILE *fp);
void check_edrtable(MRPH *mrph_p, CELL *x);
void check_table(MRPH *morpheme);
void check_table_for_rengo(MRPH *mrph_p);
int check_table_for_undef(int hinsi, int bunrui);
void connect_matrix(FILE *fp_out);
void read_matrix(FILE *fp);
int check_matrix(int postcon, int precon);
int check_matrix_left(int precon);
int check_matrix_right(int postcon);

/* zentohan.c */
unsigned char	*zentohan(unsigned char *str1);
unsigned char	*hantozen(unsigned char *str1);

/* for edr-dic */
void check_edrtable(MRPH *mrph_p, CELL *x);


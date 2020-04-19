/*
==============================================================================
	trans.c
                                 1990/11/12/Mon Yutaka MYOKI(Nagao Lab., KUEE)
                                        Last Update: 2012/9/28 Sadao Kurohashi
==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include 	"makeint.h"

#define         MRPH_BUF_MAX   2000
#define         KEITAISO_NUM_MAX  20
/*
------------------------------------------------------------------------------
	GLOBAL:
	definition of global variables
------------------------------------------------------------------------------
*/

extern char	*ProgName;
extern char	CurPath[FILENAME_MAX];
extern char	JumanPath[FILENAME_MAX];
extern CLASS	Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE	Type[TYPE_NO];
extern FORM	Form[TYPE_NO][FORM_NO];
extern int	LineNo;
extern int	LineNoForError;

/*
------------------------------------------------------------------------------
	LOCAL:
	definition of global variables
------------------------------------------------------------------------------
*/

static  MRPH            mrph_buffer[MRPH_BUF_MAX];
static  int             mrph_buffer_num;

enum	ErrorCode	{NotList, IllegalForm, ConflictGobi,
			   NoMidasi, LongMidasi, 
			   NoYomi,   LongYomi, 
			   NoKatuyou,IllegalWeight,
			   LongRengo,ShortRengo,
                           NoKatuyoukei,HankakuChr};

U_CHAR *Japanese_Hiragana[][5] = {
    {"ぁ", "ぃ", "ぅ", "ぇ", "ぉ"},        
    {"あ", "い", "う", "え", "お"},
    {"か", "き", "く", "け", "こ"},
    {"が", "ぎ", "ぐ", "げ", "ご"},
    {"さ", "し", "す", "せ", "そ"},
    {"ざ", "じ", "ず", "ぜ", "ぞ"},
    {"た", "ち", "つ", "て", "と"},
    {"だ", "ぢ", "づ", "で", "ど"},
    {"な", "に", "ぬ", "ね", "の"},
    {"は", "ひ", "ふ", "へ", "ほ"},
    {"ば", "び", "ぶ", "べ", "ぼ"},
    {"ぱ", "ぴ", "ぷ", "ぺ", "ぽ"},
    {"ま", "み", "む", "め", "も"},
    {"ら", "り", "る", "れ", "ろ"},
    {NULL}
};

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<init_mrph>:
------------------------------------------------------------------------------
*/

static void init_mrph(MRPH *mrph_p)
{
     mrph_p->hinsi    = 0;
     mrph_p->bunrui   = 0;
     mrph_p->katuyou1 = 0;
     mrph_p->katuyou2 = 0;
     mrph_p->weight   = 0;
     mrph_p->con_tbl  = 0;
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<print_mrph>:
------------------------------------------------------------------------------
*/

static void output_mrph(FILE *fp, MRPH *mrph_p)
{
    long       imiptr;

    fprintf(fp, "%s\t", mrph_p->midasi);
    numeral_encode(fp, mrph_p->hinsi);
    numeral_encode(fp, mrph_p->bunrui);
    numeral_encode(fp, mrph_p->katuyou1);
    numeral_encode(fp, mrph_p->katuyou2);
    numeral_encode(fp, mrph_p->weight);
    numeral_encode(fp, mrph_p->con_tbl);
    fprintf(fp, "%s ", mrph_p->yomi);

    if (strcmp(mrph_p->midasi, mrph_p->midasi2)) {
        fprintf(fp, "%s ", mrph_p->midasi2);
    } else {
        fprintf(fp, " ");
    }

    imi_print(fp, mrph_p->imi);
}

static void numeral_encode(FILE *fp, int num)
{
    /* -1〜数万を，+1して2バイトに
       いずれのバイトも0x20以上に */
    fputc((num+1)/(0x100-0x20)+0x20, fp);
    fputc((num+1)%(0x100-0x20)+0x20, fp);
}

static void imi_print(FILE *fp, CELL *cell)
{
    char buf[BUFSIZE];

    if (Null(cell)) {
        numeral_encode(fp, 0);
    } else {
        buf[0] = '\0';
        _imi_print(buf, cell);
        numeral_encode(fp, strlen(buf));
        fprintf(fp, "%s", buf);
    }
}

static void _imi_print(char *buf, CELL *cell)
{
    if (Null(cell))
	strcat(buf, NILSYMBOL);
    else {
	switch (_Tag(cell)) {
	case CONS:
	    strcat(buf, "(");
	    _imi_print(buf, _Car(cell));
	    _imi_print_cdr(buf, _Cdr(cell));
	    strcat(buf, ")");
	    break;
	case ATOM:
	    strcat(buf, _Atom(cell));
	    break;
	default:
	    error(OtherError, "Illegal cell(in s_print)", EOA);
	}
    }
}

static void _imi_print_cdr(char *buf, CELL *cell)
{
     if (!Null(cell)) {
	 if (Consp(cell)) {
	     strcat(buf, " ");
	     _imi_print(buf, _Car(cell));
	     _imi_print_cdr(buf, _Cdr(cell));
	 } else {
	     strcat(buf, " ");
	     _imi_print(buf, cell);
	 }
     }
}

static void print_mrph(MRPH *mrph_p) /* by yamaji */
{
    mrph_buffer[mrph_buffer_num++] = *mrph_p;
    if (mrph_buffer_num == MRPH_BUF_MAX) error_in_trans(LongRengo , NULL);
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<error_in_trans>: local error processing
------------------------------------------------------------------------------
*/
    
static void error_in_trans(int n, void *c)
{
     fprintf(stderr, "\n%s: syntax error between line %d and %d.\n",
	     ProgName, LineNoForError, LineNo);
     switch (n) {
     case NotList:
	  fprintf(stderr, "\tis not list: ");
	  s_print(stderr, (CELL *)c);
	  break;
     case IllegalForm:
	  fprintf(stderr, "\tLIST for morpheme contains illegal form: ");
	  s_print(stderr, (CELL *)c);
	  break;
     case NoMidasi:
	  fprintf(stderr, 
		  "\tLIST for morpheme don't contain the list for MIDASI.\n");
	  s_print(stderr, (CELL *)c);
	  break;
     case LongMidasi:
	  fprintf(stderr, 
		  "\tMIDASI is too long: %s\n", (U_CHAR *)c);
	  break;
     case NoYomi:
	  fprintf(stderr, 
		  "\tLIST for morpheme don't contain the list for YOMI\n");
	  s_print(stderr, (CELL *)c);
	  break;
     case LongYomi:
	  fprintf(stderr,
		  "\tYOMI is too long: %s\n", (U_CHAR *)c);
	  break;
     case NoKatuyou:
	  fprintf(stderr, 
		  "\tLIST for morpheme don't contain the list for KATUYOU.\n");
	  s_print(stderr, (CELL *)c);
	  break;
     case ConflictGobi:
	  fprintf(stderr, 
		  "\tConflicting between <midasigo> and <katuyoukata>:");
	  fprintf(stderr, "%s\n", (U_CHAR *)c);
	  break;
     case IllegalWeight:
	  fprintf(stderr, 
		  "\t0.0 <= weight <= 25.6:");
	  s_print(stderr, (CELL *)c);
	  break;
     case LongRengo:
	  fprintf(stderr,
		  "\tRENGO is too long\n");
     case ShortRengo:
	  fprintf(stderr,
		  "\tRENGO is too short\n");
	  break;
     case NoKatuyoukei:
	  fprintf(stderr, 
		  "\tLIST for RENGO don't contain the list for KATUYOUKEI.:");
	  fprintf(stderr, "%s\n", (U_CHAR *)c);
	  break;
     case HankakuChr:
	  fprintf(stderr, "\tLIST for morpheme contains HANKAKU character: ");
	  fprintf(stderr, "%s\n", (U_CHAR *)c);
	  break;
     default:
	  error(ProgramError, "error_in_trans received an unexpected code.");
	  break;
     }

     my_exit(DicError);
}

/*
------------------------------------------------------------------------------
	FUNCTION:	** not used now ** 1992/9/10
	<midasi>: sub-routine of <trans>
------------------------------------------------------------------------------
*/

static U_CHAR *midasi(CELL *x)
{
     CELL	*y;
     U_CHAR	*s;

     if (Null(y = assoc(tmp_atom((U_CHAR *)"見出し語"), x)))
	  error_in_trans(NoMidasi, x);
     if (!Atomp(car(cdr(y)))) 
	  error_in_trans(IllegalForm, y);

     s = (U_CHAR *)_Atom(car(cdr(y)));

     if (strlen(s) > MIDASI_MAX)
	  error_in_trans(LongMidasi, s);

     return s;
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<midasi_list>: sub-routine of <trans>
------------------------------------------------------------------------------
*/

static CELL *midasi_list(CELL *x)
{
     CELL	*y;
     U_CHAR	*s;

     if (Null(y = assoc(tmp_atom((U_CHAR *)"見出し語"), x)))
	  error_in_trans(NoMidasi, x);

     return cdr(y);
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<yomi>: sub-routine of <trans>
------------------------------------------------------------------------------
*/

static U_CHAR *yomi(CELL *x)
{
     CELL	*y;
     U_CHAR	*s;

     if (Null(y = assoc(tmp_atom((U_CHAR *)"読み"), x)))
	  error_in_trans(NoYomi, x);
     if (!Atomp(car(cdr(y))))
	  error_in_trans(IllegalForm, y);

     s = (U_CHAR *)_Atom(car(cdr(y)));

     if (strlen(s) > YOMI_MAX)
	  error_in_trans(LongYomi, s);

     return s;
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<katuyou1>: sun-routine of <trans>
------------------------------------------------------------------------------
*/

static int katuyou1(CELL *x)
{
    CELL	*y;
    int	i;

    if (Null(y = assoc(tmp_atom((U_CHAR *)"活用型"), x)))
      error_in_trans(NoKatuyou, x);
    if (!Atomp(car(cdr(y))))
      error_in_trans(IllegalForm, y);
    
    return get_type_id(_Atom(car(cdr(y))));
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<katuyou2>: sun-routine of <trans>
------------------------------------------------------------------------------
*/

static int katuyou2(CELL *x , int type)
{
    CELL	*y;
    int	i;

    if (Null(y = assoc(tmp_atom((U_CHAR *)"活用形"), x)))
      error_in_trans(NoKatuyou, x);
    if (!Atomp(car(cdr(y))))
      error_in_trans(IllegalForm, y);
    
    return get_form_id(_Atom(car(cdr(y))) , type);
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<imi>: sub-routine of <trans>
------------------------------------------------------------------------------
*/

static CELL *imi(CELL *x)
{
     CELL	*y;

     y = assoc(tmp_atom((U_CHAR *)"意味情報"), x);
     return car(cdr(y));
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<trim_yomi_gobi> <trim_midasi_gobi>: sub-routine of <trans>
------------------------------------------------------------------------------
*/

static void trim_yomi_gobi(MRPH *mrph_p)
{
    U_CHAR	*str = (U_CHAR *)"基本形";
    int	i;

    for (i = 1; strcmp(Form[mrph_p->katuyou1][i].name, str); i++);

    mrph_p->yomi[strlen(mrph_p->yomi) - 
                 strlen(Form[mrph_p->katuyou1][i].gobi_yomi)]='\0';
}

static void trim_midasi_gobi(MRPH *mrph_p)
{
    U_CHAR	*str = (U_CHAR *)"基本形";
    int	i;
    
    for (i = 1; strcmp(Form[mrph_p->katuyou1][i].name, str); i++);
    
    if (compare_end_str(mrph_p->midasi, Form[mrph_p->katuyou1][i].gobi))
        mrph_p->midasi[strlen(mrph_p->midasi) - 
                       strlen(Form[mrph_p->katuyou1][i].gobi)]='\0';
    else
        error_in_trans(ConflictGobi, mrph_p->midasi);
}

static void change_kana(U_CHAR *kana, char vowel)
{
    /* ご e -> げ */
    int i, j;

    for (i = 0; Japanese_Hiragana[i][0]; i++) {
        for (j = 0; j < 5; j++) {
            if (!strcmp(kana, Japanese_Hiragana[i][j])) {
                switch(vowel) {
                case 'a': strcpy(kana, Japanese_Hiragana[i][0]); break;
                case 'i': strcpy(kana, Japanese_Hiragana[i][1]); break;
                case 'u': strcpy(kana, Japanese_Hiragana[i][2]); break;
                case 'e': strcpy(kana, Japanese_Hiragana[i][3]); break;
                case 'o': strcpy(kana, Japanese_Hiragana[i][4]); break;
                }
                return;
            }
        }
    }
}

static void change_gobi(U_CHAR *midasi, int katuyou1, int katuyou2)
{
    U_CHAR *str = (U_CHAR *)"基本形";
    int i;

    for (i = 1; strcmp(Form[katuyou1][i].name, str); i++);
    midasi[strlen(midasi) - strlen(Form[katuyou1][i].gobi)] = '\0';

    if (Form[katuyou1][katuyou2].gobi[0] == '-') {
        /* すご -eえ -> すげえ */
        change_kana(midasi + strlen(midasi) - 3, Form[katuyou1][katuyou2].gobi[1]);
        strcat(midasi, Form[katuyou1][katuyou2].gobi + 2);
    } else {
        strcat(midasi, Form[katuyou1][katuyou2].gobi);
    }
}

static void change_gobi_yomi(U_CHAR *yomi, int katuyou1, int katuyou2)
{
    U_CHAR *str = (U_CHAR *)"基本形";
    int i;

    for (i = 1; strcmp(Form[katuyou1][i].name, str); i++);
    yomi[strlen(yomi) - strlen(Form[katuyou1][i].gobi_yomi)] = '\0';

    if (Form[katuyou1][katuyou2].gobi_yomi[0] == '-') {
        /* すご -eえ -> すげえ */
        change_kana(yomi + strlen(yomi) - 3, Form[katuyou1][katuyou2].gobi_yomi[1]);
        strcat(yomi, Form[katuyou1][katuyou2].gobi_yomi + 2);
    } else {
        strcat(yomi, Form[katuyou1][katuyou2].gobi_yomi);
    }
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<hankaku_check>: sub-routine of <trans>
------------------------------------------------------------------------------
*/

int hankaku_check(U_CHAR *s)
{
#ifdef IO_ENCODING_SJIS
    return(0);
#else
    while (*s) {
	if (*s < 0x80) return(1);
	s++;
    }
    return(0);
#endif
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<trans>: translate from <fp_in> to <fp_out>
------------------------------------------------------------------------------
*/

void trans(FILE *fp_in, FILE *fp_out)
{
    CELL	*cell,*cell1;
    int         keitaiso_num;
    int         keitaiso_p[KEITAISO_NUM_MAX],keitaiso_c[KEITAISO_NUM_MAX];
    		/* keitaiso_p[], keitaiso_c[]は各連語の構成要素がbufferの何番目から
                   入っているか．keitaiso_p[]は固定，keitaiso_c[]は組合せ時に利用 */
    int         i,f;
    float	float_weight;
    int 	int_weight;
    MRPH        *mrph_p;
    MRPH        mrph_t;
    U_CHAR      str_midasi[MIDASI_MAX*10];
    U_CHAR      str_midasi2[MIDASI_MAX*10];
    U_CHAR      str_yomi[YOMI_MAX*10];
    int         rengo_con_tbl;

    /* fprintf(fp_out, I_FILE_ID); */
    
    LineNo = 1;
    while (! s_feof(fp_in)) {
	LineNoForError = LineNo;

	lisp_alloc_push();
	cell = s_read(fp_in);
	  
	if (Atomp(cell)) error_in_trans(NotList, cell);
	if (!Atomp(car(cell))) error_in_trans(IllegalForm, car(cell));
	
        /* 連語の場合 */
	if (get_hinsi_id(_Atom(car(cell))) == RENGO_ID) {
	    keitaiso_num = mrph_buffer_num = 0;
	    cell1 = car(cdr(cell));
            /* 要素（形態素）情報を全て読み込む
               ※ 活用語：末尾以外は活用固定で一つ
                          末尾は固定の場合は一つ，自由の場合は展開
            */
	    while (!Null(car(cell1))) {  
		keitaiso_p[keitaiso_num] = mrph_buffer_num;
		_trans(car(cell1) , TRUE);
		if (++keitaiso_num >= KEITAISO_NUM_MAX)
		    error_in_trans(LongRengo , NULL);
		cell1 = cdr(cell1);
	    }
	    keitaiso_p[keitaiso_num] = mrph_buffer_num;
	    if (keitaiso_num <= 1) error_in_trans(ShortRengo , NULL);

	    if (Null(cdr(cdr(cell)))) {
		int_weight = (int)(RENGO_DEFAULT_WEIGHT * 10 + 0.1);
	    } else {
		if (sscanf((char *)_Atom(car(cdr(cdr(cell)))),"%f",
			   &float_weight) == 0)
		    error_in_trans(IllegalForm, cell);
		int_weight = (int)(float_weight * 10 + 0.1);
		if (int_weight < 0 || int_weight > 256)
		    error_in_trans(IllegalWeight, cell);
	    }

	    /* 連語情報を出力する */
	    for (i = 0; i < keitaiso_num; i++) keitaiso_c[i] = keitaiso_p[i];
	    f = 1;
	    while (f) {
		str_midasi[0] = str_midasi2[0] = str_yomi[0] = '\0';
		for (i = 0 ; i < keitaiso_num ; i++) {
		    mrph_p = &mrph_buffer[keitaiso_c[i]];
                    strcat(str_midasi, mrph_p->midasi);
                    if (i == (keitaiso_num - 1)) {
                        strcat(str_midasi2, mrph_p->midasi2);
                    } else {
                        strcat(str_midasi2, mrph_p->midasi);
                    }
                    strcat(str_yomi, mrph_p->yomi);
		    if (strlen(str_midasi) > MIDASI_MAX)
			error_in_trans(LongMidasi, str_midasi);
		    if (strlen(str_midasi2) > MIDASI_MAX)
			error_in_trans(LongMidasi, str_midasi2);
		    if (strlen(str_yomi) > YOMI_MAX)
			error_in_trans(LongYomi, str_yomi);
		}

		mrph_t.hinsi = RENGO_ID;
		mrph_t.bunrui = keitaiso_num;		/* bunruiの場所に要素数 */
		mrph_t.katuyou1 = mrph_buffer[keitaiso_c[keitaiso_num-1]].katuyou1;
		mrph_t.katuyou2 = mrph_buffer[keitaiso_c[keitaiso_num-1]].katuyou2;
		mrph_t.weight = int_weight;
		strcpy(mrph_t.midasi, str_midasi);
		strcpy(mrph_t.midasi2, str_midasi2);
		strcpy(mrph_t.yomi, str_yomi);
		/* 連語として連接規則があるかどうか調べる
                   使うのは katuyou1 と midasi2
                   連接規則がなければ -1 */
		check_table_for_rengo(&mrph_t);
                mrph_t.imi = NIL;	/* 連語は意味情報なし */

                /* 出力：連語全体 */
                output_mrph(fp_out, &mrph_t);

                /* 出力：各形態素 */
		for (i = 0 ; i < keitaiso_num ; i++)
		    output_mrph(fp_out, &mrph_buffer[keitaiso_c[i]]);
		fprintf(fp_out, "\n");
		
                /* 構成要素の組合せを変更（すべての組合せを作り出す） */
		i = keitaiso_num - 1;
		while (1) {
		    if (++keitaiso_c[i] == keitaiso_p[i+1]) {
			keitaiso_c[i] = keitaiso_p[i];
			if (--i == -1) {f = 0; break;}
		    } else break;
		}
	    }
	} 

        /* 形態素の場合 */
        else {
	    mrph_buffer_num = 0;
	    _trans(cell , FALSE);
	    for (i = 0 ; i < mrph_buffer_num ; i++) {
		output_mrph(fp_out, &mrph_buffer[i]);
		fprintf(fp_out, "\n");
	    }
	}

	lisp_alloc_pop();
    }
}
	  
static void _trans(CELL *cell , int rengo_p)
{
    CELL   *main_loop, *main_block, *sub_loop, *sub_block;
    MRPH   *mrph_p;
    MRPH   mrph_t;

    mrph_p = &mrph_t;
    init_mrph(mrph_p);

    mrph_p->hinsi = get_hinsi_id(_Atom(car(cell)));	/* 形態品詞 */
	  
    main_loop = cdr(cell);
    while (!Null(main_block = car(main_loop))) {

	/* 細分類がある場合 */
	if (Atomp(car(main_block))) {
	    mrph_p->bunrui =
		get_bunrui_id(_Atom(car(main_block)), mrph_p->hinsi);
	    sub_loop = cdr(main_block);
	    while (!Null(sub_block = car(sub_loop))) {
		__trans(sub_block, mrph_p, rengo_p);
		sub_loop = cdr(sub_loop);
	    }
	} 

	/* 細分類がない場合 */
	else {
	    mrph_p->bunrui = 0;
	    __trans(main_block, mrph_p, rengo_p);
	}

	main_loop = cdr(main_loop);
    }
}

static void __trans(CELL *block, MRPH *mrph_p, int rengo_p)
{
    CELL 	*loop, *midasi_cell;
    MRPH 	mrph_t;
    U_CHAR 	*midasi_cp = NULL;
    float	float_weight;
    int 	int_weight, i;

    strcpy(mrph_p->yomi, yomi(block))  ;		/* 読み     */
    mrph_p->imi = imi(block);				/* 意味情報 */
    if (Class[mrph_p->hinsi][mrph_p->bunrui].kt) {
	mrph_p->katuyou1 = katuyou1(block);		/* 活用型   */
	mrph_p->katuyou2 =              		/* 活用 */
            (rengo_p) ? katuyou2(block , mrph_p->katuyou1) : 0;
    } else {
	mrph_p->katuyou1 = 0;
        mrph_p->katuyou2 = 0;
    }

    loop = midasi_list(block);				/* 見出し語 */
    while (!Null(midasi_cell = car(loop))) {

	/* (見出し語 ×××) の場合 */
	if (Atomp(midasi_cell)) {
	    midasi_cp = _Atom(midasi_cell);
	    mrph_p->weight = MRPH_DEFAULT_WEIGHT;
	} 
	
	/* (見出し語 (××× weight)) の場合 */
	else if (Atomp(car(midasi_cell))) {
	    midasi_cp = _Atom(car(midasi_cell));

	    if (Null(cdr(midasi_cell))) {
		mrph_p->weight = MRPH_DEFAULT_WEIGHT;
            } else if (Atomp(car(cdr(midasi_cell)))) {
		if (sscanf((char *)_Atom(car(cdr(midasi_cell))),"%f",
			   &float_weight) == 0) {
		    error_in_trans(IllegalForm, midasi_cell);
                }
		int_weight = (int)(float_weight * MRPH_DEFAULT_WEIGHT + 0.1);
		if (int_weight < 0 || int_weight > 256) {
		    error_in_trans(IllegalWeight, midasi_cell);
                }
		mrph_p->weight = (U_CHAR)int_weight;
	    } else {
		error_in_trans(IllegalForm, midasi_cell);
            }
	} else {
	    error_in_trans(IllegalForm, midasi_cell);	      
	}  

	if (strlen(midasi_cp) > MIDASI_MAX)
	    error_in_trans(LongMidasi, midasi_cp);
	strcpy(mrph_p->midasi, midasi_cp);
	strcpy(mrph_p->midasi2, midasi_cp);

        /* 連接情報(活用する場合，ここは基本形，後で補正) */
        check_table(mrph_p);

        /* 活用語 */
	if (Class[mrph_p->hinsi][mrph_p->bunrui].kt) {
            /* 活用固定の場合（連語の中，連語の最後の一部） */
            if (mrph_p->katuyou2 != 0)  {
                change_gobi(mrph_p->midasi, mrph_p->katuyou1, mrph_p->katuyou2);
                change_gobi_yomi(mrph_p->yomi, mrph_p->katuyou1, mrph_p->katuyou2);
                mrph_p->con_tbl += (mrph_p->katuyou2 - 1);
                print_mrph(mrph_p);
            }
            /* 普通は活用を展開 */
            else {
                for (i = 1; Form[mrph_p->katuyou1][i].name; i++) {
                    mrph_t = *mrph_p;
                    mrph_t.katuyou2 = i;
                    change_gobi(mrph_t.midasi, mrph_p->katuyou1, i);
                    change_gobi_yomi(mrph_t.yomi, mrph_p->katuyou1, i);
                    mrph_t.con_tbl += (i - 1);
                    /* 「する」の語幹はNULLなので登録しない，「静かだ」の語幹は登録する */
                    if (strlen(mrph_t.midasi)) print_mrph(&mrph_t);
                }
	    }
	}
        /* 非活用語 */
        else {
            print_mrph(mrph_p);
	}

	loop = cdr(loop);
    }
}


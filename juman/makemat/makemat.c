/*
==============================================================================
	makemat.c
==============================================================================
*/

/*
  1. rensetu_tblを作る．
     - まずJUMAN.connect.cで語彙指定のものを取り出し，活用語であればすべての活用を展開
     - 次に，品詞（細分類）レベルで，活用語であればすべての活用を展開
      -> jumandic.tabの各行（3カラム以降の情報）

  2. rensetu_tbl x rensetu_tbl のmtrixについて，JUMAN.connect.cの情報をうめる

  3. 上記matrixで同じ行，列を圧縮
      -> jumandic.mat
     rensetu_tblの各位置が，圧縮されたmatricの行，列のどの位置かを記録
      -> jumandic.tabの各行の1つめ，2つめの数字
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include	"makemat.h"

#define         KANKEI_MAX      200   /* 100 */
#define         REN_TBL_MAX     10000 /* 6000 */
#define         REN_MTR_MAX     10000 /* 6000 */

/*
------------------------------------------------------------------------------
	definition of global variables
------------------------------------------------------------------------------
*/

extern char	*ProgName;
extern FILE	*Jumanrc_Fileptr;
extern FILE	*Cha_stderr;

/*
FILE            *FpLogf;
FILE            *FpMain;
FILE            *FpMida;
FILE            *FpYomi;
FILE            *FpImis;
*/
/* ROOT            Root; */
char		CurPath[FILENAME_MAX];
char		JumanPath[FILENAME_MAX];

/*
------------------------------------------------------------------------------
	GLOBAL:
	definition of global variables
------------------------------------------------------------------------------
*/

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

static H_B_T_KANKEI    kankei_tbl[KANKEI_MAX];
static int             TBL_NUM;
static RENSETU_PAIR    rensetu_tbl[REN_TBL_MAX];
static int             I_NUM;
static int             J_NUM;
static U_CHAR          rensetu_mtr1[REN_MTR_MAX][REN_MTR_MAX];
static U_CHAR          rensetu_mtr2[REN_MTR_MAX][REN_MTR_MAX];

/*
------------------------------------------------------------------------------
	FUNCTION:
	<read_hinshi_katuyou>:
------------------------------------------------------------------------------
*/	       

void read_h_b_t_kankei(FILE *fp)
{
     CELL       *cell1, *cell2;
     int        j = 0;
     int        hinsi_id, bunrui_id, type_id;

     LineNo = 1;
     while (! s_feof(fp)) {
          LineNoForError = LineNo;

          cell1 = s_read(fp);

	  hinsi_id = get_hinsi_id(_Atom(car(car(cell1))));

	  if ( Null(cell2 = car(cdr(car(cell1)))) ||
	       !strcmp(_Atom(cell2), "*") )
	    bunrui_id = 0;
	  else
	    bunrui_id = get_bunrui_id(_Atom(cell2), hinsi_id);

          cell1 = car(cdr(cell1));

          while (!Null(cell2 = car(cell1))) {
               type_id = get_type_id(_Atom(cell2));

	       kankei_tbl[j].hinsi  = hinsi_id;
	       kankei_tbl[j].bunrui = bunrui_id;
	       kankei_tbl[j].type   = type_id;
	       j++;
	       /* 
		 bug fixed by kurohashi on 92/03/16
	       */
	       if ( j >= KANKEI_MAX ) 
		 error(OtherError, "Not enough size for KANKEI_MAX", EOA);
	       kankei_tbl[j].hinsi  = -1;

               cell1 = cdr(cell1);
          }
     }
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<make_rensetu_tbl>:
------------------------------------------------------------------------------
*/	       

void make_rensetu_tbl(FILE *fp)
{
     CELL       *cell;
     int         i, j;
     int         tbl_count = 1;     /* tbl_count = 0 は、文頭・文末 用 */


     /* 語彙を指定しているものをテーブルに登録 */

     LineNo = 1;
     while (! s_feof(fp)) {
          LineNoForError = LineNo;

          cell = s_read(fp);
	  _make_rensetu_tbl1(car(cell), &tbl_count);
	  _make_rensetu_tbl1(car(cdr(cell)), &tbl_count);      
     }


     /* 活用を展開してテーブルに登録 */

     for ( i=1; Class[i][0].id; i++ ) {
	  for ( j=0; Class[i][j].id; j++ )
	    _make_rensetu_tbl2(i, j, &tbl_count);
     }


     TBL_NUM = tbl_count;
     for ( i=0; i<TBL_NUM; i++ ) {
	  rensetu_tbl[i].i_pos = i;
	  rensetu_tbl[i].j_pos = i;
     }


     /* print for check */

     fprintf(stderr, "(table size %d) ", TBL_NUM);
}

void _make_rensetu_tbl1(CELL *cell1, int *cnt)
{
     int                i, flag;
     RENSETU_PAIR       r_pair;
     CELL               *cell11;

     while (!Null(cell11 = car(cell1))) {
	  if ( !Null(car(cdr(cdr(cdr(cdr(cell11)))))) ) {
	       get_pair_id1(cell11, &r_pair);
	       
	       flag = 1;
	       for ( i=1; i<*cnt; i++ ) 
		   if ( pair_match1(&rensetu_tbl[i], &r_pair) ) {
		       flag = 0;
		       break;
		   }
	       
	       if ( flag && r_pair.type )
		 for ( i=1; Form[r_pair.type][i].name != NULL; i++ ) {
		      rensetu_tbl[*cnt].hinsi  = r_pair.hinsi;
		      rensetu_tbl[*cnt].bunrui = r_pair.bunrui;
		      rensetu_tbl[*cnt].type   = r_pair.type;
		      rensetu_tbl[*cnt].form   = i;
		      rensetu_tbl[*cnt].goi    = r_pair.goi;
		      (*cnt)++;
		      if ( *cnt >= REN_TBL_MAX ) 
			error(OtherError, "Not enough size for table", EOA);
		 }
	       else if ( flag ) {
		    rensetu_tbl[*cnt].hinsi  = r_pair.hinsi;
		    rensetu_tbl[*cnt].bunrui = r_pair.bunrui;
		    rensetu_tbl[*cnt].type   = r_pair.type;
		    rensetu_tbl[*cnt].form   = r_pair.form;
		    rensetu_tbl[*cnt].goi    = r_pair.goi;
		    (*cnt)++;
		    if ( *cnt >= REN_TBL_MAX ) 
		      error(OtherError, "Not enough size for table", EOA);
	       }
	  }
	  cell1 = cdr(cell1);	       
     }
}

void _make_rensetu_tbl2(int hinsi, int bunrui, int *cnt)
{
     int i, j;

     if ( Class[hinsi][bunrui].kt ) {                  /* 活用あり */
	  i = 0;
	  while ( kankei_tbl[i].hinsi != -1 ) {
	       if ( kankei_tbl[i].hinsi == hinsi &&
		    kankei_tbl[i].bunrui == bunrui ) {
		    for ( j=1; Form[kankei_tbl[i].type][j].name != NULL;
			 j++ ) {
			 rensetu_tbl[*cnt].hinsi  = hinsi;
			 rensetu_tbl[*cnt].bunrui = bunrui;
			 rensetu_tbl[*cnt].type   = kankei_tbl[i].type;
			 rensetu_tbl[*cnt].form   = j;
			 rensetu_tbl[*cnt].goi    = NULL;
			 (*cnt)++;
			 if ( *cnt >= REN_TBL_MAX ) 
			   error(OtherError, "Not enough size for table", EOA);
		    }
	       }
	       i++;
	  }
     } else {                                         /* 活用なし */
	  rensetu_tbl[*cnt].hinsi  = hinsi;
	  rensetu_tbl[*cnt].bunrui = bunrui;
	  rensetu_tbl[*cnt].type   = 0;
	  rensetu_tbl[*cnt].form   = 0;
	  rensetu_tbl[*cnt].goi    = NULL;
	  (*cnt)++;
	  /* 
	    bug fixed by kurohashi on 92/03/16
	  */
	  if ( *cnt >= REN_TBL_MAX ) 
	    error(OtherError, "Not enough size for table", EOA);
     }
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<read_rensetu>:
------------------------------------------------------------------------------
*/	       

void read_rensetu(FILE *fp)
{
     int            i, j;
     CELL           *cell, *cell1, *cell11, *cell2, *cell22, *cell2_stock;
     RENSETU_PAIR2  pair1, pair2;
     U_CHAR         c_weight;

     for ( i=0; i<REN_MTR_MAX; i++ )
       for ( j=0; j<REN_MTR_MAX; j++ )
	 rensetu_mtr1[i][j] = '\0';

     LineNo = 1;
     while (! s_feof(fp)) {
          LineNoForError = LineNo;
	  
          cell = s_read(fp);
	  cell1 = car(cell);
	  cell2 = car(cdr(cell));

	  /* added by T.Utsuro for weight of rensetu matrix */
	  if( Null(cdr(cdr(cell))) )
	    c_weight = DEFAULT_C_WEIGHT;
	  else 
	    c_weight = (U_CHAR) atoi(_Atom(car(cdr(cdr(cell)))));

	  cell2_stock = cell2;
          while (!Null(cell11 = car(cell1))) {
	       get_pair_id2(cell11, &pair1);
	       cell2 = cell2_stock;

	       while (!Null(cell22 = car(cell2))) {
		    get_pair_id2(cell22, &pair2);
		    fill_matrix(&pair1, &pair2, c_weight);
		    cell2 = cdr(cell2);
	       }
               cell1 = cdr(cell1);
          }
     }
}

/* modified by T.Utsuro for weight of rensetu matrix */
void fill_matrix(RENSETU_PAIR2 *pair_p1, RENSETU_PAIR2 *pair_p2, U_CHAR c_weight)
{
    int         i, j;
    int         *tmp_tbl;
    int 	 flag = 0;
    
    tmp_tbl = (int *)my_alloc(sizeof(int) * TBL_NUM);
    
    if ( pair_p2->hinsi == -1 ) {        /* hinsi = -1 : 文末 */
	tmp_tbl[0] = 1;
	for ( j=1; j<TBL_NUM; j++ )
	  tmp_tbl[j] = 0;
    } else {
	for ( j=0; j<TBL_NUM; j++ ) {
	    if ( pair_match2(pair_p2, &rensetu_tbl[j]) )
	      tmp_tbl[j] = 1;
	    else
	      tmp_tbl[j] = 0;
	}
    }
    
    if ( pair_p1->hinsi == -1 ) {       /* hinsi = -1 : 文頭 */
	for ( j=0; j<TBL_NUM; j++ )
	  if ( tmp_tbl[j] ) {    	/* 後にあるルールの重みを優先 */
	      rensetu_mtr1[0][j] = c_weight;
	      flag ++;
	  }
    } else {
	for ( i=0; i<TBL_NUM; i++ )
	  if ( pair_match2(pair_p1, &rensetu_tbl[i]) )
	    for ( j=0; j<TBL_NUM; j++ )
	      if ( tmp_tbl[j] ) {	/* 後にあるルールの重みを優先 */
		  rensetu_mtr1[i][j] = c_weight;
		  flag ++;
	      }
    }
    
    free(tmp_tbl);

    if ( ! flag ) {
	warning(0, "Invalid katuyou_kei", pair_p1->form, "or", pair_p2->form, EOA);
	fprintf(stderr,"     between line %d and %d.\n \n "
		,LineNoForError,LineNo);
    }
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	get id and match id
------------------------------------------------------------------------------
*/	       

void get_pair_id1(CELL *cell, RENSETU_PAIR *pair)
{
     int i, j;
     CELL *cell_p;

     pair->hinsi  = 0;
     pair->bunrui = 0;
     pair->type   = 0;
     pair->form   = 0;
     pair->goi    = NULL;

     if ( !Null(cell_p = car(cell) ) )
       pair->hinsi = get_hinsi_id(_Atom(cell_p));
     else return;
	 
     if ( !Null(cell_p = car(cdr(cell)) ) )
       pair->bunrui = get_bunrui_id(_Atom(cell_p), pair->hinsi);
     else return;
     
     if ( !Null(cell_p = car(cdr(cdr(cell))) ) )
       pair->type = get_type_id(_Atom(cell_p));
     else return;

     if ( !Null(cell_p = car(cdr(cdr(cdr(cell)))) ) )
       pair->form = get_form_id(_Atom(cell_p), pair->type);
     else return;

     if ( !Null(cell_p = car(cdr(cdr(cdr(cdr(cell))))) ) ) {
	 pair->goi = (U_CHAR *)my_alloc(sizeof(U_CHAR) * MIDASI_MAX );
	 strcpy(pair->goi, _Atom(cell_p));
     }
}

void get_pair_id2(CELL *cell, RENSETU_PAIR2 *pair)
{
     int i, j;
     CELL *cell_p;

     pair->hinsi  = 0;
     pair->bunrui = 0;
     pair->type   = 0;
     pair->form   = NULL;
     pair->goi    = NULL;

     if ( !Null(cell_p = car(cell) ) ) {
	  if ( strcmp("文頭", _Atom(cell_p)) == 0 ||
	       strcmp("文末", _Atom(cell_p)) == 0 ) {
	       pair->hinsi = -1;
	       return;
	  } else 
	    pair->hinsi = get_hinsi_id(_Atom(cell_p));
     } else return;
	 
     if ( !Null(cell_p = car(cdr(cell)) ) )
       pair->bunrui = get_bunrui_id(_Atom(cell_p), pair->hinsi);
     else return;
     
     if ( !Null(cell_p = car(cdr(cdr(cell))) ) )
	  pair->type = get_type_id(_Atom(cell_p));
     else return;

     if ( !Null(cell_p = car(cdr(cdr(cdr(cell)))) ) ) {
	  if ( strcmp(_Atom(cell_p), "*") ) {
	       pair->form = (U_CHAR *)my_alloc(sizeof(U_CHAR) * MIDASI_MAX );
	       strcpy(pair->form, _Atom(cell_p));
	  }
     } else return;

     if ( !Null(cell_p = car(cdr(cdr(cdr(cdr(cell))))) ) ) {
	  pair->goi = (U_CHAR *)my_alloc(sizeof(U_CHAR) * MIDASI_MAX );
	  strcpy(pair->goi, _Atom(cell_p));
     }
}

int pair_match1(RENSETU_PAIR *pair1, RENSETU_PAIR *pair2)
{
     if ( pair1->hinsi != pair2->hinsi ||
	  pair1->bunrui != pair2->bunrui ||
	  pair1->type != pair2->type ||
	  pair1->form != pair2->form ||
	  strcmp(pair1->goi, pair2->goi) )
       return 0;
     else
       return 1;
}

int pair_match2(RENSETU_PAIR2 *pair, RENSETU_PAIR *tbl)
{
     if ( pair->hinsi && (pair->hinsi != tbl->hinsi) )
       return 0;
     if ( pair->bunrui && (pair->bunrui != tbl->bunrui) )
       return 0;
     if ( pair->type && (pair->type != tbl->type) )
       return 0;
     if ( pair->form && ( tbl->form == 0 || 
			  strcmp(pair->form, Form[tbl->type][tbl->form].name)))
       return 0;
     if ( pair->goi && ( ! tbl->goi || strcmp(pair->goi, tbl->goi) ) )
       return 0;

     if ( pair->hinsi == 0 && tbl->hinsi == RENGO_ID)
       return 0; /* 正規表現は連語には通用しない */

     return 1;
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<condense_matrix>:
------------------------------------------------------------------------------
*/	       

void condense_matrix()
{
     int        i, j, k;
     int        i_num = 0;
     int        j_num = 0;

     for ( j=0; j<TBL_NUM; j++ )  {
	  for ( k=0; k<j_num; k++ ) {
	       if ( compare_vector1(k, j, TBL_NUM) ) {
		    rensetu_tbl[j].j_pos = k;
		    break;
	       }
	  }
	  if ( rensetu_tbl[j].j_pos == j ) {
	       copy_vector1(j, j_num, TBL_NUM);
	       rensetu_tbl[j].j_pos = j_num;
	       j_num++;
	  }
     }

     for ( i=0; i<TBL_NUM; i++ )  {
          for ( k=0; k<i_num; k++ ) {
               if ( compare_vector2(k, i, j_num) ) {
                    rensetu_tbl[i].i_pos = k;
                    break;
               }
          }
          if ( rensetu_tbl[i].i_pos == i ) {
               copy_vector2(i, i_num, j_num);
	       rensetu_tbl[i].i_pos = i_num;
               i_num++;
          }
     }

     I_NUM = i_num;
     J_NUM = j_num;


     /* print for check */

     fprintf(stderr, "matrix size %d, %d\n", I_NUM, J_NUM);
}

int compare_vector1(int k, int j, int num)
{
     int i;
     
     for ( i=0; i<num; i++ )
       if ( rensetu_mtr2[i][k] != rensetu_mtr1[i][j] )
	 return 0;

     return 1;
}

void copy_vector1(int j, int j_num, int num)
{
     int i;

     for ( i=0; i<num; i++ )
       rensetu_mtr2[i][j_num] = rensetu_mtr1[i][j];
}

int compare_vector2(int k, int i, int num)
{
     int j;

     for ( j=0; j<num; j++ )
       if ( rensetu_mtr2[i][j] != rensetu_mtr1[k][j] )
	 return 0;
     
     return 1;
}

void copy_vector2(int i, int i_num, int num)
{
     int j;

     for ( j=0; j<num; j++ )
       rensetu_mtr1[i_num][j] = rensetu_mtr2[i][j];
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<write_table>, <write_matrix>:
------------------------------------------------------------------------------
*/	       
void write_table(FILE *fp)
{
     int i;

     fprintf(fp, "%d\n", TBL_NUM);
     for ( i=0; i<TBL_NUM; i++ ) {

	  /* comment */

/*	  fprintf(fp, "%s ", Class[rensetu_tbl[i].hinsi]
		                  [rensetu_tbl[i].bunrui].id);
	  if ( rensetu_tbl[i].type )
	    fprintf(fp, "%s %s ", Type[rensetu_tbl[i].type], 
		   Form[rensetu_tbl[i].type][rensetu_tbl[i].form]);
	  if ( rensetu_tbl[i].goi )
	    fprintf(fp, "%s", rensetu_tbl[i].goi);
	  fprintf(fp, "\n");*/

	  /* data */

	  fprintf(fp, "%d ", rensetu_tbl[i].i_pos);
	  fprintf(fp, "%d ", rensetu_tbl[i].j_pos);
	  fprintf(fp, "%d ", rensetu_tbl[i].hinsi);
	  fprintf(fp, "%d ", rensetu_tbl[i].bunrui);
	  fprintf(fp, "%d ", rensetu_tbl[i].type);
	  fprintf(fp, "%d ", rensetu_tbl[i].form);
	  if ( rensetu_tbl[i].goi )
	    fprintf(fp, "%s\n", rensetu_tbl[i].goi);
	  else 
	    fprintf(fp, "*\n");
     }
}

void write_matrix(FILE *fp)
{
     int i, j;
     
     fprintf(fp, "%d %d\n", I_NUM, J_NUM);

     for ( i=0; i<I_NUM; i++ ) {
	  for ( j=0; j<J_NUM; j++ )
	    fprintf(fp, "%d ", (int)rensetu_mtr1[i][j]);
	  fprintf(fp, "\n");
     }
}

/*
------------------------------------------------------------------------------
	FUNCTION:
	<main>:
------------------------------------------------------------------------------
*/	       

int main(int argc, char **argv)
{
    FILE       *fp_t, *fp_m, *fp_k, *fp_c;
    char       file_path[FILENAME_MAX];

    int i;

    ProgName = argv[0];
    Cha_stderr = stderr;

    if ((argc == 3)&&(strncmp(argv[1], "-r", 2) == 0)) {
	set_jumanrc_fileptr(argv[2], FALSE, FALSE);
    } else {
	set_jumanrc_fileptr(NULL, TRUE, FALSE);
    }
    if (Jumanrc_Fileptr) {
	set_jumangram_dirname();
    }

    grammar(NULL);
    katuyou(NULL);
     
    getpath(CurPath, JumanPath);
    
    /* 出力ファイルのオープン 
       JumanPathからCurPathへ変更 (2002/11/08)
     */
    
    if ( (fp_t = pathfopen(TABLEFILE, "w", CurPath, file_path)) != NULL );
    else error(OpenError, "can't open", TABLEFILE, ".", EOA);
    
    if ( (fp_m = pathfopen(MATRIXFILE, "w", CurPath, file_path)) != NULL );
    else error(OpenError, "can't open", MATRIXFILE, ".", EOA);
    
    /* 活用ファイルのオープン */

    if ( (fp_k = pathfopen(KANKEIFILE, "r", ""     , file_path)) != NULL );
    else if ((fp_k= pathfopen(KANKEIFILE, "r", CurPath, file_path)) != NULL );
    else if ((fp_k= pathfopen(KANKEIFILE, "r", JumanPath, file_path)) != NULL );
    else error(OpenError, "can't open", KANKEIFILE, ".", EOA);

    /* 活用ファイルの処理 */
	
    fprintf(stderr, "%s parsing... ", file_path);
    read_h_b_t_kankei(fp_k);     
    fprintf(stderr, "done.\n\n");
    
    /* 連接規則ファイルのオープン */
       
    if ( (fp_c = pathfopen(CONNECTFILE, "r", ""     , file_path)) != NULL );
    else if ((fp_c= pathfopen(CONNECTFILE, "r", CurPath, file_path))!= NULL );
    else if ((fp_c= pathfopen(CONNECTFILE, "r", JumanPath, file_path))!= NULL );
    else error(OpenError, "can't open", CONNECTFILE, ".", EOA);

    /* 連接規則ファイルの処理 */
       
    fprintf(stderr, "%s parsing... ", file_path);
    make_rensetu_tbl(fp_c);
    rewind(fp_c);
    read_rensetu(fp_c);
    fprintf(stderr, "done.\n\n");
    
    /* 連接行列の圧縮 */

    condense_matrix();
    
    write_table(fp_t);
    write_matrix(fp_m);
    
    fclose(fp_c);
    fclose(fp_k);
    fclose(fp_t);
    fclose(fp_m);

    exit(0);
}

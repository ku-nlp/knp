/*
==============================================================================
	connect.c
		1990/12/17/Mon	Yutaka MYOKI(Nagao Lab., KUEE)
==============================================================================
*/

#include	<juman.h>

/*
------------------------------------------------------------------------------
	definition of global variables
------------------------------------------------------------------------------
*/

char		CurPath[FILENAME_MAX];
char		JumanPath[FILENAME_MAX];

/*
------------------------------------------------------------------------------
	GLOBAL:
	definition of global variables
------------------------------------------------------------------------------
*/

extern char	*ProgName;
extern CLASS	Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE	Type[TYPE_NO];
extern FORM	Form[TYPE_NO][FORM_NO];

/*
------------------------------------------------------------------------------
	LOCAL:
	definition of global variables
------------------------------------------------------------------------------
*/

static int             TBL_NUM;          /* 連接表のサイズ */
static int             I_NUM;            /* 連接行列の行   */
static int             J_NUM;            /* 連接行列の列   */
RENSETU_PAIR	       *rensetu_tbl;
U_CHAR	               *rensetu_mtr;

/*
------------------------------------------------------------------------------
        rensetu table
------------------------------------------------------------------------------
*/

void connect_table(FILE *fp_out)
{

     FILE	*fp;
     char	tablefile_path[FILENAME_MAX];

     getpath(CurPath, JumanPath);
     while (1) {
	  if ( (fp = pathfopen(TABLEFILE, "r", ""     , tablefile_path))
	      != NULL )	break;
	  if ( (fp = pathfopen(TABLEFILE, "r", CurPath, tablefile_path))
	      != NULL ) break;
	  if ( (fp = pathfopen(TABLEFILE, "r", JumanPath, tablefile_path))
	      != NULL ) break;
	  error(OpenError, "can't open", TABLEFILE, ".", EOA);
     }

     if (fp_out != NULL) {
	  print_current_time(fp_out);
	  fprintf(fp_out, "%s parsing... ", tablefile_path);
     }

     read_table(fp);

     if (fp_out != NULL)
	  fputs("done.\n\n", fp_out);

     fclose(fp);
}






void read_table(FILE *fp)
{
     int i;
     U_CHAR tmp_char[MIDASI_MAX];
     int ret;                          /* for EDRdic '94.Mar */

     fscanf(fp, "%d\n", &TBL_NUM);

     rensetu_tbl = (RENSETU_PAIR *)my_alloc(sizeof(RENSETU_PAIR) * TBL_NUM);

     for ( i=0; i<TBL_NUM; i++ ) {
/*	  while ( fgetc(fp) != '\n' );	  */
	  fscanf(fp, "%d", &(rensetu_tbl[i].i_pos));
	  fscanf(fp, "%d", &(rensetu_tbl[i].j_pos));

          ret = fscanf(fp, "%d", &(rensetu_tbl[i].hinsi));
          if( ret != 0 ) {
	      fscanf(fp, "%d", &(rensetu_tbl[i].bunrui));
	      fscanf(fp, "%d", &(rensetu_tbl[i].type));
	      fscanf(fp, "%d", &(rensetu_tbl[i].form));
	      fscanf(fp, "%s\n", tmp_char);
	      if ( tmp_char[0] == '*' )
		rensetu_tbl[i].goi = NULL;
	      else {
	        rensetu_tbl[i].goi = (U_CHAR *)my_alloc(sizeof(U_CHAR) * 
						   MIDASI_MAX );
	        strcpy(rensetu_tbl[i].goi, tmp_char);
	      }
	  }
          else{                       /* for EDRdic '94.Mar */
              rensetu_tbl[i].hinsi = -1 ;
              fscanf(fp, "%s\n", tmp_char);

              rensetu_tbl[i].goi = (U_CHAR *)my_alloc(sizeof(U_CHAR) *
                                                      MIDASI_MAX );
              strcpy(rensetu_tbl[i].goi, tmp_char);
          }
     }
}

void check_edrtable(MRPH *mrph_p, CELL *x) /* for EDRdic '94.Mar */
{
     int i;

     for ( i=0; i<TBL_NUM; i++ ) {
          if ( rensetu_tbl[i].hinsi == -1 &&
               strcmp(_Atom(x), rensetu_tbl[i].goi) == 0 ) {
               mrph_p->con_tbl = i;
               return;
          }
     }
     error(OtherError, "No morpheme in EDR-table !!", EOA);
}

void check_table(MRPH *mrph_p)
{
     int i;

     for ( i=0; i<TBL_NUM; i++ ) {
	  if ( mrph_p->hinsi    == rensetu_tbl[i].hinsi &&
	       mrph_p->bunrui   == rensetu_tbl[i].bunrui &&
	       mrph_p->katuyou1 == rensetu_tbl[i].type &&
	       ( rensetu_tbl[i].goi == NULL || 
		 strcmp(mrph_p->midasi, rensetu_tbl[i].goi) == 0 ) ) {
	       mrph_p->con_tbl = i;
	       return;
	  }
     }
     error(OtherError, "No morpheme in table !!", EOA);
}

void check_table_for_rengo(MRPH *mrph_p)
{
     int i;

     for ( i=0; i<TBL_NUM; i++ ) {
	 if (rensetu_tbl[i].hinsi == atoi(RENGO_ID) &&
	     rensetu_tbl[i].type == mrph_p->katuyou1 &&
	     strcmp(rensetu_tbl[i].goi , mrph_p->midasi) == 0) {
	     mrph_p->con_tbl = i;
	     return;
	 }
     }
     mrph_p->con_tbl = -1;
}

int check_table_for_undef(int hinsi, int bunrui)
{
     int i;

     for ( i=0; i<TBL_NUM; i++ )
       if ( rensetu_tbl[i].hinsi == hinsi &&
            rensetu_tbl[i].bunrui == bunrui ) {
	    return i;
       }
     return -1;
}



/*
------------------------------------------------------------------------------
        rensetu matrix
------------------------------------------------------------------------------
*/

void connect_matrix(FILE *fp_out)
{

     FILE	*fp;
     char	matrixfile_path[FILENAME_MAX];

     getpath(CurPath, JumanPath);
     while (1) {
	  if ( (fp = pathfopen(MATRIXFILE, "r", ""     , matrixfile_path))
	      != NULL )	break;
	  if ( (fp = pathfopen(MATRIXFILE, "r", CurPath, matrixfile_path))
	      != NULL ) break;
	  if ( (fp = pathfopen(MATRIXFILE, "r", JumanPath, matrixfile_path))
	      != NULL ) break;
	  error(OpenError, "can't open", MATRIXFILE, ".", EOA);
     }

     if (fp_out != NULL) {
	  print_current_time(fp_out);
	  fprintf(fp_out, "%s parsing... ", matrixfile_path);
     }

     read_matrix(fp);

     if (fp_out != NULL)
	  fputs("done.\n\n", fp_out);

     fclose(fp);
}

void read_matrix(FILE *fp)
{
     int i, j, num;

     fscanf(fp, "%d", &I_NUM);
     fscanf(fp, "%d", &J_NUM);

     rensetu_mtr = (U_CHAR *)my_alloc(sizeof(U_CHAR) * I_NUM * J_NUM );

     for ( i=0; i<I_NUM; i++ )
       for ( j=0; j<J_NUM; j++ ) {
	    if ( fscanf(fp, "%d", &num) == EOF )
	      error(OtherError, "No entry in matrix !!", EOA);
	    rensetu_mtr[i*J_NUM +j] = (char)num;
       }
}

int check_matrix(int postcon, int precon)
{
     if ( postcon == -1 || precon == -1 ) 
       return DEFAULT_C_WEIGHT;

     return ((int)rensetu_mtr[ rensetu_tbl[postcon].i_pos * J_NUM
			       + rensetu_tbl[precon].j_pos ]);
}

/* その連語に特有の左連接規則が記述されているか */
int check_matrix_left(int precon)
{
    int i;

    if (precon == -1) return FALSE;
    for (i = 0 ; i < I_NUM ; i++)
	if ((int)rensetu_mtr[ i * J_NUM
			       + rensetu_tbl[precon].j_pos ]) return TRUE;
    return FALSE;
}

/* その連語に特有の右連接規則が記述されているか */
int check_matrix_right(int postcon)
{
    int j;

    if (postcon == -1) return FALSE;
    for (j = 0 ; j < J_NUM ; j++)
	if ((int)rensetu_mtr[ rensetu_tbl[postcon].i_pos * J_NUM
			       + j ]) return TRUE;
    return FALSE;
}

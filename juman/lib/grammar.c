/*
==============================================================================
	grammar.c
		1990/11/14/Wed	Yutaka MYOKI(Nagao Lab., KUEE)
==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include	<juman.h>
#define        HINSI_ERROR   0
#define        BUNRUI_ERROR  1
#define        PARSE_ERROR   2


/*
------------------------------------------------------------------------------
	definition of global variables
------------------------------------------------------------------------------
*/

CLASS		Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];

char		CurPath[FILENAME_MAX];
char		JumanPath[FILENAME_MAX];

extern char	*ProgName;
extern int      LineNo;
extern int      LineNoForError;

extern COST_OMOMI       cost_omomi;     /*k.n*/
extern char             Jumangram_Dirname[FILENAME_MAX];  /*k.n*/


/*
------------------------------------------------------------------------------
	PROCEDURE
 	<error_in_grammar>: local error processing
------------------------------------------------------------------------------
*/

void error_in_grammar(int n, int line_no)
{
     switch (n) {
     case HINSI_ERROR:
	  fprintf(stderr, "\nparse error at line %d\n", line_no);
	  fprintf(stderr, "\ttoo many classfication.\n");
	  my_exit(GramError);
	  break;
     case BUNRUI_ERROR:
	  fprintf(stderr, "\nparse error at line %d\n", line_no);
	  fprintf(stderr, "\ttoo many sub-classfication.\n");
	  my_exit(GramError);
	  break;
     default:
	  fprintf(stderr, "\nparse error at line %d\n", line_no);
	  my_exit(GramError);
	  break;
     }
}

/*
------------------------------------------------------------------------------
	PROCEDURE
	<initialize_class>: initialize <Class[][]>
------------------------------------------------------------------------------
*/

void initialize_class(void)
{
     int i, j;

     for (i = 0; i < CLASSIFY_NO + 1; i++) {
	  for (j = 0; j < CLASSIFY_NO + 1; j++) {
	       Class[i][j].id = (U_CHAR *)NULL;
	       Class[i][j].kt = FALSE;
	       Class[i][j].cost = 0;  /* k.n */
	  }
     }
}

/*
------------------------------------------------------------------------------
	PROCEDURE
	<print_class_>: print <Class[][]> on <fp> according to format
------------------------------------------------------------------------------
*/

void print_class_(FILE *fp, int tab1, int tab2, char *flag)
{
     int	i, j, n;

     for (i = 1; (Class[i][0].id != NULL) && (i < CLASSIFY_NO); i++) {
	  if (tab1 > 0) for (n = 0; n < tab1; n++) fputc(' ', fp); 
	  fprintf(fp, "%3d: %s", i, Class[i][0].id);
	  if (Class[i][0].kt) fputs(flag, fp);
	  fputc('\n', fp);
	  for (j = 1; (Class[i][j].id != NULL) && (j < CLASSIFY_NO); j++) {
	       if (tab2 > 0) for (n = 0; n < tab2; n++) fputc(' ', fp);
	       fprintf(fp, "        %3d: %s", j, Class[i][j].id);
	       if (Class[i][j].kt) fputs(flag, fp);
	       fputc('\n', fp);
	  }
     }
}

/*
------------------------------------------------------------------------------
	PROCEDURE
	<read_class>: read-in <Class[][]> from <fp>
------------------------------------------------------------------------------
*/

void read_class(FILE *fp)
{
     CELL       *cell1, *cell2;
     int        i, j;
     int 	katuyou_flag = 0;

     LineNo = 1;
     i = 1;
     while (! s_feof(fp)) {
	  j = 0;
          LineNoForError = LineNo;

          cell1 = s_read(fp);

	  if ( !Null(cell2 = car(cell1)) ) {
	       Class[i][j].id = (U_CHAR *)
		 my_alloc((sizeof(U_CHAR)*strlen(_Atom(car(cell2)))) + 1);
               strcpy(Class[i][j].id, _Atom(car(cell2)));
	       if ( !Null(cdr(cell2)) ) {
		    katuyou_flag = 1;
		    Class[i][j].kt = TRUE;
	       } else
		 katuyou_flag = 0;
               cell1 = car(cdr(cell1));
	       j++;
	  } else {
	       error_in_grammar(PARSE_ERROR, LineNo);
	  }

          while (!Null(cell2 = car(cell1))) {
               Class[i][j].id = (U_CHAR *)
                    my_alloc((sizeof(U_CHAR)*strlen(_Atom(car(cell2)))) + 1);
	       strcpy(Class[i][j].id, _Atom(car(cell2)));

	       if ( katuyou_flag || !Null(cdr(cell2)) )
		 Class[i][j].kt = TRUE;

               cell1 = cdr(cell1);
	       j++;
	       if ( j >= CLASSIFY_NO ) error_in_grammar(BUNRUI_ERROR, LineNo);
          }
          i++;
	  if ( i >= CLASSIFY_NO ) error_in_grammar(HINSI_ERROR, LineNo);
     }
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<grammar>: initialize <Class[][]> and read-in <Class[][]>
------------------------------------------------------------------------------
*/

void grammar(FILE *fp_out)
{

     FILE	*fp;
     char	grammarfile_path[FILENAME_MAX];
     char	*prog_basename = NULL;

     /* program basename (juman, makeint, ...) */
     if (ProgName) {
         if ((prog_basename = strrchr(ProgName, '/'))) {
             prog_basename++;
         }
         else {
             prog_basename = ProgName;
         }
     }

     getpath(CurPath, JumanPath);
     while (1) {
	  if ( (fp = pathfopen(GRAMMARFILE, "r", ""     , grammarfile_path))
	      != NULL )	break;
	  if ( (fp = pathfopen(GRAMMARFILE, "r", CurPath, grammarfile_path))
	      != NULL ) break;
	  if ( prog_basename && strcmp(prog_basename, "juman") && (fp = pathfopen(GRAMMARFILE, "r", "../dic/", grammarfile_path)) /* for compilation (program is not juman) */
	      != NULL ) break;
	  if ( (fp = pathfopen(GRAMMARFILE, "r", JumanPath, grammarfile_path))
	      != NULL ) break;
	  error(OpenError, "can't open", GRAMMARFILE, ".", EOA);
     }

     if (fp_out != NULL) {
	  print_current_time(fp_out);
	  fprintf(fp_out, "%s parsing... ", grammarfile_path);
     }

     initialize_class(); read_class(fp);

     if (fp_out != NULL)
	  fputs("done.\n\n", fp_out);

     fclose(fp);
}

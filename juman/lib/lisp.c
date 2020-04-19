/*
==============================================================================
	lisp.c
		utility functions like LISP
		1990/11/12/Mon	Yutaka MYOKI(Nagao Lab., KUEE)
		1990/12/16/Mon	Last Modified
		special thanks to Itsuki NODA
==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include	<juman.h>

/*
------------------------------------------------------------------------------
	definition of global variables
------------------------------------------------------------------------------
*/

extern int		LineNo;
extern int		LineNoForError;

/*
------------------------------------------------------------------------------
	LOCAL:
	definition of global variables
------------------------------------------------------------------------------
*/

U_CHAR		Buffer[BUFSIZE];
CELL		_TmpCell;
CELL		*TmpCell = &_TmpCell;
CELLTABLE	*CellTbl = NULL;

CELLTABLE	CellTbl_save;

/*
  以下の定義はchasenのlisp.cから流用
  chasenのサーバーモードでは\n.\nをEOFと見なす。それに対応するようにchasenの
  コードを流用
  それにともないfgetc関数をすべてcha_fgetc関数に変更
  また、jumanでは文法ファイルなどをファイルから読み込むためEOF.\nもEOFと
  見なすようにis_bolの設定部分を変更
  NACSIS 吉岡
*/
extern int fgetc(FILE *fp);
extern int ungetc(int c, FILE *fp);

static int (*my_getc)(FILE *fp) = fgetc;
static int (*my_ungetc)(int c, FILE *fp) = ungetc;

static int is_bol = 1;
static int c_stacked = EOF;

static int cha_getc(fp)
    FILE *fp;
{
    int c;

    if (c_stacked != EOF) {
	c = c_stacked;
	c_stacked = EOF;
    } else
      c = getc(fp);

    /* skip '\r' */
    if (c == '\r')
      c = getc(fp);

    if (c == EOf && is_bol) {
	/* skip '\r' */
	if ((c = getc(fp)) == '\r')
	  c = getc(fp);
	if (c == '\n')
	  c = EOF;
	/* \nの続かない04があった場合,04はスキップされてしまう */
    }

    if (c == '\n' || c == EOF)
      is_bol = 1;
    else
      is_bol = 0;

#if 0
    putc(c,stdout);fflush(stdout);
#endif

    return c;
}

static int cha_ungetc(c, fp)
    int c;
    FILE *fp;
{
    c_stacked = c;
}

void set_cha_getc()
{
    my_getc = cha_getc;
    my_ungetc = cha_ungetc;
}
void unset_cha_getc()
{
    my_getc = fgetc;
    my_ungetc = ungetc;
}

/*
------------------------------------------------------------------------------
	local error processing
------------------------------------------------------------------------------
*/

void error_in_lisp(void)
{
     fprintf(stderr, "\nparse error between line %d and %d.\n", 
	     LineNoForError, LineNo);
     my_exit(DicError);
}

void error_in_program(void)
{
     fprintf(stderr, "\n\"ifnextchar\" returns an unexpected code.\n");
     my_exit(ProgramError);
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<s_feof>:
	after skipping "whitespace" , return TRUE if fp points to EOF
------------------------------------------------------------------------------
*/

int s_feof(FILE *fp) /* bug fixed by kurohashi on 92/4/25 */
{
  int	code, c;
   
  if (s_feof_comment(fp) == EOF) {
    code = TRUE;
  } else {
    if ((c = my_getc(fp)) == EOF) {
      code = TRUE;
    } else {
      if ( (U_CHAR)c == '\n') {
	LineNo++;
	code = s_feof(fp);
      } else if ( (U_CHAR)c == ' ' || (U_CHAR)c == '\t') {
	code = s_feof(fp);
      } else {
	my_ungetc(c, fp);
	code = FALSE;
      }
    }
  }
  return code;
}

int s_feof_comment(FILE *fp)
{
     int	n;
     U_CHAR	Buffer[BUFSIZE];

     if ((n = ifnextchar(fp, (int)COMMENTCHAR)) == TRUE) {
	  while (my_getc(fp) != '\n' && !feof(fp)) {}
	  LineNo++;
	  return s_feof_comment(fp);
     }

     return n;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<make_cell>: make a new cell
------------------------------------------------------------------------------
*/

CELL *make_cell(void)
{
    return((CELL *)lisp_alloc(sizeof(CELL)));
}

/*
CELL *make_cell(void)
{
     if (CellTbl == NULL)
	  CellTbl = make_cell_table(NULL, CELLALLOCSTEP);
     if (CellTbl->max <= CellTbl->n)
	  CellTbl = make_cell_table(CellTbl, CELLALLOCSTEP);

     return &(CellTbl->cell[CellTbl->n++]);
}

CELLTABLE *make_cell_table(CELLTABLE *pre, int size)
{
     CELLTABLE	*tbl;

     tbl = (CELLTABLE *)my_alloc(sizeof(CELLTABLE));

     tbl->cell = (CELL *)my_alloc(sizeof(CELL) * size);
     tbl->pre = pre;
     tbl->next = NULL;
     tbl->max = size;
     tbl->n = 0;

     if (pre != NULL) pre->next = tbl;

     return tbl;
}

void free_cell(CELL *cell)
{
     if (Atomp(cell)) {
	  free(cell->value.atom);
     } else if (Consp(cell)) {
	  free_cell(car(cell));
	  free_cell(cdr(cell));
     } else {
	  return;
     }

     free(cell);
}
*/

/*
------------------------------------------------------------------------------
	FUNCTION
	<tmp_atom>: use <TmpCell>
------------------------------------------------------------------------------
*/

CELL *tmp_atom(U_CHAR *atom)

{
     _Tag(TmpCell) = ATOM;
     _Atom(TmpCell) = atom;

     return TmpCell;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<cons>: make <cons> from <car> & <cdr>
------------------------------------------------------------------------------
*/

CELL *cons(void *car, void *cdr)
{
     CELL *cell;

     cell = make_cell();
     _Tag(cell) = CONS;
     _Car(cell) = car;
     _Cdr(cell) = cdr;

     return cell;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<car>: take <car> from <cons>
------------------------------------------------------------------------------
*/

CELL *car(CELL *cell)
{
     if (Consp(cell))
	  return _Car(cell);
     else if (Null(cell))
	  return NIL;
     else {
	  s_print(stderr, cell);
	  fprintf(stderr, "is not list. in <car>\n");
	  error_in_lisp();
     }
     return NIL;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<cdr>: take <cdr> from <cons>
------------------------------------------------------------------------------
*/

CELL *cdr(CELL *cell)
{
    if (Consp(cell))
      return _Cdr(cell);
    else if (Null(cell))
      return NIL;
    else {
	s_print(stderr, cell);
	fprintf(stderr, "is not list. in <cdr>\n");
	error_in_lisp();
    }
    return NIL;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<equal>:
------------------------------------------------------------------------------
*/

int equal(void *x, void *y)
{
     if (Eq(x, y)) return TRUE;
     else if (Null(x) || Null(y)) return FALSE;
     else if (_Tag(x) != _Tag(y)) return FALSE;
     else if (_Tag(x) == ATOM)	  return !strcmp(_Atom(x), _Atom(y));
     else if (_Tag(x) == CONS)
	  return (equal(_Car(x), _Car(y)) && equal(_Cdr(x), _Cdr(y)));
     else
	  return FALSE;
}

int length(CELL *list)
{
     int	i;

     for (i = 0; Consp(list); i++) {
	  list = _Cdr(list);
     }

     return i;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<ifnextchar>: if next char is <c> return 1, otherwise return 0
------------------------------------------------------------------------------
*/

int ifnextchar(FILE *fp, int i)
{
     int 	c;

     do {
	  c = my_getc(fp);
	  if (c == '\n') LineNo++;
     } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

     if (c == EOF) return EOF;

     if (i == c) 
	  return TRUE;
     else {
	  my_ungetc(c, fp);
	  return FALSE;
     }
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<comment>: skip comment-line(s)
------------------------------------------------------------------------------
*/

int comment(FILE *fp)
{
     int	n;

     if ((n = ifnextchar(fp, (int)COMMENTCHAR)) == TRUE) {
	  while (my_getc(fp) != '\n' && !feof(fp)) {}
/*	  if (fgets(Buffer, BUFSIZE, fp) == NULL)
	       error(SystemError, "\"fgets\" error in <comment>.", EOA);*/
	  LineNo++;
	  comment(fp);
     } else if (n == EOF) {
	  /*
	  error_in_lisp();
	  */
     }

     return n;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<s_read>: read S-expression
------------------------------------------------------------------------------
*/

CELL *s_read(FILE *fp)
{
     int n;

     if ((n = ifnextchar(fp, (int)BPARENTHESIS)) == TRUE) {
	  return s_read_car(fp);
     } else if (n == FALSE) {
	  return s_read_atom(fp);
     }

     if (n == EOF) error_in_lisp();
     else	   error_in_program();

     return NIL;
}

int myscanf(FILE *fp, U_CHAR *cp)
{
    int code;

    code = my_getc(fp);
    if ( dividing_code_p(code) )
      return 0;
    else if ( code == EOF )
      return EOF;
    else if ( code == '"' ) {
	*cp++ = code;
	while ( 1 ) {
	    code = my_getc(fp);
	    if ( code == EOF )
	      error_in_lisp();
	    else if ( code == '"' ) {
		*cp++ = code;
		*cp++ = '\0';
		return 1;
	    }
#ifndef IO_ENCODING_SJIS
	    else if ( code == '\\' ) {
		*cp++ = code;
		if ( (code = my_getc(fp)) == EOF ) 
		    error_in_lisp();
		*cp++ = code;
	    }
#endif
	    else {
		*cp++ = code;
	    }
	}
    }
    else {
	*cp++ = code;
#ifndef IO_ENCODING_SJIS
	if (code == '\\') 
	    *(cp-1) = my_getc(fp); /* kuro on 12/01/94 */
#endif
	while ( 1 ) {
	    code = my_getc(fp);
	    if ( dividing_code_p(code) || code == EOF ) {
		*cp++ = '\0';
		my_ungetc(code, fp);
		return 1;
	    }
	    else {
		*cp++ = code;
#ifndef IO_ENCODING_SJIS
		if (code == '\\') 
		    *(cp-1) = my_getc(fp); /* kuro on 12/01/94 */
#endif
	    }
	}
    }
}

int dividing_code_p(int code)
{
    switch (code) {
      case '\n': case '\r': case '\t': case ';': case ' ':
      case BPARENTHESIS:
      case EPARENTHESIS:
	return 1;
      default:
	return 0;
    }
}

CELL *s_read_atom(FILE *fp)
{
     CELL *cell;
     U_CHAR *c;
     int n;
/* #ifdef _WIN32
    char *eucstr;
    #endif */

     comment(fp);
     
     /* changed by kurohashi.

     if (((n = fscanf(fp, SCANATOM, Buffer)) == 0) || (n == EOF)) {
	  error_in_lisp();
     }
     */

     if (((n = myscanf(fp, Buffer)) == 0) || (n == EOF)) {
	  error_in_lisp();
     }

/* #ifdef _WIN32
	eucstr = toStringEUC(Buffer);
	strcpy(Buffer, eucstr);
	free(eucstr);
	#endif */

     if (!strcmp(Buffer, NILSYMBOL)) {
	  cell = NIL;
     } else {
	  cell = new_cell();
	  _Tag(cell) = ATOM;
	  c = (U_CHAR *)lisp_alloc(sizeof(U_CHAR) * (strlen(Buffer)+1));
	  strcpy(c, Buffer);
	  _Atom(cell) = c;
     }

     return cell;
}

CELL *s_read_car(FILE *fp)
{
     CELL	*cell;
     int	n;

     comment(fp);

     if ((n = ifnextchar(fp, (int)EPARENTHESIS)) == TRUE) {
	  cell = (CELL *)NIL;
	  return cell;
     } else if (n == FALSE) {
	  cell = new_cell();
	  _Car(cell) = s_read(fp);
	  _Cdr(cell) = s_read_cdr(fp);
	  return cell;
     }

     if (n == EOF) error_in_lisp();
     else          error_in_program();

     return NIL;
}

CELL *s_read_cdr(FILE *fp)
{
     CELL	*cell;
     int	n;

     comment(fp);
     if ((n = ifnextchar(fp, (int)EPARENTHESIS)) == TRUE) {
	  cell = (CELL *)NIL;
	  return cell;
     } else if (n == FALSE) {
	  cell = s_read_car(fp);
	  return cell;
     }

     if (n == EOF) error_in_lisp();
     else          error_in_program();

     return NIL;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<assoc>:
------------------------------------------------------------------------------
*/

CELL *assoc(CELL *item, CELL *alist)
{
     for ( ; (!equal(item, (car(car(alist)))) && (!Null(alist)));
	      alist = cdr(alist))
	  ;
     return car(alist);
}
	      
/*
------------------------------------------------------------------------------
	PROCEDURE
	<s_print>: pretty print S-expression
------------------------------------------------------------------------------
*/

CELL *s_print(FILE *fp, CELL *cell)
{
     _s_print_(fp, cell);
     fputc('\n', fp);
}

CELL *_s_print_(FILE *fp, CELL *cell)
{
     if (Null(cell))
	  fprintf(fp, "%s", NILSYMBOL);
     else {
	  switch (_Tag(cell)) {
	  case CONS:
	       fprintf(fp, "%c", BPARENTHESIS);
	       _s_print_(fp, _Car(cell));
	       _s_print_cdr(fp, _Cdr(cell));
	       fprintf(fp, "%c", EPARENTHESIS);
	       break;
	  case ATOM:
	       fprintf(fp, "%s", _Atom(cell));
	       break;
	  default:
	       error(OtherError, "Illegal cell(in s_print)", EOA);
	  }
     }

     return cell;
}

CELL *_s_print_cdr(FILE *fp, CELL *cell)
{
     if (!Null(cell)) {
	  if (Consp(cell)) {
	       fprintf(fp, " ");
	       _s_print_(fp, _Car(cell));
	       _s_print_cdr(fp, _Cdr(cell));
	  } else {
	       fputc(' ', fp);
	       _s_print_(fp, cell);
	  }
     }

     return cell;
}

/*
------------------------------------------------------------------------------
	PROCEDURE			by yamaji
	<lisp_alloc>: あらかじめ一定領域を確保しておいて malloc を行う
------------------------------------------------------------------------------
*/

void *lisp_alloc(int n)
{
     CELLTABLE	*tbl;
     CELL *p;

     if (n % sizeof(CELL)) n = n/sizeof(CELL)+1; else n /= sizeof(CELL);
     if (CellTbl == NULL || CellTbl != NULL && CellTbl->n+n > CellTbl->max) {
	 /* 新たに一定領域を確保 */
	 if (CellTbl != NULL && CellTbl->next != NULL) {
	     CellTbl = CellTbl->next;
	     CellTbl->n = 0;
	 } else {
	     tbl = (CELLTABLE *)my_alloc(sizeof(CELLTABLE));
	     tbl->cell = (CELL *)my_alloc(sizeof(CELL)*BLOCKSIZE);
	     tbl->pre  = CellTbl;
	     tbl->next = NULL;
	     tbl->max  = BLOCKSIZE;
	     tbl->n    = 0;
	     if (CellTbl != NULL) CellTbl->next = tbl;
	     CellTbl = tbl;
	 }
     }
     p = CellTbl->cell + CellTbl->n;
     CellTbl->n += n;
     if (CellTbl->n > CellTbl->max) error_in_lisp();

     return((void *)p);
}

/*
------------------------------------------------------------------------------
	PROCEDURE			by yamaji
	<lisp_alloc_push>: 現在のメモリアロケート状態を記憶する
------------------------------------------------------------------------------
*/

void lisp_alloc_push(void)
{
    CellTbl_save = *CellTbl;
}

/*
------------------------------------------------------------------------------
	PROCEDURE			by yamaji
	<lisp_alloc_pop>: 記憶したメモリアロケート状態に戻す
------------------------------------------------------------------------------
*/

void lisp_alloc_pop(void)
{
    CellTbl->cell = CellTbl_save.cell;
    CellTbl->n    = CellTbl_save.n;
}


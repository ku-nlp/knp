
/*
==============================================================================
	makeint.h
		1990/11/09/Fri Yutaka MYOKI(Nagao Lab., KUEE)
==============================================================================
*/

#include	<juman.h>

/*
------------------------------------------------------------------------------
	prototype definition of functions
------------------------------------------------------------------------------
*/

/* makeint.c */
void	usage(void);
void 	translate(char *arg);
int 	main(int argc, char *argv[]);

/* trans.c */
static void 	init_mrph(MRPH *mrph_p);
static void     output_mrph(FILE *fp, MRPH *mrph_p);
static void	numeral_encode(FILE *fp, int num);
static void	imi_print(FILE *fp, CELL *cell);
static void	_imi_print(char *buf, CELL *cell);
static void	_imi_print_cdr(char *buf, CELL *cell);
static void 	print_mrph(MRPH *mrph_p);
static void 	print_mrph_loop(MRPH *mrph_p);
static void 	error_in_trans(int n, void *c);
static U_CHAR	*midasi(CELL *x);
static CELL	*midasi_list(CELL *x);
static U_CHAR	*yomi(CELL *x);
static int	katuyou1(CELL *x);
static int      katuyou2(CELL *x , int type);
static CELL     *edrconnect(CELL *x);
static CELL	*imi(CELL *x);
static void	trim_yomi_gobi(MRPH *mrph_p);
static void	trim_midasi_gobi(MRPH *mrph_p);
static void 	change_kana(U_CHAR *kana, char vowel);
static void 	change_gobi(U_CHAR *midasi, int katuyou1, int katuyou2);
static void 	change_gobi_yomi(U_CHAR *yomi, int katuyou1, int katuyou2);
void		trans(FILE *fp_in, FILE *fp_out);
static void     _trans(CELL *cell , int rengo_p);
static void     __trans(CELL *block, MRPH *mrph_p, int rengo_p);

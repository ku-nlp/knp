/*
==============================================================================
	makemat.h
==============================================================================
*/

#include	<juman.h>

/*
------------------------------------------------------------------------------
	LOCAL:
	type definition of structures
------------------------------------------------------------------------------
*/

typedef         struct          _H_B_T_KANKEI {
     int   hinsi;
     int   bunrui;
     int   type;
} H_B_T_KANKEI;

typedef         struct          _RENSETU_PAIR2 {
     int   i_pos;
     int   j_pos;
     int   hinsi;
     int   bunrui;
     int   type;
     U_CHAR  *form;
     U_CHAR  *goi;
} RENSETU_PAIR2;

/*
------------------------------------------------------------------------------
	prototype definition of functions
------------------------------------------------------------------------------
*/

/* makemat.c */
void    read_h_b_k_kankei(FILE *fp);
void    make_rensetu_tbl();
void    _make_rensetu_tbl1(CELL *cell1, int *cnt);
void    _make_rensetu_tbl2(int hinsi, int bunrui, int *cnt);
void    read_rensetu(FILE *fp);
void    fill_matrix(RENSETU_PAIR2 *pair_p1, RENSETU_PAIR2 *pair_p2, U_CHAR c_weight);
void    get_pair_id1(CELL *cell, RENSETU_PAIR *pair);
void    get_pair_id2(CELL *cell, RENSETU_PAIR2 *pair);
int     pair_match1(RENSETU_PAIR *pair1, RENSETU_PAIR *pair2);
int     pair_match2(RENSETU_PAIR2 *pair, RENSETU_PAIR *tbl);
void    copy_vector1(int i, int i_num, int num);
int     compare_vector1(int k, int j, int num);
void    copy_vector2(int j, int j_num, int num);
int     compare_vector2(int k, int i, int num);
void    write_table(FILE *fp);
void    write_matrix(FILE *fp);

/*====================================================================

		    形態素，文節マッチングルーチン

                                               K.Yamagami
                                               M.Nishida
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

#define MRPH_HINSHI	1
#define MRPH_BUNRUI	2
#define MRPH_KATA	3
#define MRPH_KEI	4
#define MRPH_GOI	5

const REGEXPMRPH RegexpMrphInitValue = { 
    MAT_FLG, NULL, 
    /* Hinshi */
    NULL, {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /* Bunrui */
    NULL, {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /* Katuyou_Kata */
    NULL, {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /* Katuyou_Kei */
    NULL, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    /* Goi */
    NULL, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}};
const REGEXPMRPHS RegexpmrphsInitValue = {NULL, 0};

const REGEXPBNST RegexpBnstInitValue = {
    MAT_FLG, NULL, NULL
};

/*==================================================================*/
/*   		              ストア                                */
/*==================================================================*/

/*==================================================================*/
		REGEXPMRPH *regexpmrph_alloc(int size)
/*==================================================================*/
{
    REGEXPMRPH *tmp;
    int i;
    
    if (!(tmp = (REGEXPMRPH *)malloc(size * sizeof(REGEXPMRPH)))) {
	fprintf(stderr, "Can't allocate memory for REGEXPMRPH\n");
	exit(-1);
    }
    for (i = 0; i < size; i++) { 
	*(tmp + i) = RegexpMrphInitValue;
    }
    return tmp;
}

/*==================================================================*/
		   REGEXPMRPHS *regexpmrphs_alloc()
/*==================================================================*/
{
    REGEXPMRPHS *tmp;
    int i;
    
    if (!(tmp = (REGEXPMRPHS *)malloc(sizeof(REGEXPMRPHS)))) {
	fprintf(stderr, "Can't allocate memory for REGEXPMRPHS\n");
	exit(-1);
    }

    return tmp;
}

/*==================================================================*/
		REGEXPBNST *regexpbnst_alloc(int size)
/*==================================================================*/
{
    REGEXPBNST *tmp;
    int i;
    
    if (!(tmp = (REGEXPBNST *)malloc(size * sizeof(REGEXPBNST)))) {
	fprintf(stderr, "Can't allocate memory for REGEXPBNST\n");
	exit(-1);
    }
    for (i = 0; i < size; i++) { 
	*(tmp + i) = RegexpBnstInitValue;
    }
    return tmp;
}

/*==================================================================*/
		   REGEXPBNSTS *regexpbnsts_alloc()
/*==================================================================*/
{
    REGEXPBNSTS *tmp;
    int i;
    
    if (!(tmp = (REGEXPBNSTS *)malloc(sizeof(REGEXPBNSTS)))) {
	fprintf(stderr, "Can't allocate memory for REGEXPBNSTS\n");
	exit(-1);
    }

    return tmp;
}

/*==================================================================*/
	   void store_mrph_nflag(REGEXPMRPH *mp, int type)
/*==================================================================*/
{
    switch (type) {
      case MRPH_HINSHI: mp->Hinshi_not = NOT_FLG; break;
      case MRPH_BUNRUI: mp->Bunrui_not = NOT_FLG; break;
      case MRPH_KATA:   mp->Kata_not = NOT_FLG; break;
      case MRPH_KEI:    mp->Kei_not = NOT_FLG; break;
      case MRPH_GOI:    mp->Goi_not = NOT_FLG; break;
      default: break;
    }
}

/*==================================================================*/
 static CELL *store_mrph_item(REGEXPMRPH *mp, CELL *mcell, int type)
/*==================================================================*/
{
    int nth;
    CELL *list_cell;
    char *tmp;

    if (Null(car(mcell))) return NULL;

    if (Atomp(car(mcell))) {
	tmp = _Atom(car(mcell));
	if (str_eq(tmp, AST_STR)) { 				/* "*" */
	    return cdr(mcell);    
	} 
	else if (str_eq(tmp, NOT_STR)) { 			/* "^" */
	    store_mrph_nflag(mp, type);
	    mcell = cdr(mcell);
	    if (Atomp(car(mcell))) 
	      list_cell = cons(car(mcell), NULL);    
	    else 
	      list_cell = car(mcell);
	}
	else if (!strncmp(tmp, NOT_STR, strlen(NOT_STR))) { 	/* "^atom" */
	    store_mrph_nflag(mp, type);
	    _Atom(car(mcell)) += strlen(NOT_STR);
	    list_cell = cons(car(mcell), NULL);    
	}
	else {							/* "atom" */
	    list_cell = cons(car(mcell), NULL);
	}
    } else {							/* "(...)" */
	list_cell = car(mcell);
    }
    
    if (type == MRPH_BUNRUI && mp->Hinshi[1] != -1) {
	fprintf(stderr, "Cannot restrict Bunrui for multiple Hinshis.\n");
	error_in_lisp();
    }

    nth = 0;
    while (!Null(car(list_cell))) {
	switch (type) {
	  case MRPH_HINSHI:
	    mp->Hinshi[nth] = get_hinsi_id(_Atom(car(list_cell)));
	    break;
	  case MRPH_BUNRUI:
	    mp->Bunrui[nth] = 
	      get_bunrui_id(_Atom(car(list_cell)), mp->Hinshi[0]);
	    break;
	  case MRPH_KATA:
	    mp->Katuyou_Kata[nth] = get_type_id(_Atom(car(list_cell)));
	    break;
	  case MRPH_KEI:
	    tmp = _Atom(car(list_cell));
	    mp->Katuyou_Kei[nth] = 
	      (char *)malloc((strlen(tmp)+1) * sizeof(char));
	    strcpy(mp->Katuyou_Kei[nth], tmp);
	    break;
	  case MRPH_GOI:
	    tmp = _Atom(car(list_cell));
	    mp->Goi[nth] = 
	      (char *)malloc((strlen(tmp)+1) * sizeof(char));
	    strcpy(mp->Goi[nth], tmp);
	    break;
	  default: break;
	}
	list_cell = cdr(list_cell);
	nth++;
    }
    return cdr(mcell);
}

/*==================================================================*/
	  char store_regexpmrph(REGEXPMRPH *mp, CELL *mcell)
/*==================================================================*/
{
    /* 形態素が特殊文字による指定の場合 */

    if (Atomp(mcell)) {
	if (str_eq(_Atom(mcell), NOT_STR)) {	 	/* "^" */
	    mp->type_flag = NOT_FLG;
	    return NOT_FLG;
	}
	else if (str_eq(_Atom(mcell), QST_STR)) { 	/* "?" */
	    mp->type_flag = QST_FLG;
	    return QST_FLG;
	}
	else if (str_eq(_Atom(mcell), AST_STR)) { 	/* "*" */
	    (mp - 1)->ast_flag = AST_FLG;
	    return AST_FLG;
	}
	else if (str_eq(_Atom(mcell), QST_STR AST_STR)) {	/* "?*" */
	    mp->type_flag = QST_FLG;
	    mp->ast_flag = AST_FLG;
	    return QST_FLG;
	}
	else {
	    fprintf(stderr, "Invalid string for meta mrph (%s).\n", 
		    _Atom(mcell));
	    error_in_lisp();
	    return NULL;
	}
    } 

    /* 形態素が通常の指定の場合 */

    else {
	mp->f_pattern.fp[0] = NULL;

	if ((mcell = store_mrph_item(mp, mcell, MRPH_HINSHI)) == NULL) 
	    return MAT_FLG;
	if ((mcell = store_mrph_item(mp, mcell, MRPH_BUNRUI)) == NULL) 
	    return MAT_FLG;
	if ((mcell = store_mrph_item(mp, mcell, MRPH_KATA)) == NULL) 
	    return MAT_FLG;
	if ((mcell = store_mrph_item(mp, mcell, MRPH_KEI)) == NULL) 
	    return MAT_FLG;
	if ((mcell = store_mrph_item(mp, mcell, MRPH_GOI)) == NULL) 
	    return MAT_FLG;

	list2feature_pattern(&(mp->f_pattern), car(mcell));
	if (Null(cdr(mcell))) {
	    return MAT_FLG;
	} else {
	    fprintf(stderr, "Invalid string for NOT_FLAG.\n");
	    error_in_lisp();
	    return NULL;
	}
    }
}

/*==================================================================*/
	  void store_regexpmrphs(REGEXPMRPHS **mspp, CELL *cell)
/*==================================================================*/
{
    int mrph_num = 0;
    REGEXPMRPH *mrph;
    
    if (cell == NULL) {
	*mspp = NULL;
	return;
    }

    *mspp = regexpmrphs_alloc();
    (*mspp)->mrph = regexpmrph_alloc(length(cell));

    while (!Null(cell)) {
	switch(store_regexpmrph(((*mspp)->mrph)+mrph_num, car(cell))) {
	  case MAT_FLG: case QST_FLG:
	    mrph_num++; break;
	  case NOT_FLG: case AST_FLG:
	    break;
	  default:
	    break;
	}
	cell = cdr(cell);
    }
    (*mspp)->mrphsize = mrph_num;
}

/*==================================================================*/
	  char store_regexpbnst(REGEXPBNST *bp, CELL *cell)
/*==================================================================*/
{
    /* 文節が特殊文字による指定の場合 */

    if (Atomp(cell)) {
	if (str_eq(_Atom(cell), NOT_STR)) {	 	/* "^" */
	    bp->type_flag = NOT_FLG;
	    return NOT_FLG;
	}
	else if (str_eq(_Atom(cell), QST_STR)) { 	/* "?" */
	    bp->type_flag = QST_FLG;
	    return QST_FLG;
	}
	else if (str_eq(_Atom(cell), AST_STR)) { 	/* "*" */
	    (bp - 1)->ast_flag = AST_FLG;
	    return AST_FLG;
	}
	else if (str_eq(_Atom(cell), QST_STR AST_STR)) {	/* "?*" */
	    bp->type_flag = QST_FLG;
	    bp->ast_flag = AST_FLG;
	    return QST_FLG;
	}
	else {
	    fprintf(stderr, "Invalid string for meta bnst (%s).\n", 
		    _Atom(cell));
	    error_in_lisp();
	    return NULL;
	}
    } 

    /* 文節が通常の指定の場合 */

    else {
	store_regexpmrphs(&(bp->mrphs), car(cell));
	if (!Null(cdr(cell))) 
	    list2feature_pattern(&(bp->f_pattern), car(cdr(cell)));
	else
	    bp->f_pattern.fp[0] = NULL;

	return MAT_FLG;
    }
}

/*==================================================================*/
	 void store_regexpbnsts(REGEXPBNSTS **bspp, CELL *cell)
/*==================================================================*/
{
    int bnst_num = 0;
    REGEXPBNST *bnst;
    
    if (cell == NULL) {
	*bspp = NULL;
	return;
    }

    *bspp = regexpbnsts_alloc();
    (*bspp)->bnst = regexpbnst_alloc(length(cell));

    while (!Null(cell)) {
	switch(store_regexpbnst(((*bspp)->bnst)+bnst_num, car(cell))) {
	  case MAT_FLG: case QST_FLG:
	    bnst_num++; break;
	  case NOT_FLG: case AST_FLG:
	    break;
	  default:
	    break;
	}
	cell = cdr(cell);
    }
    (*bspp)->bnstsize = bnst_num;
}

/*==================================================================*/
/*   		            マッチング                              */
/*==================================================================*/

/*==================================================================*/
    int rule_HBK_cmp(char flg, int r_data[], int data)
/*==================================================================*/
{
    /* 品詞、細分類，活用型のマッチング */
    int i, tmp_ret = FALSE;

    if (r_data[0] == -1 || r_data[0] == 0)
      return TRUE;
    else {
	for (i = 0; r_data[i]!=-1; i++)
	  if (r_data[i] == data) {
	      tmp_ret = TRUE;
	      break;
	  }
	if ((flg == MAT_FLG && tmp_ret == TRUE) ||
	    (flg == NOT_FLG && tmp_ret == FALSE))
	  return TRUE;
	else 
	  return FALSE;
    }
}

/*==================================================================*/
    int rule_Kei_cmp(char flg, char *r_string[], int kata, int kei)
/*==================================================================*/
{
    /* 活用形のマッチング */
    int i, tmp_ret = FALSE;

    if (r_string[0] == NULL || str_eq(r_string[0], AST_STR))
      return TRUE;
    else if (kata == 0 || kei == 0) 
      return FALSE;
    else {
	for (i = 0; r_string[i]; i++)
	  if (str_eq(r_string[i], Form[kata][kei].name)) {
	      tmp_ret = TRUE;
	      break;
	  }
	if ((flg == MAT_FLG && tmp_ret == TRUE) ||
	    (flg == NOT_FLG && tmp_ret == FALSE))
	  return TRUE;
	else 
	  return FALSE;
    }
}

/*==================================================================*/
	   int mrph_check_function(char *rule, char *data)
/*==================================================================*/
{
    int i;

    if (!strncmp(rule, "&語彙:", 6)) {
	/* データベースを使う場合 */
	if (DicForRuleDBExist == TRUE) {
	    if (strstr((char *)get_rulev(data), rule+6)) {
		return TRUE;
	    }
	}
	/* データベースを使わない場合 */
	else {
	    for (i = 0; i < CurDicForRuleVSize; i++) {
		if (str_eq(DicForRuleVArray[i].key, data) && check_feature(DicForRuleVArray[i].f, rule+6))
		    return TRUE;
	    }
	    return FALSE;
	}
    }
    else {
	fprintf(stderr, "Invalid Mrph-Feature-Function (%s)\n", rule);
	return FALSE;
    }
    return FALSE;
}

/*==================================================================*/
     int rule_Goi_cmp(char flg, char *r_string[], char *d_string)
/*==================================================================*/
{
    /* 語彙のマッチング */
    int i, tmp_ret = FALSE;

    if (r_string[0] == NULL || str_eq(r_string[0], AST_STR))
      return TRUE;
    else {
	for (i = 0; r_string[i]; i++) {
	    /* 関数呼び出し */
	    if (r_string[i][0] == '&')
		if (mrph_check_function(r_string[i], d_string)) {
		    tmp_ret = TRUE;
		    break;
		}
	    if (str_eq(r_string[i],d_string)) {
		tmp_ret = TRUE;
		break;
	    }
	}
	if ((flg == MAT_FLG && tmp_ret == TRUE) ||
	    (flg == NOT_FLG && tmp_ret == FALSE))
	  return TRUE;
	else 
	  return FALSE;
    }
}

/*==================================================================*/
	 int regexpmrph_match(REGEXPMRPH *ptr1, MRPH_DATA *ptr2)
/*==================================================================*/
{
    /* 形態素のマッチング */

    int ret_mrph;

    if (ptr1->type_flag == QST_FLG)
      return TRUE;
    else {
	if (rule_HBK_cmp(ptr1->Hinshi_not,ptr1->Hinshi,ptr2->Hinshi) &&
	    rule_HBK_cmp(ptr1->Bunrui_not,ptr1->Bunrui,ptr2->Bunrui) &&
	    rule_HBK_cmp(ptr1->Kata_not,ptr1->Katuyou_Kata,
			  ptr2->Katuyou_Kata) &&
	    rule_Kei_cmp(ptr1->Kei_not,ptr1->Katuyou_Kei,
			 ptr2->Katuyou_Kata,ptr2->Katuyou_Kei) &&
	    rule_Goi_cmp(ptr1->Goi_not,ptr1->Goi,ptr2->Goi))
	    ret_mrph = TRUE;
	else
	    ret_mrph = FALSE;

	if ((ptr1->type_flag == MAT_FLG && ret_mrph == TRUE) ||
	    (ptr1->type_flag == NOT_FLG && ret_mrph == FALSE))
	    return 
		feature_pattern_match(&(ptr1->f_pattern), ptr2->f, 
				      NULL, ptr2);
	else
	    return FALSE;
    }
}
         
/*==================================================================*/
	   int regexpmrphs_match(REGEXPMRPH *r_ptr,int r_num,
				 MRPH_DATA *d_ptr, int d_num, 
				 int fw_or_bw, 
				 int all_or_part, 
				 int short_or_long)
/*==================================================================*/
{
    /* 形態素列に対してもfeatureを与えられるように変更 99/04/09 */

    int ret, step, return_num;

    (fw_or_bw == FW_MATCHING) ? (step = 1) : (step = -1);

    if (r_num == 0) {
	if (d_num == 0 || all_or_part == PART_MATCHING)
	    return d_num;
	else 
	    return -1;
    } else {
        if (r_ptr->ast_flag == AST_FLG) {
	    
	    /* 
	       パターンに"condition*"がある場合，次の可能性を調べる

	        1. パターンのみ進める(パターンの"*"をスキップ)
	        2. データのみ進める(conditionがデータとマッチすれば)

	       1を先にすればSHORT_MATCHING, 2を先にすればLONG_MATCHING
	    */
	    
	    if (short_or_long == SHORT_MATCHING) {
		if ((return_num = 
		     regexpmrphs_match(r_ptr+step,r_num-1,
				       d_ptr,d_num,
				       fw_or_bw,
				       all_or_part,
				       short_or_long)) != -1)
		    return return_num;
		else if (d_num &&
			regexpmrph_match(r_ptr, d_ptr) &&
			(return_num = 
			 regexpmrphs_match(r_ptr,r_num,
					   d_ptr+step,d_num-1,
					   fw_or_bw,
					   all_or_part,
					   short_or_long)) != -1)
		    return return_num;
		else 
		    return -1;
	    } else {
		if (d_num &&
		    regexpmrph_match(r_ptr, d_ptr) &&
		    (return_num = 
		     regexpmrphs_match(r_ptr,r_num,
				       d_ptr+step,d_num-1,
				       fw_or_bw,
				       all_or_part,
				       short_or_long)) != -1)
		    return return_num;
		else if ((return_num = 
			  regexpmrphs_match(r_ptr+step,r_num-1,
					    d_ptr,d_num,
					    fw_or_bw,
					    all_or_part,
					    short_or_long)) != -1)
		    return return_num;
		else 
		    return -1;
	    }		
        } else {
	    if (d_num &&
		regexpmrph_match(r_ptr,d_ptr) &&
		(return_num = 
		 regexpmrphs_match(r_ptr+step,r_num-1,
				   d_ptr+step,d_num-1,
				   fw_or_bw,
				   all_or_part,
				   short_or_long)) != -1)
		return return_num;
	    else 
		return -1;
	} 
    }
}

/*==================================================================*/
     int regexpmrphrule_match(MrphRule *r_ptr, MRPH_DATA *d_ptr)
/*==================================================================*/
{
    /* 
       pre_pattern  (shortest match でよい)
       self_pattern (longest match  がよい)
       post_pattern (shortest match でよい)
       
       まず，pre_patternを調べ，次にself_patternのlongest matchから
       順に，その後でpost_patternを調べる
    */

    int match_length, match_rest;

    /* まず，pre_patternを調べる */

    if (r_ptr->pre_pattern != NULL &&
	regexpmrphs_match(r_ptr->pre_pattern->mrph + 
			  r_ptr->pre_pattern->mrphsize - 1,
			  r_ptr->pre_pattern->mrphsize,
			  d_ptr - 1, 
			  d_ptr - mrph_data,
			  BW_MATCHING, 
			  PART_MATCHING, 
			  SHORT_MATCHING) == -1)
	return -1;

    
    /* 次にself_patternのlongest matchから順に，その後でpost_patternを調べる
       match_length は self_pattern の match の(可能性の)長さ */

    match_length = Mrph_num - (d_ptr - mrph_data);

    while (match_length > 0) {
	if (r_ptr->self_pattern == NULL) {
	    match_length = 1;	/* self_pattern がなければ
				   マッチの長さは1にしておく */
	}
	else if ((match_rest = 
		  regexpmrphs_match(r_ptr->self_pattern->mrph, 
				    r_ptr->self_pattern->mrphsize,
				    d_ptr,
				    match_length,
				    FW_MATCHING, 
				    PART_MATCHING,
				    LONG_MATCHING)) != -1) {
	    match_length -= match_rest;
	}
	else {
	    return -1;
	}

	if (r_ptr->post_pattern == NULL || 
	    regexpmrphs_match(r_ptr->post_pattern->mrph, 
			      r_ptr->post_pattern->mrphsize,
			      d_ptr + match_length,
			      Mrph_num - (d_ptr - mrph_data) - match_length,
			      FW_MATCHING, 
			      PART_MATCHING, 
			      SHORT_MATCHING) != -1) {
	    return match_length;
	}
	match_length --;
    }

    return -1;
}

/*==================================================================*/
     int _regexpmrphrule_match2(MrphRule *r_ptr, MRPH_DATA *d_ptr,
				int bw_length, int fw_length)
/*==================================================================*/
{
    /* regexpmrphrule_match2との違いはマッチする範囲が指定されて
       いること */

    /* 
       pre_pattern  (shortest match でよい)
       self_pattern (longest match  がよい)
       post_pattern (shortest match でよい)
       
       まず，pre_patternを調べ，次にself_patternのlongest matchから
       順に，その後でpost_patternを調べる
    */

    int match_length, match_rest;

    /* まず，pre_patternを調べる */

    if ((r_ptr->pre_pattern == NULL &&	/* 違い */
	 bw_length != 0) ||
	(r_ptr->pre_pattern != NULL &&
	 regexpmrphs_match(r_ptr->pre_pattern->mrph + 
			   r_ptr->pre_pattern->mrphsize - 1,
			   r_ptr->pre_pattern->mrphsize,
			   d_ptr - 1, 
			   bw_length,	/* 違い */
			   BW_MATCHING, 
			   ALL_MATCHING,/* 違い */
			   SHORT_MATCHING) == -1))
	return -1;

    
    /* 次にself_patternのlongest matchから順に，その後でpost_patternを調べる
       match_length は self_pattern の match の(可能性の)長さ */

    match_length = fw_length;		/* 違い */

    while (match_length > 0) {
	if (r_ptr->self_pattern == NULL) {
	    match_length = 1;	/* self_pattern がなければ
				   マッチの長さは1にしておく */
	}
	else if ((match_rest = 
		  regexpmrphs_match(r_ptr->self_pattern->mrph, 
				    r_ptr->self_pattern->mrphsize,
				    d_ptr,
				    match_length,
				    FW_MATCHING, 
				    PART_MATCHING,
				    LONG_MATCHING)) != -1) {
	    match_length -= match_rest;
	}
	else {
	    return -1;
	}

	if (r_ptr->post_pattern == NULL || 
	    regexpmrphs_match(r_ptr->post_pattern->mrph, 
			      r_ptr->post_pattern->mrphsize,
			      d_ptr + match_length,
			      fw_length - match_length,	/* 違い */
			      FW_MATCHING, 
			      ALL_MATCHING,		/* 違い */ 
			      SHORT_MATCHING) != -1) {
	    return match_length;
	}
	match_length --;
    }

    return -1;
}

/*==================================================================*/
        int regexpmrphrule_match2(MrphRule *r_ptr,
				  MRPH_DATA *d_ptr, int length,
				  int *start, int *end)
/*==================================================================*/
{
    /* 範囲の中で，各場所から前後のマッチング関数を呼び出す */

    int i, match_length;

    for (i = 0; i < length; i++) {
	if ((match_length = 
	     _regexpmrphrule_match2(r_ptr, d_ptr+i, i, length-i))
	    != -1) {
	    *start = i;
	    *end = i + match_length;
	    return 0;
	}
    }
    return -1;
}

/*==================================================================*/
      int _regexpbnst_match(REGEXPMRPHS *r_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    /* 将来はいらない */

    return regexpmrphs_match(r_ptr->mrph, r_ptr->mrphsize, 
			     b_ptr->mrph_ptr, b_ptr->mrph_num, 
			     FW_MATCHING, ALL_MATCHING, SHORT_MATCHING);
}

/*==================================================================*/
	 int regexpbnst_match(REGEXPBNST *ptr1, BNST_DATA *ptr2)
/*==================================================================*/
{
    /* 文節のマッチング */

    int ret_mrph;

    if (ptr1->type_flag == QST_FLG)
      return TRUE;
    else {
	if (regexpmrphs_match(ptr1->mrphs->mrph, ptr1->mrphs->mrphsize, 
			      ptr2->mrph_ptr, ptr2->mrph_num, 
			      FW_MATCHING, ALL_MATCHING, SHORT_MATCHING)
	    != -1) {
	    ret_mrph = TRUE;
	} else {
	    ret_mrph = FALSE;
	}	    

	if ((ptr1->type_flag == MAT_FLG && ret_mrph == TRUE) ||
	    (ptr1->type_flag == NOT_FLG && ret_mrph == FALSE))
	    return 
		feature_pattern_match(&(ptr1->f_pattern), ptr2->f,
				      NULL, ptr2);
	else
	    return FALSE;
    }
}

/*==================================================================*/
	   int regexpbnsts_match(REGEXPBNST *r_ptr,int r_num,
				 BNST_DATA *d_ptr, int d_num, 
				 int fw_or_bw, 
				 int all_or_part)
/*==================================================================*/
{
    int ret, step;

    (fw_or_bw == FW_MATCHING) ? (step = 1) : (step = -1);

    if (r_num == 0) {
	if (d_num == 0 || all_or_part == PART_MATCHING)
	    return TRUE;
	else 
	    return FALSE;
    } else {
        if (r_ptr->ast_flag == AST_FLG) {
	    
	    /* パターンに"condition*"がある場合 :
	       	パターンのみ進める(パターンの"*"をスキップ) or
		conditionがデータとマッチすればデータのみ進める */
	    
            if (regexpbnsts_match(r_ptr+step,r_num-1,d_ptr,d_num,
				  fw_or_bw,all_or_part))
		return TRUE;
	    else if (d_num &&
		     regexpbnst_match(r_ptr, d_ptr) &&
		     regexpbnsts_match(r_ptr,r_num,d_ptr+step,d_num-1, 
				      fw_or_bw,all_or_part))
		return TRUE;
            else 
		return FALSE;
        } else {
	    if (d_num &&
		regexpbnst_match(r_ptr,d_ptr) &&
		regexpbnsts_match(r_ptr+step,r_num-1,d_ptr+step,d_num-1,
				 fw_or_bw,all_or_part))
		return TRUE;
	    else 
		return FALSE;
	} 
    }
}

/*==================================================================*/
     int regexpbnstrule_match(BnstRule *r_ptr, BNST_DATA *d_ptr)
/*==================================================================*/
{
    if ((r_ptr->self_pattern == NULL ||
	 regexpbnst_match(r_ptr->self_pattern->bnst, d_ptr) == TRUE) &&
	(r_ptr->pre_pattern == NULL ||
	 regexpbnsts_match(r_ptr->pre_pattern->bnst + 
			   r_ptr->pre_pattern->bnstsize - 1,
			   r_ptr->pre_pattern->bnstsize,
			   d_ptr - 1, 
			   d_ptr - bnst_data,
			   BW_MATCHING, PART_MATCHING) == TRUE) &&
	(r_ptr->post_pattern == NULL ||
	 regexpbnsts_match(r_ptr->post_pattern->bnst, 
			   r_ptr->post_pattern->bnstsize,
			   d_ptr + 1,
			   Bnst_num - (d_ptr - bnst_data) - 1,
			   FW_MATCHING, PART_MATCHING) == TRUE)) {
	return TRUE;
    } else {
	return FALSE;
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

		      分類語彙表  検索プログラム

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	bgh_db;
int		BGHExist;

/*==================================================================*/
			   void init_bgh()
/*==================================================================*/
{
    char *filename;

    if (DICT[BGH_DB]) {
	filename = check_dict_filename(DICT[BGH_DB], TRUE);
    }
    else {
	filename = check_dict_filename(BGH_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((bgh_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	BGHExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open BGH dictionary <%s>.\n", filename);
#endif
    } else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	BGHExist = TRUE;
    }
    free(filename);

    /* 意味素 => 意味素コード */
    if (Thesaurus == USE_BGH) {
	if (DICT[SM2CODE_DB]) {
	    filename = check_dict_filename(DICT[SM2CODE_DB], TRUE);
	}
	else {
	    filename = check_dict_filename(SM2BGHCODE_DB_NAME, FALSE);
	}

	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... ", filename);
	}

	if ((sm2code_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	    if (OptDisplay == OPT_DEBUG) {
		fputs("failed.\n", Outfp);
	    }
	    SM2CODEExist = FALSE;
#ifdef DEBUG
	    fprintf(stderr, ";; Cannot open BGH sm dictionary <%s>.\n", filename);
#endif
	}
	else {
	    if (OptDisplay == OPT_DEBUG) {
		fputs("done.\n", Outfp);
	    }
	    SM2CODEExist = TRUE;
	}
	free(filename);
    }
}

/*==================================================================*/
                    void close_bgh()
/*==================================================================*/
{
    if (BGHExist == TRUE)
	DB_close(bgh_db);
}

/*==================================================================*/
		 char *_get_bgh(char *cp, char *arg)
/*==================================================================*/
{
    return db_get(bgh_db, cp);
}

/*==================================================================*/
	    char *meishi_setubi(BNST_DATA *ptr, char *cp)
/*==================================================================*/
{
    /* ▼▼ 使っていない */

    int i, flag = 0;		/* case_print で使用 */

    *cp = '\0';    
    for (i = 0; i < ptr->fuzoku_num; i++) {

	if (!strcmp(Class[(ptr->fuzoku_ptr + i)->Hinshi][0].id, "接尾辞") &&
	    !strcmp(Class[(ptr->fuzoku_ptr + i)->Hinshi]
		    [(ptr->fuzoku_ptr + i)->Bunrui].id, "名詞性名詞接尾辞")) {
	    strcat(cp, (ptr->fuzoku_ptr + i)->Goi);
	    flag = 1;
	}	
	else if (flag == 1)
	  break;
	else
	  strcat(cp, (ptr->fuzoku_ptr + i)->Goi);
    }    
    
    if (flag == 0)
      *cp = '\0';

    return cp;
}

/*==================================================================*/
		int bgh_code_match(char *c1, char *c2)
/*==================================================================*/
{
    int i, point = 0;

    /* 1桁目一致 -> 1,2,3,4,5,6-7,8-10桁目比較

       1桁目不一致 -> 1桁目が4(その他)以外なら 2〜4桁目比較 
       		      2桁目以降一致の場合 1桁目は一致とみなす */

    if (c1[0] == c2[0]) {
	point = 1;
	for (i = 1; c1[i] == c2[i] && i < BGH_CODE_SIZE; i++)
	    if (i != 5 && i != 7 && i != 8)
		point ++;
    }
    else if (c1[0] != '4' && c2[0] != '4' && c1[1] == c2[1]) {
	point = 2;
	for (i = 2; c1[i] == c2[i] && i < 4; i++)	
	    point ++;
    }	     

    return point;
}

/*==================================================================*/
	  int bgh_code_match_for_case(char *cp1, char *cp2)
/*==================================================================*/
{
    /* 例の分類語彙表コードのマッチング度の計算 */

    int match = 0;

    /* 単位の項目は無視 */
    if (!strncmp(cp1, "11960", 5) || !strncmp(cp2, "11960", 5))
	return 0;
    
    /* 比較 */
    match = bgh_code_match(cp1, cp2);

    /* 代名詞の項目は類似度を押さえる */
    if ((!strncmp(cp1, "12000", 5) || !strncmp(cp2, "12000", 5)) &&
	match > 3)
	return 3;
    
    return match;
}

/*==================================================================*/
		  int comp_bgh(char *cpp, char *cpd)
/*==================================================================*/
{
    int i;

    if (cpp[0] == cpd[0]) {
	for (i = 1; i < BGH_CODE_SIZE; i++) {
	    if (cpp[i] == '*') {
		return i;
	    }
	    else if (cpp[i] != cpd[i]) {
		return 0;
	    }
	}
    }
    else if (cpp[0] != '4' && cpd[0] != '4' && cpp[1] == cpd[1]) {
	for (i = 2; i < 4; i++) {
	    if (cpp[i] == '*') {
		return i;
	    }
	    else if (cpp[i] != cpd[i]) {
		return 0;
	    }
	}
    }	     
    return BGH_CODE_SIZE;
}

/*==================================================================*/
	     int bgh_match_check(char *pat, char *codes)
/*==================================================================*/
{
    int i;

    if (codes == NULL) {
	return FALSE;
    }

    for (i = 0; *(codes+i); i += BGH_CODE_SIZE) {
	if (comp_bgh(pat, codes+i) > 0) {
	    return TRUE;
	}
    }
    return FALSE;
}

/*====================================================================
                               END
====================================================================*/

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
	filename = (char *)check_dict_filename(DICT[BGH_DB], TRUE);
    }
    else {
	filename = (char *)check_dict_filename(BGH_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((bgh_db = DBM_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	BGHExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, "Cannot open BGH dictionary <%s>.\n", filename);
#endif
    } else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	BGHExist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
                    void close_bgh()
/*==================================================================*/
{
    if (BGHExist == TRUE)
	DBM_close(bgh_db);
}

/*==================================================================*/
                    char *get_bgh(char *cp)
/*==================================================================*/
{
    if (BGHExist == TRUE)
	return db_get(bgh_db, cp);
    else
	return NULL;
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
	void overflowed_function(char *str, int max, char *function)
/*==================================================================*/
{
    str[max-1] = '\0';
    fprintf(stderr, "Too long key <%s> in %s.\n", str, function);
    str[max-1] = GUARD;
}

/*==================================================================*/
		  void get_bgh_code(BNST_DATA *ptr)
/*==================================================================*/
{
    int strt, end, stop, i;
    char str_buffer[BNST_LENGTH_MAX], *code;
    char feature_buffer[BNST_LENGTH_MAX];

    /* 初期化 */
    *(ptr->BGH_code) = '\0';

    if (BGHExist == FALSE) return;

    /* 
       複合語の扱い
       		まず付属語を固定，自立語を減らしていく
		各形態素列に対してまず表記列で調べ，次に読み列で調べる
    */

    str_buffer[BNST_LENGTH_MAX-1] = GUARD;

    /* ptr->BGH_num はinit_bnstで0に初期化されている */

    for (stop = 0; stop < ptr->fuzoku_num; stop++) 
	if (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "判定詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助動詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "特殊") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "動詞") || /* 用言のときの付属語の動詞を排除 */
	    (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "接尾辞") &&
	     strcmp(Class[(ptr->fuzoku_ptr + stop)->Bunrui][0].id, "名詞性名詞接尾辞")))
	    break;

    end = ptr->settou_num + ptr->jiritu_num + stop;
    for (strt =0 ; strt < (ptr->settou_num + ptr->jiritu_num); strt++) {

	/* 表記のまま */

	*str_buffer = '\0';
	for (i = strt; i < end; i++) {
	    if (strlen(str_buffer)+strlen((ptr->mrph_ptr + i)->Goi2)+2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bgh_code");
		return;
	    }
	    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	}

	code = get_bgh(str_buffer);

	/* あるとき */
	if (code) {
	    if (strlen(code) > EX_ELEMENT_MAX*BGH_CODE_SIZE) {
		strncpy(ptr->BGH_code, code, EX_ELEMENT_MAX*BGH_CODE_SIZE);
		ptr->BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE] = '\0';
		fprintf(stderr, "Too many BGH code <%s>.\n", str_buffer);
	    }
	    strcpy(ptr->BGH_code, code);
	    free(code);
	}
	if (*(ptr->BGH_code)) goto Match;

	/* 表記，最後原形 */

	if (!str_eq((ptr->mrph_ptr + end - 1)->Goi,
		    (ptr->mrph_ptr + end - 1)->Goi2)) {
	    *str_buffer = '\0';
	    for (i = strt; i < end - 1; i++) {
		if (strlen(str_buffer)+strlen((ptr->mrph_ptr + i)->Goi2)+2 > BNST_LENGTH_MAX) {
		    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bgh_code");
		    return;
		}
		strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	    }

	    if (strlen(str_buffer)+strlen((ptr->mrph_ptr + end - 1)->Goi)+2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bgh_code");
		return;
	    }
	    strcat(str_buffer, (ptr->mrph_ptr + end - 1)->Goi);

	    /* ナ形容詞の場合は語幹で検索 */
	    if (str_eq(Class[(ptr->mrph_ptr + end - 1)->Hinshi][0].id,
		       "形容詞") &&
		(str_eq(Type[(ptr->mrph_ptr + end - 1)->Katuyou_Kata].name,
			"ナ形容詞") ||
		 str_eq(Type[(ptr->mrph_ptr + end - 1)->Katuyou_Kata].name,
			"ナ形容詞特殊") ||
		 str_eq(Type[(ptr->mrph_ptr + end - 1)->Katuyou_Kata].name,
			"ナノ形容詞"))) 
		str_buffer[strlen(str_buffer)-2] = NULL;

	    code = get_bgh(str_buffer);

	    if (code) {
		if (strlen(code) > EX_ELEMENT_MAX*BGH_CODE_SIZE) {
		    strncpy(ptr->BGH_code, code, EX_ELEMENT_MAX*BGH_CODE_SIZE);
		    ptr->BGH_code[EX_ELEMENT_MAX*BGH_CODE_SIZE] = '\0';
		    fprintf(stderr, "Too many BGH code <%s>.\n", str_buffer);
		}
		strcpy(ptr->BGH_code, code);
		free(code);
	    }
	    if (*(ptr->BGH_code)) goto Match;
	}
    }

  Match:
    ptr->BGH_num = strlen(ptr->BGH_code) / BGH_CODE_SIZE;

    if (ptr->BGH_num > 0) {
	sprintf(feature_buffer, "BGH:%s", str_buffer);
	assign_cfeature(&(ptr->f), feature_buffer);
    }
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

/*====================================================================
                               END
====================================================================*/

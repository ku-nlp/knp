/*====================================================================

		      意味素  検索プログラム

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	sm_db;
DBM_FILE	sm2code_db;
DBM_FILE	smp2smg_db;
int		SMExist;
int		SM2CODEExist;
int		SMP2SMGExist;

/*==================================================================*/
			    void init_sm()
/*==================================================================*/
{
    char *filename;

    /* 単語 <=> 意味素コード */
    if (DICT[SM_DB]) {
	filename = (char *)check_dict_filename(DICT[SM_DB], TRUE);
    }
    else {
	filename = (char *)check_dict_filename(SM_DB_NAME, FALSE);
    }

    if ((sm_db = DBM_open(filename, O_RDONLY, 0)) == NULL) {
	SMExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, "Cannot open NTT word dictionary <%s>.\n", filename);
#endif
    }
    else {
	SMExist = TRUE;
    }
    free(filename);

    /* 意味素 <=> 意味素コード */
    if (DICT[SM2CODE_DB]) {
	filename = (char *)check_dict_filename(DICT[SM2CODE_DB], TRUE);
    }
    else {
	filename = (char *)check_dict_filename(SM2CODE_DB_NAME, FALSE);
    }

    if ((sm2code_db = DBM_open(filename, O_RDONLY, 0)) == NULL) {
	SM2CODEExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, "Cannot open NTT sm dictionary <%s>.\n", filename);
#endif
    }
    else {
	SM2CODEExist = TRUE;
    }
    free(filename);

    /* 固有名詞体系 <=> 一般名詞体系 */
    if (DICT[SMP2SMG_DB]) {
	filename = (char *)check_dict_filename(DICT[SMP2SMG_DB], TRUE);
    }
    else {
	filename = (char *)check_dict_filename(SMP2SMG_DB_NAME, FALSE);
    }

    if ((smp2smg_db = DBM_open(filename, O_RDONLY, 0)) == NULL) {
	SMP2SMGExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, "Cannot open NTT smp smg table <%s>.\n", filename);
#endif
    }
    else {
	SMP2SMGExist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
			   void close_sm()
/*==================================================================*/
{
    if (SMExist == TRUE)
	DBM_close(sm_db);

    if (SM2CODEExist == TRUE)
	DBM_close(sm2code_db);

    if (SMP2SMGExist == TRUE)
	DBM_close(smp2smg_db);
}

/*==================================================================*/
			char *get_sm(char *cp)
/*==================================================================*/
{
    int i, pos, length;
    char *code;

    if (SMExist == FALSE) {
	fprintf(stderr, "Can not open Database <%s>.\n", SM_DB_NAME);
	exit(1);
    }

    code = db_get(sm_db, cp);

    if (code) {
	length = strlen(code);
	/* 溢れたら、縮める (撲滅対象) */
	if (length > SM_CODE_SIZE*SM_ELEMENT_MAX) {
#ifdef DEBUG
	    fprintf(stderr, "Too long SM content <%s>.\n", code);
#endif
	    code[SM_CODE_SIZE*SM_ELEMENT_MAX] = '\0';
	}

	pos = 0;
	/* 意味素を付与する品詞 */
	for (i = 0; code[i]; i+=SM_CODE_SIZE) {
	    if (code[i] == '3' ||	/* 名 */
		code[i] == '4' ||	/* 名(形式) */
		code[i] == '5' ||	/* 名(形動) */
		code[i] == '6' ||	/* 名(転生) */
		code[i] == '7' ||	/* サ変 */
		code[i] == '9' ||	/* 時詞 */
		code[i] == 'a' ||	/* 代名 */
		code[i] == '2') {	/* 固 */
		strncpy(code+pos, code+i, SM_CODE_SIZE);
		pos += SM_CODE_SIZE;
	    }
	}
	code[pos] = '\0';
    }
    return code;
}

/*==================================================================*/
             void get_sm_code(BNST_DATA *ptr)
/*==================================================================*/
{
    int strt, end, stop, i;
    char str_buffer[BNST_LENGTH_MAX], *code;
    char feature_buffer[SM_CODE_SIZE*SM_ELEMENT_MAX+1];

    /* 初期化 */
    *(ptr->SM_code) = '\0';

    if (SMExist == FALSE) return;

    /* 
       複合語の扱い
       		まず付属語を固定，自立語を減らしていく
		各形態素列に対してまず表記列で調べ，次に読み列で調べる
    */

    str_buffer[BNST_LENGTH_MAX-1] = GUARD;

    /* ptr->SM_num はinit_bnstで0に初期化されている */

    for (stop = 0; stop < ptr->fuzoku_num; stop++) 
	if (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "判定詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助動詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "特殊") ||
	    (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "接尾辞") &&
	    strcmp(Class[(ptr->fuzoku_ptr + stop)->Bunrui][0].id, "名詞性名詞接尾辞")))
	    break;

    end = ptr->settou_num + ptr->jiritu_num + stop;
    for (strt =0 ; strt < (ptr->settou_num + ptr->jiritu_num); strt++) {

	/* 表記のまま */

	*str_buffer = '\0';
	for (i = strt; i < end; i++) {
	    if (strlen(str_buffer)+strlen((ptr->mrph_ptr + i)->Goi2)+2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
		return;
	    }
	    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	}

	code = get_sm(str_buffer);

	if (code) {
	    strcpy(ptr->SM_code, code);
	    free(code);
	}
	if (*(ptr->SM_code)) goto Match;

	/* 表記，最後原形 */

	if (!str_eq((ptr->mrph_ptr + end - 1)->Goi,
		    (ptr->mrph_ptr + end - 1)->Goi2)) {
	    *str_buffer = '\0';
	    for (i = strt; i < end - 1; i++) {
		if (strlen(str_buffer)+strlen((ptr->mrph_ptr + i)->Goi2)+2 > BNST_LENGTH_MAX) {
		    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
		    return;
		}
		strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	    }

	    if (strlen(str_buffer)+strlen((ptr->mrph_ptr + end - 1)->Goi)+2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
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

	    code = get_sm(str_buffer);

	    if (code) {
		strcpy(ptr->SM_code, code);
		free(code);
	    }
	    if (*(ptr->SM_code)) goto Match;
	}
    }

  Match:
    ptr->SM_num = strlen(ptr->SM_code) / SM_CODE_SIZE;

    if (ptr->SM_num > 0) {
	sprintf(feature_buffer, "SM:%s:%s", str_buffer, ptr->SM_code);
	assign_cfeature(&(ptr->f), feature_buffer);
    }
}

/*==================================================================*/
		       char *sm2code(char *cp)
/*==================================================================*/
{
    char *code;

    /* sm と code は 1:1 対応 
       -> cont_str は溢れない */

    if (SM2CODEExist == FALSE) {
	cont_str[0] = '\0';
	return cont_str;
    }

    code = db_get(sm2code_db, cp);
    if (code) {
	strcpy(cont_str, code);
	free(code);
    }
    else {
	cont_str[0] = '\0';
    }
    return cont_str;
}

/*==================================================================*/
		       char *smp2smg(char *cp)
/*==================================================================*/
{
    char *code;

    /* 値は長くても 52 bytes ぐらい */

    if (SMP2SMGExist == FALSE) {
	cont_str[0] = '\0';
	return cont_str;
    }
    
    code = db_get(smp2smg_db, cp);
    if (code) {
	strcpy(cont_str, code);
	free(code);
    }
    else {
	cont_str[0] = '\0';
    }
    return cont_str;
}

/*==================================================================*/
		     int sm_code_depth(char *cp)
/*==================================================================*/
{
    int i;

    /* 意味素コードの深さを返す関数 (0 .. SM_CODE_SIZE-1) */

    for (i = 1; i < SM_CODE_SIZE; i++) {
	if (*(cp+i) == '*') {
	    return i-1;
	}
    }
    return SM_CODE_SIZE-1;
}

/*==================================================================*/
		float ntt_code_match(char *c1, char *c2)
/*==================================================================*/
{
    int i, d1, d2, min;

    if ((*c1 == '2' && *c2 != '2') || 
	(*c1 != '2' && *c2 == '2')) {
	return 0;
    }

    d1 = sm_code_depth(c1);
    d2 = sm_code_depth(c2);

    if (d1 + d2 == 0) {
	return 0;
    }

    min = d1 < d2 ? d1 : d2;

    if (min == 0) {
	return 0;
    }

    for (i = 1; i <= min; i++) {
	if (*(c1+i) != *(c2+i)) {
	    return (float)2*(i-1)/(d1+d2);
	}
    }
    return (float)2*min/(d1+d2);
}

/*==================================================================*/
		      int sm_time_match(char *c)
/*==================================================================*/
{
    char *p, flag = 0;

    /* 固有名詞のとき以外で、すべての意味属性が時間であれば TRUE */
    for (p = c;*p; p+=SM_CODE_SIZE) {
	/* 固有名詞のときをのぞく */
	if (*p == '2') {
	    continue;
	}
	/* 時間のコード */
	if (!comp_sm("1128********", p, 1)) {
	    return FALSE;
	}
	else if (!flag) {
	    flag = 1;
	}
    }

    if (flag) {
	return TRUE;
    }
    else {
	return FALSE;
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

		      シソーラス 検索プログラム

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include <knp.h>

/*==================================================================*/
	   char *get_str_code(unsigned char *cp, int flag)
/*==================================================================*/
{
    int i, exist;
    char *code;
    unsigned char *hira;
    char *(*get_code)();

    /* 文字列の意味素コードを取得 */

    if (flag == USE_NTT) {
	exist = SMExist;
	get_code = _get_sm;
    }
    else if (flag == USE_BGH) {
	exist = BGHExist;
	get_code = _get_bgh;
    }

    if (exist == FALSE) return NULL;

    if ((code = get_code(cp))) {
	return code;
    }

    /* 意味素がない場合で、
       すべての文字がカタカナの場合はひらがなに変換して辞書引き */

    for (i = 0; i < strlen(cp); i += 2) {
	if (*(cp+i) != 0xa5) {
	    return NULL;
	}
    }
    hira = katakana2hiragana(cp);
    code = get_code(hira);
    free(hira);
    return code;
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
	       void get_bnst_code(BNST_DATA *ptr, int flag)
/*==================================================================*/
{
    int strt, end, stop, i, jiritu;
    char str_buffer[BNST_LENGTH_MAX], *code;
    char *result_code;
    int *result_num, exist, code_unit;

    /* 文節の意味素コードを取得 */

    if (flag == USE_NTT) {
	result_code = ptr->SM_code;
	result_num = &ptr->SM_num;
	exist = SMExist;
	code_unit = SM_CODE_SIZE;
    }
    else if (flag == USE_BGH) {
	result_code = ptr->BGH_code;
	result_num = &ptr->BGH_num;
	exist = BGHExist;
	code_unit = BGH_CODE_SIZE;
    }

    /* 初期化 */
    *result_code = '\0';

    if (exist == FALSE) return;

    /* 
       複合語の扱い
       		まず付属語を固定，自立語を減らしていく
		各形態素列に対してまず表記列で調べ，次に読み列で調べる
    */

    str_buffer[BNST_LENGTH_MAX-1] = GUARD;

    /* result_num はinit_bnstで0に初期化されている */

    /* 分類語彙表の場合:
       「する」以外の付属語の動詞は削除する
       「結婚し始める」: 「始める」は削除し、「結婚する」で検索
       (分類語彙表ではサ変名詞は「する」付きで登録されている) */

    for (stop = 0; stop < ptr->fuzoku_num; stop++) {
	if (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助詞") || 
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "判定詞") || 
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助動詞") || 
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "特殊") || 
	    (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "動詞") && /* 用言のときの付属語の動詞を排除 */
	     (flag == USE_NTT || strcmp((ptr->fuzoku_ptr + stop)->Goi, "する"))) || 
	    (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "接尾辞") && 
	    strcmp(Class[(ptr->fuzoku_ptr + stop)->Bunrui][0].id, "名詞性名詞接尾辞")))
	    break;
    }

    /* 「側」など付属語的なものでは辞書を引かない */
    for (jiritu = 0; jiritu < ptr->jiritu_num; jiritu++) {
	if (check_feature((ptr->jiritu_ptr + jiritu)->f, "Ｔ固有末尾") && 
	    jiritu != 0) {
	    stop = 0;
	    break;
	}
    }

    end = ptr->settou_num + jiritu + stop;
    for (strt =0 ; strt < (ptr->settou_num + jiritu); strt++) {

	/* 表記のまま */

	*str_buffer = '\0';
	for (i = strt; i < end; i++) {
	    if (strlen(str_buffer)+strlen((ptr->mrph_ptr + i)->Goi2)+2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bnst_code");
		return;
	    }
	    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	}

	code = get_str_code(str_buffer, flag);

	if (code) {
	    strcpy(result_code, code);
	    free(code);
	}
	else {
	    /* 「お鍋」など一形態素になっている場合があるので
	       "お" をとってみる => 形態素マッチ */
	    if (!strncmp(str_buffer, "お", 2)) {
		code = get_str_code(str_buffer+2, flag);
		if (code) {
		    strcpy(result_code, code);
		    free(code);
		}
	    }
	}
	if (*result_code) goto Match;

	/* 表記，最後原形 */

	if (!str_eq((ptr->mrph_ptr + end - 1)->Goi,
		    (ptr->mrph_ptr + end - 1)->Goi2)) {
	    *str_buffer = '\0';
	    for (i = strt; i < end - 1; i++) {
		if (strlen(str_buffer)+strlen((ptr->mrph_ptr + i)->Goi2)+2 > BNST_LENGTH_MAX) {
		    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bnst_code");
		    return;
		}
		strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	    }

	    if (strlen(str_buffer)+strlen((ptr->mrph_ptr + end - 1)->Goi)+2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bnst_code");
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
		str_buffer[strlen(str_buffer)-2] = '\0';

	    code = get_str_code(str_buffer, flag);

	    if (code) {
		strcpy(result_code, code);
		free(code);
	    }
	    if (*result_code) goto Match;
	}
    }

  Match:
    *result_num = strlen(result_code) / code_unit;

    if (*result_num > 0) {
	if (flag == USE_NTT) {
	    char feature_buffer[BNST_LENGTH_MAX+SM_CODE_SIZE*SM_ELEMENT_MAX+4];
	    sprintf(feature_buffer, "SM:%s:%s", str_buffer, result_code);
	    assign_cfeature(&(ptr->f), feature_buffer);
	}
	else if (flag == USE_BGH) {
	    char feature_buffer[BNST_LENGTH_MAX+4];
	    sprintf(feature_buffer, "BGH:%s", str_buffer);
	    assign_cfeature(&(ptr->f), feature_buffer);
	}
    }
}

/*==================================================================*/
	      int DeleteSpecifiedSM(char *sm, char *del)
/*==================================================================*/
{
    int i, j, flag, pos = 0;
    if (Thesaurus == USE_BGH) return 0;

    for (i = 0; sm[i]; i += SM_CODE_SIZE) {
	flag = 1;
	/* 固有ではないときを対象とする */
	if (sm[i] != '2') {
	    for (j = 0; del[j]; j += SM_CODE_SIZE) {
		if (!strncmp(sm+i+1, del+j+1, SM_CODE_SIZE-1)) {
		    flag = 0;
		    break;
		}
	    }
	}
	if (flag) {
	    strncpy(sm+pos, sm+i, SM_CODE_SIZE);
	    pos += SM_CODE_SIZE;
	}
    }
    *(sm+pos) = '\0';
    return 1;
}

/*==================================================================*/
	       int DeleteMatchedSM(char *sm, char *del)
/*==================================================================*/
{
    int i, j, flag, pos = 0;
    if (Thesaurus == USE_BGH) return 0;

    for (i = 0; sm[i]; i += SM_CODE_SIZE) {
	flag = 1;
	/* 固有ではないときチェック */
	if (sm[i] != '2') {
	    for (j = 0; del[j]; j += SM_CODE_SIZE) {
		if (_sm_match_score(sm+i, del+j, SM_NO_EXPAND_NE) > 0) {
		    flag = 0;
		    break;
		}
	    }
	}
	if (flag) {
	    strncpy(sm+pos, sm+i, SM_CODE_SIZE);
	    pos += SM_CODE_SIZE;
	}
    }
    *(sm+pos) = '\0';
    return 1;
}

/*==================================================================*/
	float CalcSimilarity(char *exd, char *exp, int expand)
/*==================================================================*/
{
    int i, j, step;
    float score = 0, tempscore;

    /* 類似度計算: 意味素 - 意味素 */

    /* どちらかに用例のコードがないとき */
    if (!(exd && exp && *exd && *exp)) {
	return score;
    }

    if (Thesaurus == USE_BGH) {
	step = BGH_CODE_SIZE;
    }
    else if (Thesaurus == USE_NTT) {
	step = SM_CODE_SIZE;
	if (expand != SM_NO_EXPAND_NE) {
	    expand = SM_EXPAND_NE_DATA;
	}
    }

    /* 最大マッチスコアを求める */
    for (j = 0; exp[j]; j+=step) {
	for (i = 0; exd[i]; i+=step) {
	    if (Thesaurus == USE_BGH) {
		tempscore = (float)_ex_match_score(exp+j, exd+i);
	    }
	    else if (Thesaurus == USE_NTT) {
		tempscore = ntt_code_match(exp+j, exd+i, expand);
	    }
	    if (tempscore > score) {
		score = tempscore;
	    }
	}
    }

    /* スコアの幅に注意
       NTT: 0 〜 1.0
       BGH: 0 〜 7 */

    return score;
}

/*==================================================================*/
	    float CalcWordSimilarity(char *exd, char *exp)
/*==================================================================*/
{
    char *smd, *smp;
    float score = 0;

    /* 類似度計算: 単語 - 単語 */

    smd = get_str_code(exd, Thesaurus);
    smp = get_str_code(exp, Thesaurus);

    if (smd && smp) {
	score = CalcSimilarity(smd, smp, 0);
    }

    if (smd) {
	free(smd);
    }
    if (smp) {
	free(smp);
    }
    return score;
}

/*==================================================================*/
float CalcSmWordSimilarity(char *smd, char *exp, char *del, int expand)
/*==================================================================*/
{
    char *smp;
    float score = 0;

    /* 類似度計算: 意味素 - 単語 */

    if ((smp = get_str_code(exp, Thesaurus)) == NULL) {
	return 0;
    }

    if (del) {
	DeleteSpecifiedSM(smp, del);
    }

    if (smd && smp[0]) {
	score = CalcSimilarity(smd, smp, expand);
    }

    free(smp);
    return score;
}

/*==================================================================*/
 float CalcWordsSimilarity(char *exd, char **exp, int num, int *pos)
/*==================================================================*/
{
    int i;
    float maxscore = 0, score;

    /* 類似度計算: 単語 - 単語群 */

    for (i = 0; i < num; i++) {
	score = CalcWordSimilarity(exd, *(exp+i));
	if (maxscore < score) {
	    maxscore = score;
	    *pos = i;
	}
    }

    return maxscore;
}

/*==================================================================*/
float CalcSmWordsSimilarity(char *smd, char **exp, int num, int *pos, char *del, int expand)
/*==================================================================*/
{
    int i;
    float maxscore = 0, score;

    /* 類似度計算: 意味素 - 単語群 */

    for (i = 0; i < num; i++) {
	score = CalcSmWordSimilarity(smd, *(exp+i), del, expand);
	if (maxscore < score) {
	    maxscore = score;
	    *pos = i;
	}
    }

    return maxscore;
}

/*====================================================================
                               END
====================================================================*/

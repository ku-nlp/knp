/*====================================================================

		      シソーラス 検索プログラム

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int 	Thesaurus = USE_NTT;
int	ParaThesaurus = USE_BGH;

/*==================================================================*/
			void init_thesaurus()
/*==================================================================*/
{
    int i;
    char *filename;

    /* tentative: 新しいシソーラスはNTTと排他的 */
    if (Thesaurus != USE_BGH && Thesaurus != USE_NTT && 
	ParaThesaurus == USE_NTT) {
	ParaThesaurus = Thesaurus;
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Thesaurus for para analysis is forced to %s.\n", THESAURUS[ParaThesaurus].name);
	}
	
    }
    else if (ParaThesaurus != USE_BGH && ParaThesaurus != USE_NTT && 
	     Thesaurus == USE_NTT) {
	Thesaurus = ParaThesaurus;
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Thesaurus for case analysis is forced to %s.\n", THESAURUS[Thesaurus].name);
	}
    }

    if (Thesaurus == USE_BGH || ParaThesaurus == USE_BGH) {
	init_bgh();
    }

    if (Thesaurus == USE_NTT || ParaThesaurus == USE_NTT) {
	init_ntt();
    }

    for (i = 0; i < THESAURUS_MAX; i++) {
	if (i == USE_BGH || i == USE_NTT || THESAURUS[i].path == NULL) {
	    continue;
	}
	filename = check_dict_filename(THESAURUS[i].path, TRUE);

	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... ", filename);
	}

	if ((THESAURUS[i].db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	    if (OptDisplay == OPT_DEBUG) {
		fputs("failed.\n", Outfp);
	    }
	    THESAURUS[i].exist = FALSE;
	}
	else {
	    if (OptDisplay == OPT_DEBUG) {
		fputs("done.\n", Outfp);
	    }
	    THESAURUS[i].exist = TRUE;
	}
	free(filename);
    }
}

/*==================================================================*/
			void close_thesaurus()
/*==================================================================*/
{
    int i;

    if (Thesaurus == USE_BGH || ParaThesaurus == USE_BGH) {
	close_bgh();
    }

    if (Thesaurus == USE_NTT || ParaThesaurus == USE_NTT) {
	close_ntt();
    }

    for (i = 0; i < THESAURUS_MAX; i++) {
	if (i == USE_BGH || i == USE_NTT || THESAURUS[i].exist == FALSE) {
	    continue;
	}
	DB_close(THESAURUS[i].db);
    }
}

/*==================================================================*/
	     char *get_code(char *cp, char *arg, int th)
/*==================================================================*/
{
    if (th == USE_NTT) {
	return _get_ntt(cp, arg);
    }
    else if (th == USE_BGH) {
	return _get_bgh(cp, arg);
    }
    return db_get(THESAURUS[th].db, cp);
}

/*==================================================================*/
	   char *get_str_code(unsigned char *cp, int flag)
/*==================================================================*/
{
    int i, th;
    char *code, arg = '\0';
    unsigned char *hira;

    /* 文字列の意味素コードを取得 */

    if (flag & USE_NTT) {
	if (code = check_noun_sm(cp)) {
	    return code;
	}

	th = USE_NTT;
	if (flag & USE_SUFFIX_SM) {
	    arg = 'm';
	}
	else if (flag & USE_PREFIX_SM) {
	    arg = 'l';
	}
    }
    else if (flag & USE_BGH) {
	th = USE_BGH;
    }
    else {
	th = flag;
    }

    if (THESAURUS[th].exist == FALSE) return NULL;

    if ((code = get_code(cp, &arg, th))) {
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
    code = get_code(hira, &arg, th);
    free(hira);
    return code;
}

/*==================================================================*/
	void overflowed_function(char *str, int max, char *function)
/*==================================================================*/
{
    str[max-1] = '\0';
    fprintf(stderr, ";; Too long key <%s> in %s.\n", str, function);
    str[max-1] = GUARD;
}

/*==================================================================*/
		void get_bnst_code_all(BNST_DATA *ptr)
/*==================================================================*/
{
    int i;

    for (i = 0; i < THESAURUS_MAX; i++) {
	get_bnst_code(ptr, i);
    }
}

/*==================================================================*/
	     void get_bnst_code(BNST_DATA *ptr, int flag)
/*==================================================================*/
{
    int strt, end, i, lookup_pos = 0;
    char str_buffer[BNST_LENGTH_MAX], *code;
    char *result_code;
    int *result_num, exist, code_unit;

    /* 文節の意味素コードを取得 */

    if (flag == USE_BGH) {
	result_code = ptr->BGH_code;
	result_num = &ptr->BGH_num;
    }
    else {
	result_code = ptr->SM_code;
	result_num = &ptr->SM_num;
    }
    exist = THESAURUS[flag].exist;
    code_unit = THESAURUS[flag].code_size;

    if (exist == FALSE) return;

    /* 初期化 */
    *result_code = '\0';

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

    if (flag == USE_BGH && 
	ptr->mrph_ptr + ptr->mrph_num - 1 > ptr->head_ptr && 
	!strcmp(Class[(ptr->head_ptr + 1)->Hinshi][0].id, "動詞") && 
	!strcmp((ptr->head_ptr + 1)->Goi, "する")) {
	end = ptr->head_ptr - ptr->mrph_ptr + 1;
    }
    else {
	end = ptr->head_ptr - ptr->mrph_ptr;
    }

    /* NTT: カウンタのみで引く */
    if (flag == USE_NTT && 
	check_feature((ptr->head_ptr)->f, "カウンタ")) {
	lookup_pos = USE_SUFFIX_SM;
	strt = end;
    }
    else {
	strt = 0;
    }

    for (; strt <= end; strt++) {

	/* 表記のまま *
	*str_buffer = '\0';
	for (i = strt; i <= end; i++) {
	    if (strlen(str_buffer) + strlen((ptr->mrph_ptr + i)->Goi2) + 2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bnst_code");
		return;
	    }
	    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	}

	code = get_str_code(str_buffer, flag | lookup_pos);

	if (code) {
	    strcpy(result_code, code);
	    free(code);
	}

	if (*result_code) goto Match;
	*/

	/* 表記，最後原形 */

	*str_buffer = '\0';
	for (i = strt; i < end; i++) {
	    if (strlen(str_buffer) + strlen((ptr->mrph_ptr + i)->Goi2) + 2 > BNST_LENGTH_MAX) {
		overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bnst_code");
		return;
	    }
	    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
	}

	if (strlen(str_buffer) + strlen((ptr->mrph_ptr + end)->Goi) + 2 > BNST_LENGTH_MAX) {
	    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_bnst_code");
	    return;
	}
	strcat(str_buffer, (ptr->mrph_ptr + end)->Goi);

	/* ナ形容詞の場合は語幹で検索 */
	if (str_eq(Class[(ptr->mrph_ptr + end)->Hinshi][0].id, 
		   "形容詞") && 
	    (str_eq(Type[(ptr->mrph_ptr + end)->Katuyou_Kata].name, 
		    "ナ形容詞") || 
	     str_eq(Type[(ptr->mrph_ptr + end)->Katuyou_Kata].name, 
		    "ナ形容詞特殊") || 
	     str_eq(Type[(ptr->mrph_ptr + end)->Katuyou_Kata].name, 
		    "ナノ形容詞")))
	    str_buffer[strlen(str_buffer) - 2] = '\0';

	/* 「共通する」がなく「する」だけになるような場合はskip */
	if (end > 0 && str_eq(str_buffer, "する")) return;

	code = get_str_code(str_buffer, flag | lookup_pos);

	if (code) {
	    strcpy(result_code, code);
	    free(code);
	}
	if (*result_code) goto Match;
    }

  Match:
    *result_num = strlen(result_code) / code_unit;

    if (*result_num > 0) {
	char feature_buffer[BNST_LENGTH_MAX + 4];

	if (flag == USE_BGH) {
	    sprintf(feature_buffer, "BGH:%s", str_buffer);
	    assign_cfeature(&(ptr->f), feature_buffer);
	}
	else {
	    sprintf(feature_buffer, "SM:%s", str_buffer);
	    assign_cfeature(&(ptr->f), feature_buffer);
	}

    }
}

/*==================================================================*/
	       int code_depth(char *cp, int code_size)
/*==================================================================*/
{
    int i;

    /* 意味素コードの深さを返す関数 (0 .. code_size-1) */

    for (i = 1; i < code_size; i++) {
	if (*(cp + i) == '*') {
	    return i - 1;
	}
    }
    return code_size - 1;
}

/*==================================================================*/
   float general_code_match(THESAURUS_FILE *th, char *c1, char *c2)
/*==================================================================*/
{
    int i, d1, d2, min, l;

    d1 = code_depth(c1, th->code_size);
    d2 = code_depth(c2, th->code_size);

    if (d1 + d2 == 0) {
	return 0;
    }

    min = d1 < d2 ? d1 : d2;

    if (min == 0) {
	return 0;
    }

    l = 0;
    for (i = 0; th->format[i]; i++) { /* 指定された桁数ごとにチェック */
	if (strncmp(c1 + l, c2 + l, th->format[i])) {
	    return (float) 2 * l / (d1 + d2);
	}
	l += th->format[i];
    }
    return (float) 2 * min / (d1 + d2);
}

/*==================================================================*/
       float calc_similarity(char *exd, char *exp, int expand)
/*==================================================================*/
{
    int i, j, step;
    float score = 0, tempscore;

    /* 類似度計算: 意味素 - 意味素 */

    /* どちらかに用例のコードがないとき */
    if (!(exd && exp && *exd && *exp)) {
	return score;
    }

    if (Thesaurus == USE_NONE) {
	return score;
    }
    else if (Thesaurus == USE_NTT) {
	if (expand != SM_NO_EXPAND_NE) {
	    expand = SM_EXPAND_NE_DATA;
	}
    }

    step = THESAURUS[Thesaurus].code_size;

    /* 最大マッチスコアを求める */
    for (j = 0; exp[j]; j+=step) {
	for (i = 0; exd[i]; i+=step) {
	    if (Thesaurus == USE_BGH) {
		tempscore = (float)bgh_code_match_for_case(exp+j, exd+i);
	    }
	    else if (Thesaurus == USE_NTT) {
		tempscore = ntt_code_match(exp+j, exd+i, expand);
	    }
	    else {
		tempscore = general_code_match(&THESAURUS[Thesaurus], exp+j, exd+i);
	    }
	    if (tempscore > score) {
		score = tempscore;
	    }
	}
    }

    /* スコアの幅に注意
       NTT: 0 〜 1.0
       BGH: 0 〜 7 */
    if (Thesaurus == USE_BGH) {
	score /= 7;
    }

    /* スコア: 0 〜 1.0 */
    return score;
}

/*==================================================================*/
	  char *get_most_similar_code(char *exd, char *exp)
/*==================================================================*/
{
    int i, j, step, ret_sm_num = 0, pre_i = -1;
    float score = 0, tempscore;
    char *ret_sm;

    /* どちらかに用例のコードがないとき */
    if (!(exd && exp && *exd && *exp)) {
	return NULL;
    }

    if (Thesaurus == USE_NONE) {
	return NULL;
    }

    step = THESAURUS[Thesaurus].code_size;

    ret_sm = (char *)malloc_data(sizeof(char)*strlen(exd)+1, "get_most_similar_code");
    *ret_sm = '\0';

    /* 最大マッチスコアを求める */
    for (i = 0; exd[i]; i+=step) {
	for (j = 0; exp[j]; j+=step) {
	    if (Thesaurus == USE_BGH) {
		tempscore = (float)bgh_code_match_for_case(exp+j, exd+i);
	    }
	    else if (Thesaurus == USE_NTT) {
		tempscore = ntt_code_match(exp+j, exd+i, SM_NO_EXPAND_NE);
	    }
	    else {
		tempscore = general_code_match(&THESAURUS[Thesaurus], exp+j, exd+i);
	    }
	    if (tempscore > score) {
		score = tempscore;
		strncpy(ret_sm, exd+i, step);
		ret_sm_num = 1;
		ret_sm[step] = '\0';
		pre_i = i;
	    }
	    else if (tempscore == score && 
		     pre_i != i) { /* 重複を避けるため直前のiとは違うときのみ */
		strncat(ret_sm, exd+i, step);
		ret_sm_num++;
		pre_i = i;
	    }
	}
    }

    return ret_sm;
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
	score = calc_similarity(smd, smp, 0);
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

    /* NTTで野菜となるので，とりあえずこれだけ削除 03/03/27 by kuro */
    if (!strcmp(exp, "ところ")) return 0;

    if ((smp = get_str_code(exp, Thesaurus)) == NULL) {
	return 0;
    }

    if (Thesaurus == USE_NTT && del) {
	delete_specified_sm(smp, del);
    }

    if (smd && smp[0]) {
	score = calc_similarity(smd, smp, expand);
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

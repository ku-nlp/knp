/*====================================================================

		      シソーラス 検索プログラム

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int 	Thesaurus = USE_BGH;
int	ParaThesaurus = USE_BGH;

/*==================================================================*/
			void init_thesaurus()
/*==================================================================*/
{
    int i;
    char *filename;

    /* tentative: 新しいシソーラスはNTTと排他的 */
    if (Thesaurus != USE_NONE && Thesaurus != USE_BGH && Thesaurus != USE_NTT && 
	ParaThesaurus == USE_NTT) {
	ParaThesaurus = Thesaurus;
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Thesaurus for para analysis is forced to %s.\n", THESAURUS[ParaThesaurus].name);
	}
	
    }
    else if (ParaThesaurus != USE_NONE && ParaThesaurus != USE_BGH && ParaThesaurus != USE_NTT && 
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

    if (flag & USE_RN) return NULL;

    /* 意味素がない場合で、
       すべての文字がカタカナの場合はひらがなに変換して辞書引き */

    for (i = 0; i < strlen(cp); i += BYTES4CHAR) { /* euc-jp */
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
       char *get_str_code_with_len(char *cp, int len, int flag)
/*==================================================================*/
{
    char bak_char = cp[len];
    char *code;

    cp[len] = '\0';
    code = get_str_code(cp, flag);
    cp[len] = bak_char;

    return code;
}

/*==================================================================*/
	void overflowed_function(char *str, int max, char *function)
/*==================================================================*/
{
    str[max-1] = '\0';
    if (OptDisplay == OPT_DEBUG) 
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
int add_rep_str(MRPH_DATA *ptr, char *str_buffer, int org_flag, int flag)
/*==================================================================*/
{
    char *rep_strt, *rep_end;
    int add_len;

    rep_strt = get_mrph_rep(ptr);
    if (rep_strt) {
	if (flag & USE_RN) {
	    if ((rep_end = strchr(rep_strt, ' ')) == NULL) {
		rep_end = strchr(rep_strt, '\"');
	    }
	}
	else {
	    rep_end = strchr(rep_strt, '/');
	}
    }
    if (rep_strt && rep_end) {
	add_len = rep_end - rep_strt;
	if (strlen(str_buffer) + add_len + 2 > BNST_LENGTH_MAX) {
	    overflowed_function(str_buffer, BNST_LENGTH_MAX, "add_rep_str");
	    return 0;
	}
	/* org_flag == 0 のときは活用させる必要がある */
	strncat(str_buffer, rep_strt, add_len);
    }
    else {
	add_len = strlen(org_flag ? ptr->Goi : ptr->Goi2);
	if (strlen(str_buffer) + add_len + 2 > BNST_LENGTH_MAX) {
	    overflowed_function(str_buffer, BNST_LENGTH_MAX, "add_rep_str");
	    return 0;
	}
	if (org_flag) {
	    /* ナ形容詞の場合は語幹で検索 */
	    if (Language == JAPANESE && 
		str_eq(Class[ptr->Hinshi][0].id, "形容詞") && 
		(str_eq(Type[ptr->Katuyou_Kata].name, "ナ形容詞") || 
		 str_eq(Type[ptr->Katuyou_Kata].name, "ナ形容詞特殊") || 
		 str_eq(Type[ptr->Katuyou_Kata].name, "ナノ形容詞"))) {
		add_len -= 2;
	    }
	    strncat(str_buffer, ptr->Goi, add_len); /* 原形 */
	}
	else {
	    strcat(str_buffer, ptr->Goi2); /* 表記 */
	}
    }

    return add_len;
}

/*==================================================================*/
int bgh_code_match_for_sm(char *result_code, char *sm)
/*==================================================================*/
{
    int i;
    char *code;

    if ((code = db_get(sm2code_db, sm))) {

	for (i = 0; *(code + i); i += BGH_CODE_SIZE) {
	    if (_cf_match_element(result_code, code + i, 0, code_depth(code + i, BGH_CODE_SIZE) + 1)) {
		strcat(result_code, sm);
		free(code);		
		return 1;
	    }
	}
	free(code);
    }
    return 0;
}

/*==================================================================*/
void make_key_and_get_code(BNST_DATA *ptr, int strt, int end, 
			   char *str_buffer, char *ret_buffer, char *used_key, int flag)
/*==================================================================*/
{
    FEATURE **fpp = &((ptr->mrph_ptr + end)->f);
    MRPH_DATA m;
    char *buf;
    char last_key[BNST_LENGTH_MAX];
    int add_len;

    /* 末尾まで文字列を作り出し終わり、DBを引く */
    if (strt > end) {
	char *code;

	/* 「サ変+する」がなく「する」だけになるような場合はskip */
	if (end > 0 && str_eq(str_buffer, "する")) {
	    return;
	}

	if (str_buffer[strlen(str_buffer)-1] == '+') {
	    str_buffer[strlen(str_buffer)-1] = '\0';
	}
	if (code = get_str_code(str_buffer, flag)) { /* DBをひく */
	    strcat(ret_buffer, code);
	    free(code);

	    if (strlen(used_key) + strlen(str_buffer) + 3 > BNST_LENGTH_MAX) {
		overflowed_function(used_key, BNST_LENGTH_MAX, "make_key_and_get_code");
		return;
	    }
	    if (*used_key) {
		strcat(used_key, "|");
	    }
	    strcat(used_key, str_buffer);
	}

	return;
    }
    /* 複合名詞の前の部分 (表記のみ, 代表表記やALTは用いていない) */
    else if (strt < end) {
	if (flag & USE_RN) {
	    buf = get_mrph_rep_from_f(ptr->mrph_ptr + strt, FALSE);
	    if (!buf) buf = (ptr->mrph_ptr + strt)->Goi2;
	}
	else {
	    buf = (ptr->mrph_ptr + strt)->Goi2;
	}
	if (strlen(str_buffer) + strlen(buf) + 3 > BNST_LENGTH_MAX) {
	    overflowed_function(str_buffer, BNST_LENGTH_MAX, "make_key_and_get_code");
	    return;
	}
	strcat(str_buffer, buf); /* 表記 */
	if (flag & USE_RN) {
	    strcat(str_buffer, "+");
	}
	make_key_and_get_code(ptr, strt + 1, end, str_buffer, ret_buffer, used_key, flag);
	str_buffer[strlen(str_buffer) - strlen(buf)] = '\0';
	return;
    }

    /* strt == end => 最後原形 */
    if ((add_len = add_rep_str(ptr->mrph_ptr + end, str_buffer, TRUE, flag)) == 0) { /* 代表表記 */
	return;
    }
    make_key_and_get_code(ptr, strt + 1, end, str_buffer, ret_buffer, used_key, flag);
    strcpy(last_key, str_buffer);
    str_buffer[strlen(str_buffer) - add_len] = '\0';

    /* ALTの代表表記 */
    while (*fpp) {
	if (!strncmp((*fpp)->cp, "ALT-", 4)) {
	    sscanf((*fpp)->cp + 4, "%[^-]-%[^-]-%[^-]-%d-%d-%d-%d-%[^\n]", 
		   m.Goi2, m.Yomi, m.Goi, 
		   &m.Hinshi, &m.Bunrui, 
		   &m.Katuyou_Kata, &m.Katuyou_Kei, m.Imi);
	    if ((add_len = add_rep_str(&m, str_buffer, TRUE, flag)) == 0) { /* 代表表記 */
		return;
	    }
	    if (strcmp(last_key, str_buffer)) { /* 異なる場合のみ調べる */
		make_key_and_get_code(ptr, strt + 1, end, str_buffer, ret_buffer, used_key, flag);
		str_buffer[strlen(str_buffer) - add_len] = '\0';
	    }
	}
	fpp = &((*fpp)->next);
    }

    /* 「幸運を得る」の「幸運」を引くためには、
       「幸運/こううんa」ではなく、もとの「幸運だ/こううんだ」で引く必要がある */
    if (!str_buffer[0] && (buf = check_feature((ptr->mrph_ptr + end)->f, "代表表記変更"))) { /* 最後の要素 */
	strcpy(str_buffer, buf + strlen("代表表記変更:"));
	if (strcmp(last_key, str_buffer)) { /* 異なる場合のみ調べる */
	    make_key_and_get_code(ptr, strt + 1, end, str_buffer, ret_buffer, used_key, flag);
	    str_buffer[0] = '\0';
	}
    }

    return;
}

/*==================================================================*/
	     void get_bnst_code(BNST_DATA *ptr, int flag)
/*==================================================================*/
{
    /* 文節の意味素コードを取得

       複合語の扱い
       		まず付属語を固定，自立語を減らしていく
		各形態素列に対して表記列で調べる

       分類語彙表の場合:
       「する」以外の付属語の動詞は削除する
       「結婚し始める」: 「始める」は削除し、「結婚する」で検索
       (分類語彙表ではサ変名詞は「する」付きで登録されている)
    */

    int strt, end, i, lookup_pos = 0;
    char str_buffer[BNST_LENGTH_MAX], used_key[BNST_LENGTH_MAX], *code, *result_code;
    int *result_num, exist, code_unit;

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

    if (exist == FALSE) {
	return;
    }

    /* 初期化 */
    *result_code = '\0';
    used_key[0] = '\0';
    str_buffer[BNST_LENGTH_MAX-1] = GUARD;
    /* result_num はinit_bnstで0に初期化されている */

    if (flag == USE_BGH && /* 分類語彙表ではサ変名詞は「する」付きで登録されている */
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
	if (ptr->type == IS_TAG_DATA) {
	    strt = ((TAG_DATA *)ptr)->settou_num;
	}
	else {
	    strt = 0;
	}
    }

    /* もっとも長いものから順に試す */
    for (; strt <= end; strt++) {
	str_buffer[0] = '\0';
	make_key_and_get_code(ptr, strt, end, str_buffer, result_code, used_key, 
			      flag | lookup_pos | OptUseRN);
	if (*result_code) {
	    if (flag == USE_BGH && !strstr(result_code, "sm")) {
		if (bgh_code_match_for_sm(result_code, "sm-sub*****"))
		    assign_cfeature(&(ptr->f), "SM-主体", FALSE);
		if (bgh_code_match_for_sm(result_code, "sm-act*****"))
		    assign_cfeature(&(ptr->f), "SM-動作", FALSE);
		if (bgh_code_match_for_sm(result_code, "sm-per*****")) 
		    assign_cfeature(&(ptr->f), "SM-人", FALSE);
		if (bgh_code_match_for_sm(result_code, "sm-loc*****"))
		    assign_cfeature(&(ptr->f), "SM-場所", FALSE);
		if (bgh_code_match_for_sm(result_code, "sm-org*****"))
		    assign_cfeature(&(ptr->f), "SM-組織", FALSE);
	    }
	    break;
	}
    }

    if (*result_code) {
	char feature_buffer[BNST_LENGTH_MAX + 4];

	*result_num = strlen(result_code) / code_unit;

	if (flag == USE_BGH) {
	    sprintf(feature_buffer, "BGH:%s", used_key);
	}
	else {
	    sprintf(feature_buffer, "NTT:%s", used_key);
	}
	assign_cfeature(&(ptr->f), feature_buffer, FALSE);
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

    min = Min(d1, d2);

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
    int i, j, code_size;
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

    code_size = THESAURUS[Thesaurus].code_size;

    /* 最大マッチスコアを求める */
    for (j = 0; exp[j]; j+=code_size) {
	for (i = 0; exd[i]; i+=code_size) {
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
    int i, j, code_size, ret_sm_num = 0, pre_i = -1;
    float score = 0, tempscore;
    char *ret_sm;

    /* どちらかに用例のコードがないとき */
    if (!(exd && exp && *exd && *exp)) {
	return NULL;
    }

    if (Thesaurus == USE_NONE) {
	return NULL;
    }

    code_size = THESAURUS[Thesaurus].code_size;

    ret_sm = (char *)malloc_data(sizeof(char)*strlen(exd)+1, "get_most_similar_code");
    *ret_sm = '\0';

    /* 最大マッチスコアを求める */
    for (i = 0; exd[i]; i+=code_size) {
	for (j = 0; exp[j]; j+=code_size) {
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
		strncpy(ret_sm, exd+i, code_size);
		ret_sm_num = 1;
		ret_sm[code_size] = '\0';
		pre_i = i;
	    }
	    else if (tempscore == score && 
		     pre_i != i) { /* 重複を避けるため直前のiとは違うときのみ */
		strncat(ret_sm, exd+i, code_size);
		ret_sm_num++;
		pre_i = i;
	    }
	}
    }

    return ret_sm;
}

/*==================================================================*/
	   float calc_word_similarity(char *exd, char *exp)
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
float calc_sm_word_similarity(char *smd, char *exp, char *del, int expand)
/*==================================================================*/
{
    char *smp;
    float score = 0;

    /* 類似度計算: 意味素 - 単語 */

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
float calc_words_similarity(char *exd, char **exp, int num, int *pos)
/*==================================================================*/
{
    int i;
    float maxscore = 0, score;

    /* 類似度計算: 単語 - 単語群 */

    for (i = 0; i < num; i++) {
	score = calc_word_similarity(exd, *(exp+i));
	if (maxscore < score) {
	    maxscore = score;
	    *pos = i;
	}
    }

    return maxscore;
}

/*==================================================================*/
    float calc_sm_words_similarity(char *smd, char **exp, int num,
				   int *pos, char *del, int expand, 
				   char *unmatch_word)
/*==================================================================*/
{
    int i;
    float maxscore = 0, score;

    /* 類似度計算: 意味素 - 単語群 */

    for (i = 0; i < num; i++) {
	if (unmatch_word && 
	    !strcmp(*(exp+i), unmatch_word)) { /* マッチさせない単語 */
	    continue;
	}
	score = calc_sm_word_similarity(smd, *(exp+i), del, expand);
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

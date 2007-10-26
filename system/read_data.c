/*====================================================================

		形態素解析列の読み込み，文節へのまとめ

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int Bnst_start[MRPH_MAX];
int Tag_start[MRPH_MAX];
int Tag_dpnd[TAG_MAX];
int Tag_type[TAG_MAX];
FEATURE *Input_bnst_feature[BNST_MAX];
FEATURE *Input_tag_feature[TAG_MAX];

int ArticleID = 0;
int preArticleID = 0;

extern char CorpusComment[BNST_MAX][DATA_LEN];

/*==================================================================*/
	void selected_imi2feature(char *str, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char *buf, *token, *cp, *imip;

    if (!strcmp(str, "NIL")) {
	return;
    }

    buf = strdup(str);

    /* 通常 "" で括られている */
    if (buf[0] == '\"') {
	imip = &buf[1];
	if (cp = strchr(imip, '\"')) {
	    *cp = '\0';
	}
    }
    else {
	imip = buf;
    }

    token = strtok(imip, " ");
    while (token) {
	/* 以下のもの以外を付与 */
	if (strncmp(token, "代表表記", strlen("代表表記")) && 
	    strncmp(token, "可能動詞", strlen("可能動詞")) && 
	    strncmp(token, "漢字読み", strlen("漢字読み")) &&
	    strncmp(token, "カテゴリ", strlen("カテゴリ")) &&
	    strncmp(token, "ドメイン", strlen("ドメイン"))) {
	    assign_cfeature(&(m_ptr->f), token, FALSE);
	}
	token = strtok(NULL, " ");
    }

    free(buf);
}

/*==================================================================*/
    void assign_feature_alt_mrph(FEATURE **fpp, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char *buf;

    buf = malloc_data(strlen(m_ptr->Goi2) + 
		      strlen(m_ptr->Yomi) + 
		      strlen(m_ptr->Goi) + 
		      strlen(m_ptr->Imi) + 20, "assign_feature_alt_mrph");
    sprintf(buf, "ALT-%s-%s-%s-%d-%d-%d-%d-%s", 
	    m_ptr->Goi2, m_ptr->Yomi, m_ptr->Goi, 
	    m_ptr->Hinshi, m_ptr->Bunrui, 
	    m_ptr->Katuyou_Kata, m_ptr->Katuyou_Kei, 
	    m_ptr->Imi);
    assign_cfeature(fpp, buf, FALSE);
    free(buf);
}

/*==================================================================*/
		 char *get_mrph_rep(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char *cp;

    if ((cp = strstr(m_ptr->Imi, "代表表記:"))) {
	return cp + strlen("代表表記:");
    }
    return NULL;
}

/*==================================================================*/
	char *get_mrph_rep_from_f(MRPH_DATA *m_ptr, int flag)
/*==================================================================*/
{
    char *cp;

    /* flagが立っていてかつ、代表表記が変更されている場合は変更前の代表表記を返す */
    if (flag && (cp = check_feature(m_ptr->f, "代表表記変更"))) {
	return cp + strlen("代表表記変更:");
    }
    
    if ((cp = check_feature(m_ptr->f, "代表表記"))) {
	return cp + strlen("代表表記:");
    }
    return NULL;
}

/*==================================================================*/
	       int get_mrph_rep_length(char *rep_strt)
/*==================================================================*/
{
    char *rep_end;

    if (rep_strt == NULL) {
	return 0;
    }

    if ((rep_end = strchr(rep_strt, ' ')) == NULL) {
	rep_end = strchr(rep_strt, '\"');
    }

    return rep_end - rep_strt;
}

/*==================================================================*/
		 char *make_mrph_rn(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char *buf;

    /* (代表表記がないときに)代表表記を作る */

    /* 基本形の方が長いかもしれないので余分に確保 */
    buf = (char *)malloc_data(strlen(m_ptr->Goi) + strlen(m_ptr->Yomi) + SMALL_DATA_LEN, "make_mrph_rn");
    sprintf(buf, "%s/%s", m_ptr->Goi, m_ptr->Yomi);

    if (m_ptr->Katuyou_Kata > 0 && m_ptr->Katuyou_Kei > 0) { /* 活用語 */
	buf[strlen(buf) - strlen(Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].gobi)] = '\0'; /* 語幹にする */
	strcat(buf, Form[m_ptr->Katuyou_Kata][get_form_id(BASIC_FORM, m_ptr->Katuyou_Kata)].gobi); /* 基本形をつける */
    }
    return buf;
}

/*==================================================================*/
	       void supplement_bp_rn(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char *buf, *rep_buf;

    /* 「意味有」列に対して疑似代表表記をfeatureとして付与 */

    for (i = 0; i < sp->Tag_num; i++) {
	for (j = 0; j < (sp->tag_data + i)->mrph_num; j++) {
	    if (check_feature(((sp->tag_data + i)->mrph_ptr + j)->f, "意味有") && 
		!check_feature(((sp->tag_data + i)->mrph_ptr + j)->f, "代表表記")) {
		rep_buf = make_mrph_rn((sp->tag_data + i)->mrph_ptr + j);
		buf = (char *)malloc_data(strlen(rep_buf) + strlen("疑似代表表記:") + 1, "supplement_bp_rn");
		sprintf(buf, "疑似代表表記:%s", rep_buf);
		assign_cfeature(&(((sp->tag_data + i)->mrph_ptr + j)->f), buf, FALSE);
		free(rep_buf);
		free(buf);
	    }
	}
    }
}

/*==================================================================*/
void lexical_disambiguation(SENTENCE_DATA *sp, MRPH_DATA *m_ptr, int homo_num)
/*==================================================================*/
{
    int i, j, k, flag, orig_amb_flag, pref_mrph, pref_rule;
    int bw_length;
    int real_homo_num;
    int uniq_flag[HOMO_MAX];		/* 実質的同形異義語なら 1 */
    int matched_flag[HOMO_MRPH_MAX];	/* いずれかの形態素とマッチした
					   ルール内形態素パターンに 1 */
    int rep_length, rep_length2;
    HomoRule	*r_ptr;
    MRPH_DATA	*loop_ptr, *loop_ptr2;
    char fname[SMALL_DATA_LEN2], *cp, *cp2, *rep_strt, *rep_end, *rep_strt2, *rep_end2;

    /* 処理する最大数を越えていれば、最大数個だけチェックする */
    if (homo_num > HOMO_MAX) {
	homo_num = HOMO_MAX;
    }

    /* 品詞(細分類)が異なる形態素だけを残し，uniq_flagを1にする
       => すべて残すように変更 (2006/10/16) */

    uniq_flag[0] = 1;
    real_homo_num = 1;
    for (i = 1; i < homo_num; i++) {
	uniq_flag[i] = 1;
	if (uniq_flag[i]) real_homo_num++;
    }

    /* 実質的同形異義語がなければ何も処理はしない */

    if (real_homo_num == 1) return;

    /* ルール (mrph_homo.rule)に従って優先する形態素を選択
       ※ 同形異義語数とルール中の形態素数が同じことが条件
          各同形異義語がルール中の形態素のいずれかにマッチすればよい
	  ルールの最初の形態素にマッチしたものを優先(pref_mrph が記憶)
    */

    flag = FALSE;
    pref_mrph = 0;
    pref_rule = 0;
    for (i = 0, r_ptr = HomoRuleArray; i < CurHomoRuleSize; i++, r_ptr++) {
	if (r_ptr->pattern->mrphsize > HOMO_MRPH_MAX) {
	    fprintf(stderr, ";; The number of Rule morphs is too large in HomoRule.\n");
	    exit(1);
	}
	
	/* そこまでの形態素列をチェック */
	bw_length = m_ptr - sp->mrph_data;
	if ((r_ptr->pre_pattern == NULL &&	/* 違い */
	     bw_length != 0) ||
	    (r_ptr->pre_pattern != NULL &&
	     regexpmrphs_match(r_ptr->pre_pattern->mrph + 
			       r_ptr->pre_pattern->mrphsize - 1,
			       r_ptr->pre_pattern->mrphsize,
			       m_ptr - 1, 
			       bw_length,	/* 違い */
			       BW_MATCHING, 
			       ALL_MATCHING,/* 違い */
			       SHORT_MATCHING) == -1)) {
	    continue;
	}
	
	pref_mrph = 0;
	for (k = 0; k < r_ptr->pattern->mrphsize; k++) matched_flag[k] = FALSE;
	for (j = 0, loop_ptr = m_ptr; j < homo_num; j++, loop_ptr++) {
	    if (uniq_flag[j] == 0) continue;
	    flag = FALSE;
	    for (k = 0; k < r_ptr->pattern->mrphsize; k++) {
		if (matched_flag[k] && (r_ptr->pattern->mrph + k)->ast_flag != AST_FLG)
		    continue;
		if (regexpmrph_match(r_ptr->pattern->mrph + k, loop_ptr) 
		    == TRUE) {
		    flag = TRUE;
		    if (k == 0) pref_mrph = j;
		    matched_flag[k] = TRUE;
		    break;
		}
	    }
	    if (flag == FALSE) break;
	}
	if (flag == TRUE) {
	    for (k = 0; k < r_ptr->pattern->mrphsize; k++) {
		if (matched_flag[k] == FALSE) {
		    flag = FALSE;
		    break;
		}
	    }
	    if (flag == TRUE) {
		pref_rule = i;
		break;
	    }
	}
    }

    /* 多義性をマークするfeatureを与える */
    assign_cfeature(&((m_ptr+pref_mrph)->f), "品曖", FALSE);

    if (flag == TRUE) { /* ルールにマッチ */
	/* ルールに記述されているfeatureを与える (「品曖」を削除するルールもある) */
	assign_feature(&((m_ptr+pref_mrph)->f), &((HomoRuleArray + pref_rule)->f), m_ptr, 0, 1, FALSE);

	if (0 && OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Lexical Disambiguation "
		    "(%dth mrph -> %dth homo by %dth rule : %s :", 
		    m_ptr - sp->mrph_data, pref_mrph, pref_rule, 
		    (m_ptr+pref_mrph)->Goi2);
	    for (i = 0, loop_ptr = m_ptr; i < homo_num; i++, loop_ptr++)
		if (uniq_flag[i]) 
		    fprintf(Outfp, " %s", 
			    Class[loop_ptr->Hinshi][loop_ptr->Bunrui].id);
	    fprintf(Outfp, ")\n");
	}
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, ";; Cannot disambiguate lexical ambiguities by rules"
		    " (%dth mrph : %s ?", m_ptr - sp->mrph_data,
		    (m_ptr+pref_mrph)->Goi2);
	    for (i = 0, loop_ptr = m_ptr; i < homo_num; i++, loop_ptr++)
		if (uniq_flag[i]) 
		    fprintf(Outfp, " %s", 
			    Class[loop_ptr->Hinshi][loop_ptr->Bunrui].id);
	    fprintf(Outfp, ")\n");
	}
    }

    rep_strt = get_mrph_rep(m_ptr+pref_mrph);
    rep_length = get_mrph_rep_length(rep_strt);

    orig_amb_flag = 0;
    if (check_feature((m_ptr+pref_mrph)->f, "品曖")) {
	for (i = 0; i < homo_num; i++) {
	    if (i != pref_mrph) {
		/* 代表表記がpref_mrphと異なる場合 */
		rep_strt2 = get_mrph_rep(m_ptr+i);
		rep_length2 = get_mrph_rep_length(rep_strt2);
		if (rep_length > 0 && 
		    (rep_length != rep_length2 || strncmp(rep_strt, rep_strt2, rep_length))) {
		    orig_amb_flag = 1;
		}

		/* もとの形態素情報をfeatureとして保存 */
		assign_feature_alt_mrph(&((m_ptr+pref_mrph)->f), m_ptr + i);

		/* pref_mrph以外の形態素がもつ意味情報をすべて付与しておく */
		selected_imi2feature((m_ptr+i)->Imi, m_ptr+pref_mrph);
	    }
	}

	for (i = 0; i < homo_num; i++) {
	    if (uniq_flag[i] == 0) continue;
	    sprintf(fname, "品曖-%s", 
		    Class[(m_ptr+i)->Hinshi][(m_ptr+i)->Bunrui].id);
	    assign_cfeature(&((m_ptr+pref_mrph)->f), fname, FALSE);
	    
	    /* 固有名詞以外には"その他"をふっておく */
	    if (m_ptr->Bunrui != 3 && /* 固有名詞 */
		m_ptr->Bunrui != 4 && /* 地名 */
		m_ptr->Bunrui != 5 && /* 人名 */
		m_ptr->Bunrui != 6) /* 組織名 */
		assign_cfeature(&((m_ptr+pref_mrph)->f), "品曖-その他", FALSE);
	}
    }

    /* 代表表記が曖昧なときはマークしておく */
    if (orig_amb_flag) {
	assign_cfeature(&((m_ptr+pref_mrph)->f), "原形曖昧", FALSE);
    }

    /* pref_mrph番目のデータをコピー */
    if (pref_mrph != 0) {
	strcpy(m_ptr->Goi2, (m_ptr+pref_mrph)->Goi2);
	strcpy(m_ptr->Yomi, (m_ptr+pref_mrph)->Yomi);
	strcpy(m_ptr->Goi, (m_ptr+pref_mrph)->Goi);
	m_ptr->Hinshi = (m_ptr+pref_mrph)->Hinshi;
	m_ptr->Bunrui = (m_ptr+pref_mrph)->Bunrui;
	m_ptr->Katuyou_Kata = (m_ptr+pref_mrph)->Katuyou_Kata;
	m_ptr->Katuyou_Kei = (m_ptr+pref_mrph)->Katuyou_Kei;
	strcpy(m_ptr->Imi, (m_ptr+pref_mrph)->Imi);
	clear_feature(&(m_ptr->f));
	m_ptr->f = (m_ptr+pref_mrph)->f;
	(m_ptr+pref_mrph)->f = NULL;
    }
}

/*==================================================================*/
		       int readtoeos(FILE *fp)
/*==================================================================*/
{
    U_CHAR input_buffer[DATA_LEN];

    while (1) {
	if (fgets(input_buffer, DATA_LEN, fp) == NULL) return EOF;
	if (str_eq(input_buffer, "EOS\n")) return FALSE;
    }
}

/*==================================================================*/
			int readtonl(FILE *fp)
/*==================================================================*/
{
    int input_buffer;

    while (1) {
	if ((input_buffer = fgetc(fp)) == EOF) return EOF;
	if (input_buffer == '\n') {
	    return FALSE;
	}
    }
}

/*==================================================================*/
	     int read_mrph_file(FILE *fp, U_CHAR *buffer)
/*==================================================================*/
{
    int len;
#ifdef _WIN32
    char *EUCbuffer;
#endif

    if (fgets(buffer, DATA_LEN, fp) == NULL) return EOF;

#ifdef _WIN32
    EUCbuffer = toStringEUC(buffer);
    strcpy(buffer, EUCbuffer);
    free(EUCbuffer);
#endif

    /* Server モードの場合は 注意 \r\n になる*/
    if (OptMode == SERVER_MODE) {
	len = strlen(buffer);
	if (len > 2 && buffer[len-1] == '\n' && buffer[len-2] == '\r') {
	    buffer[len-2] = '\n';
	    buffer[len-1] = '\0';
	}

	if (buffer[0] == EOf) 
	    return EOF;
    }

    return TRUE;
}

/*==================================================================*/
	     int imi2feature(char *str, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char *token;

    token = strtok(str, " ");
    while (token) {
	assign_cfeature(&(m_ptr->f), token, FALSE);
	token = strtok(NULL, " ");
    }
}

/*==================================================================*/
	   void delete_existing_features(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    delete_cfeature(&(m_ptr->f), "カテゴリ");
    delete_cfeature(&(m_ptr->f), "ドメイン");
    delete_cfeature(&(m_ptr->f), "可能動詞");
    delete_cfeature(&(m_ptr->f), "漢字読み");
    delete_cfeature(&(m_ptr->f), "注釈");
    delete_cfeature(&(m_ptr->f), "謙譲動詞");
    delete_cfeature(&(m_ptr->f), "尊敬動詞");
    delete_cfeature(&(m_ptr->f), "丁寧動詞");
    delete_cfeature(&(m_ptr->f), "標準");
    delete_cfeature(&(m_ptr->f), "省略");
}

/*==================================================================*/
	    void copy_mrph(MRPH_DATA *dst, MRPH_DATA *src)
/*==================================================================*/
{
    char *imip, *cp;

    strcpy(dst->Goi, src->Goi);
    strcpy(dst->Yomi, src->Yomi);
    dst->Hinshi = src->Hinshi;
    dst->Bunrui = src->Bunrui;
    dst->Katuyou_Kata = src->Katuyou_Kata;
    dst->Katuyou_Kei = src->Katuyou_Kei;
    strcpy(dst->Imi, src->Imi);

    /* 意味情報をfeatureへ */
    if (src->Imi[0] == '\"') { /* 通常 "" で括られている */
	imip = &src->Imi[1];
	if (cp = strchr(imip, '\"')) {
	    *cp = '\0';
	}
    }
    else {
	imip = src->Imi;
    }

    imi2feature(imip, dst);
}

/*==================================================================*/
	     int feature_string2f(char *str, FEATURE **f)
/*==================================================================*/
{
    char *token;

    token = strtok(str, "><");
    while (token) {
	assign_cfeature(f, token, FALSE);
	token = strtok(NULL, "><");
    }
}

/*==================================================================*/
int store_one_annotation(SENTENCE_DATA *sp, TAG_DATA *tp, char *token)
/*==================================================================*/
{
    char flag, rel[SMALL_DATA_LEN], word[BNST_LENGTH_MAX];
    int tag_n, sent_n;

    sscanf(token, "%[^/]/%c/%[^/]/%d/%d/%*[^;]", rel, &flag, word, &tag_n, &sent_n);
    tp->c_cpm_ptr->cf.pp[tp->c_cpm_ptr->cf.element_num][0] = pp_kstr_to_code(rel);
    tp->c_cpm_ptr->cf.pp[tp->c_cpm_ptr->cf.element_num][1] = END_M;

    if (tp->c_cpm_ptr->cf.pp[tp->c_cpm_ptr->cf.element_num][0] == END_M) {
	fprintf(stderr, ";; Unknown case <%s>\n", rel);
	return TRUE;
    }

    if (flag == 'E') { /* 不特定 */
	tp->c_cpm_ptr->elem_b_ptr[tp->c_cpm_ptr->cf.element_num] = NULL;
	tp->c_cpm_ptr->elem_s_ptr[tp->c_cpm_ptr->cf.element_num] = NULL;
	
    }
    else {
	if (sent_n > 0) {
	    /* 異常なタグ単位が指定されているかチェック */
	    if (sp->Sen_num - sent_n < 1 || 
		tag_n >= (sentence_data + sp->Sen_num - 1 - sent_n)->Tag_num) {
		fprintf(stderr, ";; discarded inappropriate annotation: %s/%c/%s/%d/%d\n", rel, flag, word, tag_n, sent_n);
		return FALSE;
	    }
	    tp->c_cpm_ptr->elem_b_ptr[tp->c_cpm_ptr->cf.element_num] = (sentence_data + sp->Sen_num - 1 - sent_n)->tag_data + tag_n;
	    tp->c_cpm_ptr->elem_s_ptr[tp->c_cpm_ptr->cf.element_num] = sentence_data + sp->Sen_num - 1 - sent_n;
	}
	/* 現在の対象文 (この文はまだsentence_dataに入っていないため、上のようには扱えない)
   	   異常なタグ単位が指定されているかのチェックはcheck_annotation()で行う */
	else {
	    tp->c_cpm_ptr->elem_b_ptr[tp->c_cpm_ptr->cf.element_num] = sp->tag_data + tag_n;
	    tp->c_cpm_ptr->elem_s_ptr[tp->c_cpm_ptr->cf.element_num] = sp;
	}
    }

    if (flag == 'C') {
	tp->c_cpm_ptr->elem_b_num[tp->c_cpm_ptr->cf.element_num] = tp->c_cpm_ptr->cf.element_num;
    }
    else if (flag == 'N') {
	tp->c_cpm_ptr->elem_b_num[tp->c_cpm_ptr->cf.element_num] = -1;
    }
    else {
	tp->c_cpm_ptr->elem_b_num[tp->c_cpm_ptr->cf.element_num] = -2;
    }

    tp->c_cpm_ptr->cf.element_num++;
    if (tp->c_cpm_ptr->cf.element_num >= CF_ELEMENT_MAX) {
	return FALSE;
    }

    return TRUE;
}

/*==================================================================*/
	 int read_annotation(SENTENCE_DATA *sp, TAG_DATA *tp)
/*==================================================================*/
{
    char *cp, *start_cp;

    /* featureから格解析結果を取得 */
    if (cp = check_feature(tp->f, "格解析結果")) {
	tp->c_cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "read_annotation");
	memset(tp->c_cpm_ptr, 0, sizeof(CF_PRED_MGR));

	cp += strlen("格解析結果:");
	cp = strchr(cp, ':') + 1;
	start_cp = cp;
	for (; *cp; cp++) {
	    if (*cp == ';') {
		if (store_one_annotation(sp, tp, start_cp) == FALSE) {
		    return FALSE;
		}
		start_cp = cp + 1;
	    }
	}
	if (store_one_annotation(sp, tp, start_cp) == FALSE) {
	    return FALSE;
	}
    }

    return TRUE;
}

/*==================================================================*/
	       int check_annotation(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, check[CF_ELEMENT_MAX];
    TAG_DATA *tp;

    for (i = 0; i < sp->Tag_num; i++) {
	tp = sp->tag_data + i;
	if (tp->c_cpm_ptr) {
	    for (j = 0; j < tp->c_cpm_ptr->cf.element_num; j++) {
		/* 対象文の場合に、異常なタグ単位が指定されているかチェック */
		if (sp == tp->c_cpm_ptr->elem_s_ptr[j] && 
		    (tp->c_cpm_ptr->elem_b_ptr[j] - sp->tag_data) >= sp->Tag_num) {
		    fprintf(stderr, ";; discarded inappropriate annotation: %s/?/%s/%d/0\n", 
			    pp_code_to_kstr(tp->c_cpm_ptr->cf.pp[j][0]), 
			    tp->c_cpm_ptr->elem_b_ptr[j]->head_ptr->Goi, 
			    tp->c_cpm_ptr->elem_b_ptr[j]->num);
		    check[j] = FALSE;
		}
		else {
		    check[j] = TRUE;
		}
	    }

	    /* ずらす */
	    k = 0;
	    for (j = 0; j < tp->c_cpm_ptr->cf.element_num; j++) {
		if (check[j] == TRUE) {
		    if (k != j) {
			tp->c_cpm_ptr->cf.pp[k][0] = tp->c_cpm_ptr->cf.pp[j][0];
			tp->c_cpm_ptr->elem_b_ptr[k] = tp->c_cpm_ptr->elem_b_ptr[j];
			tp->c_cpm_ptr->elem_s_ptr[k] = tp->c_cpm_ptr->elem_s_ptr[j];
			tp->c_cpm_ptr->elem_b_num[k] = tp->c_cpm_ptr->elem_b_num[j];
		    }
		    k++;
		}
	    }

	    if (k) {
		tp->c_cpm_ptr->cf.element_num = k;
	    }
	    else { /* 1つもなくなったらfree */
		free(tp->c_cpm_ptr);
		tp->c_cpm_ptr = NULL;
	    }
	}
    }
}

/*==================================================================*/
	      int read_mrph(SENTENCE_DATA *sp, FILE *fp)
/*==================================================================*/
{
    U_CHAR input_buffer[DATA_LEN], rev_ibuffer[DATA_LEN], rest_buffer[DATA_LEN], Hinshi_str[DATA_LEN], Bunrui_str[DATA_LEN];
    U_CHAR Katuyou_Kata_str[DATA_LEN], Katuyou_Kei_str[DATA_LEN];
    MRPH_DATA  *m_ptr = sp->mrph_data;
    int homo_num, offset, mrph_item, bnst_item, tag_item, i, j, homo_flag;

    sp->Mrph_num = 0;
    homo_num = 0;
    ErrorComment = NULL;
    PM_Memo[0] = '\0';
    input_buffer[DATA_LEN-1] = '\n';

    while (1) {
	if (read_mrph_file(fp, input_buffer) == EOF) return EOF;

	if (input_buffer[DATA_LEN-1] != '\n') {
	    input_buffer[DATA_LEN-1] = '\0';
	    fprintf(stderr, ";; Too long mrph <%s> !\n", input_buffer);
	    return readtonl(fp);
	}
	else if (input_buffer[strlen(input_buffer)-1] != '\n') {
	    fprintf(stderr, ";; Too long mrph <%s> !\n", input_buffer);
	    return FALSE;
	}

	/* -i によるコメント行 */
	if (OptIgnoreChar && *input_buffer == OptIgnoreChar) {
	    fprintf(Outfp, "%s", input_buffer);
	    fflush(Outfp);
	    continue;
	}

	/* # による正規のコメント行 */

	if (input_buffer[0] == '#') {
	    input_buffer[strlen(input_buffer)-1] = '\0';
	    sp->Comment = strdup(input_buffer);
	    sp->KNPSID = (char *)malloc_data(strlen(input_buffer), "read_mrph");
	    sscanf(input_buffer, "# %s", sp->KNPSID);

	    /* 文章が変わったら固有名詞スタック, 前文データをクリア */
	    if (!strncmp(input_buffer, "# S-ID", 6) && 
		strchr(input_buffer+6, '-')) { /* 「記事ID-文ID」という形式ならば */

		/* 様々な文章IDに対応するためコメント行を逆順にしてsscanfする */
		/* このためArticleIDは本来のArticleIDを逆順にしたもの(ex. 135→531)になっている*/
		i = strlen(input_buffer);
		j = 0;
		while (i > 0) {
		    i--;
		    rev_ibuffer[j++] = input_buffer[i];
		    /* 記事番号以降のコメントを除外するため2文字目(#とS-IDの間の空白)以外の */
		    /* 空白以降を除外する */
		    if (i != 1 && (input_buffer[i] == ' ' || input_buffer[i] == '\t')) {
			j = 0;
		    }
		}
		rev_ibuffer[j++] = '\0';

		/* intは2147483647までしか取れないため上位9桁(元の下位9桁)のみ読んで比較 */
		sscanf(rev_ibuffer, "%*d-%9d", &ArticleID);

		if (ArticleID && preArticleID && ArticleID != preArticleID) {
		    if (OptDisplay == OPT_DEBUG) fprintf(stderr, "New Article %s\n", input_buffer);
		    if (OptCorefer) {
			ClearSentences(sp);
		    }
		    if (OptNE) {
			clear_ne_cache();
		    }
		}
		preArticleID = ArticleID;
	    }
	}

	/* 解析済みの場合 */
	/* 文節行 */
	else if (input_buffer[0] == '*') {
	    if (sp->Mrph_num == 0) {
		OptInput |= OPT_PARSED;
		if (OptEllipsis) {
		    OptAnalysis = OPT_CASE2;
		}
		sp->Bnst_num = 0;
		sp->Tag_num = 0;
		memset(Bnst_start, 0, sizeof(int)*MRPH_MAX);
		memset(Tag_start, 0, sizeof(int)*MRPH_MAX);
		if (OptReadFeature) {
		    memset(Input_bnst_feature, 0, sizeof(FEATURE *) *BNST_MAX);
		    memset(Input_tag_feature, 0, sizeof(FEATURE *) *TAG_MAX);
		}
	    }

	    if (OptInput == OPT_RAW) {
		fprintf(stderr, ";; Invalid input <%s> !\n", input_buffer);
		return readtoeos(fp);
	    }

	    bnst_item = sscanf(input_buffer, "* %d%c %[^\n]", 
			       &(sp->Best_mgr->dpnd.head[sp->Bnst_num]),
			       &(sp->Best_mgr->dpnd.type[sp->Bnst_num]),
			       rest_buffer);

	    /* 文節の入力されたfeatureを使う */
	    if (bnst_item == 3) {
		if (OptReadFeature) { 
		    /* featureを<>でsplitしてfに変換 */
		    feature_string2f(rest_buffer, &Input_bnst_feature[sp->Bnst_num]);
		}
	    }
	    else if (bnst_item != 2) {
		fprintf(stderr, ";; Invalid input <%s> !\n", input_buffer);
		OptInput = OPT_RAW;
		return readtoeos(fp);
	    }

	    Bnst_start[sp->Mrph_num - homo_num] = 1;
	    sp->Bnst_num++;
	}
	/* タグ単位行 */
	else if (input_buffer[0] == '+') {
	    if (OptInput == OPT_RAW) {
		fprintf(stderr, ";; Invalid input <%s> !\n", input_buffer);
		return readtoeos(fp);
	    }

	    tag_item = sscanf(input_buffer, "+ %d%c %[^\n]", 
			      &Tag_dpnd[sp->Tag_num],
			      &Tag_type[sp->Tag_num],
			      rest_buffer);

	    /* タグ単位の入力されたfeatureを使う */
	    if (tag_item == 3) {
		if (OptReadFeature) { 
		    /* featureを<>でsplitしてfに変換 */
		    feature_string2f(rest_buffer, &Input_tag_feature[sp->Tag_num]);
		}
	    }
	    else if (tag_item != 2) {
		fprintf(stderr, ";; Invalid input <%s> !\n", input_buffer);
		OptInput = OPT_RAW;
		return readtoeos(fp);
	    }

	    Tag_start[sp->Mrph_num - homo_num] = 1;
	    sp->Tag_num++;
	}

	/* 文末 */

	else if (str_eq(input_buffer, "EOS\n")) {
	    /* 形態素が一つもないとき */
	    if (sp->Mrph_num == 0) {
		return FALSE;
	    }

	    /* タグ単位のない解析済の場合 */
	    if ((OptInput & OPT_PARSED) && sp->Tag_num == 0) {
		OptInput |= OPT_INPUT_BNST;
	    }

	    if (homo_num) {	/* 前に同形異義語セットがあれば処理する */
		lexical_disambiguation(sp, m_ptr - homo_num - 1, homo_num + 1);
		sp->Mrph_num -= homo_num;
		m_ptr -= homo_num;
		for (i = 0; i < homo_num; i++) {
		    clear_feature(&((m_ptr+i)->f));
		}
		homo_num = 0;
	    }
	    return TRUE;
	}

	/* 通常の形態素 */

	else {

	    /* 同形異義語かどうか */
	    if (input_buffer[0] == '@' && input_buffer[1] == ' ' && input_buffer[2] != '@') {
		homo_flag = 1;
	    }
	    else {
		homo_flag = 0;
	    }
	    
	    if (homo_flag == 0 && homo_num) {

		/* 同形異義語マークがなく，前に同形異義語セットがあれば
	           lexical_disambiguationを呼んで処理 */		   

		lexical_disambiguation(sp, m_ptr - homo_num - 1, homo_num + 1);
		sp->Mrph_num -= homo_num;
		m_ptr -= homo_num;
		for (i = 0; i < homo_num; i++) {
		    clear_feature(&((m_ptr+i)->f));
		}
		homo_num = 0;
	    }

	    /* 最大数を越えないようにチェック */
	    if (sp->Mrph_num >= MRPH_MAX) {
		fprintf(stderr, ";; Too many mrph (%s %s%s...)!\n", 
			sp->Comment ? sp->Comment : "", sp->mrph_data, sp->mrph_data+1);
		sp->Mrph_num = 0;
		return readtoeos(fp);
	    }

	    /* 形態素情報 :
	       語彙(活用形) 読み 語彙(原型) 
	       品詞(+番号) 細分類(+番号) 活用型(+番号) 活用形(+番号) 
	       意味情報
	    */

	    offset = homo_flag ? 2 : 0;
	    mrph_item = sscanf(input_buffer + offset,
			       "%s %s %s %s %d %s %d %s %d %s %d %[^\n]", 
			       m_ptr->Goi2, m_ptr->Yomi, m_ptr->Goi, 
			       Hinshi_str, &(m_ptr->Hinshi), Bunrui_str, &(m_ptr->Bunrui), 
			       Katuyou_Kata_str, &(m_ptr->Katuyou_Kata), 
			       Katuyou_Kei_str, &(m_ptr->Katuyou_Kei), 
			       rest_buffer);
	    if (Language == CHINESE) { /* transfer POS to word features for Chinese */
		assign_cfeature(&(m_ptr->f), Hinshi_str, FALSE);
		strcpy(m_ptr->Pos, Hinshi_str);  
	    }

	    if (mrph_item == 12) {
		char *imip, *cp, *rep_buf;
		/* 意味情報をfeatureへ */
		if (strncmp(rest_buffer, "NIL", 3)) {

		    /* 通常 "" で括られている */
		    if (rest_buffer[0] == '\"') {
			imip = &rest_buffer[1];
			if (cp = strchr(imip, '\"')) {
			    *cp = '\0';
			}
			/* 疑似代表表記を追加する */
			if (strcmp(Hinshi_str, "特殊") && strcmp(Hinshi_str, "判定詞") && 
			    strcmp(Hinshi_str, "助動詞") && strcmp(Hinshi_str, "助詞") && 
			    !strstr(imip, "代表表記")) {
			    rep_buf = make_mrph_rn(m_ptr);
			    if (strlen(imip) + strlen(" 疑似代表表記 代表表記:") +
				strlen(rep_buf) + 2 < DATA_LEN) {
				strcat(imip, " 疑似代表表記 代表表記:");
				strcat(imip, rep_buf);
			    }
			    free(rep_buf);
			}
			sprintf(m_ptr->Imi, "\"%s\"", imip);
		    }
		    else {
			imip = rest_buffer;
			if (cp = strchr(imip, ' ')) {
			    *cp = '\0';
			}
			strcpy(m_ptr->Imi, imip);
		    }

		    imi2feature(imip, m_ptr);
		}
		else {
		    /* 疑似代表表記を追加する */
		    rep_buf = make_mrph_rn(m_ptr);			
		    if (strcmp(Hinshi_str, "特殊") && strcmp(Hinshi_str, "判定詞") && 
			strcmp(Hinshi_str, "助動詞") &&	strcmp(Hinshi_str, "助詞") && 
			strlen(" 疑似代表表記 代表表記:") + strlen(rep_buf) + 1 < DATA_LEN) {
			imip = rest_buffer;		    
			*imip = '\0';
			strcat(imip, "疑似代表表記 代表表記:");
			strcat(imip, rep_buf);
			sprintf(m_ptr->Imi, "\"%s\"", imip);
			imi2feature(imip, m_ptr);		    
		    }
		    else {
			strcpy(m_ptr->Imi, "NIL");
		    }
		    free(rep_buf);
		}
	    }
	    else if (mrph_item == 11) {
		strcpy(m_ptr->Imi, "NIL");
	    }
	    else {
		fprintf(stderr, ";; Invalid input (%d items)<%s> !\n", 
			mrph_item, input_buffer);
		if (sp->Comment) fprintf(stderr, "(%s)\n", sp->Comment);
		return readtoeos(fp);
	    }   

	    if (OptInput & OPT_PARSED) {
		m_ptr->Hinshi = get_hinsi_id(Hinshi_str);
		m_ptr->Bunrui = get_bunrui_id(Bunrui_str, m_ptr->Hinshi);
		m_ptr->Katuyou_Kata = get_type_id(Katuyou_Kata_str);
		m_ptr->Katuyou_Kei = get_form_id(Katuyou_Kei_str, m_ptr->Katuyou_Kata);
	    }

	    /* clear_feature(&(m_ptr->f)); 
	       mainの文ごとのループの先頭で処理に移動 */

	    /* 同形異義語は一旦 sp->mrph_data にいれる */
	    if (homo_flag) homo_num++;

	    sp->Mrph_num++;
	    m_ptr++;
	}
    }
}

/*==================================================================*/
void change_one_mrph_rep(MRPH_DATA *m_ptr, int modify_feature_flag, char suffix_char)
/*==================================================================*/
{
    int i;
    char pre[WORD_LEN_MAX + 1], str1[WORD_LEN_MAX + 1], str2[WORD_LEN_MAX + 1], post[WORD_LEN_MAX + 1], *cp;

    /* 「代表表記:動く/うごく」->「代表表記:動き/うごきv」 */

    /* 活用する品詞ではない場合 */
    if (m_ptr->Katuyou_Kata == 0 || m_ptr->Katuyou_Kei == 0) {
	return;
    }

    if (cp = strstr(m_ptr->Imi, "代表表記:")) {
	cp += strlen("代表表記:");
	sscanf(cp, "%[^/]", str1);

	pre[0] = '\0';
	strncat(pre, m_ptr->Imi, cp - m_ptr->Imi);

	cp += strlen(str1) + 1;
	sscanf(cp, "%[^ \"]", str2);
	post[0] = '\0';
	strcat(post, cp + strlen(str2));
    }
    else {
	return;
    }

    /* 語幹にする */
    str1[strlen(str1) - strlen(Form[m_ptr->Katuyou_Kata][get_form_id(BASIC_FORM, m_ptr->Katuyou_Kata)].gobi)] = '\0';
    str2[strlen(str2) - strlen(Form[m_ptr->Katuyou_Kata][get_form_id(BASIC_FORM, m_ptr->Katuyou_Kata)].gobi)] = '\0';

    /* 活用形をつける */
    strcat(str1, Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].gobi);
    strcat(str2, Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].gobi);

    /* 意味情報の修正 */
    sprintf(m_ptr->Imi, "%s%s/%s%c%s", pre, str1, str2, suffix_char, post);

    /* featureの修正 */
    if (modify_feature_flag) {
	if (cp = check_feature(m_ptr->f, "代表表記")) { /* 古い代表表記の保存 */
	    cp += strlen("代表表記:");
	    sprintf(pre, "代表表記変更:%s", cp);
	    assign_cfeature(&(m_ptr->f), pre, FALSE);
	}
	sprintf(pre, "代表表記:%s/%s%c", str1, str2, suffix_char);
	assign_cfeature(&(m_ptr->f), pre, FALSE);
    }
}

/*==================================================================*/
	  void change_one_mrph(MRPH_DATA *m_ptr, FEATURE *f)
/*==================================================================*/
{
    char h_buffer[62], b_buffer[62], kata_buffer[62], kei_buffer[62];
    int num;

    m_ptr->Hinshi = 0;
    m_ptr->Bunrui = 0;
    m_ptr->Katuyou_Kata = 0;
    m_ptr->Katuyou_Kei = 0;

    num = sscanf(f->cp, "%*[^:]:%[^:]:%[^:]:%[^:]:%[^:]", 
		 h_buffer, b_buffer, kata_buffer, kei_buffer);

    m_ptr->Hinshi = get_hinsi_id(h_buffer);
    if (num >= 2) {
	if (!strcmp(b_buffer, "*"))
	    m_ptr->Bunrui = 0;
	else 
	    m_ptr->Bunrui = get_bunrui_id(b_buffer, m_ptr->Hinshi);
    }
    if (num >= 3) {
	m_ptr->Katuyou_Kata = get_type_id(kata_buffer);
	m_ptr->Katuyou_Kei = get_form_id(kei_buffer, 
					 m_ptr->Katuyou_Kata);
    }
    
    /* 品詞変更が活用なしの場合は原型も変更する */
    /* ▼ 逆(活用なし→活用あり)は扱っていない */
    if (m_ptr->Katuyou_Kata == 0) {
	strcpy(m_ptr->Goi, m_ptr->Goi2);
    }
}

/*==================================================================*/
	  void change_alt_mrph(MRPH_DATA *m_ptr, FEATURE *f)
/*==================================================================*/
{
    FEATURE **fpp = &(m_ptr->f), *ret_fp = NULL;
    MRPH_DATA m;

    /* ALT中の「代表表記:動く/うごく」->「代表表記:動き/うごきv」 */

    while (*fpp) {
	if (!strncmp((*fpp)->cp, "ALT-", 4)) {
	    sscanf((*fpp)->cp + 4, "%[^-]-%[^-]-%[^-]-%d-%d-%d-%d-%[^\n]", 
		   m.Goi2, m.Yomi, m.Goi, 
		   &m.Hinshi, &m.Bunrui, 
		   &m.Katuyou_Kata, &m.Katuyou_Kei, m.Imi);
	    change_one_mrph_rep(&m, 0, 'v');
	    change_one_mrph(&m, f);
	    assign_feature_alt_mrph(&ret_fp, &m);
	    free((*fpp)->cp); /* 古いALTは削除 */
	    *fpp = (*fpp)->next;
	}
	else {
	    fpp = &((*fpp)->next);
	}
    }

    /* 新しいALT */
    if (ret_fp) {
	append_feature(&(m_ptr->f), ret_fp);
    }
}

/*==================================================================*/
	    void change_mrph(MRPH_DATA *m_ptr, FEATURE *f)
/*==================================================================*/
{
    char org_buffer[DATA_LEN];
    int num;

    /* もとの形態素情報をfeatureとして保存 */
    sprintf(org_buffer, "品詞変更:%s-%s-%s-%d-%d-%d-%d-%s", 
	    m_ptr->Goi2, m_ptr->Yomi, m_ptr->Goi, 
	    m_ptr->Hinshi, m_ptr->Bunrui, 
	    m_ptr->Katuyou_Kata, m_ptr->Katuyou_Kei, m_ptr->Imi);
    assign_cfeature(&(m_ptr->f), org_buffer, FALSE);

    change_one_mrph_rep(m_ptr, 1, 'v'); /* 代表表記を修正 */
    change_one_mrph(m_ptr, f); /* 品詞などを修正 */

    change_alt_mrph(m_ptr, f); /* ALTの中も修正 */
}

/*==================================================================*/
		      int get_Bunrui(char *cp)
/*==================================================================*/
{
    int j;

    for (j = 1; Class[6][j].id; j++) {
	if (str_eq(Class[6][j].id, cp))
	    return j;
    }
}

/*==================================================================*/
		    int break_feature(FEATURE *fp)
/*==================================================================*/
{
    while (fp) {
	if (!strcmp(fp->cp, "&break:normal")) 
	    return RLOOP_BREAK_NORMAL;
	else if (!strcmp(fp->cp, "&break:jump")) 
	    return RLOOP_BREAK_JUMP;
	else if (!strncmp(fp->cp, "&break", strlen("&break")))
	    return RLOOP_BREAK_NORMAL;
	fp = fp->next;
    }
    return RLOOP_BREAK_NONE;
}

/*==================================================================*/
       void assign_mrph_feature(MrphRule *s_r_ptr, int r_size,
				MRPH_DATA *s_m_ptr, int m_length,
				int mode, int break_mode, int direction, 
				int also_assign_flag, int temp_assign_flag)
/*==================================================================*/
{
    /* ある範囲(文全体,文節内など)に対して形態素のマッチングを行う */

    int i, j, k, match_length, feature_break_mode;
    MrphRule *r_ptr;
    MRPH_DATA *m_ptr;

    /* 逆方向に適用する場合はデータのおしりをさしておく必要がある */
    if (direction == RtoL)
	s_m_ptr += m_length-1;
    
    /* MRM
       	1.self_patternの先頭の形態素位置
	  2.ルール
	    3.self_patternの末尾の形態素位置
	の順にループが回る (3のループはregexpmrphrule_matchの中)
	
	break_mode == RLOOP_BREAK_NORMAL
	    2のレベルでbreakする
	break_mode == RLOOP_BREAK_JUMP
	    2のレベルでbreakし，self_pattern長だけ1のループを進める
     */

    if (mode == RLOOP_MRM) {
	for (i = 0; i < m_length; i++) {
	    r_ptr = s_r_ptr;
	    m_ptr = s_m_ptr+(i*direction);
	    for (j = 0; j < r_size; j++, r_ptr++) {
		if ((match_length = 
		     regexpmrphrule_match(r_ptr, m_ptr, 
					  direction == LtoR ? i : m_length-i-1, 
					  direction == LtoR ? m_length-i : i+1)) != -1) {
		    for (k = 0; k < match_length; k++)
			assign_feature(&((s_m_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_m_ptr+i*direction, k, match_length - k, temp_assign_flag);
		    feature_break_mode = break_feature(r_ptr->f);
		    if (break_mode == RLOOP_BREAK_NORMAL ||
			feature_break_mode == RLOOP_BREAK_NORMAL) {
			break;
		    } else if (break_mode == RLOOP_BREAK_JUMP ||
			       feature_break_mode == RLOOP_BREAK_JUMP) {
			i += match_length - 1;
			break;
		    }
		}
	    }
	}
    }

    /* RMM
       	1.ルール
	  2.self_patternの先頭の形態素位置
	    3.self_patternの末尾の形態素位置
	の順にループが回る (3のループはregexpmrphrule_matchの中)
	
	break_mode == RLOOP_BREAK_NORMAL||RLOOP_BREAK_JUMP
	    2のレベルでbreakする (※この使い方は考えにくいが)
    */

    else if (mode == RLOOP_RMM) {
	r_ptr = s_r_ptr;
	for (j = 0; j < r_size; j++, r_ptr++) {
	    feature_break_mode = break_feature(r_ptr->f);
	    for (i = 0; i < m_length; i++) {
		m_ptr = s_m_ptr+(i*direction);
		if ((match_length = 
		     regexpmrphrule_match(r_ptr, m_ptr, 
					  direction == LtoR ? i : m_length-i-1, 
					  direction == LtoR ? m_length-i : i+1)) != -1) {
		    for (k = 0; k < match_length; k++)
			assign_feature(&((s_m_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_m_ptr+i*direction, k, match_length - k, temp_assign_flag);
		    if (break_mode == RLOOP_BREAK_NORMAL ||
			break_mode == RLOOP_BREAK_JUMP ||
			feature_break_mode == RLOOP_BREAK_NORMAL ||
			feature_break_mode == RLOOP_BREAK_JUMP) {
			break;
		    }
		}
	    }
	}
    }
}

/*==================================================================*/
void assign_tag_feature(BnstRule *s_r_ptr, int r_size,
			TAG_DATA *s_b_ptr, int b_length,
			int mode, int break_mode, int direction, 
			int also_assign_flag, int temp_assign_flag)
/*==================================================================*/
{
    /* ある範囲(文全体,文節内など)に対してタグ単位のマッチングを行う */

    int i, j, k, match_length, feature_break_mode;
    BnstRule *r_ptr;
    TAG_DATA *b_ptr;

    /* 逆方向に適用する場合はデータのおしりをさしておく必要がある */
    if (direction == RtoL)
	s_b_ptr += b_length-1;
    
    /* MRM
       	1.self_patternの先頭の文節位置
	  2.ルール
	    3.self_patternの末尾の文節位置
	の順にループが回る (3のループはregexpbnstrule_matchの中)
	
	break_mode == RLOOP_BREAK_NORMAL
	    2のレベルでbreakする
	break_mode == RLOOP_BREAK_JUMP
	    2のレベルでbreakし，self_pattern長だけ1のループを進める
     */

    if (mode == RLOOP_MRM) {
	for (i = 0; i < b_length; i++) {
	    r_ptr = s_r_ptr;
	    b_ptr = s_b_ptr+(i*direction);
	    for (j = 0; j < r_size; j++, r_ptr++) {
		if ((match_length = 
		     regexptagrule_match(r_ptr, b_ptr, 
					 direction == LtoR ? i : b_length-i-1, 
					 direction == LtoR ? b_length-i : i+1)) != -1) {
		    for (k = 0; k < match_length; k++) {
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			if (also_assign_flag) { /* 属する文節にも付与する場合 */
			    assign_feature(&((s_b_ptr+i*direction+k)->b_ptr->f), 
					   &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			}
		    }
		    feature_break_mode = break_feature(r_ptr->f);
		    if (break_mode == RLOOP_BREAK_NORMAL ||
			feature_break_mode == RLOOP_BREAK_NORMAL) {
			break;
		    } else if (break_mode == RLOOP_BREAK_JUMP ||
			       feature_break_mode == RLOOP_BREAK_JUMP) {
			i += match_length - 1;
			break;
		    }
		}
	    }
	}
    }

    /* RMM
       	1.ルール
	  2.self_patternの先頭の文節位置
	    3.self_patternの末尾の文節位置
	の順にループが回る (3のループはregexpbnstrule_matchの中)
	
	break_mode == RLOOP_BREAK_NORMAL||RLOOP_BREAK_JUMP
	    2のレベルでbreakする (※この使い方は考えにくいが)
    */

    else if (mode == RLOOP_RMM) {
	r_ptr = s_r_ptr;
	for (j = 0; j < r_size; j++, r_ptr++) {
	    feature_break_mode = break_feature(r_ptr->f);
	    for (i = 0; i < b_length; i++) {
		b_ptr = s_b_ptr+(i*direction);
		if ((match_length = 
		     regexptagrule_match(r_ptr, b_ptr, 
					 direction == LtoR ? i : b_length-i-1, 
					 direction == LtoR ? b_length-i : i+1)) != -1) {
		    for (k = 0; k < match_length; k++) {
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			if (also_assign_flag) { /* 属する文節にも付与する場合 */
			    assign_feature(&((s_b_ptr+i*direction+k)->b_ptr->f), 
					   &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			}
		    }
		    if (break_mode == RLOOP_BREAK_NORMAL ||
			break_mode == RLOOP_BREAK_JUMP ||
			feature_break_mode == RLOOP_BREAK_NORMAL ||
			feature_break_mode == RLOOP_BREAK_JUMP) {
			break;
		    }
		}
	    }
	}
    }
}

/*==================================================================*/
void assign_bnst_feature(BnstRule *s_r_ptr, int r_size,
			 BNST_DATA *s_b_ptr, int b_length,
			 int mode, int break_mode, int direction, 
			 int also_assign_flag, int temp_assign_flag)
/*==================================================================*/
{
    /* ある範囲(文全体,文節内など)に対して文節のマッチングを行う */

    int i, j, k, match_length, feature_break_mode;
    BnstRule *r_ptr;
    BNST_DATA *b_ptr;

    /* 逆方向に適用する場合はデータのおしりをさしておく必要がある */
    if (direction == RtoL)
	s_b_ptr += b_length-1;
    
    /* MRM
       	1.self_patternの先頭の文節位置
	  2.ルール
	    3.self_patternの末尾の文節位置
	の順にループが回る (3のループはregexpbnstrule_matchの中)
	
	break_mode == RLOOP_BREAK_NORMAL
	    2のレベルでbreakする
	break_mode == RLOOP_BREAK_JUMP
	    2のレベルでbreakし，self_pattern長だけ1のループを進める
     */

    if (mode == RLOOP_MRM) {
	for (i = 0; i < b_length; i++) {
	    r_ptr = s_r_ptr;
	    b_ptr = s_b_ptr+(i*direction);
	    for (j = 0; j < r_size; j++, r_ptr++) {
		if ((match_length = 
		     regexpbnstrule_match(r_ptr, b_ptr, 
					  direction == LtoR ? i : b_length-i-1, 
					  direction == LtoR ? b_length-i : i+1)) != -1) {
		    for (k = 0; k < match_length; k++) {
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			if (also_assign_flag) { /* headのタグ単位にも付与する場合 */
			    assign_feature(&(((s_b_ptr+i*direction+k)->tag_ptr + (s_b_ptr+i*direction+k)->tag_num - 1)->f), 
					   &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			}
		    }
		    feature_break_mode = break_feature(r_ptr->f);
		    if (break_mode == RLOOP_BREAK_NORMAL ||
			feature_break_mode == RLOOP_BREAK_NORMAL) {
			break;
		    } else if (break_mode == RLOOP_BREAK_JUMP ||
			       feature_break_mode == RLOOP_BREAK_JUMP) {
			i += match_length - 1;
			break;
		    }
		}
	    }
	}
    }

    /* RMM
       	1.ルール
	  2.self_patternの先頭の文節位置
	    3.self_patternの末尾の文節位置
	の順にループが回る (3のループはregexpbnstrule_matchの中)
	
	break_mode == RLOOP_BREAK_NORMAL||RLOOP_BREAK_JUMP
	    2のレベルでbreakする (※この使い方は考えにくいが)
    */

    else if (mode == RLOOP_RMM) {
	r_ptr = s_r_ptr;
	for (j = 0; j < r_size; j++, r_ptr++) {
	    feature_break_mode = break_feature(r_ptr->f);
	    for (i = 0; i < b_length; i++) {
		b_ptr = s_b_ptr+(i*direction);
		if ((match_length = 
		     regexpbnstrule_match(r_ptr, b_ptr, 
					  direction == LtoR ? i : b_length-i-1, 
					  direction == LtoR ? b_length-i : i+1)) != -1) {
		    for (k = 0; k < match_length; k++) {
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			if (also_assign_flag) { /* headのタグ単位にも付与する場合 */
			    assign_feature(&(((s_b_ptr+i*direction+k)->tag_ptr + (s_b_ptr+i*direction+k)->tag_num - 1)->f), 
					   &(r_ptr->f), s_b_ptr+i*direction, k, match_length - k, temp_assign_flag);
			}
		    }
		    if (break_mode == RLOOP_BREAK_NORMAL ||
			break_mode == RLOOP_BREAK_JUMP ||
			feature_break_mode == RLOOP_BREAK_NORMAL ||
			feature_break_mode == RLOOP_BREAK_JUMP) {
			break;
		    }
		}
	    }
	}
    }
}

/*==================================================================*/
void assign_general_feature(void *data, int size, int flag, int also_assign_flag, int temp_assign_flag)
/*==================================================================*/
{
    int i;
    void (*assign_function)();

    /* 形態素, タグ単位, 文節の場合分け */
    if (flag == MorphRuleType || flag == NeMorphRuleType) {
	assign_function = assign_mrph_feature;
    }
    else if (flag == TagRuleType || flag == AfterDpndTagRuleType || flag == PostProcessTagRuleType) {
	assign_function = assign_tag_feature;
    }
    else if (flag == BnstRuleType || flag == AfterDpndBnstRuleType) {
	assign_function = assign_bnst_feature;
    }

    for (i = 0; i < GeneralRuleNum; i++) {
	if ((GeneralRuleArray + i)->type == flag) {
	    assign_function((GeneralRuleArray+i)->RuleArray, 
			    (GeneralRuleArray+i)->CurRuleSize, 
			    data, size, 
			    (GeneralRuleArray+i)->mode, 
			    (GeneralRuleArray+i)->breakmode, 
			    (GeneralRuleArray+i)->direction, 
			    also_assign_flag, temp_assign_flag);
	}
    }
}

/*==================================================================*/
      BNST_DATA *init_bnst(SENTENCE_DATA *sp, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    int i;
    char *cp;
    BNST_DATA *b_ptr;

    b_ptr = sp->bnst_data + sp->Bnst_num;
    b_ptr->type = IS_BNST_DATA;
    b_ptr->num = sp->Bnst_num;
    sp->Bnst_num++;
    if (sp->Bnst_num > BNST_MAX) {
	fprintf(stderr, ";; Too many bnst (%s %s%s...)!\n", 
		sp->Comment ? sp->Comment : "", sp->mrph_data, sp->mrph_data+1);
	sp->Bnst_num = 0;
	return NULL;
    }

    b_ptr->mrph_ptr = m_ptr;
    b_ptr->mrph_num = 0;

    b_ptr->BGH_num = 0;
    b_ptr->SM_num = 0;

    b_ptr->para_key_type = PARA_KEY_O;
    b_ptr->para_top_p = FALSE;
    b_ptr->para_type = PARA_NIL;
    b_ptr->to_para_p = FALSE;

    b_ptr->cpm_ptr = NULL;
    b_ptr->voice = 0;

    b_ptr->space = 0;

    b_ptr->pred_b_ptr = NULL;
    
    for (i = 0, cp = b_ptr->SCASE_code; i < SCASE_CODE_SIZE; i++, cp++) *cp = 0;

    /* clear_feature(&(b_ptr->f));
       mainの文ごとのループの先頭で処理に移動 */

    return b_ptr;
}

/*==================================================================*/
	void make_Jiritu_Go(SENTENCE_DATA *sp, BNST_DATA *ptr)
/*==================================================================*/
{
    MRPH_DATA *mp;

    ptr->Jiritu_Go[0] = '\0';

    /* 主辞より前の部分で接頭辞以外を自立語としておいておく */
    for (mp = ptr->mrph_ptr; mp <= ptr->head_ptr; mp++) {
	if (!check_feature(mp->f, "接頭")) {
	    if (strlen(ptr->Jiritu_Go) + strlen(mp->Goi) + 2 > BNST_LENGTH_MAX) {
		fprintf(stderr, ";; Too big bunsetsu (%s %s...)!\n", 
			sp->Comment ? sp->Comment : "", ptr->mrph_ptr);
		return;
	    }
	    strcat(ptr->Jiritu_Go, mp->Goi);
	}
    }
}

/*==================================================================*/
		 void decide_head_ptr(BNST_DATA *ptr)
/*==================================================================*/
{
    int i;

    if (ptr->type == IS_TAG_DATA) {
	for (i = ptr->mrph_num - 1; i >= 0 ; i--) {
	    if (check_feature((ptr->mrph_ptr + i)->f, "意味有")) {
		ptr->head_ptr = ptr->mrph_ptr + i;
		return;
	    }
	}
    }
    /* 文節のときは形式名詞「の」をheadとしない */
    else {
	for (i = ptr->mrph_num - 1; i >= 0 ; i--) {
	    if (!check_feature((ptr->mrph_ptr + i)->f, "独立タグ非見出語") && /* 「の」 */
		check_feature((ptr->mrph_ptr + i)->f, "意味有")) {
		ptr->head_ptr = ptr->mrph_ptr + i;
		assign_cfeature(&(ptr->head_ptr->f), "文節主辞", FALSE);
		return;
	    }
	}
    }

    /* 付属語しかない場合 */
    ptr->head_ptr = ptr->mrph_ptr + ptr->mrph_num - 1;
}

/*==================================================================*/
      int calc_bnst_length(SENTENCE_DATA *sp, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int j;
    MRPH_DATA *m_ptr;

    b_ptr->length = 0;
    for (j = 0, m_ptr = b_ptr->mrph_ptr; j < b_ptr->mrph_num; j++, m_ptr++) {
	if (Language == CHINESE) {
	    b_ptr->length += strlen(m_ptr->Goi2) * 2 / 3;
	}
	else {
	    b_ptr->length += strlen(m_ptr->Goi2);
	}

	if (b_ptr->length > BNST_LENGTH_MAX) {
	    fprintf(stderr, ";; Too big bunsetsu (%s %s...)!\n", 
		    sp->Comment ? sp->Comment : "", b_ptr->mrph_ptr);
	    return FALSE;
	}
    }
    return TRUE;
}

/*==================================================================*/
		 int make_bunsetsu(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    MRPH_DATA	*m_ptr;
    BNST_DATA	*b_ptr = NULL;
    
    sp->Bnst_num = 0;
    sp->Max_New_Bnst_num = 0;

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
	if (check_feature(m_ptr->f, "文節始")) {
	    if ((b_ptr = init_bnst(sp, m_ptr)) == NULL) return FALSE;
	}
	b_ptr->mrph_num++;
    }

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	if (calc_bnst_length(sp, b_ptr) == FALSE) {
	    return FALSE;
	}
    }
    return TRUE;
}

/*==================================================================*/
	       int make_bunsetsu_pm(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char *cp;
    MRPH_DATA	*m_ptr;
    BNST_DATA	*b_ptr = sp->bnst_data;

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
	if (Bnst_start[i]) {
	    if (i != 0) b_ptr++;
	    b_ptr->type = IS_BNST_DATA;
	    b_ptr->num = b_ptr-sp->bnst_data;
	    b_ptr->mrph_ptr = m_ptr;
	    b_ptr->mrph_num = 1;
	    b_ptr->cpm_ptr = NULL;
	    b_ptr->voice = 0;
	    b_ptr->pred_b_ptr = NULL;
	    for (j = 0, cp = b_ptr->SCASE_code; j < SCASE_CODE_SIZE; j++, cp++)
		*cp = 0;
	    /* clear_feature(&(b_ptr->f));
	       mainの文ごとのループの先頭で処理に移動 */
	}
	else {
	    b_ptr->mrph_num++;
	}
    }

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	if (OptReadFeature) {
	    b_ptr->f = Input_bnst_feature[i];
	}
	assign_cfeature(&(b_ptr->f), "解析済", FALSE);
	if (calc_bnst_length(sp, b_ptr) == FALSE) {
	    return FALSE;
	}
    }
    return TRUE;
}

/*==================================================================*/
	   void push_tag_units(TAG_DATA *tp, MRPH_DATA *mp)
/*==================================================================*/
{
    if (check_feature(mp->f, "非独立タグ接頭辞")) {
	if (tp->settou_num == 0) {
	    tp->settou_ptr = mp;
	}
	tp->settou_num++;
    }
    else if (check_feature(mp->f, "自立") || 
	     check_feature(mp->f, "独立タグ接頭辞") || 
	     check_feature(mp->f, "独立タグ接尾辞") || 
	     check_feature(mp->f, "独立タグ非見出語")) {
	if (tp->jiritu_num == 0) {
	    tp->jiritu_ptr = mp;
	}
	tp->jiritu_num++;
    }
    else {
	if (tp->fuzoku_num == 0) {
	    tp->fuzoku_ptr = mp;
	}
	tp->fuzoku_num++;
    }
    tp->mrph_num++;
}

/*==================================================================*/
	     void after_make_tag_units(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    TAG_DATA *tp;

    for (i = 0; i < sp->Tag_num; i++) {
	tp = sp->tag_data + i;

	tp->type = IS_TAG_DATA;

	decide_head_ptr((BNST_DATA *)tp);

	if (OptReadFeature) {
	    tp->f = Input_tag_feature[i];
	    read_annotation(sp, tp);
	}
	else {
	    tp->c_cpm_ptr = NULL;
	}

	/* BNST_DATAにcastしている tricky? */
	get_bnst_code_all((BNST_DATA *)tp);

	if (tp->inum != 0) {
	    assign_cfeature(&(tp->f), "文節内", FALSE); /* case_analysis.rule で使っている */
	    assign_cfeature(&(tp->f), "係:文節内", FALSE);
	}
	else {
	    /* headのときは文節のfeatureをコピー */
	    /* <文頭>, <文末>もつくが、文頭の文節が2タグ単位以上もつ場合は、
	       <文頭>のつく位置が間違っているので下で修正する */
	    copy_feature(&(tp->f), tp->b_ptr->f);
	    delete_cfeature(&(tp->f), "サ変"); /* <サ変>は文節とタグ単位では異なる */

	    /* 形式名詞「の」に用言がコピーされるので削除 */
	    if (check_feature(tp->head_ptr->f, "独立タグ非見出語")) {
		delete_cfeature(&(tp->f), "用言");
	    }
	}

	/* 各タグ単位の長さを計算しておく */
	calc_bnst_length(sp, (BNST_DATA *)tp);
    }

    /* <文頭>の修正 */
    if (sp->bnst_data->tag_num > 1) {
	delete_cfeature(&((sp->bnst_data->tag_ptr + sp->bnst_data->tag_num - 1)->f), "文頭");
	assign_cfeature(&(sp->tag_data->f), "文頭", FALSE);
    }

    /* タグ単位ルールを適用する */
    assign_general_feature(sp->tag_data, sp->Tag_num, TagRuleType, FALSE, FALSE);

    /* NTTコードをfeatureに表示 */
    sm2feature(sp);
}

/*==================================================================*/
       void make_tag_unit_set_inum(SENTENCE_DATA *sp, int num)
/*==================================================================*/
{
    int j, count = 0;

    for (j = num - 2; j >= 0; j--) {
	(sp->tag_data + j)->inum = ++count;
	if ((sp->tag_data + j)->bnum >= 0) {
	    break;
	}
    }
}

/*==================================================================*/
		void make_tag_units(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char *flag;
    MRPH_DATA *mp;
    TAG_DATA *tp = NULL;
    BNST_DATA *bp = sp->bnst_data, *pre_bp;

    sp->Tag_num = 0;

    for (i = 0; i < sp->Mrph_num; i++) {
	mp = sp->mrph_data + i;
	flag = check_feature(mp->f, "タグ単位始");

	/* 文節始まりの形態素だけど<タグ単位始>がついていない場合も許す */
	if (flag || 
	    (bp != NULL && bp->mrph_ptr == mp)) {
	    tp = sp->tag_data + sp->Tag_num;

	    if (flag == NULL) {
		fprintf(stderr, ";; morpheme %d must be <タグ単位始>! (%s)\n", i, 
			sp->KNPSID ? sp->KNPSID : "?");
	    }

	    memset(tp, 0, sizeof(TAG_DATA));
	    tp->num = sp->Tag_num;
	    tp->mrph_ptr = mp;

	    /* 文節区切りと一致するとき */
	    if (bp != NULL && bp->mrph_ptr == tp->mrph_ptr) {
		/* 遡ってinumを付与 */
		if (sp->Tag_num > 0 && (tp - 1)->bnum < 0) {
		    make_tag_unit_set_inum(sp, sp->Tag_num);
		}
		tp->bnum = bp->num;
		tp->b_ptr = bp;		/* タグ単位から文節へマーク */
		bp->tag_ptr = tp;	/* 文節からタグ単位へマーク */
		bp->tag_num = 1;
		pre_bp = bp;
		if (bp->num < sp->Bnst_num - 1) {
		    bp++;
		}
		else {
		    /* 最後の文節が終わった */
		    bp = NULL;
		}
	    }
	    else {
		tp->bnum = -1;
		tp->b_ptr = pre_bp;
		pre_bp->tag_num++;
	    }
	    sp->Tag_num++;
	}
	push_tag_units(tp, mp);
    }

    if ((sp->tag_data + sp->Tag_num - 1)->bnum < 0) {
	make_tag_unit_set_inum(sp, sp->Tag_num);
    }

    after_make_tag_units(sp);
}

/*==================================================================*/
	      void make_tag_units_pm(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    MRPH_DATA *mp;
    TAG_DATA *tp = sp->tag_data;
    BNST_DATA *bp = sp->bnst_data, *pre_bp;

    for (i = 0; i < sp->Mrph_num; i++) {
	mp = sp->mrph_data + i;

	if (Tag_start[i]) {
	    if (i != 0) tp++;

	    if (check_feature(mp->f, "タグ単位始") == NULL) {
		fprintf(stderr, ";; morpheme %d must be <タグ単位始>! (%s)\n", i, 
			sp->KNPSID ? sp->KNPSID : "?");
	    }

	    memset(tp, 0, sizeof(TAG_DATA));
	    tp->num = tp - sp->tag_data;
	    tp->mrph_ptr = mp;

	    /* 文節区切りと一致するとき */
	    if (bp != NULL && bp->mrph_ptr == tp->mrph_ptr) {
		/* 遡ってinumを付与 */
		if (tp->num > 0 && (tp - 1)->bnum < 0) {
		    make_tag_unit_set_inum(sp, tp->num);
		}
		tp->bnum = bp->num;
		tp->b_ptr = bp;		/* タグ単位から文節へマーク */
		bp->tag_ptr = tp;	/* 文節からタグ単位へマーク */
		bp->tag_num = 1;
		pre_bp = bp;
		if (bp->num < sp->Bnst_num - 1) {
		    bp++;
		}
		else {
		    /* 最後の文節が終わった */
		    bp = NULL;
		}
	    }
	    else {
		tp->bnum = -1;
		tp->b_ptr = pre_bp;
		pre_bp->tag_num++;
	    }
	}
	push_tag_units(tp, mp);
    }

    if ((sp->tag_data + sp->Tag_num - 1)->bnum < 0) {
	make_tag_unit_set_inum(sp, sp->Tag_num);
    }

    after_make_tag_units(sp);
}


/*==================================================================*/
	     void dpnd_info_to_tag_pm(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 係り受けに関する種々の情報を DPND から TAG_DATA にコピー (解析済版) */

    int		i;

    for (i = 0; i < sp->Tag_num; i++) {
	(sp->tag_data + i)->dpnd_head = Tag_dpnd[i];
	(sp->tag_data + i)->dpnd_type = Tag_type[i];
    }
}

/*====================================================================
				 END
====================================================================*/

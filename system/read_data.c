/*====================================================================

		形態素解析列の読み込み，文節へのまとめ

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

/* Server Client extention */
extern FILE  *Infp;
extern FILE  *Outfp;
extern int   OptMode;

int Bnst_start[MRPH_MAX];
int ArticleID = 0;
int preArticleID = 0;

extern char CorpusComment[BNST_MAX][DATA_LEN];

/*==================================================================*/
     void lexical_disambiguation(MRPH_DATA *m_ptr, int homo_num)
/*==================================================================*/
{
    int i, j, k, flag, pref_mrph, pref_rule;
    int real_homo_num;
    int uniq_flag[30];		/* 他と品詞が異なる形態素なら 1 */
    int matched_flag[10];	/* いずれかの形態素とマッチした
				   ルール内形態素パターンに 1 */
    HomoRule	*r_ptr;
    MRPH_DATA	*loop_ptr, *loop_ptr2;
    char fname[256];

    /* 品詞(細分類)が異なる形態素だけを残し，uniq_flagを1にする */

    uniq_flag[0] = 1;
    real_homo_num = 1;
    for (i = 1; i < homo_num; i++) {
	loop_ptr = m_ptr + i;
	uniq_flag[i] = 1;
	for (j = 0; j < i; j++) {
	    loop_ptr2 = m_ptr + j;

	    /* 読み以外すべて同じ --> 無視 */
	    if (loop_ptr2->Hinshi == loop_ptr->Hinshi &&
		loop_ptr2->Bunrui == loop_ptr->Bunrui &&
		str_eq(loop_ptr2->Goi, loop_ptr->Goi) &&
		loop_ptr2->Katuyou_Kata == loop_ptr->Katuyou_Kata &&
		loop_ptr2->Katuyou_Kei == loop_ptr->Katuyou_Kei) {
		uniq_flag[i] = 0;
		break;		    
	    }
	    /* 原形,活用型,活用形のいずれかだけ違う --> 無視(将来扱う) */
	    else if (loop_ptr2->Hinshi == loop_ptr->Hinshi &&
		     loop_ptr2->Bunrui == loop_ptr->Bunrui &&
		     (!str_eq(loop_ptr2->Goi, loop_ptr->Goi) ||
		      loop_ptr2->Katuyou_Kata != loop_ptr->Katuyou_Kata ||
		      loop_ptr2->Katuyou_Kei != loop_ptr->Katuyou_Kei)) {
		uniq_flag[i] = 0;
		break;
	    }
	}
	if (uniq_flag[i]) real_homo_num++;
    }

    /* 実質的同形異義語がなければ何も処理はしない */

    if (real_homo_num == 1) return;


    /* 多義性をマークするfeatureを与える */

    for (i = 0; i < homo_num; i++) {
	if (uniq_flag[i] == 0) continue;
	sprintf(fname, "品曖-%s", 
		Class[(m_ptr+i)->Hinshi][(m_ptr+i)->Bunrui].id);
	assign_cfeature(&(m_ptr->f), fname);

	/* 固有名詞以外には"その他"をふっておく */
	if (m_ptr->Bunrui != 3 && /* 固有名詞 */
	    m_ptr->Bunrui != 4 && /* 地名 */
	    m_ptr->Bunrui != 5 && /* 人名 */
	    m_ptr->Bunrui != 6) /* 組織名 */
	    assign_cfeature(&(m_ptr->f), "品曖-その他");
    }

    /* ルール (mrph_homo.rule)に従って優先する形態素を選択
       ※ 同形異義語数とルール中の形態素数が同じことが条件
          各同形異義語がルール中の形態素のいずれかにマッチすればよい
	  ルールの最初の形態素にマッチしたものを優先(pref_mrph が記憶)
    */

    flag = FALSE;
    pref_mrph = 0;
    pref_rule = 0;
    for (i = 0, r_ptr = HomoRuleArray; i < CurHomoRuleSize; i++, r_ptr++) {
	if (r_ptr->pattern->mrphsize != real_homo_num) continue;
	pref_mrph = 0;
	for (k = 0; k < r_ptr->pattern->mrphsize; k++) matched_flag[k] = FALSE;
	for (j = 0, loop_ptr = m_ptr; j < homo_num; j++, loop_ptr++) {
	    if (uniq_flag[j] == 0) continue;
	    flag = FALSE;
	    for (k = 0; k < r_ptr->pattern->mrphsize; k++) {
		if (matched_flag[k]) continue;
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
	    pref_rule = i;
	    break;
	}
    }

    if (flag == TRUE) {

	if (0 && OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Lexical Disambiguation "
		    "(%dth mrph -> %dth homo by %dth rule : %s :", 
		    m_ptr - mrph_data, pref_mrph, pref_rule, 
		    (m_ptr+pref_mrph)->Goi2);
	    for (i = 0, loop_ptr = m_ptr; i < homo_num; i++, loop_ptr++)
		if (uniq_flag[i]) 
		    fprintf(Outfp, " %s", 
			    Class[loop_ptr->Hinshi][loop_ptr->Bunrui].id);
	    fprintf(Outfp, ")\n");
	}

	/* pref_mrph番目のデータをコピー */
	strcpy(m_ptr->Goi2, (m_ptr+pref_mrph)->Goi2);
	strcpy(m_ptr->Yomi, (m_ptr+pref_mrph)->Yomi);
	strcpy(m_ptr->Goi, (m_ptr+pref_mrph)->Goi);
	m_ptr->Hinshi = (m_ptr+pref_mrph)->Hinshi;
	m_ptr->Bunrui = (m_ptr+pref_mrph)->Bunrui;
	m_ptr->Katuyou_Kata = (m_ptr+pref_mrph)->Katuyou_Kata;
	m_ptr->Katuyou_Kei = (m_ptr+pref_mrph)->Katuyou_Kei;

	assign_feature(&(m_ptr->f), &((HomoRuleArray + pref_rule)->f), 
		     m_ptr);

    } else {

	if (1 || OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, ";; Cannot disambiguate lexical ambiguities"
		    " (%dth mrph : %s ?", m_ptr - mrph_data,
		    (m_ptr+pref_mrph)->Goi2);
	    for (i = 0, loop_ptr = m_ptr; i < homo_num; i++, loop_ptr++)
		if (uniq_flag[i]) 
		    fprintf(Outfp, " %s", 
			    Class[loop_ptr->Hinshi][loop_ptr->Bunrui].id);
	    fprintf(Outfp, ")\n");
	}
    }
}

/*==================================================================*/
		       int read_mrph(FILE *fp)
/*==================================================================*/
{
    U_CHAR input_buffer[1024+IMI_MAX];
    U_CHAR imi_buffer[IMI_MAX];
    MRPH_DATA  *m_ptr = mrph_data, *ptr;
    int homo_num, offset, mrph_item, i,len;

    preArticleID = ArticleID;
    ArticleID = 0;
    Mrph_num = 0;
    homo_num = 0;
    Comment[0] = '\0';
    PM_Memo[0] = '\0';

    while (1) {

	if (fgets(input_buffer, 1024+IMI_MAX, fp) == NULL) return EOF;

	/* Server モードの場合は 注意 \r\n になる*/
	if (OptMode == SERVER_MODE) {
	  len = strlen(input_buffer);
	  if (len > 2 && input_buffer[len-1] == '\n' && input_buffer[len-2] == '\r') {
	    input_buffer[len-2] = '\n';
	    input_buffer[len-1] = '\0';
	  }

	  if (input_buffer[0] == EOf) 
	    return EOF;
	} 

	if (input_buffer[strlen(input_buffer)-1] != '\n') {
	    fprintf(stderr, "Too long mrph <%s> !\n", input_buffer);
	    return FALSE;
	}

	/* -i によるコメント行 */
	if ( OptIgnoreChar && *input_buffer == OptIgnoreChar ) {
	    fprintf(Outfp, "%s", input_buffer);
	    fflush(Outfp);
	    continue;
	}

	/* # による正規のコメント行 */

	if (input_buffer[0] == '#') {
	    input_buffer[strlen(input_buffer)-1] = '\0';
	    strcpy(Comment, input_buffer);

	    /* 文章が変わったら固有名詞スタックをクリア */
	    sscanf(Comment, "# S-ID:%d", &ArticleID);
	    if (ArticleID && ArticleID != preArticleID)
		clearNE();
	}

	/* 解析済みの場合 */

	else if (Mrph_num == 0 && input_buffer[0] == '*') {
	    OptAnalysis = OPT_PM;
	    Bnst_num = 0;
	    for (i = 0; i < MRPH_MAX; i++) Bnst_start[i] = 0;
	    if (sscanf(input_buffer, "* %d%c", 
		       &Best_mgr.dpnd.head[Bnst_num],
		       &Best_mgr.dpnd.type[Bnst_num]) != 2)  {
		fprintf(stderr, "Invalid input <%s> !\n", input_buffer);
		return FALSE;
	    }
	    Bnst_start[Mrph_num] = 1;
	    Bnst_num++;
	}
	else if (input_buffer[0] == '*') {
	    if (OptAnalysis != OPT_PM || 
		sscanf(input_buffer, "* %d%c", 
		       &Best_mgr.dpnd.head[Bnst_num],
		       &Best_mgr.dpnd.type[Bnst_num]) != 2) {
		fprintf(stderr, "Invalid input <%s> !\n", input_buffer);
		return FALSE;
	    }
	    Bnst_start[Mrph_num] = 1;
	    Bnst_num++;
	}	    

	/* 文末 */

	else if (str_eq(input_buffer, "EOS\n")) {
	    
	    if (homo_num) {	/* 前に同形異義語セットがあれば処理する */
		lexical_disambiguation(m_ptr - homo_num - 1, homo_num + 1);
		Mrph_num -= homo_num;
		m_ptr -= homo_num;
		homo_num = 0;
	    }
	    return TRUE;
	}

	/* 通常の形態素 */

	else {
	    
	    if (input_buffer[0] != '@' && homo_num) {

		/* 同形異義語マークがなければ，前に同形異義語セットがあれば
	           lexical_disambiguationを呼んで処理 */		   

		lexical_disambiguation(m_ptr - homo_num - 1, homo_num + 1);
		Mrph_num -= homo_num;
		m_ptr -= homo_num;
		homo_num = 0;
	    }

	    /* 最大数を越えないようにチェック */
	    if (Mrph_num >= MRPH_MAX) {
		fprintf(stderr, "Too many mrph (%s %s%s...)!\n", 
			Comment, mrph_data, mrph_data+1);
		Mrph_num = 0;
		return readtoeos(fp);
	    }

	    /* 形態素情報 :
	       語彙(活用形) 読み 語彙(原型) 
	       品詞(+番号) 細分類(+番号) 活用型(+番号) 活用形(+番号) 
	       意味情報
	    */

	    offset = (input_buffer[0] == '@') ? 2 : 0;
	    mrph_item = sscanf(input_buffer + offset,
			       "%s %s %s %*s %d %*s %d %*s %d %*s %d %s", 
			       m_ptr->Goi2, m_ptr->Yomi, m_ptr->Goi, 
			       &(m_ptr->Hinshi), &(m_ptr->Bunrui),
			       &(m_ptr->Katuyou_Kata), &(m_ptr->Katuyou_Kei),
			       m_ptr->Imi);

	    if (mrph_item == 8) {
		;
	    }
	    else if (mrph_item == 7) {
		strcpy(m_ptr->Imi, "NIL");
	    }
	    else {
		fprintf(stderr, "Invalid input (%d items)<%s> !\n", 
			mrph_item, input_buffer);
		if (Comment[0]) fprintf(stderr, "(%s)\n", Comment);
		return FALSE;
	    }   
	    m_ptr->type = 0;
	    /* clear_feature(&(m_ptr->f)); 
	       mainの文ごとのループの先頭で処理に移動 */

	    /* 同形異義語は一旦 mrph_data にいれる */
	    if (input_buffer[0] == '@')	homo_num++;

	    Mrph_num++;
	    m_ptr++;
	}
    }
}

/*==================================================================*/
		       int readtoeos(FILE *fp)
/*==================================================================*/
{
    U_CHAR input_buffer[1024+IMI_MAX];

    while (1) {
	if (fgets(input_buffer, 1024+IMI_MAX, fp) == NULL) return EOF;
	if (str_eq(input_buffer, "EOS\n")) return FALSE;
    }
}

/*==================================================================*/
	    void change_mrph(MRPH_DATA *m_ptr, FEATURE *f)
/*==================================================================*/
{
    char *cp;
    char h_buffer[62], b_buffer[62], kata_buffer[62], kei_buffer[62];
    int i, num;

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
		       void assign_mrph_prop()
/*==================================================================*/
{
    int i, j, k, match_length;
    MrphRule	*r_ptr;
    MRPH_DATA	*m_ptr;
    
    for (j = 0, r_ptr = MrphRuleArray; j < CurMrphRuleSize; j++, r_ptr++) {
	for (i = 0; i < Mrph_num; i++) {
	    m_ptr = mrph_data + i;

	    if ((match_length = regexpmrphrule_match(r_ptr, m_ptr)) != -1) {
		for (k = i; k < i + match_length; k++) {
		    m_ptr = mrph_data + k;
		    assign_feature(&(m_ptr->f), &(r_ptr->f), m_ptr);
		}
	    }
	}
    }
}

/*==================================================================*/
	 void assign_mrph_feature(MrphRule *r_ptr, int size)
/*==================================================================*/
{
    int i, j, k, match_length;
    MRPH_DATA	*m_ptr;

    for (j = 0; j < size; j++, r_ptr++) {
	for (i = 0; i < Mrph_num; i++) {
	    m_ptr = mrph_data + i;

	    if ((match_length = regexpmrphrule_match(r_ptr, m_ptr)) != -1) {
		for (k = i; k < i + match_length; k++) {
		    m_ptr = mrph_data + k;
		    assign_feature(&(m_ptr->f), &(r_ptr->f), m_ptr);
		}
	    }
	}
    }
}

/*==================================================================*/
	       BNST_DATA *init_bnst(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    int i;
    char *cp;
    BNST_DATA *b_ptr;

    b_ptr = bnst_data + Bnst_num;
    b_ptr->num = Bnst_num;
    Bnst_num++;
    if (Bnst_num > BNST_MAX) {
	fprintf(stderr, "Too many bnst (%s %s%s...)!\n", 
		Comment, mrph_data, mrph_data+1);
	Bnst_num = 0;
	return NULL;
    }

    b_ptr->mrph_ptr = m_ptr;

    b_ptr->mrph_num = 0;
    b_ptr->settou_num = 0;
    b_ptr->jiritu_num = 0;
    b_ptr->fuzoku_num = 0;
    b_ptr->settou_ptr = NULL;
    b_ptr->jiritu_ptr = NULL;
    b_ptr->fuzoku_ptr = NULL;

    b_ptr->Jiritu_Go[0] = '\0';
    b_ptr->BGH_num = 0;
    b_ptr->SM_num = 0;

    b_ptr->para_key_type = PARA_KEY_O;
    b_ptr->para_top_p = FALSE;
    b_ptr->para_type = PARA_NIL;
    b_ptr->to_para_p = FALSE;

    b_ptr->cpm_ptr = NULL;

    b_ptr->length = 0;
    b_ptr->space = 0;
    
    for (i = 0, cp = b_ptr->SCASE_code; i < 11; i++, cp++) *cp = 0;

    /* clear_feature(&(b_ptr->f));
       mainの文ごとのループの先頭で処理に移動 */

    CorpusComment[b_ptr->num][0] = '\0';

    return b_ptr;
}

/*==================================================================*/
	 void push_prfx(BNST_DATA *b_ptr, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    if (b_ptr->settou_num == 0) 
      b_ptr->settou_ptr = m_ptr;
    b_ptr->settou_num++;
    b_ptr->mrph_num++;
}

/*==================================================================*/
	 void push_indp(BNST_DATA *b_ptr, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    if (b_ptr->jiritu_num == 0)
	b_ptr->jiritu_ptr = m_ptr;

    if ((strlen(b_ptr->Jiritu_Go) + strlen(m_ptr->Goi)) >= WORD_LEN_MAX) {
	fprintf(stderr, ";; Too big Jiritu_Go (%s%s...)",
		b_ptr->Jiritu_Go, m_ptr->Goi);
    } else {
	strcat(b_ptr->Jiritu_Go, m_ptr->Goi);
    }
    b_ptr->jiritu_num++;
    b_ptr->mrph_num++;
}

/*==================================================================*/
	 void push_sufx(BNST_DATA *b_ptr, MRPH_DATA *m_ptr)
/*==================================================================*/
{
    if (b_ptr->fuzoku_num == 0) 
      b_ptr->fuzoku_ptr = m_ptr;
    b_ptr->fuzoku_num++;
    b_ptr->mrph_num++;
}

#define MRPH_PRFX 0	/* 形態素列を文節にまとめる際の状態フラグ */
#define MRPH_INDP 1
#define MRPH_SUFX 2

/*==================================================================*/
			 int make_bunsetsu()
/*==================================================================*/
{
    int prev_stat, now_stat;
    int i, j;
    MRPH_DATA	*m_ptr;
    BNST_DATA	*b_ptr = NULL;
    
    Bnst_num = 0;
    prev_stat = MRPH_SUFX;

    for (i = 0, m_ptr = mrph_data; i < Mrph_num; i++, m_ptr++) {

	if (check_feature(m_ptr->f, "←複合") ||
	    check_feature(m_ptr->f, "自立"))
	    now_stat = MRPH_INDP;
	else if (check_feature(m_ptr->f, "付属"))
	    now_stat = MRPH_SUFX;
	else if (check_feature(m_ptr->f, "接頭"))
	    now_stat = MRPH_PRFX;
	else
	    fprintf(Outfp, ";; Invalid input \n");

	/* ▼ 不適切な入力 (接尾辞ではじまる，接頭＋接尾)
	   接尾辞を自立語として扱う (暫定的) */

	if (i == 0 && now_stat == MRPH_SUFX) {
	    fprintf(Outfp, 
		    ";; Invalid input (prefix and suffix)\"%s%s ... \"!\n",
		    mrph_data[0].Goi2, mrph_data[1].Goi2);
	    /* return FALSE; */
	    now_stat = MRPH_INDP;
	} else if (prev_stat == MRPH_PRFX &&
		   now_stat == MRPH_SUFX) {
	    fprintf(Outfp, 
		    ";; Invalid input (prefix and suffix)\"... %s%s ... \"!\n",
		    mrph_data[i-1].Goi2, mrph_data[i].Goi2);
	    /* return FALSE; */
	    now_stat = MRPH_INDP;
	}

	/* 処理 */

	switch (now_stat) {
	case MRPH_PRFX:
	    switch (prev_stat) {
	    case MRPH_PRFX:
		push_prfx(b_ptr, m_ptr);
		break;
	    default:
		if ((b_ptr = init_bnst(m_ptr)) == NULL) return FALSE;
		push_prfx(b_ptr, m_ptr);
		break;
	    }
	    break;
	  case MRPH_INDP:
	    switch (prev_stat) {
	    case MRPH_PRFX:
		push_indp(b_ptr, m_ptr);
		break;
	    case MRPH_INDP:
		if (!check_feature(m_ptr->f, "←複合"))
		    if ((b_ptr = init_bnst(m_ptr)) == NULL) return FALSE;
		push_indp(b_ptr, m_ptr);
		break;
	    case MRPH_SUFX:
		if ((b_ptr = init_bnst(m_ptr)) == NULL) return FALSE;
		push_indp(b_ptr, m_ptr);
		break;
	    }
	    break;
	  case MRPH_SUFX:
	    if (m_ptr->type == BNST_RULE_SKIP)
	      b_ptr->mrph_num++;
	    else 
	      push_sufx(b_ptr, m_ptr);
	    break;
	}
	
	prev_stat = now_stat;
    }    

    for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++) {
	for (j = 0, m_ptr = b_ptr->mrph_ptr; j < b_ptr->mrph_num;
					     j++, m_ptr++) {
	    if ((b_ptr->length += strlen(m_ptr->Goi2)) >
		BNST_LENGTH_MAX) {
		fprintf(stderr, "Too big bnst (%s %s%s...)!\n", 
			Comment, b_ptr->mrph_ptr, b_ptr->mrph_ptr+1);
		return FALSE;
	    }
	}
    }
    return TRUE;
}

/*==================================================================*/
			int make_bunsetsu_pm()
/*==================================================================*/
{
    int i, j;
    char *cp;
    MRPH_DATA	*m_ptr;
    BNST_DATA	*b_ptr = bnst_data;

    for (i = 0, m_ptr = mrph_data; i < Mrph_num; i++, m_ptr++) {
	if (Bnst_start[i]) {
	    if (i != 0) b_ptr++;
	    b_ptr->mrph_ptr = m_ptr;
	    b_ptr->mrph_num = 1;
	    b_ptr->jiritu_ptr = m_ptr;
	    b_ptr->jiritu_num = 1;
	    b_ptr->settou_num = 0;
	    b_ptr->fuzoku_num = 0;
	    b_ptr->length = 0;
	    for (j = 0, cp = b_ptr->SCASE_code; j < 11; j++, cp++)
		*cp = 0;
	    /* clear_feature(&(b_ptr->f));
	       mainの文ごとのループの先頭で処理に移動 */
	} else {
	    b_ptr->mrph_num ++;
	    b_ptr->jiritu_num ++;
	}
    }
    
    for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++) {
	
	assign_cfeature(&(b_ptr->f), "解析済");

	for (j = 0, m_ptr = b_ptr->mrph_ptr; j < b_ptr->mrph_num;
					     j++, m_ptr++) {
	    if ((b_ptr->length += strlen(m_ptr->Goi2)) >
		BNST_LENGTH_MAX) {
		fprintf(stderr, "Too big bunsetsu (%s...)!\n", 
			b_ptr->mrph_ptr);
		return FALSE;
	    }
	}
    }
    return TRUE;
}

/*====================================================================
				 END
====================================================================*/

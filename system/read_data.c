/*====================================================================

		形態素解析列の読み込み，文節へのまとめ

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int Bnst_start[MRPH_MAX];
int ArticleID = 0;
int preArticleID = 0;

extern char CorpusComment[BNST_MAX][DATA_LEN];

/*==================================================================*/
void lexical_disambiguation(SENTENCE_DATA *sp, MRPH_DATA *m_ptr, int homo_num)
/*==================================================================*/
{
    int i, j, k, flag, pref_mrph, pref_rule;
    int real_homo_num;
    int uniq_flag[HOMO_MAX];		/* 他と品詞が異なる形態素なら 1 */
    int matched_flag[HOMO_MRPH_MAX];	/* いずれかの形態素とマッチした
					   ルール内形態素パターンに 1 */
    HomoRule	*r_ptr;
    MRPH_DATA	*loop_ptr, *loop_ptr2;
    char fname[SMALL_DATA_LEN2];

    /* 処理する最大数を越えていれば、最大数個だけチェックする */
    if (homo_num > HOMO_MAX) {
	homo_num = HOMO_MAX;
    }

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
	    /* 活用型,活用形のいずれかが違う (いる, くる, ゆく, ...) --> 無視 */
	    else if (loop_ptr2->Hinshi == loop_ptr->Hinshi &&
		     loop_ptr2->Bunrui == loop_ptr->Bunrui &&
		     str_eq(loop_ptr2->Goi, loop_ptr->Goi) &&
		      (loop_ptr2->Katuyou_Kata != loop_ptr->Katuyou_Kata ||
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

    assign_cfeature(&(m_ptr->f), "品曖");
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
	if (r_ptr->pattern->mrphsize > HOMO_MRPH_MAX) {
	    fprintf(stderr, ";; The number of Rule morphs is too large in HomoRule.\n");
	    exit(1);
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

    if (flag == TRUE) {

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

	/* pref_mrph番目のデータをコピー */
	strcpy(m_ptr->Goi2, (m_ptr+pref_mrph)->Goi2);
	strcpy(m_ptr->Yomi, (m_ptr+pref_mrph)->Yomi);
	strcpy(m_ptr->Goi, (m_ptr+pref_mrph)->Goi);
	m_ptr->Hinshi = (m_ptr+pref_mrph)->Hinshi;
	m_ptr->Bunrui = (m_ptr+pref_mrph)->Bunrui;
	m_ptr->Katuyou_Kata = (m_ptr+pref_mrph)->Katuyou_Kata;
	m_ptr->Katuyou_Kei = (m_ptr+pref_mrph)->Katuyou_Kei;
	strcpy(m_ptr->Imi, (m_ptr+pref_mrph)->Imi);

	assign_feature(&(m_ptr->f), &((HomoRuleArray + pref_rule)->f), 
		     m_ptr);

    } else {

	if (1 || OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, ";; Cannot disambiguate lexical ambiguities"
		    " (%dth mrph : %s ?", m_ptr - sp->mrph_data,
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
	      int read_mrph(SENTENCE_DATA *sp, FILE *fp)
/*==================================================================*/
{
    U_CHAR input_buffer[DATA_LEN];
    MRPH_DATA  *m_ptr = sp->mrph_data;
    int homo_num, offset, mrph_item, i, homo_flag;

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
		sscanf(input_buffer, "# S-ID:%d", &ArticleID);
		if (ArticleID && preArticleID && ArticleID != preArticleID) {
		    if (OptEllipsis) {
			ClearSentences(sp);
		    }
		}
		preArticleID = ArticleID;
	    }
	}

	/* 解析済みの場合 */

	else if (sp->Mrph_num == 0 && input_buffer[0] == '*') {
	    OptInput = OPT_PARSED;
	    if (OptEllipsis) {
		OptAnalysis = OPT_CASE2;
	    }
	    sp->Bnst_num = 0;
	    for (i = 0; i < MRPH_MAX; i++) Bnst_start[i] = 0;
	    if (sscanf(input_buffer, "* %d%c", 
		       &(sp->Best_mgr->dpnd.head[sp->Bnst_num]),
		       &(sp->Best_mgr->dpnd.type[sp->Bnst_num])) != 2)  {
		fprintf(stderr, ";; Invalid input <%s> !\n", input_buffer);
		OptInput = OPT_RAW;
		return readtoeos(fp);
	    }
	    Bnst_start[sp->Mrph_num] = 1;
	    sp->Bnst_num++;
	}
	else if (input_buffer[0] == '*') {
	    if (OptInput != OPT_PARSED || 
		sscanf(input_buffer, "* %d%c", 
		       &(sp->Best_mgr->dpnd.head[sp->Bnst_num]),
		       &(sp->Best_mgr->dpnd.type[sp->Bnst_num])) != 2) {
		fprintf(stderr, ";; Invalid input <%s> !\n", input_buffer);
		return readtoeos(fp);
	    }
	    Bnst_start[sp->Mrph_num] = 1;
	    sp->Bnst_num++;
	}	    

	/* 文末 */

	else if (str_eq(input_buffer, "EOS\n")) {
	    /* 形態素が一つもないとき */
	    if (sp->Mrph_num == 0) {
		return FALSE;
	    }

	    if (homo_num) {	/* 前に同形異義語セットがあれば処理する */
		lexical_disambiguation(sp, m_ptr - homo_num - 1, homo_num + 1);
		sp->Mrph_num -= homo_num;
		m_ptr -= homo_num;
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
	    
	    if (homo_num && homo_flag == 0) {

		/* 同形異義語マークがなければ，前に同形異義語セットがあれば
	           lexical_disambiguationを呼んで処理 */		   

		lexical_disambiguation(sp, m_ptr - homo_num - 1, homo_num + 1);
		sp->Mrph_num -= homo_num;
		m_ptr -= homo_num;
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
		fprintf(stderr, ";; Invalid input (%d items)<%s> !\n", 
			mrph_item, input_buffer);
		if (sp->Comment) fprintf(stderr, "(%s)\n", sp->Comment);
		return readtoeos(fp);
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
	    void change_mrph(MRPH_DATA *m_ptr, FEATURE *f)
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
				int mode, int break_mode, int direction)
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
				       &(r_ptr->f), s_m_ptr+i*direction+k);
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
				       &(r_ptr->f), s_m_ptr+i*direction+k);
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
			int mode, int break_mode, int direction)
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
		    for (k = 0; k < match_length; k++)
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction+k);
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
		    for (k = 0; k < match_length; k++)
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction+k);
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
			 int mode, int break_mode, int direction)
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
		    for (k = 0; k < match_length; k++)
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction+k);
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
		    for (k = 0; k < match_length; k++)
			assign_feature(&((s_b_ptr+i*direction+k)->f), 
				       &(r_ptr->f), s_b_ptr+i*direction+k);
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
     void assign_general_feature(void *data, int size, int flag)
/*==================================================================*/
{
    int i;
    void (*assign_function)();

    /* 形態素, タグ単位, 文節の場合分け */
    if (flag == MorphRuleType) {
	assign_function = assign_mrph_feature;
    }
    else if (flag == TagRuleType) {
	assign_function = assign_tag_feature;
    }
    else if (flag == BnstRuleType) {
	assign_function = assign_bnst_feature;
    }

    for (i = 0; i < GeneralRuleNum; i++) {
	/* タグ単位の場合は文節ルールでもよい */
	if ((GeneralRuleArray + i)->type == flag || 
	    (flag == TagRuleType && (GeneralRuleArray + i)->type == BnstRuleType)) {
	    assign_function((GeneralRuleArray+i)->RuleArray, 
			    (GeneralRuleArray+i)->CurRuleSize, 
			    data, size, 
			    (GeneralRuleArray+i)->mode, 
			    (GeneralRuleArray+i)->breakmode, 
			    (GeneralRuleArray+i)->direction);
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

    b_ptr->length = 0;
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

    for (i = ptr->mrph_num - 1; i >= 0 ; i--) {
	if (check_feature((ptr->mrph_ptr + i)->f, "付属")) {
	    /* カウンタなど、付属語であるが意味素をそこから得る場合 */
	    if (check_feature((ptr->mrph_ptr + i)->f, "有意味接尾辞") || 
		check_feature((ptr->mrph_ptr + i)->f, "独立語")) {
		ptr->head_ptr = ptr->mrph_ptr + i;
		return;
	    }
	}
	/* 最後の自立語 */
	else {
	    ptr->head_ptr = ptr->mrph_ptr + i;
	    return;
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

    for (j = 0, m_ptr = b_ptr->mrph_ptr; j < b_ptr->mrph_num; j++, m_ptr++) {
	if ((b_ptr->length += strlen(m_ptr->Goi2)) > BNST_LENGTH_MAX) {
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
	    b_ptr->num = b_ptr-sp->bnst_data;
	    b_ptr->mrph_ptr = m_ptr;
	    b_ptr->mrph_num = 1;
	    b_ptr->length = 0;
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
	assign_cfeature(&(b_ptr->f), "解析済");
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
    if (check_feature(mp->f, "接頭非独立語")) {
	if (tp->settou_num == 0) {
	    tp->settou_ptr = mp;
	}
	tp->settou_num++;
    }
    else if (check_feature(mp->f, "独立語")) {
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
		void make_tag_units(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, settou_num = 0, count;
    char *flag;
    MRPH_DATA *mp, *settou_ptr;
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
		    count = 0;
		    for (j = sp->Tag_num - 2; j >= 0; j--) {
			(sp->tag_data + j)->inum = ++count;
			if ((sp->tag_data + j)->bnum >= 0) {
			    break;
			}
		    }
		}
		tp->bnum = bp->num;
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
		pre_bp->tag_num++;
	    }
	    sp->Tag_num++;
	}
	push_tag_units(tp, mp);
    }

    if ((sp->tag_data + sp->Tag_num - 1)->bnum < 0) {
	count = 0;
	for (j = sp->Tag_num - 2; j >= 0; j--) {
	    (sp->tag_data + j)->inum = ++count;
	    if ((sp->tag_data + j)->bnum >= 0) {
		break;
	    }
	}
    }

    for (i = 0; i < sp->Tag_num; i++) {
	tp = sp->tag_data + i;

	decide_head_ptr((BNST_DATA *)tp);

	/* BNST_DATAにcastしている tricky? */
	get_bnst_code((BNST_DATA *)tp, USE_BGH);
	get_bnst_code((BNST_DATA *)tp, USE_NTT);

	if (tp->inum != 0) {
	    /* case_analysis.rule で使っている */
	    assign_cfeature(&(tp->f), "文節内");
	}

	/* 各タグ単位の長さを計算しておく */
	calc_bnst_length(sp, (BNST_DATA *)tp);
    }

    /* <文頭>, <文末> */
    assign_cfeature(&(sp->tag_data->f), "文頭");
    if (sp->Tag_num > 0) {
	assign_cfeature(&((sp->tag_data + sp->Tag_num - 1)->f), "文末");
    }
    else {
	assign_cfeature(&(sp->tag_data->f), "文末");
    }

    /* 文節ルールを適用する */
    assign_general_feature(sp->tag_data, sp->Tag_num, TagRuleType);
}

/*====================================================================
				 END
====================================================================*/

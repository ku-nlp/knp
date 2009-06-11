/*====================================================================

			     鈎括弧の処理
				   
                                               S.Kurohashi 1996. 4.18
                                               S.Ozaki     1994.12. 1

    $Id$

====================================================================*/
#include "knp.h"

QUOTE_DATA quote_data;

#define PAREN_B "（"
#define PAREN_E "）"
#define PAREN_COMMENT_TEMPLATE "括弧始:（ 括弧終:） 括弧位置:"

/*==================================================================*/
		  void init_quote(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < QUOTE_MAX; i++) {
	quote_data.in_num[i] = -1;
	quote_data.out_num[i] = -1;    
    }

    for (i = 0; i < sp->Bnst_num; i++) {
	for (j = 0; j < sp->Bnst_num; j++) {
	    Quote_matrix[i][j] = 1;
	    Chi_quote_start_matrix[i][j] = -1;
	    Chi_quote_end_matrix[i][j] = -1;
	}
    }
}

/*==================================================================*/
                         void print_quote()
/*==================================================================*/
{
    int i;

    for (i = 0; quote_data.in_num[i] >= 0; i++) {
	fprintf(Outfp,"Quote_num %d in %d out %d \n", i,
		quote_data.in_num[i], quote_data.out_num[i]);
    }
}

/*==================================================================*/
		  int check_quote(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /*
      "．．「○○」．．．「××」．．"  
      "．．「○○「×××」○○」．．" 
      "．．「○○○○○○○○○○○○"  
      "××××××××××××」．．"  などのパターンに対処
    */

    int i, k, stack[QUOTE_MAX], s_num, quote_p = FALSE;

    k = 0;
    s_num = -1;

    for (i = 0; i < sp->Bnst_num; i++) {

	if (check_feature(sp->bnst_data[i].f, "括弧始")) {
	    /* 最大数を越えないかチェック(最後の要素が番人なので、それを変えては
	       いけない) */
	    if (k >= QUOTE_MAX-1) {
		fprintf(stderr, ";; Too many quote (%s) ...\n", sp->Comment ? sp->Comment : "");
		return CONTINUE;
	    }
	    s_num ++;
	    stack[s_num] = k;
	    quote_data.in_num[k] = i;
	    k++;

	    /* 「『‥ を扱うため上のことを繰り返す */
	    if (check_feature(sp->bnst_data[i].f, "括弧始２")) {
		if (k >= QUOTE_MAX-1) {
		    fprintf(stderr, ";; Too many quote (%s) ...\n", sp->Comment ? sp->Comment : "");
		    return CONTINUE;
		}
		s_num ++;
		stack[s_num] = k;
		quote_data.in_num[k] = i;
		k++;
	    }
	}
	if (check_feature(sp->bnst_data[i].f, "括弧終")) {
	    if (s_num == -1) {
		if (k >= QUOTE_MAX-1) {
		    fprintf(stderr, ";; Too many quote (%s) ...\n", sp->Comment ? sp->Comment : "");
		    return CONTINUE;
		}
		quote_data.out_num[k] = i; /* 括弧終が多い場合 */
		k++;
	    } else {
		quote_data.out_num[stack[s_num]] = i;
		s_num--;
	    }


	    /* ‥』」 を扱うため上のことを繰り返す */
	    if (check_feature(sp->bnst_data[i].f, "括弧終２")) {
		if (s_num == -1) {
		    if (k >= QUOTE_MAX-1) {
			fprintf(stderr, ";; Too many quote (%s) ...\n", sp->Comment ? sp->Comment : "");
			return CONTINUE;
		    }
		    quote_data.out_num[k] = i; /* 括弧終が多い場合 */
		    k++;
		} else {
		    quote_data.out_num[stack[s_num]] = i;
		    s_num--;
		}
	    }
	}		  
    }

    for (i = 0; i < k; i++) {

	/* 括弧が閉じていない場合は, 文頭または文末を境界に */	

	if (quote_data.in_num[i] == -1) 
	    quote_data.in_num[i] = 0;
	if (quote_data.out_num[i] == -1)
	    quote_data.out_num[i] = sp->Bnst_num - 1;

	/* 一文節の括弧を考慮しない場合
	if (quote_data.in_num[i] != quote_data.out_num[i])
	quote_p = TRUE;
	*/

	quote_p = TRUE;
    }

    return quote_p;
}

/*==================================================================*/
		  void mask_quote(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, l, start, end;

    for (k = 0; quote_data.in_num[k] >= 0; k++) {

	start = quote_data.in_num[k];
	end = quote_data.out_num[k];

	if (start == end) continue;	/* １文節だけの括弧は無視 */

	/* 括弧の上のマスク */

	for (i = 0; i < start; i++) {
	    for (j = start; j < end; j++)
		Quote_matrix[i][j] = 0;

	    /* 
	       括弧内末尾の文節には連格,連体,ノ格,同格連体,括弧並列のみ
	       係れるとする．
	       		例) 「私の「本当の気持ち」は…」

	       用言に連用が係ることも稀にはあるが，それを許すと通常の場合の
	       解析誤りが大量に生まれるので無視する．
	       		例) 「彼が「東京にいった」ことは…」
	    */

	    if (Quote_matrix[i][end] &&
		(check_feature(sp->bnst_data[i].f, "係:連格") ||
		 check_feature(sp->bnst_data[i].f, "係:連体") ||
		 check_feature(sp->bnst_data[i].f, "係:ノ格") ||
		 check_feature(sp->bnst_data[i].f, "係:同格連体") ||
		 check_feature(sp->bnst_data[i].f, "係:括弧並列")))
		;
	    else 
		Quote_matrix[i][end] = 0;
	}

	/* 括弧の右のマスク */

	for (i = start; i < end; i++)
	    for (j = end + 1; j < sp->Bnst_num; j++)
		Quote_matrix[i][j] = 0;

	/* 括弧内の句点の右上のマスク 
	   (句点の右は開けておく --> 次の文末とPになる) */

	for (l = start; l < end; l++)
	    if (check_feature(sp->bnst_data[l].f, "係:文末"))
		for (i = start; i < l; i++)
		    for (j = l + 1; j <= end; j++)
			Quote_matrix[i][j] = 0;
    }
}

/*==================================================================*/
		  void mask_quote_for_chi(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, start, end;

    for (k = 0; quote_data.in_num[k] >= 0; k++) {

	start = quote_data.in_num[k];
	end = quote_data.out_num[k];

	if (start == end) continue;	/* １文節だけの括弧は無視 */

	if ((!OptChiPos && check_feature((sp->bnst_data+start)->f, "PU") && check_feature((sp->bnst_data+end)->f, "PU")) || OptChiPos) {
	    /* 括弧の上のマスク */

	    for (i = 0; i < start; i++) {
		Quote_matrix[i][start] = 0;
		Quote_matrix[i][end] = 0;
	    }

	    /* 括弧の右のマスク */

	    for (i = end + 1; i < sp->Bnst_num; i++) {
		Quote_matrix[start][i] = 0;
		Quote_matrix[end][i] = 0;
	    }

	    Quote_matrix[start][end] = 0;

	    for (j = start; j <= end; j++) {
		for (i = j; i <= end; i++) {
		    Chi_quote_start_matrix[j][i] = start;
		    Chi_quote_end_matrix[j][i] = end;
		}
	    }
	    Chi_quote_start_matrix[start][end] = -1;
	    Chi_quote_end_matrix[start][end] = -1;
	}
    }
}

/*==================================================================*/
		     int quote(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int quote_p = FALSE;

    init_quote(sp);

   if (Language != CHINESE ||
	(Language == CHINESE && !OptChiGenerative)) {
	if ((quote_p = check_quote(sp))) {	/* 鈎括弧の検出 */
	    if (quote_p == CONTINUE) return quote_p;

	    if (OptDisplay == OPT_DEBUG && Language != CHINESE) print_quote();

	    if (Language != CHINESE) {
		mask_quote(sp);			/* 行列の書き換え */
	    }
	    else {
		mask_quote_for_chi(sp); // mask quote for Chinese
	    }
	}
    }

    return quote_p;
}

/*==================================================================*/
	void add_comment(SENTENCE_DATA *sp, char *add_string)
/*==================================================================*/
{
    if (sp->Comment) { /* 既存のコメントと結合 */
	char *orig_comment = sp->Comment;
	sp->Comment = (char *)malloc_data(strlen(sp->Comment) + strlen(add_string) + 2, "add_comment");
	sprintf(sp->Comment, "%s %s", orig_comment, add_string);
	free(orig_comment);
    }
    else { /* 新たなコメント */
	sp->Comment = strdup(add_string);
    }
}

/*==================================================================*/
 int process_input_paren(SENTENCE_DATA *sp, SENTENCE_DATA **paren_spp)
/*==================================================================*/
{
    int i, j, paren_mrph_num = 0, paren_level = 0, paren_start, *paren_table, paren_num = 0;
    MRPH_DATA  *m_ptr = sp->mrph_data;
    SENTENCE_DATA next_sentence_data;

    paren_table = (int *)malloc_data(sizeof(int) * sp->Mrph_num, "process_input_paren");
    memset(paren_table, 0, sizeof(int) * sp->Mrph_num); /* initialization */

    /* 括弧チェック */
    for (i = 0; i < sp->Mrph_num; i++) {
	if (!strcmp((m_ptr + i)->Goi, PAREN_B)) { /* beginning of parenthesis */
	    if (paren_level == 0) {
		paren_start = i;
	    }
	    paren_level++;
	}
	else if (!strcmp((m_ptr + i)->Goi, PAREN_E)) { /* end of parenthesis */
	    paren_level--;
	    if (paren_level == 0 && i != paren_start + 1) { /* （）のような中身がない場合は除く */
		/* 数詞は対象外にする? */
		*(paren_table + paren_start) = 'B'; /* beginning */
		*(paren_table + i) = 'E'; /* end */
		paren_mrph_num += 2;
		for (j = paren_start + 1; j < i; j++) {
		    *(paren_table + j) = 'I'; /* intermediate */
		    paren_mrph_num++; /* 括弧部分の形態素数 */
		}
		paren_num++; /* 括弧数 */
	    }
	}
    }

    if (paren_num == 0 || paren_num >= PAREN_MAX || 
	(paren_start == 0 && *(paren_table + sp->Mrph_num - 1) == 'E')) { /* 全体が括弧の時は対象外 */
	return 0;
    }
    else {
	int paren_count = -1, pre_mrph_is_paren = FALSE, char_pos = 0;

	*paren_spp = (SENTENCE_DATA *)malloc_data(sizeof(SENTENCE_DATA) * paren_num, "process_input_paren");

	/* 各括弧文 */
	for (i = 0; i < paren_num; i++) {
	    (*paren_spp + i)->Mrph_num = 0;
	    (*paren_spp + i)->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA) * paren_mrph_num, "process_input_paren");
	    (*paren_spp + i)->KNPSID = (char *)malloc_data(strlen(sp->KNPSID) + 4, "process_input_paren");
	    sprintf((*paren_spp + i)->KNPSID, "%s-%02d", sp->KNPSID, i + 2); /* 括弧文のIDは-02から */
	    (*paren_spp + i)->Comment = (char *)malloc_data(strlen(PAREN_COMMENT_TEMPLATE) + 4, "process_input_paren");
	    sprintf((*paren_spp + i)->Comment, "%s", PAREN_COMMENT_TEMPLATE);
	}
	strcat(sp->KNPSID, "-01"); /* 本文のIDに-01をつける */
	add_comment(sp, "括弧削除"); /* 本文のコメント行に */

	/* 本文と括弧文を分離 */
	for (i = j = 0; i < sp->Mrph_num; i++) {
	    if (*(paren_table + i) == 0) { /* 括弧ではない部分 */
		if (i != j) {
		    *(m_ptr + j) = *(m_ptr + i);
		    (m_ptr + j)->num = j;
		    (m_ptr + i)->f = NULL;
		}
		j++;
		pre_mrph_is_paren = FALSE;
	    }
	    else { /* 括弧部分 */
		if (pre_mrph_is_paren == FALSE) { /* 括弧部分に突入 */
		    paren_count++;
		}
		if (*(paren_table + i) == 'B') { /* 括弧始 */
		    sprintf((*paren_spp + paren_count)->Comment, "%s%d", (*paren_spp + paren_count)->Comment, char_pos);
		}
		if (*(paren_table + i) == 'I') { /* 括弧内部 */
		    *((*paren_spp + paren_count)->mrph_data + (*paren_spp + paren_count)->Mrph_num) = *(m_ptr + i);
		    ((*paren_spp + paren_count)->mrph_data + (*paren_spp + paren_count)->Mrph_num)->num = (*paren_spp + paren_count)->Mrph_num;
		    (*paren_spp + paren_count)->Mrph_num++;
		    (m_ptr + i)->f = NULL;
		}
		pre_mrph_is_paren = TRUE;
	    }
	    char_pos += strlen((m_ptr + i)->Goi2) / BYTES4CHAR;
	}
	sp->Mrph_num -= paren_mrph_num;
	free(paren_table);

	return paren_num;
    }
}

/*==================================================================*/
void prepare_paren_sentence(SENTENCE_DATA *sp, SENTENCE_DATA *paren_sp)
/*==================================================================*/
{
    int i;

    sp->KNPSID = paren_sp->KNPSID;
    sp->Comment = paren_sp->Comment;
    sp->Mrph_num = paren_sp->Mrph_num;
    for (i = 0; i < paren_sp->Mrph_num; i++) {
	*(sp->mrph_data + i) = *(paren_sp->mrph_data + i);
    }
}

/*====================================================================
                               END
====================================================================*/

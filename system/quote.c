/*====================================================================

			     鈎括弧の処理
				   
                                               S.Kurohashi 1996. 4.18
                                               S.Ozaki     1994.12. 1

    $Id$
====================================================================*/
#include "knp.h"

QUOTE_DATA quote_data;

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

/*==================================================================*/
		     int quote(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int quote_p = FALSE;

    init_quote(sp);

    if ((quote_p = check_quote(sp))) {	/* 鈎括弧の検出 */
	if (quote_p == CONTINUE) return quote_p;

	if (OptDisplay == OPT_DEBUG) print_quote();

	if (Language != CHINESE) {
	    mask_quote(sp);			/* 行列の書き換え */
	}
	else {
	    mask_quote_for_chi(sp); // mask quote for Chinese
	}
    }

    return quote_p;
}

/*====================================================================
                               END
====================================================================*/

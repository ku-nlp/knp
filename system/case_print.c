/*====================================================================

			   格構造解析: 表示

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int	EX_PRINT_NUM = 10;

/*==================================================================*/
	void print_depend_type(CF_PRED_MGR *cpm_ptr, int num)
/*==================================================================*/
{
    int i;

    /* 係タイプの出力 */

    /* 省略のとき */
    if (cpm_ptr->elem_b_num[num] == -2) {
	fputs("《省》", Outfp);
	return;
    }

    fputs("《", Outfp);
    for (i = 0; cpm_ptr->cf.pp[num][i] != END_M; i++) {
	if (i) {
	    fputc('/', Outfp);
	}
	if (cpm_ptr->cf.pp[num][i] < 0) {
	    fputs("--", Outfp);
	}
	else {
	    fprintf(Outfp, "%s", pp_code_to_kstr(cpm_ptr->cf.pp[num][i]));
	}
    }
    fputs("》", Outfp);
}

/*==================================================================*/
 void print_data_cframe(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr)
/*==================================================================*/
{
    int i;

    fputs("【", Outfp);

    if (cpm_ptr->result_num > 0 && cmm_ptr->cf_ptr->entry) {
	fprintf(Outfp, "%s", cmm_ptr->cf_ptr->entry);
    }
    else {
	fprintf(Outfp, "%s", L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi);
    }

    if (cpm_ptr->cf.voice == VOICE_SHIEKI)
	fputs("(使役)】", Outfp);
    else if (cpm_ptr->cf.voice == VOICE_UKEMI)
	fputs("(受身)】", Outfp);
    else if (cpm_ptr->cf.voice == VOICE_MORAU)
	fputs("(使役or受身)】", Outfp);
    else
	fputs("】", Outfp);

    fprintf(Outfp, " %s [%d]", cpm_ptr->cf.ipal_id, 
	    cpm_ptr->pred_b_ptr->cf_num > 1 ? cpm_ptr->pred_b_ptr->cf_num-1 : 1);

    /* 格フレームを決定した方法 */
    if (OptDisc == OPT_DISC) {
	if (cpm_ptr->decided == CF_DECIDED) {
	    fputs(" D", Outfp);
	}
	else if (cpm_ptr->decided == CF_CAND_DECIDED) {
	    fputs(" C", Outfp);
	}
	else {
	    fputs(" U", Outfp);
	}
    }

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {

	fputc(' ', Outfp);
	_print_bnst(cpm_ptr->elem_b_ptr[i]);

	/* 係タイプの出力 */
	print_depend_type(cpm_ptr, i);

	/* 意味マーカ */

	fputc('[', Outfp);

	if (cpm_ptr->cf.sm[i][0]) {
	    fputs("SM:○", Outfp);
	}
	else {
	    fputs("SM:×", Outfp);
	}

	/* 分類語彙表コード */

	if (Thesaurus == USE_BGH) {
	    if (cpm_ptr->cf.ex[i][0]) {
		fputs("BGH:○", Outfp);
	    }
	    else {
		fputs("BGH:×", Outfp);
	    }
	}

	fputc(']', Outfp);	

	if (cpm_ptr->cf.oblig[i] == FALSE)
	    fputc('*', Outfp);
    }
    fputc('\n', Outfp);
}

/*==================================================================*/
   void print_crrspnd(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr)
/*==================================================================*/
{
    int i, j, k, num, print_num;

    if (cmm_ptr->cf_ptr->ipal_address == -1)	/* IPALにない場合 */
	return;

    /* 得点, 意味の表示 */

    fprintf(Outfp, "★%6.2f点 (%d/%.3f) ", cmm_ptr->score, cmm_ptr->pure_score[0], sqrt((double)(count_pat_element(cmm_ptr->cf_ptr, &(cmm_ptr->result_lists_p[0])))));

    if (cmm_ptr->cf_ptr->concatenated_flag == 1)
	fprintf(Outfp, "<文節結合フレーム:%s> ", cmm_ptr->cf_ptr->ipal_id);

    if (cmm_ptr->cf_ptr->voice == FRAME_PASSIVE_I)
	fprintf(Outfp, "(間受)");
    else if (cmm_ptr->cf_ptr->voice == FRAME_PASSIVE_1)
	fprintf(Outfp, "(直受1)");
    else if (cmm_ptr->cf_ptr->voice == FRAME_PASSIVE_2)
	fprintf(Outfp, "(直受2)");
    else if (cmm_ptr->cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||
	     cmm_ptr->cf_ptr->voice == FRAME_CAUSATIVE_WO ||
             cmm_ptr->cf_ptr->voice == FRAME_CAUSATIVE_NI)
	fprintf(Outfp, "(使役)");
    else if (cmm_ptr->cf_ptr->voice == FRAME_POSSIBLE)
	fprintf(Outfp, "(可能)");
    else if (cmm_ptr->cf_ptr->voice == FRAME_POLITE)
	fprintf(Outfp, "(尊敬)");
    else if (cmm_ptr->cf_ptr->voice == FRAME_SPONTANE)
	fprintf(Outfp, "(自発)");

    /* fprintf(Outfp, "%s\n", i_ptr->DATA+i_ptr->imi); */
    fputs("-----------------------------------\n", Outfp);

    /* 格要素対応の表示 */

    for (k = 0; k < cmm_ptr->result_num; k++) {
	if (k != 0)
	    fputs("---\n", Outfp);
	for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[k].flag[i];

	    if (cmm_ptr->cf_ptr->adjacent[i] == TRUE)
		fputs(" ◎ ", Outfp);
	    else
		fputs(" ● ", Outfp);

	    if (num == UNASSIGNED || cmm_ptr->score == -2) { /* -2は全体で不一致 */
		fputs("--", Outfp);
	    }
	    else {
		_print_bnst(cpm_ptr->elem_b_ptr[num]);

		/* 係タイプの出力 */
		print_depend_type(cpm_ptr, num);

		if (num != UNASSIGNED && cpm_ptr->cf.oblig[num] == FALSE)
		    fputc('*', Outfp);

		/* 格ごとのスコアを表示 */
		if (cmm_ptr->result_lists_p[k].score[i] >= 0)
		    fprintf(Outfp, "［%2d点］", cmm_ptr->result_lists_p[k].score[i]);
	    }

	    fprintf(Outfp, " : 《");

	    for (j = 0; cmm_ptr->cf_ptr->pp[i][j]!= END_M; j++) {
		if (j != 0) fputc('/', Outfp);
		fprintf(Outfp, "%s", pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][j]));
	    }
	    fprintf(Outfp, "》");

	    /* 用例の出力 */
	    if (cmm_ptr->cf_ptr->voice == FRAME_PASSIVE_I ||
		cmm_ptr->cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||
		cmm_ptr->cf_ptr->voice == FRAME_CAUSATIVE_WO ||
		cmm_ptr->cf_ptr->voice == FRAME_CAUSATIVE_NI) {
		if (i == 0)
		    fprintf(Outfp, "(彼)");
		else if (cmm_ptr->cf_ptr->ex_list[i]) {
		    print_num = EX_PRINT_NUM < 0 ? cmm_ptr->cf_ptr->ex_num[i] : 
			cmm_ptr->cf_ptr->ex_num[i] > EX_PRINT_NUM ? 
			EX_PRINT_NUM : cmm_ptr->cf_ptr->ex_num[i];
		    fputc('(', Outfp);
		    for (j = 0; j < print_num; j++) {
			if (j != 0) fputc('/', Outfp);
			if (j == cmm_ptr->result_lists_p[k].pos[i]) fputs("【", Outfp);
			fprintf(Outfp, "%s", cmm_ptr->cf_ptr->ex_list[i][j]);
			if (j == cmm_ptr->result_lists_p[k].pos[i]) fputs("】", Outfp);
		    }
		    if (cmm_ptr->result_lists_p[k].pos[i] >= print_num) {
			fputs("/【", Outfp);
			fprintf(Outfp, "%s", cmm_ptr->cf_ptr->ex_list[i][cmm_ptr->result_lists_p[k].pos[i]]);
			fputs("】", Outfp);
		    }
		    if (print_num != cmm_ptr->cf_ptr->ex_num[i])
			fputs("...", Outfp);
		    fputc(')', Outfp);
		}
		/* else if (cmm_ptr->cf_ptr->examples[i])
		    fprintf(Outfp, "(%s)", 
			    cmm_ptr->cf_ptr->examples[i]); */
	    } /* else if (cmm_ptr->cf_ptr->examples[i]) {
		fprintf(Outfp, "(%s)", cmm_ptr->cf_ptr->examples[i]);
	    } */
	    else if (cmm_ptr->cf_ptr->ex_list[i]) {
		print_num = EX_PRINT_NUM < 0 ? cmm_ptr->cf_ptr->ex_num[i] : 
		    cmm_ptr->cf_ptr->ex_num[i] > EX_PRINT_NUM ? 
		    EX_PRINT_NUM : cmm_ptr->cf_ptr->ex_num[i];
		fputc('(', Outfp);
		for (j = 0; j < print_num; j++) {
		    if (j != 0) fputc('/', Outfp);
		    if (j == cmm_ptr->result_lists_p[k].pos[i]) fputs("【", Outfp);
		    fprintf(Outfp, "%s", cmm_ptr->cf_ptr->ex_list[i][j]);
		    if (j == cmm_ptr->result_lists_p[k].pos[i]) fputs("】", Outfp);
		}
		if (cmm_ptr->result_lists_p[k].pos[i] >= print_num) {
		    fputs("/【", Outfp);
		    fprintf(Outfp, "%s", cmm_ptr->cf_ptr->ex_list[i][cmm_ptr->result_lists_p[k].pos[i]]);
		    fputs("】", Outfp);
		}
		if (print_num != cmm_ptr->cf_ptr->ex_num[i])
		    fputs("...", Outfp);
		fputc(')', Outfp);
	    }
	  
	    if (cmm_ptr->cf_ptr->oblig[i] == FALSE)
		fputc('*', Outfp);

	    /* 意味素の出力 */
	    if (cmm_ptr->cf_ptr->semantics[i]) {
		fprintf(Outfp, "[%s]", cmm_ptr->cf_ptr->semantics[i]);
	    }

	    fputc('\n', Outfp);
	}
    }
}

/*==================================================================*/
	  void print_good_crrspnds(CF_PRED_MGR *cpm_ptr,
				   CF_MATCH_MGR *cmm_ptr, int ipal_num)
/*==================================================================*/
{
    int i, j, *check;
    int max_num, max_score, max_counts, all_max_score = 0;

    check = (int *)malloc_data(sizeof(int)*ipal_num, "print_good_crrspnds");
    for (i = 0; i < ipal_num; i++) check[i] = 1;
    for (i = 0; i < ipal_num; i++) {
	max_num = -1;
	max_score = -10;	/* case_analysis では -1 の時がある */
	for (j = 0; j < ipal_num; j++) {
	    if (check[j] && (cmm_ptr+j)->score > max_score) {
		max_score = (cmm_ptr+j)->score;
		max_num = j;
		max_counts = 1;
	    }
	    else if (check[j] && (cmm_ptr+j)->score == max_score) {
		max_counts ++;
	    }
	}
	if (i == 0) all_max_score = max_score;

	/* 表示の停止条件
	if (OptDisplay == OPT_NORMAL || OptDisplay == OPT_DETAIL) {
	    if (max_score != all_max_score && i >= 3) 
		break;
	}
	*/

	print_crrspnd(cpm_ptr, cmm_ptr+max_num);
	check[max_num] = 0;
    }
    free(check);
}

/*==================================================================*/
	      void print_case_result(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    TOTAL_MGR *tm = sp->Best_mgr;

    fputs("<Case Structure Analysis Data>\n", Outfp);
    fprintf(Outfp, "■ %d Score:%d, Dflt:%d, Possibility:%d/%d ■\n", 
	    sp->Sen_num, tm->score, tm->dflt, tm->pssb+1, 1);

    /* 上記出力の最後の引数(依存構造の数)は1にしている．
       ちゃんと扱ってない */

    for (i = tm->pred_num-1; i >= 0; i--) {
	print_data_cframe(&(tm->cpm[i]), &(tm->cpm[i].cmm[0]));
	for (j = 0; j < tm->cpm[i].result_num; j++) {
	    if (OptDisc == OPT_DISC) {
		print_crrspnd(tm->cpm[i].cmm[j].cpm ? tm->cpm[i].cmm[j].cpm : &(tm->cpm[i]), 
			      &(tm->cpm[i].cmm[j]));
		free(tm->cpm[i].cmm[j].cpm);
	    }
	    else {
		print_crrspnd(&(tm->cpm[i]), &(tm->cpm[i].cmm[j]));
	    }
	}
	fputc('\n', Outfp);
    }
    fputs("</Case Structure Analysis Data>\n", Outfp);
}

/*====================================================================
                               END
====================================================================*/

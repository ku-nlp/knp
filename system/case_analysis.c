/*====================================================================

			      格構造解析

                                               S.Kurohashi 91.10. 9
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

extern int Possibility;

CF_MATCH_MGR	Cf_match_mgr[IPAL_FRAME_MAX * 5];	/* 作業領域 */
TOTAL_MGR	Dflt_mgr;
TOTAL_MGR	Work_mgr;

/*====================================================================
		       格助詞の文字−コード対応
====================================================================*/

struct PP_STR_TO_CODE {
    char *hstr;
    char *kstr;
    int  code;
} PP_str_to_code[] = {
    {"φ", "φ", 0},		/* 格助詞のないもの(数量表現等) */
    {"が", "ガ", 1},
    {"を", "ヲ", 2},
    {"に", "ニ", 3},
    {"から", "カラ", 4},
    {"へ", "ヘ", 5},
    {"より", "ヨリ", 6},
    {"と", "ト", 7},
    {"で", "デ", 8},
    {"によって", "ニヨッテ", 9},
    {"をめぐる", "ヲメグル", 10},	/* 複合辞関係 */
    {"をつうじる", "ヲツウジル", 11},
    {"をつうずる", "ヲツウズル", 12},
    {"をふくむ", "ヲフクム", 13},
    {"をはじめる", "ヲハジメル", 14},
    {"にからむ", "ニカラム", 15},
    {"にそう", "ニソウ", 16},
    {"にむける", "ニムケル", 17},
    {"にともなう", "ニトモナウ", 18},
    {"にもとづく", "ニモトヅク", 19},
    {"をのぞく", "ヲノゾク", 20},
    {"による", "ニヨル", 21},
    {"にたいする", "ニタイスル", 22},
    {"にかんする", "ニカンスル", 23},
    {"にかわる", "ニカワル", 24},
    {"におく", "ニオク", 25},
    {"につく", "ニツク", 26},
    {"にとる", "ニトル", 27},
    {"にくわえる", "ニクワエル", 28},
    {"にかぎる", "ニカギリル", 29},
    {"につづく", "ニツヅク", 30},
    {"にあわせる", "ニアワセル", 31},
    {"にくらべる", "ニクラベル", 32},
    {"にならぶ", "ニナラブ", 33},
    {"とする", "トスル", 34},
    {"によるず", "ニヨルズ", 35},
    {"にかぎるず", "ニカギルズ", 36},
    {"は", "ハ", 1},		/* NTT辞書では「ガガ」構文が「ハガ」
				   ※ NTT辞書の「ハ」は1(code)に変換されるが,
				      1は配列順だけで「ガ」に変換される */
    {"＊", "＊", -2},		/* 埋め込み文の被修飾詞 */
    {NULL, NULL, -1}		/* 格助詞の非明示のもの(提題助詞等) */
};

/*====================================================================
			 文字−コード対応関数
====================================================================*/
int pp_kstr_to_code(char *cp)
{
    int i;
    for (i = 0; PP_str_to_code[i].kstr; i++)
      if (str_eq(PP_str_to_code[i].kstr, cp))
	return PP_str_to_code[i].code;
    
    if (str_eq(cp, "ニトッテ"))		/* 「待つ」 IPALのバグ ?? */
      return pp_kstr_to_code("ニヨッテ");
    else if (str_eq(cp, "ノ"))		/* 格要素でなくなる場合 */
      return -1;

    /* fprintf(stderr, "Invalid string (%s) in PP !\n", cp); */
    return -1;
}

int pp_hstr_to_code(char *cp)
{
    int i;
    for (i = 0; PP_str_to_code[i].hstr; i++)
      if (str_eq(PP_str_to_code[i].hstr, cp))
	return PP_str_to_code[i].code;
    return -1;
}

char *pp_code_to_kstr(int num)
{
    return PP_str_to_code[num].kstr;
}

char *pp_code_to_hstr(int num)
{
    return PP_str_to_code[num].hstr;
}

/*
int sm_zstr_to_code(char *cp)
{
    int i;
    for (i = 0; SM_str_to_code[i].zstr; i++)
      if (str_eq(SM_str_to_code[i].zstr, cp))
	return SM_str_to_code[i].code;
    * fprintf(stderr, "Invalid string (%s) in SM !\n", cp); *
    return -1;
}

int sm_str_to_code(char *cp)
{
    int i;
    
    for (i = 0; SM_str_to_code[i].str; i++)
      if (str_eq(SM_str_to_code[i].str, cp))
	return SM_str_to_code[i].code;
    * fprintf(stderr, "Invalid string (%s) in SM !\n", cp); *
    return -1;
}

char *sm_code_to_str(int code)
{
    int i;

    for (i = 0; SM_str_to_code[i].str; i++)
      if (SM_str_to_code[i].code == code)
	return SM_str_to_code[i].str;
    fprintf(stderr, "Invalid code (%d) in SM !\n", code);
    return NULL;
}
*/

/*==================================================================*/
     int case_analysis(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    /*
                                              戻値
      入力の格要素がない場合                    -3
      格フレームがない場合                      -2
      入力側に必須格が残る場合(解析不成功)      -1
      解析成功                               score (0以上)
    */

    CASE_FRAME *cf_ptr = &(cpm_ptr->cf);
    int i, frame_num, max_score;
    
    /* 初期化 */
    cpm_ptr->pred_b_ptr = b_ptr;
    cpm_ptr->score = -1;
    cpm_ptr->result_num = 0;

    /* 入力文側の格要素設定 */
    cf_ptr->voice = b_ptr->voice;
    make_data_cframe(cpm_ptr);

    /* 格フレーム解析スキップ
    if (cf_ptr->element_num == 0) {
	cpm_ptr->cmm[0].cf_ptr = NULL;
	return -3;
    }
    */

    /* 格フレーム設定 */
    frame_num = cpm_ptr->pred_b_ptr->cf_num;
    if (frame_num == 0) {
	return -2;
    }
    for (i = 0; i < frame_num; i++)
      (Cf_match_mgr + i)->cf_ptr = cpm_ptr->pred_b_ptr->cf_ptr + i;

    /* 格要素なしの時の実験 98/12/16 */
    if (cf_ptr->element_num == 0) {
	case_frame_match(cf_ptr, Cf_match_mgr, OptCFMode);
	cpm_ptr->score = Cf_match_mgr->score;
	cpm_ptr->cmm[0] = *Cf_match_mgr;
	cpm_ptr->result_num = 1;
    }
    else { /* このelseはtentative */
	for (i = 0; i < frame_num; i++) {

	    /* 選択可能
	       EXAMPLE
	       SEMANTIC_MARKER

	       ChangeLog:
	       意味マーカの利用に変更 (1998/10/02)
	       オプションで選択       (1999/06/15)
	       */

	    case_frame_match(cf_ptr, Cf_match_mgr+i, OptCFMode);

	    /* その格フレームとの対応付けがスコア最大であれば記憶 */

	    if ((Cf_match_mgr+i)->score > cpm_ptr->score) {
		cpm_ptr->score = (Cf_match_mgr+i)->score;
		cpm_ptr->cmm[0] = *(Cf_match_mgr+i);
		cpm_ptr->result_num = 1;
	    }

	    /* その格フレームとの対応付けがスコア最大と同点でも記憶 */

	    else if ((Cf_match_mgr+i)->score == cpm_ptr->score) {
		if (cpm_ptr->result_num >= CMM_MAX)
		    fprintf(stderr, "Not enough cmm.\n");
		else
		    cpm_ptr->cmm[cpm_ptr->result_num++] = *(Cf_match_mgr+i);
	    }
	}
    }

    /* corpus based case analysis 00/01/04

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] == -1) {
	    score -= cpm_ptr->cf.pred_b_ptr.dpnd_dflt * 2;
	} else {
	    score -= cpm_ptr->elem_b_ptr[i].dpnd_dflt * 2;
	}	
    }
    */

    if (OptDisplay == OPT_DEBUG) {
	print_data_cframe(cf_ptr);
	print_good_crrspnds(cpm_ptr, Cf_match_mgr, frame_num);
    }

    return cpm_ptr->score;
}

/*==================================================================*/
      int all_case_analysis(BNST_DATA *b_ptr, TOTAL_MGR *t_ptr)
/*==================================================================*/
{
    CF_PRED_MGR *cpm_ptr;
    int i;
    int one_case_point;
    
    if (b_ptr->para_top_p != TRUE && check_feature(b_ptr->f, "用言") && 
	!check_feature(b_ptr->f, "複合辞")) {

	cpm_ptr = &(t_ptr->cpm[t_ptr->pred_num]);

	one_case_point = case_analysis(cpm_ptr, b_ptr);

	/* 解析不成功(入力側に必須格が残る)場合にその依存構造の解析を
	   やめる場合
	if (one_case_point == -1) return FALSE;
	*/

	t_ptr->score += one_case_point;
	
	if (t_ptr->pred_num++ >= CPM_MAX) {
	    fprintf(stderr, "too many predicates in a sentence.\n");
	    exit(1);
	}
    }

    for (i = 0; b_ptr->child[i]; i++)
      if (all_case_analysis(b_ptr->child[i], t_ptr) == FALSE)
	return FALSE;

    return TRUE;
}

/*==================================================================*/
		  void call_case_analysis(DPND dpnd)
/*==================================================================*/
{
    int i;
    int topic_score;
    char *cp;

    /* 格構造解析のメイン関数 */

    /* 依存構造木作成 */

    dpnd_info_to_bnst(&dpnd);
    make_dpnd_tree();
	
    if (OptDisplay == OPT_DEBUG)
	print_kakari();

    /* 格解析作業領域の初期化 */
	
    Work_mgr.pssb = Possibility;
    Work_mgr.dpnd = dpnd;
    Work_mgr.score = 0;
    Work_mgr.pred_num = 0;
    Work_mgr.dflt = 0;
    for (i = 0; i < sp->Bnst_num; i++)
	Work_mgr.dflt += dpnd.dflt[i];
    
    /* 格解析呼び出し */

    if (all_case_analysis(sp->bnst_data+sp->Bnst_num-1, &Work_mgr) == TRUE)
	Possibility++;
    else
	return;

    /* corpus based case analysis 00/01/04
       ここでdefaultとの距離のずれ,提題を処理 */
    Work_mgr.score -= Work_mgr.dflt * 2;
    for (i = 0; i < sp->Bnst_num; i++) {
	if (check_feature((sp->bnst_data+i)->f, "提題") &&
	    (cp = (char *)check_feature(
		(sp->bnst_data+(sp->bnst_data+i)->dpnd_head)->f, "提題受"))) {
	    sscanf(cp, "%*[^:]:%d", &topic_score);
	    Work_mgr.score += topic_score;
	}
    }
        
    /* 後処理 */

    if (Work_mgr.score > Best_mgr.score ||
	(Work_mgr.score == Best_mgr.score && 
	 compare_dpnd(&Work_mgr, &Best_mgr) == TRUE))
	Best_mgr = Work_mgr; 
    if (Work_mgr.dflt == 0) Dflt_mgr = Work_mgr;
}

/*==================================================================*/
		     void record_case_analysis()
/*==================================================================*/
{
    int i, j, k, num;
    char feature_buffer[256];
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr;
    BNST_DATA *pred_b_ptr;

    /* 格解析の結果(Best_mgrが管理)をfeatureとして用言文節に与える */

    for (j = 0; j < Best_mgr.pred_num; j++) {

	cpm_ptr = &(Best_mgr.cpm[j]);
	cmm_ptr = &(cpm_ptr->cmm[0]);
	cf_ptr = cmm_ptr->cf_ptr;
	pred_b_ptr = cpm_ptr->pred_b_ptr;

	if (!cf_ptr) continue;	/* 格フレームがない場合 */

	sprintf(feature_buffer, "意味:%s", cf_ptr->imi);
	assign_cfeature(&(pred_b_ptr->f), feature_buffer);

	for (i = 0; i < cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];

	    /* 対応関係 */

	    if (num == UNASSIGNED || cmm_ptr->score == -2) 
		sprintf(feature_buffer, "深層格N%d:NIL:", i+1);
	    else
		sprintf(feature_buffer, "深層格N%d:%d:", i+1, 
			cpm_ptr->elem_b_ptr[num]->num);
	    
	    /* 表層格 */

	    for (k = 0; cf_ptr->pp[i][k] != END_M; k++) {
		if (k != 0) 
		    sprintf(feature_buffer+strlen(feature_buffer), "/");
		sprintf(feature_buffer+strlen(feature_buffer), 
			"%s", pp_code_to_kstr(cf_ptr->pp[i][k]));
	    }
	    if (cf_ptr->oblig[i] == FALSE)
		sprintf(feature_buffer+strlen(feature_buffer), "*");

	    sprintf(feature_buffer+strlen(feature_buffer), ":");
	    
	    /* 意味素 */

	    for (k = 0; cf_ptr->sm[i][k]; k+=12) {
		if (k != 0) 
		    sprintf(feature_buffer+strlen(feature_buffer), "/");
		sprintf(feature_buffer+strlen(feature_buffer), 
			"%12.12s", &(cf_ptr->sm[i][k]));
	    }

	    /* feature付与 */

	    assign_cfeature(&(pred_b_ptr->f), feature_buffer);
	}
    }
}	    

/*====================================================================
                               END
====================================================================*/

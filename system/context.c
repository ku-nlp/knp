/*====================================================================

			       文脈解析

                                               S.Kurohashi 98. 9. 8

    $Id$
====================================================================*/
#include "knp.h"


extern int ArticleID;
/* extern char KNPSID[]; */
extern char SID_box[];

char Ga_Memory[256];
char KAKKO[] = "【’";
char IU[] = "いう";
char NO[] = "の";
char KOTO[] = "こと";
char TO[] = "と";
char BEKI[] = "べし";
char GA[] = "が";
char SEMI_CL[]=":";
char HA[] = "は";
char KUTEN[] = "、";



/*==================================================================*/
			 void copy_sentence()
/*==================================================================*/
{
    /* 文解析結果の保持 */

    int i, j, k;

    SENTENCE_DATA *sp_new = sentence_data + sp->Sen_num - 1;

    sp_new->Sen_num = sp->Sen_num;

    sp_new->Mrph_num = sp->Mrph_num;
    sp_new->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA)*sp->Mrph_num, 
						 "MRPH DATA");
    for (i = 0; i < sp->Mrph_num; i++) {
	sp_new->mrph_data[i] = sp->mrph_data[i];
    }

    sp_new->Bnst_num = sp->Bnst_num;
    sp_new->bnst_data = 
	(BNST_DATA *)malloc_data(sizeof(BNST_DATA)*(sp->Bnst_num + sp->New_Bnst_num), 
				 "BNST DATA");
    for (i = 0; i < sp->Bnst_num + sp->New_Bnst_num; i++) {

	sp_new->bnst_data[i] = sp->bnst_data[i]; /* ここでbnst_dataの全メンバをコピー */

	/* 以下, 単純にコピーしたことによるポインタアドレスのずれを補正 */
	sp_new->bnst_data[i].mrph_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].settou_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].jiritu_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].fuzoku_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].parent += sp_new->bnst_data - sp->bnst_data;
	for (j = 0; sp_new->bnst_data[i].child[j]; j++) {
	    sp_new->bnst_data[i].child[j]+= sp_new->bnst_data - sp->bnst_data;
	}
    }

    sp_new->para_data = (PARA_DATA *)malloc_data(sizeof(PARA_DATA)*Para_num, 
				 "PARA DATA");
    for (i = 0; i < Para_num; i++) {
	sp_new->para_data[i] = sp->para_data[i];
	sp_new->para_data[i].manager_ptr += sp_new->para_manager - sp->para_manager;
    }

    sp_new->para_manager = 
	(PARA_MANAGER *)malloc_data(sizeof(PARA_MANAGER)*Para_M_num, 
				    "PARA MANAGER");
    for (i = 0; i < Para_M_num; i++) {
	sp_new->para_manager[i] = sp->para_manager[i];
	sp_new->para_manager[i].parent = sp_new->para_manager - sp->para_manager;
	for (j = 0; j < sp_new->para_manager[i].child_num; j++) {
	    sp_new->para_manager[i].child[j] = sp_new->para_manager - sp->para_manager;
	}
	sp_new->para_manager[i].bnst_ptr = sp_new->bnst_data - sp->bnst_data;
    }
}


/*==================================================================*/
                int check_f_modal_b(BNST_DATA *b_ptr)
/*==================================================================*/
{
    	if (check_feature(b_ptr->f, "Modality-意思-意志")||
	    check_feature(b_ptr->f, "Modality-意思-願望")||
	    check_feature(b_ptr->f, "Modality-意思-禁止")||
	    check_feature(b_ptr->f, "Modality-意思-勧誘")||
	    check_feature(b_ptr->f, "Modality-意思-命令")||
	    check_feature(b_ptr->f, "Modality-意思-依頼")||
	    check_feature(b_ptr->f, "Modality-当為")||
	    check_feature(b_ptr->f, "Modality-当為-許可")||
	    check_feature(b_ptr->f, "Modality-判断-推量")||
	    check_feature(b_ptr->f, "Modality-判断-伝聞")||
	    check_feature(b_ptr->f, "Modality-判断-様態")||
	    check_feature(b_ptr->f, "Modality-判断-可能性")||
	    check_feature(b_ptr->f, "Modality-判断-可能性-不可能")){
	    return TRUE;
	}
	else {
	    return FALSE;
	}
}

/*==================================================================*/
                int check_f_modal_m(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    	if (check_feature(m_ptr->f, "Modality-意思-意志")||
	    check_feature(m_ptr->f, "Modality-意思-願望")||
	    check_feature(m_ptr->f, "Modality-意思-禁止")||
	    check_feature(m_ptr->f, "Modality-意思-勧誘")||
	    check_feature(m_ptr->f, "Modality-意思-命令")||
	    check_feature(m_ptr->f, "Modality-意思-依頼")||
	    check_feature(m_ptr->f, "Modality-当為")||
	    check_feature(m_ptr->f, "Modality-当為-許可")||
	    check_feature(m_ptr->f, "Modality-判断-推量")||
	    check_feature(m_ptr->f, "Modality-判断-伝聞")||
	    check_feature(m_ptr->f, "Modality-判断-様態")||
	    check_feature(m_ptr->f, "Modality-判断-可能性")||
	    check_feature(m_ptr->f, "Modality-判断-可能性-不可能")){
	    return TRUE;
	}
	else {
	    return FALSE;
	}
}


/*==================================================================*/
                int check_f_modal_ISHI(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    if(check_feature(m_ptr->f, "Modality-意思-意志")||
       check_feature(m_ptr->f, "Modality-意思-願望")){

	    return TRUE;
	}
	else {
	    return FALSE;
	}
}



/*==================================================================*/
  int check_primary_child(BNST_DATA *fst_b_ptr, 
			  BNST_DATA *b_ptr,
			  SENTENCE_DATA *local_sp)
/*==================================================================*/
/* 
 *    子孫の"係:未格", "係:ガ格"を持ってくる
 */
{
    int i,j;
    int rk, fl, stop;
    int rk_2;
    BNST_DATA *tmp_bptr, *pos_bptr;
    BNST_DATA *child_box[2][64];

    rk = 0; fl = 0;    /* 行:rank, 列:file */
    rk_2 = 1;
    i = 0 ; j = 0;
    stop = 1;
    child_box[0][0] = b_ptr;

    for(stop; stop; ){
	for(fl = 0; fl < stop; fl++){
	    tmp_bptr = child_box[rk][fl];


	    for(i = 0, j; tmp_bptr->child[i]; i++, j++){

		if ((check_feature(tmp_bptr->f, "モ〜") &&
		     check_feature(tmp_bptr->child[i]->f, "数量")&&
		     check_feature(tmp_bptr->child[i]->f, "モ")) ||
		    (check_feature(tmp_bptr->child[i]->f, "時間") &&
		     check_feature(tmp_bptr->child[i]->f, "モ"))){
		    return FALSE;
		}
		
		else if (check_feature(tmp_bptr->child[i]->f, "係:ガ格")){
		    pos_bptr = skip_para_top(tmp_bptr->child[i], "ch");
		    assign_GA2pred(fst_b_ptr, pos_bptr, "ガ格", local_sp);
		    return TRUE;
		}
		else if (check_feature(tmp_bptr->child[i]->f, "係:未格")){
		    pos_bptr = skip_para_top(tmp_bptr->child[i], "ch");
		    assign_GA2pred(fst_b_ptr, pos_bptr, "未格", local_sp);
		    return TRUE;
		}

 
#if 0
		else if ((!check_feature(tmp_bptr->f, "ID:〜ば") ||
			  !check_feature(tmp_bptr->child[i]->f, "モ")) &&
			 check_feature(tmp_bptr->child[i]->f, "係:未格")){
		    /*
		     * 「AならばBもCする」のとき, Aに対して「Bも」はガ格ではない
		     * (例) 業界は管理下にあれば新規参入もない
		     */
		    assign_GA2pred(fst_b_ptr, tmp_bptr->child[i], "未格", local_sp);
		    return TRUE;
		}
#endif
		
		else child_box[rk_2][j] = tmp_bptr->child[i];
	    }
	}
	child_box[rk_2][j] = NULL; /* 行の最後に番兵を入れとく */
	stop = j;
	j = 0;

	/* rk と rk_2 を swap */
	if (rk == 1){
	    rk = 0; rk_2 = 1;
	} else {
	    rk = 1; rk_2 = 0;
	}

    }

    return FALSE; 
}





/*==================================================================*/
BNST_DATA *skip_para_top(BNST_DATA *b_ptr, char *flag)
/*==================================================================*/
/*
 *   ガ格候補が, PARA TOP だったら、その先の子か親を返す
 *   "Cガ格推定" の feature を持っている b_ptr には, 
 */
{
    /* check child */
    if (strcmp(flag, "ch") == 0){
	if (b_ptr->para_top_p == TRUE){
	    return skip_para_top(b_ptr->child[0], "ch");
	}
	else return b_ptr;
    }

    /* check parent */
    else if (strcmp(flag, "pa") == 0){
	if (b_ptr->para_top_p == TRUE){
	    return skip_para_top(b_ptr->parent, "pa");
	}
	else return b_ptr;
    }

    else {
	printf("ERROR in skip_para_top() : Invalid flag %s\n", flag);
	return NULL;
    }

}


/*==================================================================*/
BNST_DATA *check_ancestor(BNST_DATA *b_ptr)
/*==================================================================*/
/* 
 *   係り先先祖のハ格, もしくはガ格を探す
 */
{
    int i, j;
    int pos, m_num;
    BNST_DATA *tmp_bptr, *tmp_bptr_next;
    BNST_DATA *ancst[64]; 
   /*
    * i+1番目の要素は, i番目の要素の親 
    * ancst[0] の子供は, b_ptr.
    * 親が PARA でも格納しておく.(あとでスキップする)
    * ancst[i]<-->ancst[i+1]
    *  (child <--> parent)
    */

    tmp_bptr = b_ptr;

    for(i = 0; tmp_bptr->parent; i++){
	tmp_bptr = tmp_bptr->parent;
	ancst[i] = tmp_bptr;
    }

    if (i == 0) return NULL; /* b_ptr が root だった */

    ancst[i] = NULL; /* 最後に番兵を入れておく */

    
    for(i = 0; ancst[i] != NULL; i++){

	/* 親がPARAなら, 次の親 */
	if (ancst[i]->para_top_p == TRUE) continue;

	tmp_bptr = ancst[i]; /* tmp_bptr は今いるところの親のつもり */

	if (i == 0){
	    pos = position_in_children(b_ptr);
	}
	else if (i > 0){
	    pos = position_in_children(ancst[i-1]);
	}
	else pos = -1;


	if(pos >= 0){

	    /* まずは先祖にかかっているハ格を探す */
	    pos = pos + 1;
	    for(pos; tmp_bptr->child[pos]; pos ++){

		m_num = tmp_bptr->child[pos]->mrph_num;
		if (check_feature(tmp_bptr->child[pos]->f, "係:未格") &&
		    ((strcmp(HA, tmp_bptr->child[pos]->mrph_ptr[m_num-1].Goi) == 0) ||
		     (strcmp(KUTEN, tmp_bptr->child[pos]->mrph_ptr[m_num-1].Goi) == 0) &&
		     (strcmp(HA, tmp_bptr->child[pos]->mrph_ptr[m_num-2].Goi) == 0))){

		    return skip_para_top(tmp_bptr->child[pos], "ch");
		}
	    }
	    /* 先祖にかかるハ格がなくて, 直系の親がガ格だったら, その人 */
	    if (check_feature(tmp_bptr->f, "係:ガ格")){
		return tmp_bptr;
	    }
	}
    }

    return NULL;
}


/*==================================================================*/
BNST_DATA *check_posterity(BNST_DATA *b_ptr, int s)
/*==================================================================*/
/* 
 *    子孫の"係:未格", "係:ガ格"を持ってくる
 */
{
    /*
     * child_box[][] (２行を交互に使う)
     *
     *           |<-     64     ->|
     *           ------------------  
     *  first    | | | | .... | | |
     *           ------------------
     *  second   | | | | .... | | |
     *           ------------------
     */

    /*  
     *   ==================
     *   THE CHILD diagram
     *   =================
     *   縦の並びが, child_box[][]の横の行にあたる  
     *
     *   ...  third  second  first
     *   ...    ↓    ↓     ↓
     *
     *                G──┐
     *                F──┤
     *                E──┤
     *                      A──┐
     *          .. ─┐          │
     *                D──┐    │
     *                      B──┤
     *                           │
     *                      C──┤
     *                           │
     *                          SELF
     * 
     */

    int i,j;
    int rk, fl, stop;
    int rk_2;
    BNST_DATA *tmp_bptr;
    BNST_DATA *child_box[2][64];

    rk = 0; fl = 0;    /* 行:rank, 列:file */
    rk_2 = 1;
    i = 0 ; j = 0;
    stop = 1;
    child_box[0][0] = b_ptr;

    for(stop; stop; ){
	for(fl = 0; fl < stop; fl++){
	    tmp_bptr = child_box[rk][fl];
/*
	    for(i = 0, j; tmp_bptr->child[i]; i++, j++){
		if(tmp_bptr->child[i]->para_top_p != TRUE &&
		   (check_feature(tmp_bptr->child[i]->f, "係:未格") ||
		    check_feature(tmp_bptr->child[i]->f, "係:ガ格"))){
		    return tmp_bptr->child[i];
		}
		else child_box[rk_2][j] = tmp_bptr->child[i];
	    }
*/
	    /* 
	     * 一回目のループのときだけ, s の値が効く 
	     * 例えば, 自分の親から check_posterity()
	     * を呼んだとき、自分は避けたいから.
	     * 子孫を調べるときは, s=0 で呼び出せばいい
	     */
	    for(i = s, j; tmp_bptr->child[i]; i++, j++){
		if(tmp_bptr->child[i]->para_top_p != TRUE &&
		   (check_feature(tmp_bptr->child[i]->f, "係:未格") ||
		    check_feature(tmp_bptr->child[i]->f, "係:ガ格"))){
		    return tmp_bptr->child[i];
		}
		else child_box[rk_2][j] = tmp_bptr->child[i];
	    }
	}
	child_box[rk_2][j] = NULL; /* 行の最後に番兵を入れとく */
	stop = j;
	j = 0;
	s = 0;

	/* rk と rk_2 を swap */
	if (rk == 1){
	    rk = 0; rk_2 = 1;
	} else {
	    rk = 1; rk_2 = 0;
	}

    }

    return NULL; 
}




/*==================================================================*/
                 int position_in_children(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i;
    for(i = 0 ; b_ptr->parent->child[i]; i++){
	if (b_ptr == (b_ptr->parent->child[i])){
	    return i;
	}
    }
    return -1;
}




/*==================================================================*/
    void assign_GA2pred(BNST_DATA *pred_ptr, BNST_DATA *GA_candid, 
			char *comment, SENTENCE_DATA *local_sp)
/*==================================================================*/
{
    int bn, sn;
    char result[256];
    char *cp;
    char MOD_HOLDER[] = "Modality保持者";
    char JISOU[] = "時相";

    /* (下注) 
     *  check_feature(GA_candid->f, "Cガ格推定") の判定は, この関数に移る
     *  前に, 既にGA_detection でやっているから, そこでflagでも立てる
     *  ほうがいいかもしれないけど, 煩雑になりそうだから今はcheck_feature
     *  をやっている. 
     */
    if (GA_candid == NULL){
	if (strcmp(MOD_HOLDER, comment) == 0){
	sprintf(result, "Cガ格推定:%s:(文#:-):(文節#:-):筆者", comment);
	}
	else if (strcmp(JISOU, comment) == 0){
	sprintf(result, "Cガ格推定:%s:(文#:-):(文節#:-):時期・時間・状況", comment);
	}
	else {
	    printf("Invalid comment : %s\n", comment);
	}
    }
    else if ((cp = check_feature(GA_candid->f, "Cガ格未発見")) != NULL){
	sprintf(result, "Cガ格未発見");
    }
    else if((cp=check_feature(GA_candid->f, "Cガ格推定")) != NULL){
	sprintf(result, "Cガ格推定:%s:%s", comment, cp + strlen("Cガ格推定:"));
    }
    else {
	bn = GA_candid - (local_sp->bnst_data); /* GA_candid の文節番号 -1 の数 */
	sn = local_sp->Sen_num;              /* 文の番号 */
	sprintf(result, "Cガ格推定:%s:(文#:%d):(文節#:%d):", comment, sn, bn+1);
	print_bnst( GA_candid, result + strlen(result));
    }
    assign_cfeature(&(pred_ptr->f),  result);
}


    /*===========================================
     * strlen("Cガ格推定:")
     * |<------------->|
     * -------------------------------------------
     * |C|ガ|格|推|定|:| | |...............| | | |
     * -------------------------------------------
     * ↑              ↑ 
     * cp          cp + strlen("Cガ格推定:")
      ===========================================*/


    /*===========================================
      配列 result 
     ===========================================*/
    /*===========================================
     *  strlen(result)
     * |<----------->|
     * -------------------------------------------
     * |C|ガ|格|推|定|:|ガ|格|...............| | | |
     * -------------------------------------------
     *↑                     ↑ 
   result       result+ strlen(result) 
     ===========================================*/
    /*===========================================
     * print_bnst(GA, result + strlen(result));
     * ---------------------------------------------
     * |C|ガ|格|推|定|:|ガ|格|:|彼|ガ| |.....| | | |
     * ---------------------------------------------
     *                         |<--->|
     *	                         *GA
     *===========================================*/


#if 0
/*==================================================================*/
               char by_case_detection(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, j, k;
    int num, flag;
    char feature_buffer[256];
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr;
    BNST_DATA *pred_b_ptr;

    /* 格解析の結果(Best_mgrが管理)をfeatureとして用言文節に与える */


}
#endif


/*==================================================================*/
		 void GA_detection(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, j, k, s, t;
    int m_num, m_flag_include_ISHI, flag_1, flag_2;
    int tmp;
    char *cp;
    BNST_DATA *pos_bptr;
    SENTENCE_DATA *prev_sp, *local_sp;
    TOTAL_MGR *tm = &Best_mgr;
    CF_PRED_MGR *cpm_ptr;

    local_sp = sp;


    /* 用言のガ格をさがす */

    /*--------------*/
    /* 無視する条件 */
    /*--------------*/

    if(b_ptr->para_top_p == TRUE) goto Match;


    if(check_feature(b_ptr->f, "用言:判") &&
       check_feature(b_ptr->f, "体言")){ 
   /*
    *       &&
    *     (!check_feature(b_ptr->f, "サ変"))){
    */
	goto Match;
    }

    if(check_feature(b_ptr->f, "用言") &&
       check_feature(b_ptr->f, "体言") &&
       check_feature(b_ptr->f, "助詞")){
	goto Match;
    }

    if (check_feature(b_ptr->f, "用言") &&
	!check_feature(b_ptr->f, "レベル:A-")){
	
	/* printf("用言発見 %s\n", b_ptr->Jiritu_Go); */


	/*------------------------*/
	/* 自分が用言でも無視する */
	/*------------------------*/


	/*===== 「〜という」など除去 =====*/

	for (i = 0; b_ptr->child[i]; i++) {
	    if(check_feature(b_ptr->child[i]->f, "ID:〜と（引用）") &&
	       !check_feature(b_ptr->child[i]->f, "外の関係") &&
	       (!check_feature(b_ptr->f, "サ変") &&
		!check_feature(b_ptr->f, "サ変スル")))
		goto Match;
	    /*----------------------------------------------------
	     * (こんなとき)
	     *
	     *      賛成だと──┐　　　　　
	     *                 いう──┐　
	     *                        意見
	     *--------------------------------------------------*/
	}

	/*===== 「〜という(のが)」「〜という(こと)」の"いう"を除去 =====*/

	for(i = 0; b_ptr->child[i]; i++){
	    m_num = b_ptr->child[i]->mrph_num;
	    if((strcmp(IU, b_ptr->mrph_ptr[0].Goi) == 0)&&
	      /*
	       * ((strcmp(NO, b_ptr->mrph_ptr[1].Goi) == 0)||
	       * (strcmp(KOTO, b_ptr->mrph_ptr[1].Goi) == 0)) && 
	       */
	       (strcmp(TO, b_ptr->child[i]->mrph_ptr[m_num-1].Goi) == 0))
		goto Match;
	    /*----------------------------------------------------
	     * (こんなとき)
	     *
	     *  擁立すべきだ」と━━┓
             *                      いう──┐　　　　　　　　　
             *                            考えが──┐　　　　　
	     *--------------------------------------------------*/
	}

	/*===== 用言 + "Modality-当為" のとき =====*/

	if (check_feature(b_ptr->f, "係:連格") &&
	    check_feature(b_ptr->f, "Modality-当為")){
	    goto Match;
	}


#if 0
	/*------------------------------*/
        /*   格解析の結果からガ格推定   */
	/*------------------------------*/
	if((cp = by_case_detection(b_ptr)) != NULL){
	    goto Match;
	}
#endif



	/*--------------------------*/
        /*    まずは例外的な処理    */
	/*--------------------------*/

	/*===== 文節に Modality の feature があるとき =====*/

	if (check_f_modal_b(b_ptr)){ /* 文節の feature に Modality がある */
	
	    /* i == 0 までカウントダウンすれば, 文節全部 Modality */
	    /*------------------------------------------------
	     *
	     *  (i == 0) ⇒  ■■■■(文節) 全部 Modality
	     *
	     *-----------------------------------------------*/
	    for (i = b_ptr->mrph_num; i > 0 ; i--){
		if(check_f_modal_m(&(b_ptr->mrph_ptr[i-1]))){
		    continue;
		}
		else {
		    break;
		}
	    }


	    if (i == 0){ /* 文節が全部 Modality だったら */

		/* j == 0 までカウントダウンすれば, 文節全部 Modality-意思 */
		/*------------------------------------------------
		 *
		 *  (j == 0) ⇒  ■■■■(文節) 全部 Modality-意思
		 *
		 *-----------------------------------------------*/
		for (j = b_ptr->mrph_num; j > 0 ; j--){
		    if(check_f_modal_ISHI(&(b_ptr->mrph_ptr[j-1]))){
			continue;
		    }
		    else {
			break;
		    }
		}

		
		/* 前の文節(子供)が Modality で終わっているかどうか調べておく */
		/*------------------------------------------------
		 *
		 *                      □■■■─┐
		 *  (flag_1 == 1) ⇒              ■■■■
		 *
		 *-----------------------------------------------*/	    
		flag_1 = 0;
		for(i = 0;  b_ptr->child[i]; i++){
		    m_num = b_ptr->child[i]->mrph_num;
		    if(check_f_modal_m(&(b_ptr->child[i]->mrph_ptr[m_num - 1]))) {
			flag_1 = 1 ;
			break;
		    } else {
			continue;
		    }
		}

		/* 後ろの文節(親)の頭が Modality かどうか調べておく */
		/*------------------------------------------------
		 *
		 *  (flag_2 == 1) ⇒    ■■■■─┐
		 *                                ■■■□
		 *
		 *-----------------------------------------------*/	    
		flag_2 = 0;
		if (b_ptr->parent &&
		    check_f_modal_ISHI(b_ptr->parent->mrph_ptr)){
		    flag_2 = 1;
		}

		/* 
		 *文節全部が Modality-意思 かつ, 前後に何もつかないときだけ, 
		 * 筆者と推定する 
		 */
		if (j ==0 && flag_1 == 0 && flag_2 == 0){ 
		    assign_GA2pred(b_ptr, NULL, "Modality保持者", local_sp);
		    goto Match;
		}

		/* 全部Modalityでも, 意思でないときは, 何もしない */
		goto Match;
	    }

	    /* 全部がModalityではないとき */
	    else {
		m_flag_include_ISHI = 0;
		/* Modality-意思 を含むかどうかを判定 */
		for (j = b_ptr->mrph_num; j > 0 ; j--){
		    if(check_f_modal_ISHI(&(b_ptr->mrph_ptr[j-1]))){
			m_flag_include_ISHI = 1;
			break;
		    }
		    else {
			continue;
		    }
		}

		/* Modality-意思 を含んでいれば */
		if(m_flag_include_ISHI == 1){
		    assign_GA2pred(b_ptr, NULL, "Modality保持者", local_sp);
		    goto Match;
		}
	    }
	}



	/*===== 時相関連の処理 =====*/
	if (check_feature(b_ptr->f, "時相関連用言")){
	    for (i = 0; b_ptr->child[i]; i++){
		if(check_feature(b_ptr->child[i]->f, "時間") &&
		   check_feature(b_ptr->child[i]->f, "外の関係")){
		    assign_GA2pred(b_ptr, NULL, "時相", local_sp);
		    goto Match;
		}
	    }
	}


	/*----------------------------*/
        /*    ここからちゃんと処理    */
	/*----------------------------*/


	/*===== まず子供をみる =====*/

        /*
	 * 子孫の"係:未格", "係:ガ格"を持ってくるのは中止
	 *  if (check_primary_child(b_ptr, b_ptr, local_sp)) goto Match; 
	 */


	for (i = 0; b_ptr->child[i]; i++) {

	    /* 普通の子供のとき */
	    if (b_ptr->child[i]->para_top_p != TRUE){
		/*「〜(数量|時間)+も+(用言)」だったら何もしないで他の子供を見る    */
		/* (例) 「国防費が１２．６％も増えているのが目立つ。」 */
		/* (例) 「今年も政局のキーマンの十人を選んでみた。」 */
		if ((check_feature(b_ptr->f, "モ〜") &&
		     check_feature(b_ptr->child[i]->f, "数量")&&
		     check_feature(b_ptr->child[i]->f, "モ")) ||
		    (check_feature(b_ptr->child[i]->f, "時間") &&
		     check_feature(b_ptr->child[i]->f, "モ"))){
		    ;
		}
		
		else if (check_feature(b_ptr->child[i]->f, "係:ガ格")){
		    assign_GA2pred(b_ptr, b_ptr->child[i], "ガ格", local_sp);
		    goto Match;
		    
		} 
		else if (check_feature(b_ptr->child[i]->f, "係:未格")){
		    assign_GA2pred(b_ptr, b_ptr->child[i], "未格", local_sp);
		    goto Match;
		}
	    }

	}


	if (check_feature(b_ptr->f, "係:連格") &&
	    /*
	     * !check_feature(b_ptr->f, "Modality-当為") &&
	     */

	    b_ptr->parent &&
	    b_ptr->parent->para_top_p != TRUE &&
	    (!check_feature(b_ptr->f, "サ変") || 
	     check_feature(b_ptr->f, "〜れる") ||
	     !check_feature(b_ptr->parent->f, "係:デ格")) &&

	    !check_feature(b_ptr->parent->f, "外の関係") &&
	    !check_feature(b_ptr->parent->f, "数量ノ")) {

	    pos_bptr = skip_para_top(b_ptr->parent,"pa");
	    assign_GA2pred(b_ptr, pos_bptr, "修飾先", local_sp);
	    goto Match;
	}


	/*===== 連用修飾先のガ格 =====*/

	if (check_feature(b_ptr->f, "係:連用") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p != TRUE &&
	    check_feature(b_ptr->parent->f, "Cガ格推定")) {

	    assign_GA2pred(b_ptr, b_ptr->parent, "親用言", local_sp);
	    goto Match;
	}
	    /*----------------------------------------------------
	     *  strlen("Cガ格推定:")
	     *   |<------------->|
	     *   -------------------------------------------
	     *   |C|ガ|格|推|定|:| | |...............| | | |
	     *   -------------------------------------------
	     *  ↑              ↑ 
	     *  cp          cp + strlen("Cガ格推定:")
	     *---------------------------------------------------*/


	/* Feb/1st */

	/* まず, 現在の用言がどの格フレーム辞書のデータなのかを特定する */
#if 0
	if (tm->cpm->pred_num == 0){
	    printf("【】なし\n");
	    printf("b_ptrデータ %s\n", b_ptr->Jiritu_Go);
	}
	else {
	    for(i = 0; tm->cpm->pred_b_ptr[i] ; i++){
		j = b_ptr - tm->cpm->pred_b_ptr[i];
		if (j == 0){
		    printf("辞書データ発見 \n");
		    printf("b_ptrデータ : %s\n", b_ptr->Jiritu_Go);
		    printf("格フレーム辞書データ : %s\n", tm->cpm->pred_b_ptr[i]->Jiritu_Go);
		    break;
		}
	    }
	    printf("格フレーム辞書登録なし\n");
	    printf("b_ptrデータ %s\n", b_ptr->Jiritu_Go);
	}
#endif

	for (i = 0; i < tm->pred_num; i++){
	    cpm_ptr = &(tm->cpm[i]);

	/* 格の出力 */
	    for (k = 0; k < cpm_ptr->cf.element_num; k++){

		printf("格要素 : ");
		for (j = 0; j < cpm_ptr->elem_b_ptr[k]->mrph_num; j++){
		    printf(" %s", (cpm_ptr->elem_b_ptr[k]->mrph_ptr + j)->Goi2);
		}
		printf("\n");

		if ((cp = (char *)check_feature(cpm_ptr->elem_b_ptr[k]->f, "係"))
		    != NULL) {
		    if (cpm_ptr->cf.pp[k][0] < 0) {
			printf("(%s)格\n", cp + strlen("係:"));
		    }
		    else {
			printf("(%s)格\n", pp_code_to_kstr(cpm_ptr->cf.pp[k][0]));
		    }
		}
	    }
	}
	/* 次の解析のために初期化しておく */
	tm->pred_num = 0;



#if 0
	/*===== 形判連体または動詞連体は、子か親の「〜の」を探す =====*/

	/* 対 親は, "係:連格"の処理で対処 */
	/* (例) 「厳しいソ連の交渉」     */

	/* 対 子供 */
	/* (例) 「打撃の大きい規制強化」 */
	for (i = 0; b_ptr->child[i]; i++) {
	    m_num = b_ptr->child[i]->mrph_num;
	    if((check_feature(b_ptr->f, "ID:（形判連体）") ||
		check_feature(b_ptr->f, "ID:（動詞連体）")) &&
	        m_num > 0 &&
	       (strcmp(NO, b_ptr->child[i]->mrph_ptr[m_num-1].Goi) == 0) &&
	        check_feature(b_ptr->child[i]->f, "体言")){

		pos_bptr = skip_para_top(b_ptr->child[i], "ch");
		assign_GA2pred(b_ptr, pos_bptr, "修飾元(AのB)", local_sp);
		goto Match;
	    }
	}
#endif

	/*===== 修飾先が同じ動詞＋形容詞の並びのとき =====*/

/* 0 -> comment*/
#if 0 
	if (check_feature(b_ptr->f, "用言:動") &&
	    (!check_feature(b_ptr->f, "補文ト")) &&
	    b_ptr->parent){
	    s = position_in_children(b_ptr);
	    if ((s > 0) &&
	        check_feature(b_ptr->parent->child[s-1]->f, "用言:形") &&
		b_ptr->child[0]){

		for(i = 0; b_ptr->child[i]; i++){
		    GA_detection(b_ptr->child[i]);
		}
		for(i = 0; b_ptr->child[i]; i++){
		    if (check_feature(b_ptr->child[i]->f, "Cガ格未発見") ||
			check_feature(b_ptr->child[i]->f, "Cガ格推定:前文") ||
			(check_feature(b_ptr->child[i]->f, "Cガ格推定") == NULL)){

			assign_GA2pred(b_ptr, "[筆者]", "省略推定", local_sp);
			goto Match;
			}

		    else if (check_feature(b_ptr->child[i]->f, "Cガ格推定")){
			/* 
			 *  "Cガ格推定" の feature を持っている b_ptr には, 
			 *  skip_para_top する必要はない.
			 */
			assign_GA2pred(b_ptr, b_ptr->child[i], "修飾元(動＋形)", local_sp);
			goto Match;
		    }
		}
	    }
	}
	    /*----------------------------------------------------
	     *  (こんなとき)
	     *
             *       良く──┐　　　　　
             *         ☆  遊んだ──┐☆  <Cガ格推定:省略推定:筆者>
             *               古い──┤　<Cガ格推定:修飾先:寺>
             *                         寺
	     *---------------------------------------------------*/
#endif

	/*
	 * if (b_ptr->para_type == PARA_NORMAL) {
	 *    for (i = 0; b_ptr->parent->child[i]; i++) {
	 * 	if (b_ptr->parent->child[i]->para_top_p != TRUE &&
	 *	    b_ptr->parent->child[i]->para_type == PARA_NIL &&
	 *	    check_feature(b_ptr->parent->child[i]->f, "係:ガ格")) {
	 *
	 *	    assign_GA2pred(b_ptr, b_ptr->parent->child[i], "ガ格", local_sp);
	 *	    goto Match;
	 *	}
	 *	else if (b_ptr->parent->child[i]->para_top_p != TRUE &&
	 *		 b_ptr->parent->child[i]->para_type == PARA_NIL &&
	 *		 check_feature(b_ptr->parent->child[i]->f, "係:未格")) {
	 *	    assign_GA2pred(b_ptr, b_ptr->parent->child[i], "未格", local_sp);
	 *	    goto Match;
	 *	}
	 *	
	 *    }		
	 *  }
	 */



	/*===== 連体修飾の先をみる =====*/

#if 0
/* 格解析の結果を用いる */

	m_num = b_ptr->mrph_num;
	if (check_feature(b_ptr->f, "係:連格") &&
	    check_feature(b_ptr->f, "用言:動") &&
	    !check_feature(b_ptr->f, "サ変") &&
	    b_ptr->parent &&
	    check_feature(b_ptr->parent->f, "係:ヲ格") &&
	    b_ptr->parent->parent &&
	    check_feature(b_ptr->parent->parent->f, "Cガ格推定")) { 

		assign_GA2pred(b_ptr, b_ptr->parent->parent, "親用言", local_sp);
		goto Match;
	}
	    /*----------------------------------------------------
	     * (こんなとき)
	     *
             *                        HOGEは   
             *                     　　　│
             *    用言:動(自分)━┓      │ <Cガ格推定:修飾先:HOGE>
             *                   ┃　　　│　
             *                係:ヲ格━━┥　
             *                           C  <Cガ格推定:未格:HOGE>
	     *
             * 例)大蔵省は認める方針を決めた
	     *--------------------------------------------------*/


	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->para_type != PARA_NORMAL &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p == TRUE &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    !check_feature(b_ptr->parent->parent->f, "外の関係")&&
	    check_feature(b_ptr->parent->parent->f, "Cガ格推定")){

	    assign_GA2pred(b_ptr, b_ptr->parent->parent, "親用言", local_sp);
	    goto Match;
	}
	    /*----------------------------------------------------
	     * (こんなとき)
	     *
             *           繰り返す━━┓     漁船員は   <Cガ格推定:漁船員は>
             *                       ┃　　　│
             *   ことのないよう <P>─┨      │
             *                       ┃　　　│　
             *           善処を <P>─PARA━━┥　
             *                          期待したい <Cガ格推定:未格:漁船員は>
	     *
             * 例)寄港は認めたものの、漁船員は岸壁にくぎ付けといった
	     *    事態を繰り返すことのないよう、関係者の善処を期待し
	     *    たい。
	     *--------------------------------------------------*/


	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p == TRUE &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    !check_feature(b_ptr->parent->parent->f, "外の関係") &&
	    (check_feature(b_ptr->parent->parent->f, "係:ガ格") ||
	     check_feature(b_ptr->parent->parent->f, "係:未格"))) {

	    pos_bptr = skip_para_top(b_ptr->parent->parent, "pa");
	    assign_GA2pred(b_ptr, pos_bptr, "修飾先", local_sp);
	    goto Match;
	}
	    /*----------------------------------------------------
	     * (こんなとき)
	     *
             *     自分━━┓     
             *             ┃　　　
             *      A <P>─┨      
             *             ┃　　　　
             *      B <P>─PARA━━┓　
             *                    HOGE(連用)は 
	     *
	     * 例) 議員は「代理」ではなく「代表」だと思ってないか
	    ----------------------------------------------------*/


	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p == TRUE &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    check_feature(b_ptr->parent->parent->f, "外の関係") &&
	    check_feature(b_ptr->parent->parent->f, "係:連用")&&
	    b_ptr->parent->parent->parent &&
	    check_feature(b_ptr->parent->parent->parent->f, "Cガ格推定")) {

	    assign_GA2pred(b_ptr, b_ptr->parent->parent->parent, "親用言", local_sp);
	    goto Match;
	}


	if (check_feature(b_ptr->f, "係:連格") && 
	    b_ptr->parent &&
	    check_feature(b_ptr->parent->f, "外の関係") &&
	    check_feature(b_ptr->parent->f, "係:ニ格") &&
	    b_ptr->parent->parent &&
	    check_feature(b_ptr->parent->parent->f, "Cガ格推定")){

	    assign_GA2pred(b_ptr, b_ptr->parent->parent, "親用言", local_sp);
	    goto Match;
	}

	    /*----------------------------------------------------
	     * (こんなとき)
	     *                    HOGEは
	     *                     │          
	     *   用言A(自分)━┓   │    <Cガ格推定:親用言:HOGEは>
	     *               〜に━┫
	     *                     用言B <Cガ格推定:未格:HOGEは>
	     *
	     * 例)日本信託は,経営再建を図ることになった
	     *---------------------------------------------------*/


	/*===== "Modality-当為" のときは除去済み. 修飾先 =====*/
	m_num = b_ptr->mrph_num;
	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->parent &&
	    (strcmp(NO, b_ptr->parent->mrph_ptr[m_num-1].Goi) == 0) &&
	    b_ptr->parent->mrph_ptr[m_num-1].Bunrui == 3 &&  /* 分類番号 3 は接続助詞  */
	    b_ptr->parent->parent &&
	    check_feature(b_ptr->parent->parent->f, "体言")){

	    pos_bptr = skip_para_top(b_ptr->parent->parent,"pa");
	    assign_GA2pred(b_ptr, pos_bptr, "修飾先(Jump Indec+Conj)", local_sp);
	    goto Match;
	}
	    /*----------------------------------------------------
	     * (こんなとき)
	     *
	     *      ねじ曲がった━━┓　　　　　
	     *                    最良の━━┓　 「の(接続助詞)」ならその先
	     *                             英知
	     *
	     * 例)ねじ曲がった最良の英知
	     *---------------------------------------------------*/





	if (check_feature(b_ptr->f, "係:連格") &&
	    /*
	     * !check_feature(b_ptr->f, "Modality-当為") &&
	     */

	    b_ptr->parent &&
	    b_ptr->parent->para_top_p != TRUE &&
	    (!check_feature(b_ptr->f, "サ変") || 
	     check_feature(b_ptr->f, "〜れる") ||
	     !check_feature(b_ptr->parent->f, "係:デ格")) &&

	    !check_feature(b_ptr->parent->f, "外の関係") &&
	    !check_feature(b_ptr->parent->f, "数量ノ")) {

	    pos_bptr = skip_para_top(b_ptr->parent,"pa");
	    assign_GA2pred(b_ptr, pos_bptr, "修飾先", local_sp);
	    goto Match;
	}




	/* この処理, 連格の前がいいのか後ろがいいのか,,, */
	/*===== PARAに係っている部分 =====*/

	if(b_ptr->para_type == PARA_NORMAL &&
	   b_ptr->parent->para_top_p == TRUE &&
	   ((pos_bptr = 
	    check_posterity(b_ptr->parent, (s = position_in_children(b_ptr)+1))) != NULL) &&
	   !check_feature(pos_bptr->f, "外の関係")){

	    assign_GA2pred(b_ptr, pos_bptr, "PARA修飾元", local_sp);
	    goto Match;
	}
	    /*-----------------------------------------------------------------------------
	     * (こんなとき)
	     *
  	     *          金権選挙、<P>─┐　　　　　　　　　　　　
             *      利益誘導政治は<P>━PARA━━┓　　　　　　　　
             *        解消されるどころか、<P>━┫　　　　　　　　<- PARA修飾元:利益誘導政治は
             *              一層まん延する<P>━PARA       　　　<- PARA修飾元:利益誘導政治は
	     *
	     * 
	     * 金権選挙、利益誘導政治は解消されるどころか、一層まん延する
	     *-----------------------------------------------------------------------------*/


	/*===== 連用修飾先のガ格 =====*/

	if (check_feature(b_ptr->f, "係:連用") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p != TRUE &&
	    check_feature(b_ptr->parent->f, "Cガ格推定")) {

	    assign_GA2pred(b_ptr, b_ptr->parent, "親用言", local_sp);
	    goto Match;
	}
	    /*----------------------------------------------------
	     *  strlen("Cガ格推定:")
	     *   |<------------->|
	     *   -------------------------------------------
	     *   |C|ガ|格|推|定|:| | |...............| | | |
	     *   -------------------------------------------
	     *  ↑              ↑ 
	     *  cp          cp + strlen("Cガ格推定:")
	     *---------------------------------------------------*/




	/*===== 遠くの親(祖先) =====*/

	if (b_ptr->parent &&
	    check_feature(b_ptr->parent->f, "係:ノ格") &&
	    check_feature(b_ptr->parent->f, "体言") &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    check_feature(b_ptr->parent->parent->f, "係:ノ格") &&
	    check_feature(b_ptr->parent->parent->f, "体言")){
	    
	    pos_bptr = skip_para_top(b_ptr->parent->parent,"pa");
	    assign_GA2pred(b_ptr, pos_bptr, "修飾先(GrandParent)", local_sp);
	    goto Match;
	}
	    /* (例) 揺れる彼女の複雑な気持の断片 */
	    /*----------------------------------------------------
             *   揺れる━━┓　　　　　　　　　<Cガ格推定:修飾先:彼女>
             *           彼女の━━┓　　　　　
             *           複雑な──┨　　　　　<Cガ格推定:修飾先:気持>
             *                   気持の──┐　
             *                            断片
	     *----------------------------------------------------*/


	if (check_feature(b_ptr->f, "ID:〜と（いう）") &&
	    b_ptr->parent->parent &&
	    check_feature(b_ptr->parent->parent->f, "係:ノ格") &&
	    check_feature(b_ptr->parent->parent->f, "体言") &&
	    b_ptr->parent->parent->parent &&
	    check_feature(b_ptr->parent->parent->parent->f, "係:ノ格") &&
	    check_feature(b_ptr->parent->parent->parent->f, "体言")){

	    pos_bptr = skip_para_top(b_ptr->parent->parent->parent,"pa");
	    assign_GA2pred(b_ptr, pos_bptr, "修飾先(GrandParent)", local_sp);
	    goto Match;
	}
	    /* (例) 押さえ込むという再選後の大統領の方針 */
	    /*----------------------------------------------------
             *  押さえ込むと━━┓　　　<Cガ格推定:修飾先(GrandParent):大統領>
             *                  いう━━┓　　　　　　　　　
             *                      再選後の━━┓　　　　　
             *                              大統領の──┐　
             *                                         方針
	     *----------------------------------------------------*/


	/*===== 係:連用, 係:連格 どちらでもよい =====*/
	if(check_feature(b_ptr->f, "ID:〜（ため）")&&
	   check_feature(b_ptr->f, "レベル:B+") &&
	   b_ptr->parent &&
	   check_feature(b_ptr->parent->f, "外の関係") &&
	   check_feature(b_ptr->parent->f, "係:連用") &&
	   check_feature(b_ptr->parent->f, "ID:〜ため") &&
	   check_feature(b_ptr->parent->f, "レベル:B+") &&
	   b_ptr->parent->parent &&
	   check_feature(b_ptr->parent->parent->f, "用言:動") &&
	   check_feature(b_ptr->parent->parent->f, "Cガ格推定")){

	    assign_GA2pred(b_ptr, b_ptr->parent->parent, "親用言", local_sp);
	    goto Match;
	}

	    /*----------------------------------------------------
	     * (こんなとき)
	     *                             HOGEは
	     *                              │
             * (レベルB+) Aする━━┓　     │　<Cガ格推定:親用言:HOGEは、>
             *       (レベルB+)   ため、━━┫　
             *      　　　　　　　　　　    ┃　
          　 *　　                   Bを──┨　
	     *                              ┃
             *                             Cした <Cガ格推定:未格:HOGEは、>
	     *
	     * 例)大蔵省は、不良債権の処理を促進するため、特別留保金
	     *    の取り崩しを認める方針を決めた
	     *---------------------------------------------------*/




	/*===== 係:連用 でも 係:連格 でもないけど, 親用言=====*/
	if(check_feature(b_ptr->f, "ト") &&
	   b_ptr->parent &&
	   check_feature(b_ptr->parent->f, "Cガ格推定")){

		assign_GA2pred(b_ptr, b_ptr->parent, "親用言", local_sp);
		goto Match;
	}
	    /* (例) 心を洗われた、と山花郁子が投書していた。*/
	    /*----------------------------------------------------
	     *  心を ──┐　　　　　
	     *      洗われた、と━━┓　<Cガ格推定:親用言:山花郁子が>
	     *        山花郁子が──┨　
             *            投書していた。<Cガ格推定:ガ格:山花郁子が>
	     *----------------------------------------------------*/


	/*===== もう一度 PARA チェック =====*/

	/*
	 * if ((b_ptr->para_type == PARA_NORMAL) &&
	 *    b_ptr->parent &&
	 *    (b_ptr->parent->para_top_p == TRUE) &&
	 *    (b_ptr->parent->parent == FALSE)){
	 */
	 
	if ((b_ptr->para_type == PARA_NORMAL) &&
	     b_ptr->parent &&
	    (b_ptr->parent->para_top_p == TRUE)){
	    /*弟妹のガ格を持ってきたい*/
	    s = position_in_children(b_ptr);
	    if ((s >= 0) &&
		b_ptr->parent->child[s+1]){
		GA_detection(b_ptr->parent->child[s+1]);
		for(i = s+1; b_ptr->parent->child[i]; i++){
		    if ((cp = (char *)check_feature(b_ptr->parent->child[i]->f, "Cガ格推定")) != NULL){

			assign_GA2pred(b_ptr, b_ptr->parent->child[i], "PARA弟", local_sp);
			goto Match;
		    }
		}
	    }
	}
	    /* (例) レーガン政権も二期目に入り、こうした現実を直視せざるを
	     *  えなくなった。 */
	    /*-------------------------------------------------------------------
	     *       レーガン政権も──┐　　　　　
             *             二期目に──┤　　　　　
             *                      入り、<P>━┓　<Cガ格推定:未格:レーガン政権>
             *     こうした──┐　　　　　　　┃　
             *               現実を──┐　　　┃　
             *  直視せざるをえなくなった。<P>━PARA<Cガ格推定:PARA弟:未格:レーガン政権>
	     *-------------------------------------------------------------------*/

/* 格解析の結果を用いるべき. ここまで */
#endif


#if 0
/* 
 * 子孫の"係:未格", "係:ガ格"を持ってくるのは, ちょっと精度が悪いので,
 * 今は保留. プログラムの動作としては, 正く動く.
 */
	/*===== あきらめる前に, 子孫を探してみよう =====*/

	if ((pos_bptr = check_posterity(b_ptr, 0)) != NULL) {

	    assign_GA2pred(b_ptr, pos_bptr, "子孫", local_sp);
	    goto Match;
	}
#endif


#if 0 /* ここの処理も格解析の結果を用いると不要になる */

	/*===== あきらめる前に, 先祖を探してみよう =====*/

	/*
	 * (NOTE)
	 * この処理が本質的に正しいとは言えない。
	 * しかし、この処理を入れると正解率は上がる。
	 * 係り先が正しく解析されていれば、正しい処理と言えるかも
	 * しれないが、、、。
	 */

	/*
	 * (アルゴリズム)
	 *
	 * まず, 親の全リストを作る.自分の直親から, 最後の祖先まで. 
	 * (最後は根っこになる?)
	 *
	 * 次に, そのリストの各要素(親, 祖先)について, 自分の弟・
	 * 妹側の子供(第一親等, つまり一段階のみ)に, ハ格またはガ格
	 * (正確に, "は" or "が"で終わるハ格 or ガ格)を持っていない
	 * かをチェック.
	 *
	 * あれば, それをガ格候補とする.
	 *
	 * 
	 */

	if ((pos_bptr = check_ancestor(b_ptr)) != NULL){
	    assign_GA2pred(b_ptr, pos_bptr, "祖先", local_sp);
	    goto Match;
	}
#endif


	/*===== 全部だめなら前の文 =====*/

	if (sp->Sen_num >= 2){
	    prev_sp = sentence_data + (sp->Sen_num - 2); 
	    if (prev_sp->Bnst_num >= 1 ){
		if ((cp = check_feature(((prev_sp->bnst_data) + prev_sp->Bnst_num - 1)->f,"Cガ格推定")) != NULL) {

		    assign_GA2pred(b_ptr, ((prev_sp->bnst_data) + prev_sp->Bnst_num - 1) , "前文", local_sp);
		    goto Match;
		}
	    }
	}

	assign_cfeature(&(b_ptr->f), "Cガ格未発見");

    }
    Match:

    for (i = 0; b_ptr->child[i]; i++) {
	GA_detection(b_ptr->child[i]);
    }
}    



/*==================================================================*/
		      void clear_context(int f)   
/*==================================================================*/

/*sentence_data があふれるか, 文章が変わったら, sentence_dataをclear*/
/* 実際の判定は, read_data.c の中でやって, そこから呼び出している   */

{
    int i;
    SENTENCE_DATA *stock_sp_1, *stock_sp_2;


    /* sentence_data があふれそうなとき (sp->Sen_num ==256)*/
    if (f == 0){  

	/* 最後の7文(人間の短期記憶マジックナンバー) のデータは残す*/
	for(i = 6; i >= 0; i--){
	    free(sentence_data - (249 + i));
	    stock_sp_1 = sentence_data - i;
	    stock_sp_2 = sentence_data - (249 + i);
	    stock_sp_2 = stock_sp_1;
	}
	for( i = 0; i <= 249; i++){
	    free(sentence_data - i);
	}
	sp->Sen_num = 7;
    } 

    /* S_ID が変わった */
    else if (f == 1){
	for(i = 0; i < sp->Sen_num ; i++){
	    free(sentence_data -i);
/*
 *	for(i = sp->Sen_num; i > 0 ; i--){
 *	    printf("Sentence Data : %s", sentence_data - i);
 *	    free(sentence_data -i);
 */
	}

	    sp->Sen_num = 0;
    }
    return;
}


/*==================================================================*/
		      void discourse_analysis()
/*==================================================================*/
{

    GA_detection(sp->bnst_data + (sp->Bnst_num -1));
    
    copy_sentence();
}


/*===== 格解析の結果をガ格推定に用いるときのアルゴリズム案 =====*/

/*
 * 以下 「格解析」を「C解」と略記
 *
 * ある用言に対して, C解の結果をのぞく
 * 
 * (I)C解の結果が一つ, つまり Best_mgr->cpm->result_num == 1 のとき
 *
 *    (I-i)
 *     ガ格のエントリーに何かあれば, それをガ格候補とする
 *
 *    (I-ii)
 *    なければ, 同一文中のガ格,未格のものについて, 格解析のガ格の
 *    エントリー例との類似度を計算して, ある点数以上で最も得点が高い
 *    ものをガ格候補とする.
 *
 *    (I-iii)
 *    同一文中になければ、前文に対して(I-ii)と同じことを行う.
 *    (どのくらい先の文まで見る?)
 *   
 * (II)C解の結果が一つ以上, つまり Best_mgr->cpm->result_num > 1 のとき
 *
 *    (II-i)
 *     とりあえず, 各項目のガ格のエントリーを見る.
 *     全部同じ, または多数派なら, それをガ格にする.
 *     そうじゃなければ, 例との類似度が高いものにするしかないかな??
 * 
 *    (II-ii)
 *     なければ, (I-ii)->(I-iii)を実行する.
 *
 * 
 */



/*====================================================================
                               END
====================================================================*/

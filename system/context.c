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

/*==================================================================*/
			 void copy_sentence()
/*==================================================================*/
{
    /* 文解析結果の保持 */

    int i, j;
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
	sp_new->bnst_data[i] = sp->bnst_data[i];
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
void assign_GA2pred(BNST_DATA *pred_ptr, BNST_DATA *GA_ptr, char *comment)
/*==================================================================*/
{
    char result[256];
    sprintf(result, "Cガ格推定:%s:", comment);
    print_bnst(GA_ptr, result+strlen(result));
    assign_cfeature(&(pred_ptr->f), result);
    /* printf("  %s\n", result); */
}    

/*==================================================================*/
		 void GA_detection(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i;
    char *cp;
    SENTENCE_DATA *prev_sp;

    /* 用言のガ格をさがすプロトタイプ */

    if (b_ptr->para_top_p == TRUE) goto Match;

    if (check_feature(b_ptr->f, "用言") &&
	!check_feature(b_ptr->f, "レベル:A-")) {

	/* printf("用言発見 %s\n", b_ptr->Jiritu_Go); */

	/* まず子供をみる */

	for (i = 0; b_ptr->child[i]; i++) {
	    if (check_feature(b_ptr->child[i]->f, "係:ガ格")) {
		assign_GA2pred(b_ptr, b_ptr->child[i], "ガ格");
		goto Match;
	    } else if (check_feature(b_ptr->child[i]->f, "係:未格")) {
		assign_GA2pred(b_ptr, b_ptr->child[i], "未格");
		goto Match;
	    }
	}

	/* PARAに係っている部分 */

	if (b_ptr->para_type == PARA_NORMAL) {
	    for (i = 0; b_ptr->parent->child[i]; i++) {
		if (b_ptr->parent->child[i]->para_type == PARA_NIL &&
		    check_feature(b_ptr->parent->child[i]->f, "係:ガ格")) {
		    assign_GA2pred(b_ptr, b_ptr->parent->child[i], "ガ格");
		    goto Match;
		}
		else if (b_ptr->parent->child[i]->para_type == PARA_NIL &&
			 check_feature(b_ptr->parent->child[i]->f, "係:未格")) {
		    assign_GA2pred(b_ptr, b_ptr->parent->child[i], "未格");
		    goto Match;
		}
		
	    }		
	}

	/* 連体修飾の先をみる */
	    
	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p != TRUE &&
	    !check_feature(b_ptr->parent->f, "外の関係")) {
	    assign_GA2pred(b_ptr, b_ptr->parent, "修飾先");
	    goto Match;
	}
	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->para_type != PARA_NORMAL &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p == TRUE &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    !check_feature(b_ptr->parent->parent->f, "外の関係")&&
	    (cp = (char *)check_feature(b_ptr->parent->parent->f, "Cガ格推定")) != NULL) {
	    /* assign_GA2pred(b_ptr, cp + strlen("Cガ格推定:"), "親用言"); */
	    goto Match;
/*
  (こんなとき)

            自分━━┓     HOGEは   <Cガ格推定:HOGE>
                    ┃　　　│
             A <P>─┨      │
                    ┃　　　│　
             B <P>─PARA━━┥　
                            C <Cガ格推定:未格:HOGE>

            例)寄港は認めたものの、漁船員は岸壁にくぎ付けといった事態
               を繰り返すことのないよう、関係者の善処を期待したい。
*/

	}
	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p == TRUE &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    !check_feature(b_ptr->parent->parent->f, "外の関係")) {
	    assign_GA2pred(b_ptr, b_ptr->parent->parent, "修飾先");
	    goto Match;
	}
	if (check_feature(b_ptr->f, "係:連格") &&
	    b_ptr->parent &&
	    b_ptr->parent->para_top_p == TRUE &&
	    b_ptr->parent->parent &&
	    b_ptr->parent->parent->para_top_p != TRUE &&
	    check_feature(b_ptr->parent->parent->f, "外の関係") &&
	    check_feature(b_ptr->parent->parent->f, "係:連用")&&
	    (cp = (char *)check_feature(b_ptr->parent->parent->parent->f, "Cガ格推定")) != NULL) {
	    /* assign_GA2pred(b_ptr, cp + strlen("Cガ格推定:"), "親用言"); */
	    goto Match;
	}


	/* 連用修飾先のガ格 */

	if (check_feature(b_ptr->f, "係:連用") &&
	    b_ptr->parent &&
	    (cp = (char *)check_feature(b_ptr->parent->f, "Cガ格推定")) != NULL) {
	    /* assign_GA2pred(b_ptr, cp + strlen("Cガ格推定:"), "親用言"); */
	    goto Match;
	}

	/* 全部だめなら前の文 */

	if (sp->Sen_num >= 2){
	    prev_sp = sentence_data + (sp->Sen_num - 2); 
	    if (prev_sp->Bnst_num >= 1 ){
		if ((cp = (char *)check_feature(prev_sp->bnst_data[prev_sp->Bnst_num - 1].f,
						"Cガ格推定")) != NULL) {
		    /* assign_GA2pred(b_ptr, cp + strlen("Cガ格推定:"), "前文"); */
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
/* sentence_data があふれるか, または文章が変わったとき( "# S-ID:"の
   番号で識別), sentence_data をclear する */
{
    int i;
    SENTENCE_DATA *stock_sp_1, *stock_sp_2;


/*  f == 0 : sp->Sen_num == 256 */
/*  f == 1 : # S-ID が変わったか, "【’"で違う社説になったことを検知 */
    if (f == 0){
	/* sentence_data があふれそうなとき*/

	/* 最後の7文(人間の短期記憶マジックナンバー) のデータは残す*/
	for(i = 6; i >= 0; i--){
	    free(sentence_data - (249 + i));
	    stock_sp_1 = sentence_data - i;
	    stock_sp_2 = sentence_data - (249 + i);
	    stock_sp_2 = stock_sp_1;
	    sp->Sen_num = 7;
	}
	for( i = 0; i <= 249; i++){
	    free(sentence_data - i);
	}
	sp->Sen_num = 7;
    } 
    else if (f == 1){

    /* S_ID が変わる, または別の社説になった("【’"で始まる行) */
	for(i = 0; i < sp->Sen_num ; i++){
	    free(sentence_data -i);
	    sp->Sen_num = 0;
	}
    }
    return;
}

/*==================================================================*/
		      void discourse_analysis()
/*==================================================================*/
{
    int i, flag;
    char *cp;
    char kakko[] = "【’";

    if(sp->Sen_num == 256){
    /* sentence_data が overflow しそう */
	printf("The program celars the sentence_data due to the overflow.\n\n");
	flag = 0;
	clear_context(flag);
	return;
    }

    /* 文章の区切りは, S-IDの番号か, 文頭が「【’」で始まる文かで検知する*/

    else if( strcmp("\\",sp->bnst_data[0].mrph_ptr->Goi) == 0){
	/* 「# S-ID:」で始まる行の sp->bnst_data[0].mrph_ptr->Goi は、なぜか"\" 
	   ("#"じゃない)*/

	if (strncmp("A-ID",sp->bnst_data[1].mrph_ptr->Goi, 4) == 0){
	}
	else if	((strncmp("S-ID",sp->bnst_data[1].mrph_ptr->Goi, 4) == 0)&&
		 (strncmp(SID_box, sp->bnst_data[1].mrph_ptr->Goi, 14) != 0)){
	/* S-ID をチェック. 番号が変わったら clear_context */

	    /* S-ID:950101008 は全部で14文字. 14文字分SID_boxにコピー */
		strncpy(SID_box, sp->bnst_data[1].mrph_ptr->Goi, 14);
		flag = 1;
		clear_context(flag);
	}
	return;

    } else if ( strcmp(kakko, sp->bnst_data[0].mrph_ptr->Goi) == 0){
	/* 「【’」 で始まる行(社説の変わり目) */
	flag = 1; 
	clear_context(flag);
	return;
    }


    GA_detection(sp->bnst_data + sp->Bnst_num -1);
    
    copy_sentence();
}

/*====================================================================
                               END
====================================================================*/

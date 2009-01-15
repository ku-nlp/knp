/*====================================================================

			       照応解析

                                         Ryohei Sasano 2007. 8. 27

    $Id$
====================================================================*/

#include "knp.h"

/* 省略解析に関するパラメータ */
#define CASE_CANDIDATE_MAX 20  /* 照応解析用格解析結果を保持する数 */
#define ELLIPSIS_RESULT_MAX 20  /* 省略解析結果を保持する数 */
#define EX_MATCH_COMPENSATE 1.0 /* マッチしすぎることを防ぐための補正項 */
#define SALIENCE_DECAY_RATE 0.8 /* salience_scoreの減衰率 */
#define SALIENCE_THREHOLD 1.0 /* 解析対象とするsalience_scoreの閾値(=は含まない) */
#define INITIAL_SCORE -10000

/* 文の出現要素に与えるsalience_score */
#define SALIENCE_THEMA 2.0 /* 重要な要素(未格,文末)に与える */
#define SALIENCE_NORMAL 1.0 /* 通常の要素に与える、基本的にSALIENCE_THREHOLDと同じ値 */
#define SALIENCE_CANDIDATE 1.1 /* 単独で先行詞候補としたい要素(ガ格,ヲ格)に与える */
#define SALIENCE_ZERO 1.0 /* ゼロ代名詞に与える */

/* 位置カテゴリ(主節や用言であるか等は無視)    */
#define	LOC_SELF             0 /* 自分自身     */
#define	LOC_PARENT           1 /* 親           */
#define	LOC_CHILD            2 /* 子供         */
#define LOC_PARA_PARENT      3 /* 並列(親側)   */
#define	LOC_PARA_CHILD       4 /* 並列(子側)   */
#define	LOC_PARENT_N_PARENT  5 /* 親体言の親   */
#define	LOC_PARENT_V_PARENT  6 /* 親用言の親   */
#define	LOC_OTHERS_BEFORE    7 /* その他(前)   */
#define	LOC_OTHERS_AFTER     8 /* その他(後)   */
#define	LOC_OTHERS_THEME     9 /* その他(主題) */

/* 位置カテゴリを保持 */
int loc_category[BNST_MAX];

/* 解析結果を保持するためのENTITY_CASE_MGR
   先頭のCASE_CANDIDATE_MAX個に照応解析用格解析の結果の上位の保持、
   次のELLIPSIS_RESULT_MAX個には省略解析結果のベスト解の保持、
   最後の1個は現在の解析結果の保持に使用する */
CF_TAG_MGR work_ctm[CASE_CANDIDATE_MAX + ELLIPSIS_RESULT_MAX + 1];

/* 省略解析の対象とする格のリスト */
char *ELLIPSIS_CASE_LIST_VERB[] = {"ガ", "ヲ", "ニ", "\0"};
char *ELLIPSIS_CASE_LIST_NOUN[] = {"ノ", "ノ？", "\0"};
char **ELLIPSIS_CASE_LIST = ELLIPSIS_CASE_LIST_VERB;

/*==================================================================*/
	    int match_ellipsis_case(char *key, char **list)
/*==================================================================*/
{
    /* keyが省略対象格のいずれかとマッチするかどうかをチェック */
    int i;

    /* 引数がある場合はそのリストを、
       ない場合はELLIPSIS_CASE_LISTをチェックする */
    if (!list) list = ELLIPSIS_CASE_LIST;

    for (i = 0; *list[i]; i++) {
	if (!strcmp(key, list[i])) return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
	       void assign_mrph_num(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 文先頭からその形態素の終りまでの文字数を与える */
    int i, count = 0;

    for (i = 0; i < sp->Mrph_num; i++) {
	count += strlen((sp->mrph_data + i)->Goi2) / 2;
	(sp->mrph_data + i)->Num = count;
    }
}

/*==================================================================*/
	   TAG_DATA *substance_tag_ptr(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* tag_ptrの実体を返す関数(並列構造への対処のため) */
    /* castすることによりbnst_ptrに対しても使用 */
    while (tag_ptr && tag_ptr->para_top_p) tag_ptr = tag_ptr->child[0];
    return tag_ptr;
}

/*==================================================================*/
int get_location(char *loc_name, int sent_num, char *kstr, MENTION *mention)
/*==================================================================*/
{
    /* 同一文の場合は*/
    if (mention->sent_num == sent_num) {
	sprintf(loc_name, "%s-%c-C%d", 
		kstr, (mention->flag == '=') ? 'S' : mention->flag,
		loc_category[mention->tag_ptr->b_ptr->num]);
	return TRUE;
    }
    else if (sent_num - mention->sent_num == 1 &&
	     (check_feature(mention->tag_ptr->f, "文頭") ||
	      check_feature(mention->tag_ptr->f, "読点")) &&
	     check_feature(mention->tag_ptr->f, "ハ")) {
	sprintf(loc_name, "%s-%c-B1B",
		kstr, (mention->flag == '=') ? 'S' : mention->flag);
	return TRUE;
    }
    else if (sent_num - mention->sent_num == 1 &&
	     check_feature(mention->tag_ptr->f, "文末") &&
	     check_feature(mention->tag_ptr->f, "用言:判")) {
	sprintf(loc_name, "%s-%c-B1E", 
		kstr, (mention->flag == '=') ? 'S' : mention->flag);
	return TRUE;
    }
    else if (sent_num - mention->sent_num > 0) {
	sprintf(loc_name, "%s-%c-B%d", 
		kstr, (mention->flag == '=') ? 'S' : mention->flag,
		(sent_num - mention->sent_num <= 3 ) ? 
		sent_num - mention->sent_num : 0);
	return TRUE;
    }
    else {
	return FALSE;
    }
}

/*==================================================================*/
     void mark_loc_category(SENTENCE_DATA *sp, TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* 文節ごとに位置カテゴリを付与する */
    /* 格要素ではなく用言(名詞も含む)側に付与 */
    int i, j;
    BNST_DATA *bnst_ptr, *parent_ptr = NULL, *pparent_ptr = NULL;

    bnst_ptr = (BNST_DATA *)substance_tag_ptr((TAG_DATA *)tag_ptr->b_ptr);

    /* 初期化 */
    /* その他(前) */
    for (i = 0; i < bnst_ptr->num; i++) loc_category[i] = LOC_OTHERS_BEFORE;
    /* その他(後) */
    for (i = bnst_ptr->num + 1; i < sp->Bnst_num; i++) 
	loc_category[i] = LOC_OTHERS_AFTER;
    /* その他(主題) */
    for (i = 0; i < bnst_ptr->num; i++) {
	if ((check_feature(sp->bnst_data[i].f, "文頭") ||
	     check_feature(sp->bnst_data[i].f, "読点")) &&
	    (sp->bnst_data[i].parent)->num > bnst_ptr->num &&
	    check_feature(sp->bnst_data[i].f, "ハ")) loc_category[i] = LOC_OTHERS_THEME;
    }
    loc_category[bnst_ptr->num] = LOC_SELF; /* 自分自身 */

    /* 自分が並列である場合 */    
    /* KNPの並列構造(半角数字は文節番号)                  */
    /*                      １と0<P>─┐　　　　　        */
    /*                      ２、1<P>─┤　　　　　        */
    /*             Ａと2<P>─┐　 　　│　　　　　        */
    /*             Ｂと3<P>─┤　 　　│　　　　　        */
    /*             Ｃ、4<P>─┤　 　　│　　　　　        */
    /* アと 5<P>─┐　   　　│　 　　│　　　　　        */
    /* イの10<P>-PARA9<P>-PARA8<P>─PARA6──┐　         */
    /*                                     関係。7        */
    /* 文節6,7のみpara_type=PARA_NIL、6,8,9がpara_top_p=1 */
    if (bnst_ptr->para_type == PARA_NORMAL) {
	for (i = 0; bnst_ptr->parent->child[i]; i++) {

	    if (bnst_ptr->parent->child[i]->para_type == PARA_NORMAL &&
		/* todo::とりあえず並列の並列は無視 */
		!bnst_ptr->parent->child[i]->para_top_p) {

		/* 並列(親側) */
		if (bnst_ptr->parent->child[i]->num > bnst_ptr->num)
		    loc_category[bnst_ptr->parent->child[i]->num] = LOC_PARA_PARENT;
		/* 並列(子供側) */
		else if (bnst_ptr->parent->child[i]->num < bnst_ptr->num)
		    loc_category[bnst_ptr->parent->child[i]->num] = LOC_PARA_CHILD;
	    }
	}
	/* 親を探索 */
	parent_ptr = bnst_ptr->parent;
	while (parent_ptr->para_top_p && parent_ptr->parent) parent_ptr = parent_ptr->parent;
	if (parent_ptr->para_top_p) parent_ptr = NULL;	
    }
    /* 自分が並列でない場合 */
    else if (bnst_ptr->parent) {
	parent_ptr = bnst_ptr->parent;
    }
    
    /* 親、親用言の親、親体言の親 */
    if (parent_ptr) {
	loc_category[parent_ptr->num] = LOC_PARENT; /* 親 */

	/* 親の親を探索 */
	if (parent_ptr->parent) {
	    pparent_ptr = parent_ptr->parent;
	    while (pparent_ptr->para_top_p && pparent_ptr->parent) pparent_ptr = pparent_ptr->parent;
	    if (pparent_ptr->para_top_p) pparent_ptr = NULL;
	}

	if (pparent_ptr) {
	    if (check_feature(pparent_ptr->f, "用言"))
		loc_category[pparent_ptr->num] = LOC_PARENT_V_PARENT; /* 親用言の親 */
	    else
		loc_category[pparent_ptr->num] = LOC_PARENT_N_PARENT; /* 親体言の親 */
	}
    }	           	

    /* 子供 */
    for (i = 0; bnst_ptr->child[i]; i++) {
	/* 子が並列の場合(ex. 聞いていた) */
	/*   彼は──┐　　　　　　　　　 */
	/*  食べながら、<P>─┐　　　　　 */
	/* 彼女は──┐　　　│　　　　　 */
	/*    飲みながら<P>─PARA──┐　 */
	/*                   聞いていた。 */   
	if (bnst_ptr->child[i]->para_top_p) { 
	    for (j = 0; bnst_ptr->child[i]->child[j]; j++) {
		/* todo::とりあえず並列の並列は無視 */		
		if (!bnst_ptr->child[i]->child[j]->para_top_p)
		    loc_category[bnst_ptr->child[i]->child[j]->num] = LOC_CHILD; /* 子供 */
	    }
	}
	else {
	    loc_category[bnst_ptr->child[i]->num] = LOC_CHILD; /* 子供 */
	}
    }		    	   	   	    
    /* 自分が並列である場合(ex. 解決や) */
    /*    その──┐　　　　　　　　　  */
    /*    早い──┤                    */
    /* 解決や<P>─┤　　　　　　　　　  */
    /* 実現の<P>─PARA──┐　　　　　  */
    /*                手伝いを──┐　  */
    /*                          する。  */
    if (bnst_ptr->para_type == PARA_NORMAL) {
	for (i = 0; bnst_ptr->parent->child[i]; i++) {

	    /* todo::とりあえず並列の並列は無視 */		
	    if (bnst_ptr->parent->child[i]->para_type == PARA_NIL) {
		loc_category[bnst_ptr->parent->child[i]->num] = LOC_CHILD; /* 子供 */
	    }
	}
    }

    if (OptDisplay == OPT_DEBUG) {
	for (i = 0; i < sp->Bnst_num; i++)
	    printf(";;LOC %d-%s target_bnst:%d-%d\n", bnst_ptr->num,
		   bnst_ptr->Jiritu_Go, i, loc_category[i]);
	printf(";;\n");
    }
}

/*==================================================================*/
	       int check_analyze_tag(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* 与えたられたtag_ptrが解析対象かどうかをチェック */

    /* 用言としての解析対象である場合:CF_PRED(=1)を返す */
    /* 名詞としての解析対象である場合:CF_NOUN(=2)を返す */
    /* それ以外の場合は0を返す */

    /* 省略解析なし */
    if (check_feature(tag_ptr->f, "省略解析なし")) return 0;

    /* 名詞として解析する場合 */
    if ((OptEllipsis & OPT_REL_NOUN) && check_feature(tag_ptr->f, "体言")) {

	/* 主辞以外は対象外 */
	if (check_feature(tag_ptr->f, "文節内")) return 0;

	/* 修飾されている句は対象外(暫定的) */
	if (tag_ptr->b_ptr->child[0] && 
	    /* 連体詞形態指示詞に修飾されている語は除く */
	    check_feature(tag_ptr->b_ptr->child[0]->f, "連体詞形態指示詞")) return 0;
	
	return CF_NOUN;
    }

    /* 用言として解析する場合 */
    if (check_feature(tag_ptr->f, "用言")) {

	/* 付属語は解析しない */
	if (check_feature(tag_ptr->mrph_ptr->f, "付属")) return 0;

	/* サ変は文節主辞のみ対象 */
	if (check_feature(tag_ptr->f, "文節内") && 
	    check_feature(tag_ptr->f, "サ変")) return 0;

	/* 判定詞以外の体言は用言としては解析しない */
	if (check_feature(tag_ptr->f, "体言") &&
	    !check_feature(tag_ptr->f, "用言:判")) return 0;
	
	/* 判定詞の解析を行わない場合は判定詞の対象外 */
	if (!(OptAnaphora & OPT_ANAPHORA_COPULA) &&
	    check_feature(tag_ptr->f, "用言:判")) return 0;

	return CF_PRED;
    }

    return 0;
}

/*==================================================================*/
int read_one_annotation(SENTENCE_DATA *sp, TAG_DATA *tag_ptr, char *token, int co_flag)
/*==================================================================*/
{
    /* 解析結果からMENTION、ENTITYを作成する */
    /* co_flagがある場合は"="のみを処理、ない場合は"="以外を処理 */
    char flag, rel[SMALL_DATA_LEN], *cp, loc_name[SMALL_DATA_LEN];
    int i, j, tag_num, sent_num, bnst_num, diff_sen;
    MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
    MENTION *mention_ptr = NULL;
    ENTITY *entity_ptr;
    
    if (!sscanf(token, "%[^/]/%c/%*[^/]/%d/%d/", rel, &flag, &tag_num, &sent_num))
	return FALSE;
    if (tag_num == -1) return FALSE;

    /* 共参照関係の読み込み */
    if (co_flag && 
	(!strcmp(rel, "=") || !strcmp(rel, "=構") || !strcmp(rel, "=役"))) {

	mention_ptr = mention_mgr->mention;
	mention_ptr->entity = 
	    substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;
	mention_ptr->explicit_mention = NULL;
	mention_ptr->salience_score = mention_ptr->entity->salience_score;
	mention_ptr->entity->salience_score += 
	    (check_feature(tag_ptr->f, "係:未格") && 
	     (check_feature(tag_ptr->f, "ハ") || check_feature(tag_ptr->f, "モ")) ||
	     check_feature(tag_ptr->f, "文末")) ? SALIENCE_THEMA : SALIENCE_NORMAL;
	strcpy(mention_ptr->cpp_string, "＊");
	if ((cp = check_feature(tag_ptr->f, "係"))) {
	    strcpy(mention_ptr->spp_string, cp + strlen("係:"));
	} 
	else if (check_feature(tag_ptr->f, "文末")) {
	    strcpy(mention_ptr->spp_string, "文末");
	} 
	else {
	    strcpy(mention_ptr->spp_string, "＊");
	}
	mention_ptr->flag = '=';

	/* entityのnameがNEでなく、tag_ptrがNEならばnameを上書き */
	if (!strchr(mention_ptr->entity->name, ':') &&
	    (cp = check_feature(tag_ptr->f, "NE"))) {
	    strcpy(mention_ptr->entity->name, cp + strlen("NE:"));
	}
    }

    /* 共参照以外の関係 */
    else if (!co_flag && 
	     (flag == 'N' || flag == 'C' || flag == 'O' || flag == 'D') &&

	     /* 用言の場合は省略対象格のみ読み込む */
	     (check_analyze_tag(tag_ptr) == CF_PRED && match_ellipsis_case(rel, NULL) ||
	      /* 名詞の場合は */
	      check_analyze_tag(tag_ptr) == CF_NOUN && 
	      /* 省略対象格の場合はそのまま読み込み */
	      (match_ellipsis_case(rel, NULL) ||
	       /* 用言の省略対象格の場合はノ？格として読み込む */
	       match_ellipsis_case(rel, ELLIPSIS_CASE_LIST_VERB) && 
	       strcpy(rel, "ノ？"))) &&
	     /* 先行詞は体言のみ */
	     check_feature(((sp - sent_num)->tag_data + tag_num)->f, "体言")) {	

	mention_ptr = mention_mgr->mention + mention_mgr->num;
 	mention_ptr->entity = 
	    substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;
	mention_ptr->explicit_mention = (flag == 'C') ?
	    substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention : NULL;
	mention_ptr->salience_score = mention_ptr->entity->salience_score;
	if (flag == 'O') 
	    mention_ptr->entity->salience_score += SALIENCE_ZERO;

	mention_ptr->tag_num = mention_mgr->mention->tag_num;
	mention_ptr->sent_num = mention_mgr->mention->sent_num;
	mention_ptr->tag_ptr = 
	    (sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
	mention_ptr->flag = flag;
	strcpy(mention_ptr->cpp_string, rel);
	if (flag == 'C' && 
	    (cp = check_feature(((sp - sent_num)->tag_data + tag_num)->f, "係"))) {
	    strcpy(mention_ptr->spp_string, cp + strlen("係:"));
	} 
	else if (flag == 'C' && 
		 (check_feature(((sp - sent_num)->tag_data + tag_num)->f, "文末"))) {
	    strcpy(mention_ptr->spp_string, "文末");
	} 
		
	else {
	    strcpy(mention_ptr->spp_string, "＊");
	}
	mention_mgr->num++;
    }

    if (!mention_ptr) return FALSE;
    mention_ptr->entity->mention[mention_ptr->entity->mentioned_num] = mention_ptr;
    mention_ptr->entity->mentioned_num++;
    if (flag == 'O' || !strcmp(rel, "=")) mention_ptr->entity->antecedent_num++;

    /* 学習用情報の出力 */
    if (OptDisplay == OPT_DEBUG && 
	flag == 'O' && strcmp(rel, "=")) {

	/* Salience Scoreの出力 */
	printf(";; SALIENCE-LEARN:");
	for (j = 0; j < entity_manager.num; j++) {
	    entity_ptr = entity_manager.entity + j;	    
	    printf(" %.2f:%c", entity_ptr->salience_score,
		   entity_ptr == mention_ptr->entity ? 'T' : 'F');
	}
	printf("\n");
	
	/* 位置カテゴリの出力 */
	mark_loc_category(sp, tag_ptr);
	for (j = 0; j < entity_manager.num; j++) {
	    entity_ptr = entity_manager.entity + j;

	    /* 何文以内にmentionを持っているかどうかのチェック */
	    diff_sen = 4;
	    for (i = 0; i < entity_ptr->mentioned_num; i++) {
		if (mention_ptr->sent_num == entity_ptr->mention[i]->sent_num &&
		    loc_category[(entity_ptr->mention[i]->tag_ptr)->b_ptr->num] == LOC_SELF) continue;

		if (mention_ptr->sent_num - entity_ptr->mention[i]->sent_num < diff_sen)
		    diff_sen = mention_ptr->sent_num - entity_ptr->mention[i]->sent_num;
	    }
	    printf(";;\n");	    

	    for (i = 0; i < entity_ptr->mentioned_num; i++) {
		/* もっとも近くの文に出現したmentionのみ出力 */
		if (mention_ptr->sent_num - entity_ptr->mention[i]->sent_num > diff_sen)
		    continue;
		
		if ( /* 自分自身はのぞく */
		    entity_ptr->mention[i]->sent_num == mention_ptr->sent_num &&
		    loc_category[(entity_ptr->mention[i]->tag_ptr)->b_ptr->num] == LOC_SELF) continue;
		
		if (get_location(loc_name, mention_ptr->sent_num, rel, entity_ptr->mention[i])) {
		    printf(";; LOCATION-LEARN: %s:%c\n", loc_name,
			   entity_ptr == mention_ptr->entity ? 'T' : 'F');
		}
	    }
	}	
    }
    return TRUE;
}

/*==================================================================*/
       int expand_result_to_parallel_entity(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* 並列要素を展開する */
    int i, j, result_num;
    CF_TAG_MGR *ctm_ptr = tag_ptr->ctm_ptr; 
    TAG_DATA *t_ptr, *para_ptr;
    ENTITY *entity_ptr, *epnd_entity_ptr;
    MENTION *mention_ptr;
    
    /* 格・省略解析結果がない場合は終了 */
    if (!ctm_ptr) return FALSE;
    
    result_num = ctm_ptr->result_num;
    for (i = 0; i < result_num; i++) {
	entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i];

	/* 格要素のentityの省略以外の直近の出現を探す */
	for (j = entity_ptr->mentioned_num - 1; j >= 0; j--) {
	    if (entity_ptr->mention[j]->flag == 'S' ||
		entity_ptr->mention[j]->flag == '=') break;
	}

	/* とりあえず同一文の場合のみを対象とする */
	if (tag_ptr->mention_mgr.mention->sent_num != entity_ptr->mention[j]->sent_num)
	    continue;
	t_ptr = entity_ptr->mention[j]->tag_ptr;

	/* 並列の要素をチェック */
	if (t_ptr->para_type == PARA_NORMAL &&
	    t_ptr->parent && t_ptr->parent->para_top_p) {
	    
	    for (j = 0; t_ptr->parent->child[j]; j++) {
		para_ptr = substance_tag_ptr(t_ptr->parent->child[j]);

		if (para_ptr != t_ptr && check_feature(para_ptr->f, "体言") &&
		    para_ptr->para_type == PARA_NORMAL) {    

		    epnd_entity_ptr = para_ptr->mention_mgr.mention->entity;
		    ctm_ptr->filled_entity[epnd_entity_ptr->num] = TRUE;
		    ctm_ptr->entity_num[ctm_ptr->result_num] = epnd_entity_ptr->num;
		    ctm_ptr->flag[ctm_ptr->result_num] = 'O';
		    ctm_ptr->cf_element_num[ctm_ptr->result_num] = 
			ctm_ptr->cf_element_num[i];
		    ctm_ptr->result_num++;

		    if (OptDisplay == OPT_DEBUG)
			printf(";;EXPANDED %s : %s -> %s\n", 
			       tag_ptr->head_ptr->Goi2, 
			       entity_ptr->name, epnd_entity_ptr->name);
		}
	    }
	}
    }
    
    return TRUE;
}

/*==================================================================*/
	   int anaphora_result_to_entity(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* 照応解析結果ENTITYに関連付ける */
    int i, j;
    char *cp;
    MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
    MENTION *mention_ptr = NULL;
    CF_TAG_MGR *ctm_ptr = tag_ptr->ctm_ptr; 

    /* 格・省略解析結果がない場合は終了 */
    if (!ctm_ptr) return FALSE;
    
    for (i = 0; i < ctm_ptr->result_num; i++) {
	mention_ptr = mention_mgr->mention + mention_mgr->num;
	mention_ptr->entity = entity_manager.entity + ctm_ptr->entity_num[i];
	mention_ptr->tag_num = mention_mgr->mention->tag_num;
	mention_ptr->sent_num = mention_mgr->mention->sent_num;
	mention_ptr->flag = ctm_ptr->flag[i];
	mention_ptr->tag_ptr = 
	    (sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
	strcpy(mention_ptr->cpp_string,
	       pp_code_to_kstr(ctm_ptr->cf_ptr->pp[ctm_ptr->cf_element_num[i]][0]));
	mention_ptr->salience_score = mention_ptr->entity->salience_score;
	/* 入力側の表層格(格解析結果のみ) */
	if (i < ctm_ptr->case_result_num) {
	    mention_ptr->explicit_mention = ctm_ptr->elem_b_ptr[i]->mention_mgr.mention;
	    if (tag_ptr->tcf_ptr->cf.pp[ctm_ptr->tcf_element_num[i]][0] >= FUKUGOJI_START &&
		tag_ptr->tcf_ptr->cf.pp[ctm_ptr->tcf_element_num[i]][0] <= FUKUGOJI_END) {
		strcpy(mention_ptr->spp_string, 
		       pp_code_to_kstr(tag_ptr->tcf_ptr->cf.pp[ctm_ptr->tcf_element_num[i]][0]));
	    }
	    else { 
		if ((cp = check_feature(ctm_ptr->elem_b_ptr[i]->f, "係"))) {
		    strcpy(mention_ptr->spp_string, cp + strlen("係:"));
		} 
		else if (check_feature(ctm_ptr->elem_b_ptr[i]->f, "文末")) {
		    strcpy(mention_ptr->spp_string, "文末");
		}
		else {
		    strcpy(mention_ptr->spp_string, "＊");
		}
	    }
	}
	else {
	    mention_ptr->explicit_mention = NULL;
	    strcpy(mention_ptr->spp_string, "Ｏ");
	    mention_ptr->entity->salience_score += SALIENCE_ZERO;
	}
	mention_mgr->num++;

	mention_ptr->entity->mention[mention_ptr->entity->mentioned_num] = mention_ptr;
	mention_ptr->entity->mentioned_num++;
    }
    
    return TRUE;
}

/*==================================================================*/
     int set_tag_case_frame(SENTENCE_DATA *sp, TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* ENTITY_PRED_MGRを作成する関数
       make_data_cframeを用いて入力文の格構造を作成するため
       CF_PRED_MGRを作り、そのcfをコピーしている */
    int i;
    TAG_CASE_FRAME *tcf_ptr = tag_ptr->tcf_ptr;
    CF_PRED_MGR *cpm_ptr;
    char *vtype = NULL;  

    /* cpmの作成 */
    cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "set_tag_case_frame");
    init_case_frame(&(cpm_ptr->cf));
    cpm_ptr->pred_b_ptr = tag_ptr;

    /* 入力文側の格要素設定 */
    /* set_data_cf_type(cpm_ptr); */
    if (check_analyze_tag(tag_ptr) == CF_PRED) {
	vtype = check_feature(tag_ptr->f, "用言");
	vtype += strlen("用言:");
	strcpy(cpm_ptr->cf.pred_type, vtype);
	cpm_ptr->cf.type = CF_PRED;
    }
    else {
	strcpy(cpm_ptr->cf.pred_type, "名");
	cpm_ptr->cf.type = CF_NOUN;
    }
    cpm_ptr->cf.type_flag = 0;
    cpm_ptr->cf.voice = tag_ptr->voice;

    /* 入力文の格構造を作成 */
    make_data_cframe(sp, cpm_ptr);
    
    /* ENTITY_PRED_MGRを作成・入力文側の格要素をコピー */
    tcf_ptr->cf = cpm_ptr->cf;
    tcf_ptr->pred_b_ptr = tag_ptr;
    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	tcf_ptr->elem_b_ptr[i] = substance_tag_ptr(cpm_ptr->elem_b_ptr[i]);
	tcf_ptr->elem_b_num[i] = cpm_ptr->elem_b_num[i];
    }

    /* todo::free(cpm_ptr); freeする必要あり
       ただし、tcf_ptr->cf.pred_b_ptr->cpm_ptrで使うのでまだできない
       get_ex_probability_with_para内 */
    return TRUE;
}

/*==================================================================*/
    int set_cf_candidate(TAG_DATA *tag_ptr, CASE_FRAME **cf_array)
/*==================================================================*/
{
    int i, l, frame_num = 0, hiragana_prefer_flag = 0;
    CFLIST *cfp;
    char *key;
    
    /* 格フレームcache */
    if (OptUseSmfix == TRUE && CFSimExist == TRUE) {
		
	if ((key = get_pred_id(tag_ptr->cf_ptr->cf_id)) != NULL) {
	    cfp = CheckCF(key);
	    free(key);

	    if (cfp) {
		for (l = 0; l < tag_ptr->cf_num; l++) {
		    for (i = 0; i < cfp->cfid_num; i++) {
			if (((tag_ptr->cf_ptr + l)->type == tag_ptr->tcf_ptr->cf.type) &&
			    ((tag_ptr->cf_ptr + l)->cf_similarity = 
			     get_cfs_similarity((tag_ptr->cf_ptr + l)->cf_id, 
						*(cfp->cfid + i))) > CFSimThreshold) {
			    *(cf_array + frame_num++) = tag_ptr->cf_ptr + l;
			    break;
			}
		    }
		}
		tag_ptr->e_cf_num = frame_num;
	    }
	}
    }

    if (frame_num == 0) {
	
	/* 表記がひらがなの場合: 
	   格フレームの表記がひらがなの場合が多ければひらがなの格フレームのみを対象に、
	   ひらがな以外が多ければひらがな以外のみを対象にする */
	if (!(OptCaseFlag & OPT_CASE_USE_REP_CF) && /* 代表表記ではない場合のみ */
	    check_str_type(tag_ptr->head_ptr->Goi) == TYPE_HIRAGANA) {
	    if (check_feature(tag_ptr->f, "代表ひらがな")) {
		hiragana_prefer_flag = 1;
	    }
	    else {
		hiragana_prefer_flag = -1;
	    }
	}

	for (l = 0; l < tag_ptr->cf_num; l++) {
	    if ((tag_ptr->cf_ptr + l)->type == tag_ptr->tcf_ptr->cf.type && 
		(hiragana_prefer_flag == 0 || 
		 (hiragana_prefer_flag > 0 && 
		  check_str_type((tag_ptr->cf_ptr + l)->entry) == TYPE_HIRAGANA) || 
		 (hiragana_prefer_flag < 0 && 
		  check_str_type((tag_ptr->cf_ptr + l)->entry) != TYPE_HIRAGANA))) {
		*(cf_array + frame_num++) = tag_ptr->cf_ptr + l;
	    }
	}
    }
    return frame_num;
}

/*==================================================================*/
double calc_score_of_ctm(CF_TAG_MGR *ctm_ptr, TAG_CASE_FRAME *tcf_ptr)
/*==================================================================*/
{
    /* 格フレームとの対応付けのスコアを計算する関数  */
    int i, e_num, debug = 0;
    double score;

    /* 対象の格フレームが選択されることのスコア */
    score = get_cf_probability(&(tcf_ptr->cf), ctm_ptr->cf_ptr);

    /* 対応付けられた要素に関するスコア(格解析結果) */
    for (i = 0; i < ctm_ptr->case_result_num; i++) {
	e_num = ctm_ptr->cf_element_num[i];
	
	/* とりあえず不要::直前格がある場合は用例に含まれているものを優先 */
	/*if (tcf_ptr->cf.adjacent[ctm_ptr->tcf_element_num[i]] == TRUE &&
	    get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf),
			       NULL, e_num, ctm_ptr->cf_ptr, FALSE) > FREQ0_ASSINED_SCORE)
			       score -= FREQ0_ASSINED_SCORE; */

	/* 名詞の場合はP(A|B)/P(A)で評価 */
	/* todo::カテゴリ、NEによる汎化 */
	if (tcf_ptr->cf.type == CF_NOUN) {
	    /* P(弁当|価格:名1,ノ格) */
	    score +=
		get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
				   tcf_ptr->elem_b_ptr[i],
				   e_num, ctm_ptr->cf_ptr, FALSE);

	    /* /P(弁当) */
	    score -= 
		get_key_probability(tcf_ptr->elem_b_ptr[i]);
	    
	    if (OptDisplay == OPT_DEBUG && debug) 
		printf(";;対応あり:%s-%s:%f:%f ", 
		       ctm_ptr->elem_b_ptr[i]->head_ptr->Goi2, 
		       pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
		       get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
					  tcf_ptr->elem_b_ptr[i],
					  e_num, ctm_ptr->cf_ptr, FALSE),
		       get_key_probability(tcf_ptr->elem_b_ptr[i]));
	}
	
	else {
	    /* 用言の場合 */
	    score += 
		get_ex_probability_with_para(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
					     e_num, ctm_ptr->cf_ptr) +
		get_case_function_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf),
					      e_num, ctm_ptr->cf_ptr);
	    
	    if (OptDisplay == OPT_DEBUG && debug) 
		printf(";;対応あり:%s-%s:%f:%f ", 
		       ctm_ptr->elem_b_ptr[i]->head_ptr->Goi2, 
		       pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
		       get_ex_probability_with_para(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
						    e_num, ctm_ptr->cf_ptr),
		       get_case_function_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
						     e_num, ctm_ptr->cf_ptr));      
	}
    }
    /* 入力文の格要素のうち対応付けられなかった要素に関するスコア */
    for (i = 0; i < tcf_ptr->cf.element_num - ctm_ptr->case_result_num; i++) {

	if (OptDisplay == OPT_DEBUG && debug) 
	    printf(";;対応なし:%s:%f ", 
		   (tcf_ptr->elem_b_ptr[ctm_ptr->non_match_element[i]])->head_ptr->Goi2,
		   score);

	/* 橋渡し指示解析の場合は無視 */
	if (tcf_ptr->cf.type == CF_NOUN) continue;
	
	score += FREQ0_ASSINED_SCORE * 3 +
	    get_case_function_probability(ctm_ptr->non_match_element[i], &(tcf_ptr->cf), 
					  NIL_ASSIGNED, ctm_ptr->cf_ptr);
    }
    if (OptDisplay == OPT_DEBUG && debug) printf(";; %f ", score);	   
    /* 格フレームの格が埋まっているかどうかに関するスコア */
    for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
	if (tcf_ptr->cf.type == CF_NOUN) continue;
	score += get_case_probability(e_num, ctm_ptr->cf_ptr, ctm_ptr->filled_element[e_num]);	
    }
    if (OptDisplay == OPT_DEBUG && debug) printf(";; %f\n", score);

    return score;
}

/*==================================================================*/
double calc_ellipsis_score_of_ctm(CF_TAG_MGR *ctm_ptr, TAG_CASE_FRAME *tcf_ptr)
/*==================================================================*/
{
    /* 格フレームとの対応付けのスコアを計算する関数(省略解析の評価) */
    int i, j, k, l, e_num, debug = 1, sent_num;
    double score = 0, max_score, tmp_ne_ct_score, tmp_score, ex_prob, prob, penalty;
    char *cp, key[SMALL_DATA_LEN], loc_name[SMALL_DATA_LEN];
    TAG_DATA *child_ptr;
    ENTITY *entity_ptr;

    /* 解析対象の基本句の文番号 */
    sent_num = tcf_ptr->pred_b_ptr->mention_mgr.mention->sent_num;

    /* 対応付けられた要素に関するスコア(省略解析結果) */
    for (i = ctm_ptr->case_result_num; i < ctm_ptr->result_num; i++) {
	e_num = ctm_ptr->cf_element_num[i];
	entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i]; /* 関連付けられたENTITY */	
	
	/* P(弁当|食べる:動2,ヲ格)/P(弁当) (∝P(食べる:動2,ヲ格|弁当)) */
	/* flag='S'または'='のmentionの中で最大となるものを使用 */	
	max_score = INITIAL_SCORE;

	for (j = 0; j < entity_ptr->mentioned_num; j++) {
	    if (entity_ptr->mention[j]->flag != 'S' &&
		entity_ptr->mention[j]->flag != '=') continue;
	    tmp_ne_ct_score = FREQ0_ASSINED_SCORE;

 	    /* カテゴリがある場合はP(食べる:動2,ヲ格|カテゴリ:人)もチェック */
	    if ((OptGeneralCF & OPT_CF_CATEGORY) && 
		(cp = check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ"))) {

		while (cp = strchr(cp, ':')) {
		    cp++;
		    sprintf(key, "CT:%s:", cp);
		    if (/* !strncmp(key, "CT:人:", 6) && */
			(prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE))) {

			/* P(カテゴリ:人|食べる:動2,ヲ格) */
			tmp_score = log(prob);
			
			/* /P(カテゴリ:人) */
			*strchr(key + 3, ':') = '\0';
			tmp_score -= get_general_probability(key, "KEY");
			if (OptDisplay == OPT_DEBUG && debug) 
			    printf(";; %s:%f(%f/%f)\n", key, tmp_score, prob, exp(get_general_probability(key, "KEY")));
			
			if (tmp_score > tmp_ne_ct_score) tmp_ne_ct_score = tmp_score;
			continue;
		    }
		}
	    }

	    /* 固有表現の場合はP(食べる:動2,ヲ格|ARTIFACT)もチェック */
	    if ((OptGeneralCF & OPT_CF_NE) && 
		(cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE")) &&
		(prob = get_ex_ne_probability(cp, e_num, ctm_ptr->cf_ptr, TRUE))) {

		/* P(ARTIFACT|食べる:動2,ヲ格) */
		tmp_score = log(prob);

		/* /P(ARTIFACT) */
		strcpy(key, cp);
		*strchr(key + 3, ':') = '\0'; /* key = NE:LOCATION */
		tmp_score -= get_general_probability(key, "KEY");
		if (OptDisplay == OPT_DEBUG && debug) 
		    printf(";; %s:%f(%f/%f)\n", key, tmp_score, prob, exp(get_general_probability(key, "KEY")));
		
		if (tmp_score > tmp_ne_ct_score) tmp_ne_ct_score = tmp_score;
	    }

	    /* P(弁当|食べる:動2,ヲ格) */
	    tmp_score = ex_prob =
		get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
				   entity_ptr->mention[j]->tag_ptr, e_num, ctm_ptr->cf_ptr, FALSE);

	    /* /P(弁当) */
	    tmp_score -= get_key_probability(entity_ptr->mention[j]->tag_ptr);
	    
	    if (OptDisplay == OPT_DEBUG && debug) {
		printf(";; %s:%f(%f/%f)\n", 
		       entity_ptr->mention[j]->tag_ptr->head_ptr->Goi2,
		       tmp_score,
		       exp(get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
					      entity_ptr->mention[j]->tag_ptr, e_num, ctm_ptr->cf_ptr, FALSE)),
		       exp(get_key_probability(entity_ptr->mention[j]->tag_ptr)));
	    }

	    /* カテゴリ、固有表現から計算された値との平均値を使用 */
	    if (ex_prob > FREQ0_ASSINED_SCORE && 
		tmp_ne_ct_score > FREQ0_ASSINED_SCORE)
		tmp_score = (tmp_score + tmp_ne_ct_score) / 2;
	    else if (tmp_ne_ct_score > FREQ0_ASSINED_SCORE)
		tmp_score = tmp_ne_ct_score;	

	    if (tmp_score > max_score) {
		max_score = tmp_score;
	    }
	}
	if (OptDisplay == OPT_DEBUG && debug) printf(";; %s:%f\n", entity_ptr->name, max_score);
	score += max_score + log(EX_MATCH_COMPENSATE);

	/* mentionごとにスコアを計算 */	
	max_score = FREQ0_ASSINED_SCORE;
	for (j = 0; j < entity_ptr->mentioned_num; j++) {
	    tmp_score = 0;

	    /* 自分自身は除外 */
	    if (entity_ptr->mention[j]->sent_num == sent_num &&
		!loc_category[(entity_ptr->mention[j]->tag_ptr)->b_ptr->num]) continue;

	    /* P(未格|O:ガ)/P(未格) (∝P(O:ガ|未格)) */
	    if (strcmp(entity_ptr->mention[j]->spp_string, "＊") &&
		strcmp(entity_ptr->mention[j]->spp_string, "Ｏ") &&
		strcmp(entity_ptr->mention[j]->spp_string, "文節内")) { /* 格要素、格がない場合は考慮しない */
		sprintf(key, "%s", entity_ptr->mention[j]->spp_string);
	    }
	    else if (strcmp(entity_ptr->mention[j]->cpp_string, "＊")) {
		sprintf(key, "%s格", entity_ptr->mention[j]->cpp_string);
	    }
	    else {
		key[0] = '\0';
	    }
	    
	    if (*key) {
		/* P(未格|O:ガ) */
		tmp_score += get_case_interpret_probability(
		    key, pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]), TRUE);
		/* /P(未格) */
		if (tmp_score != UNKNOWN_CASE_SCORE) {
		    tmp_score -= get_general_probability(key, "表層格");
		}
	    }

	    if (OptDisplay == OPT_DEBUG && debug) 
		printf(";;   %s|%s:%f (%f/%f)\t", 
		       pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
		       key, tmp_score,
		       exp(get_case_interpret_probability(key, pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]), TRUE)),
		       exp(get_general_probability(key, "表層格"))
		    );
	    
	    /* 位置カテゴリ */
	    /* 省略格、flag(S,=,O,N,C)ごとに位置カテゴリごとに先行詞となる確率を考慮
	       位置カテゴリは、以前の文であれば B + 何文前か(4文前以上は0)
	       同一文内であれば C + loc_category という形式(ex. ガ-O-C3、ヲ-=-B2) */
	    get_location(loc_name, sent_num,
			 pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
			 entity_ptr->mention[j]);
	    tmp_score += get_general_probability("T", loc_name);
	    if (OptDisplay == OPT_DEBUG && debug) 
		printf("T|%s:%f\n", loc_name, tmp_score);	   

	    if (tmp_score > max_score) {
		max_score = tmp_score;
		/* 最大のスコアとなった基本句を保存(並列への対処のため) */
		ctm_ptr->elem_b_ptr[i] = entity_ptr->mention[j]->tag_ptr;
	    }		
	}
	score += max_score;
	if (OptDisplay == OPT_DEBUG && debug) 
	    printf(";; %s:%f\n;; score = %f\n", entity_ptr->name, max_score, score);
    }   

    /* 橋渡し指示の場合は以下の処理はしない */
    if (tcf_ptr->cf.type == CF_NOUN) {
	if (OptDisplay == OPT_DEBUG && debug) 
	    printf(";; %s : the score = %f\n;;\n", ctm_ptr->cf_ptr->cf_id, score);
	return score;
    }

    /* 対応付けられなかった格スロットに関するスコア */
    for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
        if (!ctm_ptr->filled_element[e_num] &&
	    match_ellipsis_case(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]), NULL) &&
	    ctm_ptr->cf_ptr->oblig[e_num])
            score += get_case_probability(e_num, ctm_ptr->cf_ptr, FALSE);
    }

    if (OptDisplay == OPT_DEBUG && debug) 
	printf(";; %s : the score = %f\n;;\n", ctm_ptr->cf_ptr->cf_id, score);	   
    
    return score;
}

/*==================================================================*/
     int copy_ctm(CF_TAG_MGR *source_ctm, CF_TAG_MGR *target_ctm)
/*==================================================================*/
{
    int i;

    target_ctm->score = source_ctm->score;
    target_ctm->case_score = source_ctm->case_score;
    target_ctm->cf_ptr = source_ctm->cf_ptr;
    target_ctm->result_num = source_ctm->result_num;
    target_ctm->case_result_num = source_ctm->case_result_num;
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	target_ctm->filled_element[i] = source_ctm->filled_element[i];
	target_ctm->non_match_element[i] = source_ctm->non_match_element[i];
	target_ctm->cf_element_num[i] = source_ctm->cf_element_num[i];
	target_ctm->tcf_element_num[i] = source_ctm->tcf_element_num[i];
	target_ctm->entity_num[i] = source_ctm->entity_num[i];
	target_ctm->elem_b_ptr[i] = source_ctm->elem_b_ptr[i];
	target_ctm->flag[i] = source_ctm->flag[i];
    }
}

/*==================================================================*/
      int preserve_ctm(CF_TAG_MGR *ctm_ptr, int start, int num)
/*==================================================================*/
{
    /* start番目からnum個のwork_ctmのスコアと比較し上位ならば保存する
       num個のwork_ctmのスコアは降順にソートされていることを仮定している
       保存された場合は1、されなかった場合は0を返す */

    int i, j;
    
    for (i = start; i < start + num; i++) {
	
	/* work_ctmに結果を保存 */
	if (ctm_ptr->score > work_ctm[i].score) {	    
	    for (j = start + num - 1; j > i; j--) {
		if (work_ctm[j - 1].score > INITIAL_SCORE) {
		    copy_ctm(&work_ctm[j - 1], &work_ctm[j]);
		}
	    }
	    copy_ctm(ctm_ptr, &work_ctm[i]);
	    return TRUE;
	}
    }
    return FALSE;
}

/*==================================================================*/
int case_analysis_for_anaphora(TAG_DATA *tag_ptr, CF_TAG_MGR *ctm_ptr, int i, int r_num)
/*==================================================================*/
{
    /* 候補の格フレームについて照応解析用格解析を実行する関数
       再帰的に呼び出す
       iにはtag_ptr->tcf_ptr->cf.element_numのうちチェックした数 
       r_numにはそのうち格フレームと関連付けられた要素の数が入る */
    
    int j, k, e_num;

    /* すでに埋まっている格フレームの格をチェック */
    memset(ctm_ptr->filled_element, 0, sizeof(int) * CF_ELEMENT_MAX);
    for (j = 0; j < r_num; j++) {
	ctm_ptr->filled_element[ctm_ptr->cf_element_num[j]] = TRUE;
	
	/* 類似している格も埋まっているものとして扱う */
	for (k = 0; ctm_ptr->cf_ptr->samecase[k][0] != END_M; k++) {
	    if (ctm_ptr->cf_ptr->samecase[k][0] == ctm_ptr->cf_element_num[j])
		ctm_ptr->filled_element[ctm_ptr->cf_ptr->samecase[k][1]] = TRUE;
	    else if (ctm_ptr->cf_ptr->samecase[k][1] == ctm_ptr->cf_element_num[j])
		ctm_ptr->filled_element[ctm_ptr->cf_ptr->samecase[k][0]] = TRUE;
	}
    }

    /* まだチェックしていない要素がある場合 */
    if (i < tag_ptr->tcf_ptr->cf.element_num) {

	for (j = 0; tag_ptr->tcf_ptr->cf.pp[i][j] != END_M; j++) {
	    
	    /* 入力文のi番目の格要素を格フレームのcf.pp[i][j]格に割り当てる */
	    for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {

		if (tag_ptr->tcf_ptr->cf.pp[i][j] == ctm_ptr->cf_ptr->pp[e_num][0]) {
		    /* 対象の格が既に埋まっている場合は不可 */
		    if (ctm_ptr->filled_element[e_num] == TRUE) return FALSE;

		    /* 非格要素は除外 */
		    if (check_feature(tag_ptr->tcf_ptr->elem_b_ptr[i]->f, "非格要素")) {
			continue;
		    }			    
		    
		    /* 形式名詞は除外 */
		    /* if (check_feature(tag_ptr->tcf_ptr->elem_b_ptr[i]->f, "形副名詞")) {
			continue;
			} */		    

		    /* 直前格である場合は制限を加える(暫定的) */
		    if (check_feature(tag_ptr->tcf_ptr->elem_b_ptr[i]->f, "助詞") &&
			tag_ptr->tcf_ptr->cf.adjacent[i] && !(ctm_ptr->cf_ptr->adjacent[e_num])) {
			continue;
		    }
		    /* 対応付け結果を記録 */
		    ctm_ptr->elem_b_ptr[r_num] = tag_ptr->tcf_ptr->elem_b_ptr[i];
		    ctm_ptr->cf_element_num[r_num] = e_num;
		    ctm_ptr->tcf_element_num[r_num] = i;
    		    ctm_ptr->flag[r_num] = tag_ptr->tcf_ptr->elem_b_num[i] == -1 ? 'N' : 'C';
		    ctm_ptr->entity_num[r_num] = 
			ctm_ptr->elem_b_ptr[r_num]->mention_mgr.mention->entity->num;

		    /* i+1番目の要素のチェックへ */
		    case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num + 1);
		}
	    }    
	}

	/* 格要素を割り当てない場合 */
	/* 入力文のi番目の要素が対応付けられなかったことを記録 */
	ctm_ptr->non_match_element[i - r_num] = i; 
	case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num);
    }

    /* すべてのチェックが終了した場合 */
    else {
	/* この段階でr_num個が対応付けられている */
	ctm_ptr->result_num = ctm_ptr->case_result_num = r_num;
	/* スコアを計算 */
	ctm_ptr->score = ctm_ptr->case_score = calc_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr);
	/* スコア上位を保存 */
	preserve_ctm(ctm_ptr, 0, CASE_CANDIDATE_MAX);
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
int ellipsis_analysis(TAG_DATA *tag_ptr, CF_TAG_MGR *ctm_ptr, int i, int r_num)
/*==================================================================*/
{
    /* 候補となる格フレームと格要素の対応付けについて省略解析を実行する関数
       再帰的に呼び出す 
       iにはELLIPSIS_CASE_LIST[]のうちチェックした数が入る
       r_numには格フレームと関連付けられた要素の数が入る
       (格解析の結果関連付けられたものも含む) */
    int j, k, e_num, exist_flag;
   
    /* すでに埋まっている格フレームの格をチェック */
    memset(ctm_ptr->filled_element, 0, sizeof(int) * CF_ELEMENT_MAX);
    memset(ctm_ptr->filled_entity, 0, sizeof(int) * ENTITY_MAX);
    for (j = 0; j < r_num; j++) {
	ctm_ptr->filled_element[ctm_ptr->cf_element_num[j]] = TRUE;
	/* 類似している格も埋まっているものとして扱う */
	for (k = 0; ctm_ptr->cf_ptr->samecase[k][0] != END_M; k++) {
	    if (ctm_ptr->cf_ptr->samecase[k][0] == ctm_ptr->cf_element_num[j])
		ctm_ptr->filled_element[ctm_ptr->cf_ptr->samecase[k][1]] = TRUE;
	    else if (ctm_ptr->cf_ptr->samecase[k][1] == ctm_ptr->cf_element_num[j])
		ctm_ptr->filled_element[ctm_ptr->cf_ptr->samecase[k][0]] = TRUE;
	}
	ctm_ptr->filled_entity[ctm_ptr->entity_num[j]] = TRUE;
    }
    /* 自分自身も不可 */
    ctm_ptr->filled_entity[tag_ptr->mention_mgr.mention->entity->num] = TRUE;
   
    /* まだチェックしていない省略解析対象格がある場合 */
    if (*ELLIPSIS_CASE_LIST[i]) {

	/* 対象の格について */
	exist_flag = 0;
	for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {

	    /* 名詞の場合は対象の格をノ格として扱う */
	    if (tag_ptr->tcf_ptr->cf.type == CF_NOUN)
		ctm_ptr->cf_ptr->pp[e_num][0] = pp_kstr_to_code("ノ");
			    
	    /* 格の一致をチェック */
	    if (ctm_ptr->cf_ptr->pp[e_num][0] != pp_kstr_to_code(ELLIPSIS_CASE_LIST[i]))
		continue;

	    if (!ctm_ptr->cf_ptr->oblig[e_num]) continue;
	    exist_flag = 1;	    

	    /* すでに埋まっていた場合は次の格をチェックする */
	    if (ctm_ptr->filled_element[e_num] == TRUE) {
		ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num);
	    }
	    /* else if (check_feature(tag_ptr->f, "格要素-ガ:＃一人称") &&
		     !strcmp(ELLIPSIS_CASE_LIST[i], "ガ")) {
		ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num);
	    } */	
	    else {
 		for (k = 0; k < entity_manager.num; k++) {
		    /* salience_scoreがSALIENCE_THREHOLD以下なら候補としない */
		    /* ただし解析対象が係っている表現は除く */
		    if ((entity_manager.entity[k].salience_score <= SALIENCE_THREHOLD) &&
			!(tag_ptr->parent &&
			  substance_tag_ptr(tag_ptr->parent)->mention_mgr.mention->entity->num
			  == entity_manager.entity[k].num)) continue;
		    /* 対象のENTITYがすでに対応付けられている場合は不可 */
		    if (ctm_ptr->filled_entity[k]) continue;

		    /* 対応付け結果を記録
		       (基本句との対応付けは取っていないためelem_b_ptrは使用しない) */
		    ctm_ptr->cf_element_num[r_num] = e_num;
		    ctm_ptr->entity_num[r_num] = k;
		    
		    /* 次の格のチェックへ */
		    ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num + 1);
		}
		/* 埋めないで次の格へ(不特定) */
		ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num);
	    }
	}
	/* 対象の格が格フレームに存在しない場合は次の格へ */
	if (!exist_flag) ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num);
    }
    
    /* すべてのチェックが終了した場合 */
    else {
	/* この段階でr_num個が対応付けられている */
	ctm_ptr->result_num = r_num;
	for (j = ctm_ptr->case_result_num; j < r_num; j++) ctm_ptr->flag[j] = 'O';
	/* スコアを計算 */
	ctm_ptr->score = calc_ellipsis_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr) + ctm_ptr->case_score;
	/* スコア上位を保存 */
	preserve_ctm(ctm_ptr, CASE_CANDIDATE_MAX, ELLIPSIS_RESULT_MAX);
    }   
}

/*==================================================================*/
	    int ellipsis_analysis_main(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* ある基本句を対象として省略解析を行う関数 */
    /* 格フレームごとにループを回す */

    int i, j, k, frame_num = 0;
    CASE_FRAME **cf_array;
    CF_TAG_MGR *ctm_ptr = work_ctm + CASE_CANDIDATE_MAX + ELLIPSIS_RESULT_MAX;

    /* 使用する格フレームの設定 */
    cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *)*tag_ptr->cf_num, 
					  "ellipsis_analysis_main");
    frame_num = set_cf_candidate(tag_ptr, cf_array);
    printf(";;CASE FRAME NUM: %d\n", frame_num);

    /* work_ctmのスコアを初期化 */
    for (i = 0; i < CASE_CANDIDATE_MAX + ELLIPSIS_RESULT_MAX; i++) 
	work_ctm[i].score = INITIAL_SCORE;

    /* 候補の格フレームについて照応解析用格解析を実行 */
    
    /* 照応解析用格解析(上位CASE_CANDIDATE_MAX個の結果を保持する) */
    for (i = 0; i < frame_num; i++) {

	/* OR の格フレーム(和フレーム)を除く */
	if (((*(cf_array + i))->etcflag & CF_SUM) && frame_num != 1) {
	    continue;
	}
  
	/* ctm_ptrの初期化 */
	ctm_ptr->score = INITIAL_SCORE;

	/* 格フレームを指定 */
 	ctm_ptr->cf_ptr = *(cf_array + i);

	/* 格解析 */
	case_analysis_for_anaphora(tag_ptr, ctm_ptr, 0, 0);	
    }
    if (work_ctm[0].score == INITIAL_SCORE) return FALSE;

    if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
	for (i = 0; i < CASE_CANDIDATE_MAX; i++) {
	    if (work_ctm[i].score == INITIAL_SCORE) break;
	    printf(";;格解析候補%d-%d:%2d %.3f %s",
		   tag_ptr->mention_mgr.mention->sent_num, tag_ptr->num,
		   i + 1, work_ctm[i].score, work_ctm[i].cf_ptr->cf_id);

	    for (j = 0; j < work_ctm[i].result_num; j++) {
		printf(" %s%s:%s",
		       work_ctm[i].cf_ptr->adjacent[work_ctm[i].cf_element_num[j]] ? "*" : "-",
		       pp_code_to_kstr(work_ctm[i].cf_ptr->pp[work_ctm[i].cf_element_num[j]][0]),
		       work_ctm[i].elem_b_ptr[j]->head_ptr->Goi2);
	    }
	    for (j = 0; j < work_ctm[i].cf_ptr->element_num; j++) {
		if (!work_ctm[i].filled_element[j] && 
		    match_ellipsis_case(pp_code_to_kstr(work_ctm[i].cf_ptr->pp[j][0]), NULL))
		    printf(" %s:×", pp_code_to_kstr(work_ctm[i].cf_ptr->pp[j][0]));
	    }
	    printf("\n");
	}
    }

    /* 上記の対応付けに対して省略解析を実行する */
    for (i = 0; i < CASE_CANDIDATE_MAX; i++) {
	if (work_ctm[i].score == INITIAL_SCORE && i > 0) break;
	copy_ctm(&work_ctm[i], ctm_ptr);
	ellipsis_analysis(tag_ptr, ctm_ptr, 0, ctm_ptr->result_num);
    }

    if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
	for (i = CASE_CANDIDATE_MAX; i < CASE_CANDIDATE_MAX + ELLIPSIS_RESULT_MAX; i++) {
 	    if (work_ctm[i].score == INITIAL_SCORE) break;
	    printf(";;省略解析候補%d-%d:%2d %.3f %s", 
		   tag_ptr->mention_mgr.mention->sent_num,
		   tag_ptr->num, i - CASE_CANDIDATE_MAX + 1, 
		   work_ctm[i].score, work_ctm[i].cf_ptr->cf_id);

	    for (j = 0; j < work_ctm[i].result_num; j++) {
		printf(" %s%s:%s%d",
		       (j < work_ctm[i].case_result_num) ? "" : "*",
		       pp_code_to_kstr(work_ctm[i].cf_ptr->pp[work_ctm[i].cf_element_num[j]][0]),
		       (entity_manager.entity + work_ctm[i].entity_num[j])->name,
		       (entity_manager.entity + work_ctm[i].entity_num[j])->num);
	    }
	    for (j = 0; j < work_ctm[i].cf_ptr->element_num; j++) {
		if (!work_ctm[i].filled_element[j] && 
		    match_ellipsis_case(pp_code_to_kstr(work_ctm[i].cf_ptr->pp[j][0]), NULL)) 
		    printf(" %s:%s", pp_code_to_kstr(work_ctm[i].cf_ptr->pp[j][0]),
			   (work_ctm[i].cf_ptr->oblig[j]) ? "×" : "-");
	    }
	    printf("\n", work_ctm[i].cf_ptr->cf_id);
	}
    }
   
    /* BEST解を保存 */
    if (work_ctm[CASE_CANDIDATE_MAX].score == INITIAL_SCORE) return FALSE;
    tag_ptr->ctm_ptr = 
	(CF_TAG_MGR *)malloc_data(sizeof(CF_TAG_MGR), "ellipsis_analysis_main");
    copy_ctm(&work_ctm[CASE_CANDIDATE_MAX], tag_ptr->ctm_ptr);

    free(cf_array);

    return TRUE;
}

/*==================================================================*/
    int make_new_entity(TAG_DATA *tag_ptr, MENTION_MGR *mention_mgr)
/*==================================================================*/
{    
    char *cp;
    ENTITY *entity_ptr;

    if (entity_manager.num >= ENTITY_MAX - 1) { 
	fprintf(stderr, "Entity buffer overflowed!\n");
	exit(1);
    }
    entity_ptr = entity_manager.entity + entity_manager.num;
    entity_ptr->num = entity_manager.num;
    entity_manager.num++;				
    entity_ptr->mention[0] = mention_mgr->mention;
    entity_ptr->mentioned_num = 1;
    entity_ptr->antecedent_num = 0;

    /* 先行詞になりやすさ(基本的に文節主辞なら1) */
    entity_ptr->salience_score = 
	(tag_ptr->inum > 0 || /* 文節内最後の基本句でない */
	 check_feature(tag_ptr->f, "形副名詞") ||
	 !check_feature(tag_ptr->f, "照応詞候補") ||
	 check_feature(tag_ptr->f, "NE内") ||
	 check_feature(tag_ptr->f, "数量"))? 0 : 
	(check_feature(tag_ptr->f, "係:未格") && 
	 (check_feature(tag_ptr->f, "ハ") || check_feature(tag_ptr->f, "モ")) ||
	 check_feature(tag_ptr->f, "文末")) ? SALIENCE_THEMA :
	(check_feature(tag_ptr->f, "読点") ||
	 check_feature(tag_ptr->f, "係:ガ格") ||
	 check_feature(tag_ptr->f, "係:ヲ格")) ? SALIENCE_CANDIDATE : SALIENCE_NORMAL;

    /* ENTITYの名前 */
    if (cp = check_feature(tag_ptr->f, "NE")) {
	strcpy(entity_ptr->name, cp + strlen("NE:"));
    }
    else if (cp = check_feature(tag_ptr->f, "照応詞候補")) {
	strcpy(entity_ptr->name, cp + strlen("照応詞候補:"));
    }
    else {
	strcpy(entity_ptr->name, tag_ptr->head_ptr->Goi2);
    }

    mention_mgr->mention->entity = entity_ptr;	    
    mention_mgr->mention->explicit_mention = NULL;	    
    strcpy(mention_mgr->mention->cpp_string, "＊");
    if ((cp = check_feature(tag_ptr->f, "係"))) {
	strcpy(mention_mgr->mention->spp_string, cp + strlen("係:"));
    }
    else {
	strcpy(mention_mgr->mention->spp_string, "＊");
    }
    mention_mgr->mention->flag = 'S'; /* 自分自身 */   
}

/*==================================================================*/
      TAG_DATA *get_analysis_tag_ptr(SENTENCE_DATA *sp, int i)
/*==================================================================*/
{
    TAG_DATA *tag_ptr;

    tag_ptr = substance_tag_ptr(sp->tag_data + i);

    /* 「砂糖を加えて混ぜて」などがある場合は解析順序を入れ替える */
    if (i > 2 && 
	check_feature((sp->tag_data + i    )->f, "用言:動") &&	    
	check_feature((sp->tag_data + i - 1)->f, "用言:動") &&	    
	check_feature((sp->tag_data + i - 2)->f, "助詞")) 
	tag_ptr = substance_tag_ptr(sp->tag_data + i - 1);
    if (i > 1 && i < sp->Tag_num - 1 &&
	check_feature((sp->tag_data + i + 1)->f, "用言:動") &&	    
	check_feature((sp->tag_data + i    )->f, "用言:動") &&	    
	check_feature((sp->tag_data + i - 1)->f, "助詞")) 
	tag_ptr = substance_tag_ptr(sp->tag_data + i + 1);

    return tag_ptr;
}

/*==================================================================*/
	    int make_context_structure(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 共参照解析結果を読み込み、省略解析を行い文の構造を構築する */
    int i, j;
    char *cp;
    TAG_DATA *tag_ptr;
    MENTION_MGR *mention_mgr;
   
    /* 省略以外のMENTIONの処理 */
    for (i = 0; i < sp->Tag_num; i++) { /* 解析文のタグ単位:i番目のタグについて */
	tag_ptr = substance_tag_ptr(sp->tag_data + i);

	/* 自分自身(MENTION)を生成 */       
	mention_mgr = &(tag_ptr->mention_mgr);
	mention_mgr->mention->tag_num = i;
	mention_mgr->mention->sent_num = sp->Sen_num;
	mention_mgr->mention->tag_ptr = tag_ptr;
	mention_mgr->mention->entity = NULL;
	mention_mgr->mention->explicit_mention = NULL;
	mention_mgr->mention->salience_score = 0;
	mention_mgr->num = 1;

	/* 入力から正解を読み込む場合 */
	if (OptReadFeature) {
	    /* 同格は正解コーパスに付与されないので自動解析 */
	    if (check_feature(tag_ptr->f, "同格") && 
		(cp = check_feature(tag_ptr->f, "Ｔ共参照"))) {
		read_one_annotation(sp, tag_ptr, cp + strlen("Ｔ共参照:"), TRUE);
	    }
	    /* featureから格解析結果を取得 */    
	    else if (cp = check_feature(tag_ptr->f, "格解析結果")) {		
		for (cp = strchr(cp + strlen("格解析結果:"), ':') + 1; *cp; cp++) {
		    if (*cp == ':' || *cp == ';') {
			read_one_annotation(sp, tag_ptr, cp + 1, TRUE);
		    }
		}
	    }
	}
	/* 自動解析の場合 */
	else if (cp = check_feature(tag_ptr->f, "Ｔ共参照")) {
	    read_one_annotation(sp, tag_ptr, cp + strlen("Ｔ共参照:"), TRUE);
	}

	/* 新しいENTITYである場合 */	
	if (!mention_mgr->mention->entity) {
	    make_new_entity(tag_ptr, mention_mgr);
	}
    }

    /* 省略のMENTIONの処理 */
    /* 入力から正解を読み込む場合 */
    if (OptReadFeature == 1) {

	/* 解析文のタグ単位:i番目のタグについて */
	for (i = sp->Tag_num - 1; i >= 0; i--) {
	    tag_ptr = get_analysis_tag_ptr(sp, i);
	    if (!check_analyze_tag(tag_ptr)) continue;	    

	    /* 解析対象格の設定 */
	    ELLIPSIS_CASE_LIST = (check_analyze_tag(tag_ptr) == CF_PRED) ?
		ELLIPSIS_CASE_LIST_VERB : ELLIPSIS_CASE_LIST_NOUN;
	    
	    /* この時点での各EntityのSALIENCE出力 */
	    if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
		printf(";;SALIENCE-%d-%d", sp->Sen_num, i);
		for (j = 0; j < entity_manager.num; j++) {
		    printf(":%.3f", (entity_manager.entity + j)->salience_score);
		}
		printf("\n");
	    }
	
	    /* featureから格解析結果を取得 */
	    if (cp = check_feature(tag_ptr->f, "格解析結果")) {		

		/* 共参照関係にある表現は格解析結果を取得しない */
		if (check_feature(tag_ptr->f, "体言") &&
		    (strstr(cp, "=/") || strstr(cp, "=構/") || strstr(cp, "=役/")))
		    continue;
		
		for (cp = strchr(cp + strlen("格解析結果:"), ':') + 1; *cp; cp++) {
		    if (*cp == ':' || *cp == ';') {
			read_one_annotation(sp, tag_ptr, cp + 1, FALSE);
		    }
		}
	    }
	}	
	return TRUE;
    }

    /* 省略解析を行う場合 */
    for (i = sp->Tag_num - 1; i >= 0; i--) { /* 解析文のタグ単位:i番目のタグについて */
	tag_ptr = get_analysis_tag_ptr(sp, i);
	if (!check_analyze_tag(tag_ptr)) continue;
	tag_ptr->tcf_ptr = NULL;
	tag_ptr->ctm_ptr = NULL;

	/* 解析対象格の設定 */
	ELLIPSIS_CASE_LIST = (check_analyze_tag(tag_ptr) == CF_PRED) ?
	    ELLIPSIS_CASE_LIST_VERB : ELLIPSIS_CASE_LIST_NOUN;

	if (tag_ptr->cf_ptr &&
	    !check_feature(tag_ptr->f, "NE") &&
	    !check_feature(tag_ptr->f, "NE内") &&
	    !check_feature(tag_ptr->f, "共参照") &&
	    !check_feature(tag_ptr->f, "共参照内")) {

	    /* tag_ptr->tcf_ptrを作成 */
	    tag_ptr->tcf_ptr = 	(TAG_CASE_FRAME *)
		malloc_data(sizeof(TAG_CASE_FRAME), "make_context_structure");
	    set_tag_case_frame(sp, tag_ptr);
	    	    	    
	    /* 位置カテゴリの生成 */	    
	    mark_loc_category(sp, tag_ptr);

	    /* この時点での各EntityのSALIENCE出力 */
	    if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
		printf(";;SALIENCE-%d-%d", sp->Sen_num, i);
		for (j = 0; j < entity_manager.num; j++) {
		    printf(":%.3f", (entity_manager.entity + j)->salience_score);
		}
		printf("\n");
	    } 
	    
	    /* 省略解析メイン */
	    ellipsis_analysis_main(tag_ptr);
	    
	    /* todo::将来的にはellipsis_analysis_mainでcpmを使わなくする */
	    free(tag_ptr->cpm_ptr);

	    /* todo::tcfを解放 (暫定的) */
	    for (j = 0; j < CF_ELEMENT_MAX; j++) {
		free(tag_ptr->tcf_ptr->cf.ex[j]);
		tag_ptr->tcf_ptr->cf.ex[j] = NULL;
		free(tag_ptr->tcf_ptr->cf.sm[j]);
		tag_ptr->tcf_ptr->cf.sm[j] = NULL;
		free(tag_ptr->tcf_ptr->cf.ex_list[j][0]);
		free(tag_ptr->tcf_ptr->cf.ex_list[j]);
		free(tag_ptr->tcf_ptr->cf.ex_freq[j]);
	    }
	    free(tag_ptr->tcf_ptr);

	    /* 並列要素を展開する */
	    expand_result_to_parallel_entity(tag_ptr);

	    /* 解析結果をENTITYと関連付ける */
	    anaphora_result_to_entity(tag_ptr);
	}
    }
}

/*==================================================================*/
		   void print_entities(int sen_num)
/*==================================================================*/
{
    int i, j;
    char *cp;
    MENTION *mention_ptr;
    ENTITY *entity_ptr;
    FEATURE *fp;
    MRPH_DATA m;

    printf(";;SENTENCE %d\n", sen_num); 
    for (i = 0; i < entity_manager.num; i++) {
	entity_ptr = entity_manager.entity + i;

	printf(";; ENTITY %d [ %s ] %f {\n", i, entity_ptr->name, entity_ptr->salience_score);
	for (j = 0; j < entity_ptr->mentioned_num; j++) {
	    mention_ptr = entity_ptr->mention[j];
	    printf(";;\tMENTION%3d {", j);
	    printf(" SEN:%3d", mention_ptr->sent_num);
	    printf(" TAG:%3d", mention_ptr->tag_num);
	    printf(" (%3d)", mention_ptr->tag_ptr->head_ptr->Num);
	    printf(" CPP: %4s", mention_ptr->cpp_string);
	    printf(" SPP: %4s", mention_ptr->spp_string);
	    printf(" FLAG: %c", mention_ptr->flag);
	    printf(" SS: %.3f", mention_ptr->salience_score);
	    printf(" WORD: %s", mention_ptr->tag_ptr->head_ptr->Goi2);

	    /* 格フレームのカバレッジを調べる際に必要となる情報 */
	    if (OptDisplay == OPT_DETAIL) {

		/* 用言の場合 */
		if (check_feature(mention_ptr->tag_ptr->f, "用言") &&
		    (mention_ptr->flag == 'C' || mention_ptr->flag == 'N' || mention_ptr->flag == 'O')) {

		    printf(" POS: %s", check_feature(mention_ptr->tag_ptr->f, "用言") + strlen("用言:"));
		    printf(" KEY: %s", make_pred_string(mention_ptr->tag_ptr, NULL, NULL, OptCaseFlag & OPT_CASE_USE_REP_CF, CF_PRED));

		    /* 代表表記が曖昧な用言の場合 */
		    if (check_feature(mention_ptr->tag_ptr->head_ptr->f, "原形曖昧")) {
			
			fp = mention_ptr->tag_ptr->head_ptr->f;
			while (fp) {
			    if (!strncmp(fp->cp, "ALT-", 4)) {
				sscanf(fp->cp + 4, "%[^-]-%[^-]-%[^-]-%d-%d-%d-%d-%[^\n]", 
				       m.Goi2, m.Yomi, m.Goi, 
				       &m.Hinshi, &m.Bunrui, 
				       &m.Katuyou_Kata, &m.Katuyou_Kei, m.Imi);
				printf("-%s", make_pred_string(mention_ptr->tag_ptr, &m, NULL, OptCaseFlag & OPT_CASE_USE_REP_CF, CF_PRED));
			    }
			    fp = fp->next;
			}
		    }
		    if (mention_ptr->tag_ptr->voice & VOICE_SHIEKI || 
			check_feature(mention_ptr->tag_ptr->f, "態:使役")) {
			printf(" VOICE: C");
		    }
		    else if (mention_ptr->tag_ptr->voice & VOICE_UKEMI ||
			     check_feature(mention_ptr->tag_ptr->f, "態:受動")) {
			printf(" VOICE: P");
		    }
		    else {
			printf(" VOICE: N");
		    }

		    /* 直接の格要素の基本句番号 */
		    if (mention_ptr->explicit_mention) {
			printf(" CTAG: %d", mention_ptr->explicit_mention->tag_num);
		    }
		}
		/* 格要素の場合 */
		else if (mention_ptr->flag == 'S' || mention_ptr->flag == '=') {

		    if (mention_ptr->tag_ptr->head_ptr == mention_ptr->tag_ptr->b_ptr->head_ptr) { /* 文節主辞であるかどうか */
			cp = get_bnst_head_canonical_rep(mention_ptr->tag_ptr->b_ptr, OptCaseFlag & OPT_CASE_USE_CN_CF);
		    }
		    else {
			cp = check_feature(mention_ptr->tag_ptr->f, "正規化代表表記");
			if (cp) cp += strlen("正規化代表表記:");
		    }

		    printf(" POS: %s", Class[mention_ptr->tag_ptr->head_ptr->Hinshi][mention_ptr->tag_ptr->head_ptr->Bunrui].id);
		    printf(" KEY: %s", cp);
		    if (check_feature(mention_ptr->tag_ptr->f, "補文")) {
			printf(" GE: 補文");
		    }
		    else if (check_feature(mention_ptr->tag_ptr->f, "時間")) {
			printf(" GE: 時間");
		    }
		    else if (check_feature(mention_ptr->tag_ptr->f, "数量")) {
			printf(" GE: 数量");
		    }
		    if ((cp = check_feature(mention_ptr->tag_ptr->head_ptr->f, "カテゴリ"))) {
			printf(" CT: %s", cp + strlen("カテゴリ:"));
		    }
		    if ((cp = check_feature(mention_ptr->tag_ptr->f, "NE"))) {
			printf(" NE: %s", cp + strlen("NE:"));
		    }
		}
	    }
	    printf(" }\n");
	}
	printf(";; }\n;;\n");
    }
}

/*==================================================================*/
	    void assign_anaphora_result(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 照応解析結果を基本句のfeatureに付与 */
    int i, j;
    char buf[DATA_LEN], tmp[IMI_MAX];
    MENTION *mention_ptr;
    TAG_DATA *tag_ptr;
	     
    for (i = 0; i < sp->Tag_num; i++) {
	tag_ptr = substance_tag_ptr(sp->tag_data + i);

	sprintf(buf, "EID:%d", tag_ptr->mention_mgr.mention->entity->num);
	assign_cfeature(&(tag_ptr->f), buf, FALSE);

	buf[0] = '\0';
	for (j = 1; j < tag_ptr->mention_mgr.num; j++) {
	    mention_ptr = tag_ptr->mention_mgr.mention + j;
	    
	    if (mention_ptr->flag == 'N' || mention_ptr->flag == 'C' ||
		mention_ptr->flag == 'O' || mention_ptr->flag == 'D') {

		if (!buf[0]) {
		    sprintf(buf, "格構造:%s:", 
			    OptReadFeature == 1 ? "?" : tag_ptr->ctm_ptr->cf_ptr->cf_id);
		}
		else {
		    strcat(buf, ";");
		}

		sprintf(tmp, "%s/%c/%s/%d",
			mention_ptr->cpp_string,
			mention_ptr->flag,
			mention_ptr->entity->name,
			mention_ptr->entity->num);
		strcat(buf, tmp);
	    }
	}
	if (buf[0]) assign_cfeature(&(tag_ptr->f), buf, FALSE);
    }
}

/*==================================================================*/
			 void decay_entity()
/*==================================================================*/
{
    /* ENTITYの活性値を減衰させる */
    int i;

    for (i = 0; i < entity_manager.num; i++) {
	entity_manager.entity[i].salience_score *= SALIENCE_DECAY_RATE;
    }
}

/*==================================================================*/
	      void anaphora_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    decay_entity();
    make_context_structure(sentence_data + sp->Sen_num - 1);
    assign_anaphora_result(sentence_data + sp->Sen_num - 1);
    if (OptAnaphora & OPT_PRINT_ENTITY) print_entities(sp->Sen_num);
}

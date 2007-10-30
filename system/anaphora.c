/*====================================================================

			       照応解析

                                         Ryohei Sasano 2007. 8. 27

    $Id$
====================================================================*/

#include "knp.h"

/*==================================================================*/
int read_one_annotation(SENTENCE_DATA *sp, TAG_DATA *tag_ptr, char *token)
/*==================================================================*/
{
    char flag, rel[SMALL_DATA_LEN], *cp;
    int tag_num, sent_num;
    MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
    MENTION *mention_ptr;

    sscanf(token, "%[^/]/%c/%*[^/]/%d/%d/", rel, &flag, &tag_num, &sent_num);

    if (flag == 'N' || flag == 'C' || flag == 'O' || flag == 'D') {
	
	if (!strcmp(rel, "=")) {
	    mention_ptr = mention_mgr->mention;
	    mention_ptr->entity = ((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;
	    mention_ptr->pp_code = 0;
	    mention_ptr->flag = '=';
	    /* entityのnameがNEでなく、tag_ptrがNEならば上書き*/
	    if (!strchr(mention_ptr->entity->name, ':') &&
		(cp = check_feature(tag_ptr->f, "NE"))) {
		strcpy(mention_ptr->entity->name, cp + strlen("NE:"));
	    }
	}
	else if (!strcmp(rel, "ガ") || !strcmp(rel, "ヲ") ||
		 !strcmp(rel, "ニ") || !strcmp(rel, "ノ")) {
	    mention_ptr = mention_mgr->mention + mention_mgr->num;
	    mention_ptr->entity = ((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;

	    mention_ptr->tag_num = mention_mgr->mention->tag_num;
	    mention_ptr->sent_num = mention_mgr->mention->sent_num;
	    mention_ptr->flag = flag;
	    mention_ptr->pp_code = pp_kstr_to_code(rel);
	    mention_mgr->num++;
	}

	if (!mention_ptr->entity) return FALSE;
	mention_ptr->entity->mention[mention_ptr->entity->mentioned_num] = mention_ptr;
	mention_ptr->entity->mentioned_num++;
	if (flag == 'O' && strcmp(rel, "=")) mention_ptr->entity->antecedent_num++;
    }
    return TRUE;
}

/*==================================================================*/
ENTITY_PRED_MGR *make_epm(SENTENCE_DATA *sp, TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* ENTITY_PRED_MGRを作成する関数
       make_data_cframeを用いて入力文の格構造を作成するため
       CF_PRED_MGRを作り、そのcfをコピーしている */

    int i;
    CF_PRED_MGR *cpm_ptr;
    ENTITY_PRED_MGR *epm_ptr;
   
    /* cpmの作成 */
    cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "make_epm");
    init_case_frame(&(cpm_ptr->cf));
    cpm_ptr->pred_b_ptr = tag_ptr;

    /* 入力文側の格要素設定 */
    set_data_cf_type(cpm_ptr);
    make_data_cframe(sp, cpm_ptr);

    /* ENTITY_PRED_MGRを作成・入力文側の格要素をコピー */
    epm_ptr = (ENTITY_PRED_MGR *)malloc_data(sizeof(ENTITY_PRED_MGR), "make_epm");
    epm_ptr->cf = cpm_ptr->cf;
    epm_ptr->pred_b_ptr = tag_ptr;
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	epm_ptr->elem_b_ptr[i] = cpm_ptr->elem_b_ptr[i];
	epm_ptr->elem_s_ptr[i] = cpm_ptr->elem_s_ptr[i];
    }

    free(cpm_ptr);
    return epm_ptr;
}

/*==================================================================*/
    int set_cf_candidate(TAG_DATA *tag_ptr, CASE_FRAME **cf_array)
/*==================================================================*/
{
    int i, l, frame_num, hiragana_prefer_flag = 0;
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
			if (((tag_ptr->cf_ptr + l)->type == tag_ptr->epm_ptr->cf.type) &&
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
	    if ((tag_ptr->cf_ptr + l)->type == tag_ptr->epm_ptr->cf.type && 
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
int ellipsis_analysis_main(SENTENCE_DATA *sp, ENTITY_PRED_MGR *epm_ptr)
/*==================================================================*/
{
    /* ある基本句を対象として省略解析を行う関数 */
    /* 格フレームごとにループを回す */

    int l, frame_num = 0;
    CASE_FRAME **cf_array;

    /* 使用する格フレームの設定 */
    cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *)*epm_ptr->pred_b_ptr->cf_num, 
					  "ellipsis_analysis_main");
    frame_num = set_cf_candidate(epm_ptr->pred_b_ptr, cf_array);

    /* 候補の格フレームについて省略解析を実行 */
    for (l = 0; l < frame_num; l++) {
	/* OR の格フレームを除く */
	if (((*(cf_array+l))->etcflag & CF_SUM) && frame_num != 1) {
	    continue;
	}

	/* 格フレームを仮指定 */
/* 	cmm.cf_ptr = *(cf_array+l); */
/* 	cpm_ptr->result_num = 1; */

/* 	/\* 今ある格要素を対応づけ *\/ */
/* 	case_frame_match(cpm_ptr, &cmm, OptCFMode, -1); */
/* 	cpm_ptr->score = cmm.score; */

/* 	/\* for (i = 0; i < cmm.result_num; i++) *\/ { */
/* 	i = 0; */

    }    

    
}

/*==================================================================*/
	    int make_context_structure(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char *cp;
    TAG_DATA *tag_ptr;
    MENTION_MGR *mention_mgr;
    ENTITY *entity_ptr;
    ENTITY_PRED_MGR *epm_ptr;
    
    /* ENTITYを生成 */
    for (i = 0; i < sp->Tag_num; i++) { /* 解析文のタグ単位:i番目のタグについて */

	tag_ptr = sp->tag_data + i; 
	
	/* 自分自身(MENTION)を生成 */
	mention_mgr = &(tag_ptr->mention_mgr);
	mention_mgr->mention->tag_num = i;
	mention_mgr->mention->sent_num = sp->Sen_num;
	mention_mgr->mention->entity = NULL;
	mention_mgr->num = 1;

	/* 入力から正解を読み込む場合 */
	if (OptReadFeature) {

	    /* featureから格解析結果を取得 */
	    if (cp = check_feature(tag_ptr->f, "格解析結果")) {		
		cp += strlen("格解析結果:");
		cp = strchr(cp, ':') + 1;	
		for (; *cp; cp++) {
		    if (*cp == ':' || *cp == ';') {
			read_one_annotation(sp, tag_ptr, cp + 1);
		    }
		}
	    }
	}
	/* 自動解析の場合 */
	else if (cp = check_feature(tag_ptr->f, "Ｔ共参照")) {
	    read_one_annotation(sp, tag_ptr, cp + strlen("Ｔ共参照:"));
	}

	/* 新しいENTITYである場合 */
	if (!mention_mgr->mention->entity) {
	    if (entity_manager.num >= ENTITY_MAX - 1) { 
		fprintf(stderr, "Entity buffer overflowed!\n");
		exit(1);
	    }
	    entity_ptr = entity_manager.entity + entity_manager.num;
	    entity_manager.num++;				
	    entity_ptr->num = entity_manager.num;
	    entity_ptr->mention[0] = mention_mgr->mention;
	    entity_ptr->mentioned_num = 1;
	    entity_ptr->antecedent_num = 0;
	    /* nameの設定(NE > 照応詞候補 > 主辞形態素の原型) */
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
	    mention_mgr->mention->pp_code = 0;
	    mention_mgr->mention->flag = 'S'; /* 自分自身 */

	}
    }

    /* 入力から正解を読み込む場合はここで終了 */
    if (OptReadFeature) return TRUE;
    
    /* 省略解析 */
    for (i = sp->Tag_num - 1; i >= 0; i--) { /* 解析文のタグ単位:i番目のタグについて */

	tag_ptr = sp->tag_data + i; 
	tag_ptr->epm_ptr = NULL;
	
	if (tag_ptr->cf_ptr &&
	    !check_feature(tag_ptr->f, "省略解析なし") &&
	    !check_feature(tag_ptr->f, "NE") &&
	    !check_feature(tag_ptr->f, "NE内") &&
	    !check_feature(tag_ptr->f, "共参照") &&
	    !check_feature(tag_ptr->f, "共参照内")) {

	    /* tag_ptr->epm_ptrを作成 */
	    tag_ptr->epm_ptr = make_epm(sp, tag_ptr);
	    
	    /* 省略解析メイン */
	    ellipsis_analysis_main(sp, tag_ptr->epm_ptr);

	    /* epmを解放 (暫定的) */
	    for (j = 0; j < CF_ELEMENT_MAX; j++) {
		free(tag_ptr->epm_ptr->cf.ex[j]);
		tag_ptr->epm_ptr->cf.ex[j] = NULL;
		free(tag_ptr->epm_ptr->cf.sm[j]);
		tag_ptr->epm_ptr->cf.sm[j] = NULL;
		free(tag_ptr->epm_ptr->cf.ex_list[j][0]);
		free(tag_ptr->epm_ptr->cf.ex_list[j]);
		free(tag_ptr->epm_ptr->cf.ex_freq[j]);
	    }
	    free(tag_ptr->epm_ptr);
	}
    }
}

/*==================================================================*/
			void print_entities()
/*==================================================================*/
{
    int i, j;
    MENTION *mention_ptr;
    ENTITY *entity_ptr;

    for (i = 0; i < entity_manager.num; i++) {
	entity_ptr = entity_manager.entity + i;

	if (entity_ptr->mentioned_num == 1) continue;
	
	printf(";;ENTITY %d [ %s ] {\n", i, entity_ptr->name);
	for (j = 0; j < entity_ptr->mentioned_num; j++) {
	    mention_ptr = entity_ptr->mention[j];
	    printf(";;\tMENTION%3d {", j);
	    printf(" SEN:%3d", mention_ptr->sent_num);
	    printf(" TAG:%3d", mention_ptr->tag_num);
	    printf(" PP: %s", pp_code_to_kstr(mention_ptr->pp_code));
	    printf(" FLAG: %c", mention_ptr->flag);
	    printf(" WORD: %s", ((sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num)->head_ptr->Goi2);
	    printf(" }\n");
	}
	printf(";;}\n\n");
    }
}

/*==================================================================*/
	      void anaphora_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    make_context_structure(sentence_data + sp->Sen_num - 1);
    print_entities();
}

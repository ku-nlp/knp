/*====================================================================

			       照応解析

                                         Ryohei Sasano 2007. 8. 27

    $Id$
====================================================================*/

#include "knp.h"

/*==================================================================*/
int read_one_annotation(SENTENCE_DATA *sp, MENTION_MGR *mention_mgr, char *token)
/*==================================================================*/
{
    char flag, rel[SMALL_DATA_LEN];
    int tag_num, sent_num;
    MENTION *mention_ptr;

    sscanf(token, "%[^/]/%c/%*[^/]/%d/%d/", rel, &flag, &tag_num, &sent_num);

    if (flag == 'N' || flag == 'C' || flag == 'O' || flag == 'D') {
	
	if (!strcmp(rel, "=")) {
	    mention_ptr = mention_mgr->mention;
	    mention_ptr->entity = ((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;
	    mention_ptr->pp_code = 0;
	    mention_ptr->flag = '=';
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
	    int make_context_structure(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char *cp;
    TAG_DATA *tag_ptr;
    MENTION_MGR *mention_mgr;
    ENTITY *entity_ptr;
    CF_PRED_MGR *cpm_ptr;

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
			read_one_annotation(sp, mention_mgr, cp + 1);
		    }
		}
	    }
	}

	/* 自動解析の場合 */
	else if (cp = check_feature(tag_ptr->f, "Ｔ共参照")) {
	    read_one_annotation(sp, mention_mgr, cp + strlen("Ｔ共参照:"));
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
	    mention_mgr->mention->entity = entity_ptr;
	    mention_mgr->mention->pp_code = 0;
	    mention_mgr->mention->flag = 'S'; /* 自分自身 */
	}
    }

    /* 入力から正解を読み込む場合はここで終了 */
    if (OptReadFeature) return 1;
    
    /* 省略解析 */
    for (i = sp->Tag_num - 1; i >= 0; i--) { /* 解析文のタグ単位:i番目のタグについて */

	tag_ptr = sp->tag_data + i; 

	if (tag_ptr->cf_ptr &&
	    !check_feature(tag_ptr->f, "省略解析なし") &&
	    !check_feature(tag_ptr->f, "NE") &&
	    !check_feature(tag_ptr->f, "NE内") &&
	    !check_feature(tag_ptr->f, "共参照") &&
	    !check_feature(tag_ptr->f, "共参照内")) {

	    cpm_ptr = tag_ptr->cpm_ptr;
	    cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "make_context_structure");
		
	    /* 初期化 */
	    init_case_frame(&(cpm_ptr->cf));
	    cpm_ptr->pred_b_ptr = tag_ptr;
	    cpm_ptr->score = -1;
	    cpm_ptr->result_num = 0;
	    cpm_ptr->tie_num = 0;
	    cpm_ptr->cmm[0].cf_ptr = NULL;
	    cpm_ptr->decided = CF_UNDECIDED;

	    /* 入力文側の格要素設定 */
	    set_data_cf_type(cpm_ptr);
	    make_data_cframe(sp, cpm_ptr);
		
	    /* cpmを解放 (暫定的) */
	    for (j = 0; j < CF_ELEMENT_MAX; j++) {
		free(cpm_ptr->cf.ex[j]);
		cpm_ptr->cf.ex[j] = NULL;
		free(cpm_ptr->cf.sm[j]);
		cpm_ptr->cf.sm[j] = NULL;
		free(cpm_ptr->cf.ex_list[j][0]);
		free(cpm_ptr->cf.ex_list[j]);
		free(cpm_ptr->cf.ex_freq[j]);
	    }
	    
	    /* 省略解析メイン */
	    ellipsis_analysis(sp, cpm_ptr
	    	
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
	
	printf("ENTITY %d {\n", i);
	for (j = 0; j < entity_ptr->mentioned_num; j++) {
	    mention_ptr = entity_ptr->mention[j];
	    printf("\tMENTION%3d {", j);
	    printf(" SEN:%3d", mention_ptr->sent_num);
	    printf(" TAG:%3d", mention_ptr->tag_num);
	    printf(" PP: %s", pp_code_to_kstr(mention_ptr->pp_code));
	    printf(" FLAG: %c", mention_ptr->flag);
	    printf(" WORD: %s", ((sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num)->head_ptr->Goi2);
	    printf(" }\n");
	}
	printf("}\n\n");
    }
}

/*==================================================================*/
	      void anaphora_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    make_context_structure(sentence_data + sp->Sen_num - 1);
    print_entities();
}

 /*====================================================================

  照応解析

  Ryohei Sasano 2007. 8. 27


  $Id$
  ====================================================================*/

#include "knp.h"
#include "model.h"

/* 省略解析に関するパラメータ */
//#define CASE_CANDIDATE_MAX  10 /* 照応解析用格解析結果を保持する数 */
#define CASE_CANDIDATE_MAX  5 /* 照応解析用格解析結果を保持する数 */

#define CASE_CAND_DIF_MAX   4.6 /* 格解析の候補として考慮するスコアの差の最大値(log(20)) */
//#define ELLIPSIS_RESULT_MAX 100  /* 省略解析結果を保持する */
#define ELLIPSIS_RESULT_MAX 10  /* 省略解析結果を保持する */
#define ELLIPSIS_CORRECT_MAX 3  /* 省略解析結果のうち正解のものを保持する */
#define SALIENCE_DECAY_RATE 0.5 /* salience_scoreの減衰率 */
#define SALIENCE_THRESHOLD 0 /* 解析対象とするsalience_scoreの閾値(=は含まない) */
#define INITIAL_SCORE -10000

/* 文の出現要素に与えるsalience_score */
#define SALIENCE_THEMA 2.0 /* 重要な要素(未格,文末)に与える */
#define SALIENCE_CANDIDATE 1.0 /* 先行詞候補とする要素(ガ格,ヲ格など)に与える */
#define SALIENCE_NORMAL 0.4 /* 上記以外の要素に与える */
#define SALIENCE_ZERO 1.0 /* ゼロ代名詞に与える */
#define SALIENCE_ASSO 0.0 /* 連想照応の先行詞に与える */


#define MODALITY_NUM 11
#define VOICE_NUM 5
#define KEIGO_NUM 3

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



/* clear_contextされた時点での文数、ENTITY数を記録 */
int base_sentence_num = 0;
int base_entity_num = 0;

/* 位置カテゴリを保持 */
int loc_category[BNST_MAX];
DBM_FILE share_case_pair_db;
DBM_FILE event_db;

/*解析が終了した文節を保持*/
int analysis_flags[SENTENCE_MAX][TAG_MAX];


/* 解析結果を保持するためのENTITY_CASE_MGR
   CASE_CANDIDATE_MAX個の照応解析用格解析の結果の上位の保持*/
CF_TAG_MGR case_candidate_ctm[CASE_CANDIDATE_MAX];
/*   次のELLIPSIS_RESULT_MAX個の省略解析結果のベスト解の保持、*/
CF_TAG_MGR ellipsis_result_ctm[ELLIPSIS_RESULT_MAX];
CF_TAG_MGR ellipsis_correct_ctm[ELLIPSIS_CORRECT_MAX];


/* 省略解析の対象とする格のリスト */
char *ELLIPSIS_CASE_LIST_VERB[] = {"ガ", "ヲ", "ニ","ガ２","\0"};
char *ELLIPSIS_CASE_LIST_NOUN[] = {"ノ", "ノ", "ノ？", "\0"};
char **ELLIPSIS_CASE_LIST = ELLIPSIS_CASE_LIST_VERB;







double overt_arguments_weight = 1.0;
double all_arguments_weight = 1.0;
double case_feature_weight[ELLIPSIS_CASE_NUM][O_FEATURE_NUM];


double def_overt_arguments_weight = 1.0;
double def_all_arguments_weight = 1.0;
double def_case_feature_weight[ELLIPSIS_CASE_NUM][O_FEATURE_NUM];


double ModifyWeight[4] = {1.2, 0.8, 0.8, 0.0};

int OptZeroPronoun = 0;

#define AUTHOR_REP_NUM 7
#define READER_REP_NUM 5

#define UNNAMED_ENTITY_NUM 5
#define UNNAMED_ENTITY_NAME_NUM 2
#define UNNAMED_ENTITY_CATEGORY_NUM 17
#define UNNAMED_ENTITY_NE_NUM 7
#define UNNAMED_ENTITY_REP_NUM 6
#define CATEGORY_NUM 19

char *unnamed_entity[UNNAMED_ENTITY_NUM]={"一人称","二人称","不特定-人","不特定-その他","補文"}; //不特定:その他とかにすると":"が後の処理で問題になる
char *unnamed_entity_name[UNNAMED_ENTITY_NUM][UNNAMED_ENTITY_NAME_NUM]={{"一人称",""},
																		{"二人称",""},
																		{"不特定:人",""},
																		{"不特定:物","不特定:状況"},
																		{""}};


char *unnamed_entity_category[UNNAMED_ENTITY_NUM][UNNAMED_ENTITY_CATEGORY_NUM]={{"人","組織・団体",""},
																				{"人",""},
																				{"人",""},
																				{"動物","動物-部位","植物","植物-部位","人工物-乗り物","人工物-衣類","人工物-食べ物","人工物-その他","場所-機能","場所-自然","場所-施設","場所-施設部位","場所-その他","自然物","抽象物","色","時間"},
																				{"時間",""}
};


char *unnamed_entity_ne[UNNAMED_ENTITY_NUM][UNNAMED_ENTITY_NE_NUM]={{"PERSON","ORGANIZATION",""},
																	{"PERSON",""},
																	{"PERSON","ORGANIZATION",""},
																	{"ARTIFACT","DATE","TIME","PERCENT","MONEY","LOCATION",""},
																	{"TIME","DATE",""}};

char *unnamed_entity_rep[UNNAMED_ENTITY_NUM][UNNAMED_ENTITY_REP_NUM]={{"私/わたし","我々/われわれ","俺/おれ","僕/ぼく","",""},
																	  {"あなた/あなた","客/きゃく","君/きみ","皆様/みなさま","",""},
																	  {"人/ひと","","","","",""},
																	  {"状況/じょうきょう","もの/もの","","","",""},
																	  {"<補文>","<時間>","<数量>","","",""}};


char *category_list[CATEGORY_NUM] = {"人","組織・団体","動物","動物-部位","植物","植物-部位","人工物-乗り物","人工物-衣類","人工物-食べ物","人工物-その他","場所-機能","場所-自然","場所-施設","場所-施設部位","場所-その他","自然物","抽象物","色","時間"};
char *alternate_category[CATEGORY_NUM][CATEGORY_NUM] = {{"組織・団体",""},
														{"人",""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""},
														{""}};

/*不特定その他は要検討*/
/*不特定:その他=不特定:物+不特定:状況*/

char *modality[MODALITY_NUM]={"意志","勧誘","命令","禁止","評価:弱","評価:強","認識-推量","認識-蓋然性","認識-証拠性","依頼Ａ","依頼Ｂ"};
int modality_count[MODALITY_NUM];
char *keigo[KEIGO_NUM]={"尊敬表現","謙譲表現","丁寧表現"};
int keigo_count[KEIGO_NUM];
int yobikake_count =0;
char *voice[VOICE_NUM] = {"使役","受動","可能","もらう","ほしい"};


#define NE_NUM 8
char *ne[NE_NUM] = {"PERSON","ORIGANIZATION","ARTIFACT","DATE","LOCATION","TIME","MONY","PERCENT"};

double context_feature[ENTITY_MAX][CONTEXT_FEATURE_NUM];

int svm_feaature_opt =1;
int candidate_entities[ENTITY_MAX];

double max_reliabirity;
TAG_DATA *max_reliabirity_tag_ptr;
int analysis_flag;
int ite_count;

/*プロトタイプ宣言*/
/*extern.hに書いていないもの*/
/*==================================================================*/
TAG_DATA *substance_tag_ptr(TAG_DATA *tag_ptr);
double get_lift_value(char *analysing_tag_pa_string,char *target_tag_pa_string,char *temp_match_string);
ENTITY *make_each_unnamed_entity(char *name);
double calc_ellipsis_score_of_ctm(CF_TAG_MGR *ctm_ptr, TAG_CASE_FRAME *tcf_ptr);
double calc_static_salience_score(TAG_DATA *tag_ptr);
void make_aresult_string(CF_TAG_MGR *ctm_ptr,char *aresult);
/*==================================================================*/


void abbreviate_NE(char *cp)
{
	char buf[DATA_LEN];
	char *temp_cp;
	strcpy(buf,cp);
	
	strcpy(cp,"");
	if(strstr(buf,"PERCENT"))
	{
		strcpy(cp,"NE:%");
	}
	else
	{
		strcpy(cp,buf);
		cp[strlen("NE:")+3] ='\0';
	}
	temp_cp=buf+strlen("NE:");
	while(*temp_cp != ':')
	{
		temp_cp++;
	}
	strcat(cp,temp_cp);

}

void set_mention_from_coreference(TAG_DATA *tag_ptr,MENTION *mention_ptr)
{

	char hypo_name[REPNAME_LEN_MAX],*cp;
	int hypo_flag,name_change_flag;
	TAG_DATA *parent_ptr;
				
	mention_ptr->explicit_mention = NULL;
	
	mention_ptr->salience_score = mention_ptr->entity->salience_score;
	
	mention_ptr->static_salience_score = calc_static_salience_score(tag_ptr);
	strcpy(mention_ptr->cpp_string, "＊");
	mention_ptr->entity->salience_score += mention_ptr->static_salience_score;
	
	parent_ptr = tag_ptr->parent;
	while (parent_ptr && parent_ptr->para_top_p)
	{
		parent_ptr = parent_ptr->parent;
	}
	if (check_feature(tag_ptr->f, "係:ニ格") || check_feature(tag_ptr->f, "係:ノ格"))
		mention_ptr->entity->tmp_salience_flag = 1;
	
	if ((cp = check_feature(tag_ptr->f, "係"))) {
		strcpy(mention_ptr->spp_string, cp + strlen("係:"));
	} 
	else if (check_feature(tag_ptr->f, "文末")) {
		strcpy(mention_ptr->spp_string, "文末");
	} 
	else {
		strcpy(mention_ptr->spp_string, "＊");
	}
	mention_ptr->type = '=';
			
	cp = strchr(mention_ptr->entity->name,'|');
	
	hypo_flag = 0;
	name_change_flag = 0;
	if(cp != NULL)
	{
		strcpy(hypo_name,cp);
		hypo_flag = 1;
	}
	if (cp = check_feature(tag_ptr->f, "NE")) {
		strcpy(mention_ptr->entity->named_entity ,cp+strlen("NE:"));
	}
	/* entityのnameが"の"ならばnameを上書き */
	/*salience_scoreの何を見ているのかは不明(2011_0202 hangyo)*/
	if (!strcmp(mention_ptr->entity->name, "の") ||
		mention_ptr->salience_score == 0 && mention_ptr->entity->salience_score > 0) {
		if (cp = check_feature(tag_ptr->f, "NE")) {
			abbreviate_NE(cp);
			strcpy(mention_ptr->entity->name, cp + strlen("NE:"));
		}
		else if (cp = check_feature(tag_ptr->f, "照応詞候補")) {
			strcpy(mention_ptr->entity->name, cp + strlen("照応詞候補:"));
		}
		else {
			strcpy(mention_ptr->entity->name, tag_ptr->head_ptr->Goi2);
		}
		name_change_flag = 1;
	}
	
	/* entityのnameがNEでなく、tag_ptrがNEならばnameを上書き */
	if (!strchr(mention_ptr->entity->name, ':') &&
		(cp = check_feature(tag_ptr->f, "NE"))) {
		abbreviate_NE(cp);
		strcpy(mention_ptr->entity->name, cp + strlen("NE:"));
		name_change_flag = 1;
	}
	/* entityのnameがNEでなく、tag_ptrが同格ならばnameを上書き */
	else if (!strchr(mention_ptr->entity->name, ':') &&
			 check_feature(tag_ptr->f, "同格")) {
		if (cp = check_feature(tag_ptr->f, "照応詞候補")) {
			strcpy(mention_ptr->entity->name, cp + strlen("照応詞候補:"));
		}
		else {
			strcpy(mention_ptr->entity->name, tag_ptr->head_ptr->Goi2);
		}
		name_change_flag = 1;
	}
	if(name_change_flag ==1)
	{
		mention_ptr->entity->rep_sen_num = mention_ptr->sent_num;
		mention_ptr->entity->rep_tag_num = tag_ptr->num;

	}
	if(name_change_flag == 1 && hypo_flag ==1)
	{
		strcat(mention_ptr->entity->name,hypo_name);
	}

}

double calc_static_salience_score(TAG_DATA *tag_ptr)
{
	return 	((check_feature(tag_ptr->f, "ハ") || check_feature(tag_ptr->f, "モ")) &&
			 check_feature(tag_ptr->f, "係:未格") && !check_feature(tag_ptr->f, "括弧終") ||
			 check_feature(tag_ptr->f, "同格") ||
			 check_feature(tag_ptr->f, "文末")) ? SALIENCE_THEMA : 
		(check_feature(tag_ptr->f, "読点") && tag_ptr->para_type != PARA_NORMAL ||
		 check_feature(tag_ptr->f, "係:ガ格") ||
		 check_feature(tag_ptr->f, "係:ヲ格")) ? SALIENCE_CANDIDATE : SALIENCE_NORMAL;
	
}


void link_entity_from_corefer_id(SENTENCE_DATA *sp,TAG_DATA *tag_ptr,char *cp)
{
	int corefer_id;
	MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
	int entity_num;
	MENTION *mention_ptr = NULL;
	TAG_DATA *parent_ptr;
	int hypo_flag,name_change_flag;
	char hypo_name[REPNAME_LEN_MAX];
	sscanf(cp,"COREFER_ID:%d",&corefer_id);
	mention_ptr = mention_mgr->mention;
	for (entity_num = 0; entity_num < entity_manager.num; entity_num++) 
	{
		if(entity_manager.entity[entity_num].corefer_id == corefer_id)
		{
			mention_ptr->entity = &(entity_manager.entity[entity_num]);
			set_mention_from_coreference(tag_ptr,mention_ptr);
			
			mention_ptr->entity->mention[mention_ptr->entity->mentioned_num] = mention_ptr;
			if (mention_ptr->entity->mentioned_num >= MENTIONED_MAX - 1) { 
				fprintf(stderr, "Entity \"%s\" mentiond too many times!\n", mention_ptr->entity->name);
			}
			else  {
				mention_ptr->entity->mentioned_num++;
			}
			
		}
	}
}

void set_candidate_entities(int sent_num)
{
	/*省略解析のEnitityの候補を決める*/
	int i;
	int entity_num;
	int mention_num;

	
	for(i = 0; i<ENTITY_MAX;i++)
	{
		if(OptReadFeature & OPT_COREFER_AUTO || OptAnaphora & OPT_PRUNING)
		{
			candidate_entities[i] =0;
		}
		else
		{
			candidate_entities[i] =1;
		}
	}
	for (entity_num = 0; entity_num < entity_manager.num; entity_num++) 
	{
		if(entity_num < UNNAMED_ENTITY_NUM)
		{
			candidate_entities[entity_num] = 1;
		}
		else
		{
			if(strcmp(entity_manager.entity[entity_num].named_entity,""))
			{
				for ( mention_num =0;mention_num < entity_manager.entity[entity_num].mentioned_num;mention_num++)
				{
					
					if(sent_num - entity_manager.entity[entity_num].mention[mention_num]->sent_num <= 3 && sent_num - entity_manager.entity[entity_num].mention[mention_num]->sent_num >= -3)
					{
						candidate_entities[entity_num] =1;
					}
				}
			}
			for (mention_num =0;mention_num < entity_manager.entity[entity_num].mentioned_num;mention_num++)
			{
				if(sent_num - entity_manager.entity[entity_num].mention[mention_num]->sent_num <=1 &&  sent_num - entity_manager.entity[entity_num].mention[mention_num]->sent_num >= 0)
				{
					candidate_entities[entity_num] =1;
				}
			}
		}
	}
}

double calc_score_of_case_frame_assingemnt(CF_TAG_MGR *ctm_ptr, TAG_CASE_FRAME *tcf_ptr)
{
	int i, j, k,e_num, debug = 1;
    double score;
    char key[SMALL_DATA_LEN];
	ENTITY *entity_ptr;
	double tmp_score;
	char *cp;
    /* 対象の格フレームが選択されることのスコア */
    score = get_cf_probability_for_pred(&(tcf_ptr->cf), ctm_ptr->cf_ptr);
	

    /* 対応付けられた要素に関するスコア(格解析結果) */
    for (i = 0; i < ctm_ptr->result_num; i++) {
		double tmp_score = FREQ0_ASSINED_SCORE;
		double prob;
		e_num = ctm_ptr->cf_element_num[i];
		entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i];
		if((OptAnaphora & OPT_UNNAMED_ENTITY)&& (entity_ptr->num < UNNAMED_ENTITY_NUM))
		{
			int entity_num = entity_ptr->num;
			
			if (OptGeneralCF & OPT_CF_CATEGORY)
			{
				for (k=0;k<UNNAMED_ENTITY_CATEGORY_NUM;k++)
				{
					if(!strcmp(unnamed_entity_category[entity_num][k],""))
					{
							break;
					}
					sprintf(key, "CT:%s:",unnamed_entity_category[entity_num][k]);
					prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE);
					if(prob && tmp_score < log(prob))
					{
						tmp_score = log(prob);
					}
					
				}
			}
			for (k=0;k<UNNAMED_ENTITY_REP_NUM;k++)
			{
				
				if(!strcmp(unnamed_entity_rep[entity_num][k],""))
				{
						break; 
				}
				sprintf(key,unnamed_entity_rep[entity_num][k]);
				prob = _get_ex_probability_internal(key,e_num ,ctm_ptr->cf_ptr );

				if(prob && tmp_score < log(prob))
				{
					tmp_score = log(prob);
				}
				
			}
			
		}
		else
		{

			for (j = 0; j < entity_ptr->mentioned_num; j++) {
				if (entity_ptr->mention[j]->type != 'S' && entity_ptr->mention[j]->type != '=') continue;
				
						
				if ((OptGeneralCF & OPT_CF_CATEGORY) && 
					(cp = check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ"))) {
					
					while (strchr(cp, ':') && (cp = strchr(cp, ':')) || (cp = strchr(cp, ';'))) {
						sprintf(key, "CT:%s:", ++cp);
						if (strchr(key + 3, ';')) *strchr(key + 3, ';') = ':'; /* tag = CT:組織・団体;抽象物: */
						prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE);
						if(prob && tmp_score < log(prob))
						{
							tmp_score = log(prob);
						}
					}
				}
				
				if ((OptGeneralCF & OPT_CF_NE) && 
					(cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE")) )
				{
					prob = get_ex_ne_probability(cp, e_num, ctm_ptr->cf_ptr, TRUE);
					
					if(prob && tmp_score < log(prob))
					{
						tmp_score = log(prob);
					}
				}
				prob = get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
										  entity_ptr->mention[j]->tag_ptr, e_num, ctm_ptr->cf_ptr, FALSE);
				if(prob && tmp_score <prob)
				{
					tmp_score = prob;
				}
			}
		}
		score += tmp_score;
		if (0&&OptDisplay == OPT_DEBUG && debug)
		{
			if(i < ctm_ptr->case_result_num)
			{
				printf(";;対応あり:%s-%s:%f:%f ", ctm_ptr->elem_b_ptr[i]->head_ptr->Goi2, 
					   pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
					   get_ex_probability_with_para(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr),
					   get_case_function_probability_for_pred(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr, TRUE));
			}
			else
			{
				if((entity_manager.entity + ctm_ptr->entity_num[i])->hypothetical_flag != 1)
				{
					printf(";;対応あり:%s-%s:%f:%f ", (entity_manager.entity + ctm_ptr->entity_num[i])->name,
						   pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
						   get_ex_probability_with_para(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr),
						   get_case_function_probability_for_pred(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr, TRUE));
				}
			}

		}	
    }
	
    /* 入力文の格要素のうち対応付けられなかった要素に関するスコア */
    for (i = 0; i < tcf_ptr->cf.element_num - ctm_ptr->case_result_num; i++) {
		if (0 &&OptDisplay == OPT_DEBUG && debug) 
		{
			if(i < ctm_ptr->case_result_num)
			{
				printf(";;対応なし:%s:%f ", 
					   (tcf_ptr->elem_b_ptr[ctm_ptr->non_match_element[i]])->head_ptr->Goi2, score);	
			}
		}
		score += FREQ0_ASSINED_SCORE + UNKNOWN_CASE_SCORE;
    }
    if (0 && OptDisplay == OPT_DEBUG && debug) printf(";; %f ", score);	   
	
    /* 格フレームの格が埋まっているかどうかに関するスコア */
    for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
		if (tcf_ptr->cf.type == CF_NOUN) continue;
		score += get_case_probability(e_num, ctm_ptr->cf_ptr, ctm_ptr->filled_element[e_num], NULL);	
    }
    if (0 &&OptDisplay == OPT_DEBUG && debug) printf(";; %f\n", score);
	
    return score;

}

int relax_compare_result(char *gresult, char *aresult)
{
	/*全ての格でどれか一個でも共通するものがあれば正解にする(並列などの対処のため)*/
	
	int gresult_entity_list[ELLIPSIS_CASE_NUM][ENTITY_MAX];
	int aresult_entity_list[ELLIPSIS_CASE_NUM][ENTITY_MAX];
	
	int gresult_entity_num[ELLIPSIS_CASE_NUM];
	int aresult_entity_num[ELLIPSIS_CASE_NUM];
	int check_case[ELLIPSIS_CASE_NUM];
	int i,j,k;
	char cp[REPNAME_LEN_MAX];
	char *tp;

	for (i=0;i<ELLIPSIS_CASE_NUM;i++)
	{
		gresult_entity_num[i]=0;
		aresult_entity_num[i]=0;
	}

	strcpy(cp,aresult);
	
	if(*cp != '\0')
	{

		tp = strtok(cp," ");
		while(tp != NULL)
		{
			if(tp !=NULL)
			{
				int case_num;
				for (case_num =0;case_num<ELLIPSIS_CASE_NUM;case_num++)
				{
					char *case_ptr;
					if(case_ptr = strstr(tp,ELLIPSIS_CASE_LIST_VERB[case_num]))
					{
						int e_num;
						case_ptr+=strlen(ELLIPSIS_CASE_LIST_VERB[case_num]);
						sscanf(case_ptr,":%d",&e_num);
						aresult_entity_list[case_num][ aresult_entity_num[case_num] ] = e_num;
						aresult_entity_num[case_num]++;
					}
				}
				tp = strtok(NULL," ");
			}
		}
	}



	strcpy(cp,gresult);
	
	if(*cp != '\0')
	{
		tp = strtok(cp," ");
		while(tp != NULL)
		{
			if(tp !=NULL)
			{
				int case_num;
				for (case_num =0;case_num<ELLIPSIS_CASE_NUM;case_num++)
				{
					char *case_ptr;
					if(case_ptr = strstr(tp,ELLIPSIS_CASE_LIST_VERB[case_num]))
					{
						int e_num;
						case_ptr+=strlen(ELLIPSIS_CASE_LIST_VERB[case_num]);
						sscanf(case_ptr,":%d",&e_num);
						gresult_entity_list[case_num][ gresult_entity_num[case_num] ] = e_num;
						gresult_entity_num[case_num]++;
					}
				}
				tp = strtok(NULL," ");
			}
		}
	}
	



	for (i=0;i<ELLIPSIS_CASE_NUM;i++)
	{
		if(aresult_entity_num[i] == 0 && gresult_entity_num[i] == 0)
		{
			check_case[i] =1;
		}
		else if (aresult_entity_num[i] == 0 || gresult_entity_num[i] == 0)
		{
			check_case[i] =-1;
		}
		else
		{
			check_case[i] =-1;
			for (j=0;j<aresult_entity_num[i];j++)
			{
				for (k=0;k<gresult_entity_num[i];k++)
				{
					if(aresult_entity_list[i][j] == gresult_entity_list[i][k])
					{
						check_case[i] =1;
					}
				}
			}
		}
	}

	for (i=0;i<ELLIPSIS_CASE_NUM;i++)
	{
		if(check_case[i] == -1)
		{
			return FALSE;
		}
	}
	return TRUE;
}

void author_detect()
{
	//著者・読者の自動推定(かなりヒューリスティック)
	int i,j,k;
	int temp_author_sen=100;
	int temp_author_tag=100;
	int temp_reader_sen=100;
	int temp_reader_tag=100;

	ENTITY *entity_ptr;
	ENTITY *author_entity=NULL;
	ENTITY *reader_entity=NULL;
	char *author_rep[AUTHOR_REP_NUM] = {"私/わたし","僕/ぼく","我々/われわれ","俺/おれ","当社/とうしゃ","弊社/へいしゃ","当店/とうてん"};
	char *reader_rep[READER_REP_NUM] = {"あなた/あなた","君/きみ","皆さん/みなさん","客/きゃく","皆様/みなさま"};
	if(((entity_manager.entity[0].real_entity == -1 ) && !(OptAnaphora & OPT_NO_AUTHOR_ENTITY)) || (entity_manager.entity[1].real_entity == -1 || !(OptAnaphora & OPT_NO_READER_ENTITY)))
	{
		for(i=0;i<entity_manager.num;i++)
		{
			entity_ptr = &(entity_manager.entity[i]);
			
			for(j=0;j<entity_ptr->mentioned_num;j++)
			{
				int author_rep_flag = 0;
				int reader_rep_flag = 0;
				if(entity_manager.entity[0].real_entity == -1)
				{
					for(k=0;k<AUTHOR_REP_NUM;k++)
					{
						char temp_rep[WORD_LEN_MAX*2+10];
						strcpy(temp_rep,"正規化代表表記:");
						strcat(temp_rep,author_rep[k]);
						if(check_feature(entity_ptr->mention[j]->tag_ptr->f,temp_rep))
						{
							author_rep_flag =1;
						}
					}
				}
				if(entity_manager.entity[1].real_entity == -1)
				{
					for(k=0;k<READER_REP_NUM;k++)
					{
						char temp_rep[WORD_LEN_MAX*2+10];
						strcpy(temp_rep,"正規化代表表記:");
						strcat(temp_rep,reader_rep[k]);
						if(check_feature(entity_ptr->mention[j]->tag_ptr->f,temp_rep))
						{
							reader_rep_flag =1;
						}
					}
				}
				
				
				if(temp_author_sen >= entity_ptr->mention[j]->sent_num &&temp_author_tag>= entity_ptr->mention[j]->tag_num )
				{
					
					/* if(check_feature(entity_ptr->mention[j]->tag_ptr->f,"NE:PERSON") ||check_feature(entity_ptr->mention[j]->tag_ptr->f,"NE:ORGANIZATION") || author_rep_flag == 1) */
					if(author_rep_flag == 1)
					{
					author_entity = entity_ptr;
					temp_author_sen= entity_ptr->mention[j]->sent_num;
					temp_author_tag = entity_ptr->mention[j]->tag_num ;
					}
					
				}
				if(temp_reader_sen >= entity_ptr->mention[j]->sent_num &&temp_reader_tag>= entity_ptr->mention[j]->tag_num )
				{
					if(reader_rep_flag ==1)
					{
						reader_entity = entity_ptr;
						temp_reader_sen= entity_ptr->mention[j]->sent_num;
						temp_reader_tag = entity_ptr->mention[j]->tag_num ;
						
					}
				}
			}
		}
		
		if(!(OptAnaphora & OPT_NO_AUTHOR_ENTITY) &&author_entity)
		{
			author_entity->hypothetical_entity = 0;
			entity_manager.entity[0].real_entity = author_entity->num;
			entity_manager.entity[0].skip_flag =1;
			strcat(author_entity->name,"|一人称");
			
		}
		if(!(OptAnaphora & OPT_NO_READER_ENTITY)&&reader_entity)
		{
			
			reader_entity->hypothetical_entity = 1;
			entity_manager.entity[1].real_entity = reader_entity->num;
			entity_manager.entity[1].skip_flag =1;
			strcat(reader_entity->name,"|二人称");
		}
	}
	
}

void merge_two_entity(ENTITY *target_entity_ptr, ENTITY *source_entity_ptr)
{
	int i,j;
	for(j=0;j<source_entity_ptr->mentioned_num;j++)
	{
		target_entity_ptr->mention[target_entity_ptr->mentioned_num  +j   ]= source_entity_ptr->mention[j];
		if(target_entity_ptr->mention[target_entity_ptr->mentioned_num  +j   ]->type == 'E')
		{
			target_entity_ptr->mention[target_entity_ptr->mentioned_num  +j   ]->type = 'O';
		}
		if(target_entity_ptr->mention[target_entity_ptr->mentioned_num  +j   ]->type == 'S' || target_entity_ptr->mention[target_entity_ptr->mentioned_num  +j   ]->type == '=')
		{
			target_entity_ptr->mention[target_entity_ptr->mentioned_num  +j   ]->tag_ptr->mention_mgr.mention->entity = target_entity_ptr;
		}
		
	}
	target_entity_ptr->mentioned_num += source_entity_ptr->mentioned_num;
	source_entity_ptr->mentioned_num = 0;
	source_entity_ptr->skip_flag=1;

	
}

void merge_hypo_real_entity_auto()
{

	ENTITY *real_entity_ptr;
	MENTION *mention_ptr;
	int entity_idx;
	int mention_idx;

	for(entity_idx=0;entity_idx<entity_manager.num;entity_idx++)
	{
		int hypo_num = -1;
		real_entity_ptr = &(entity_manager.entity[entity_idx]);
		for (mention_idx=0;mention_idx < real_entity_ptr->mentioned_num;mention_idx++)
		{
			mention_ptr = entity_manager.entity[entity_idx].mention[mention_idx];
			if(mention_ptr->type != '=' &&mention_ptr->type != 'S')
			{
				continue;
			}
			if(mention_ptr->sent_num == author_sen &&mention_ptr->tag_num == author_tag)
			{
				hypo_num =0;
			}
			else if(mention_ptr->sent_num == reader_sen &&mention_ptr->tag_num == reader_tag)
			{
				hypo_num =1;
			}
			if(hypo_num != -1)
			{
				entity_manager.entity[hypo_num].real_entity = entity_idx;
				real_entity_ptr->hypothetical_entity = hypo_num;
				merge_two_entity(real_entity_ptr,&(entity_manager.entity[hypo_num]));
				strcat(real_entity_ptr->name,"|");
				strcat(real_entity_ptr->name,entity_manager.entity[hypo_num].name);
				break;
			}
		}
	}
}

void merge_hypo_real_entity()
{
	//後方照応を修正する
	int i,j,k;
	ENTITY *hypo_entity_ptr;
	ENTITY *real_entity_ptr;
	
	for(i=0;i<entity_manager.num;i++)
	{
		if(entity_manager.entity[i].hypothetical_flag !=1)
		{
			continue;
		}
		hypo_entity_ptr = &(entity_manager.entity[i]);
		if(i < UNNAMED_ENTITY_NUM)
		{
		
			if(hypo_entity_ptr->real_entity != -1)
			{
				real_entity_ptr = &(entity_manager.entity[hypo_entity_ptr->real_entity]);
				merge_two_entity(real_entity_ptr,hypo_entity_ptr);
			}
		}
		else
		{
			for(j=0;j<entity_manager.num;j++)
			{
				if(!strcmp(hypo_entity_ptr->name,entity_manager.entity[j].hypothetical_name))
				{
					real_entity_ptr = &(entity_manager.entity[j]);
					merge_two_entity(real_entity_ptr,hypo_entity_ptr);
				}
			}
		}		
	}
}

int link_hypothetical_entity(char *token,ENTITY *entity_ptr)
{
	char type, rel[SMALL_DATA_LEN],entity_name[REPNAME_LEN_MAX];
	char *temp;
	int i, j, tag_num, sent_num;
	ENTITY *hypo_entity_ptr;
	for (token = strchr(token + strlen("格解析結果:"), ':') + 1; *token; token++) {
		if (*token == ':' || *token == ';') {
			token++;
			if (!sscanf(token, "%[^/]/%c/%[^/]/%d/%d/", rel, &type,entity_name, &tag_num, &sent_num))
			{
				continue;
			}
			if((!strcmp(rel, "=") || !strcmp(rel, "=構") || !strcmp(rel, "=役")) &&tag_num ==-1)
			{
				for(i=0;i<UNNAMED_ENTITY_NUM;i++)
				{
					for(j=0;j<UNNAMED_ENTITY_NAME_NUM;j++)
					{
						if(!strcmp(unnamed_entity_name[i][j],""))
						{
							break;
						}
						if(strcmp(unnamed_entity_name[i][j],"") && !strcmp(unnamed_entity_name[i][j],entity_name))
						{
							entity_ptr->hypothetical_entity = i;
							strcpy(entity_ptr->hypothetical_name,entity_name);
							entity_manager.entity[i].real_entity = entity_ptr->num;
							strcat(entity_ptr->name,"|");
							strcat(entity_ptr->name,entity_name);
							return TRUE;
						}
					}
				}
				for(i=0;i<entity_manager.num;i++)
				{
					if(!strcmp(entity_manager.entity[i].hypothetical_name ,entity_name))
					{
						merge_two_entity(&(entity_manager.entity[i]),entity_ptr);
						return TRUE;
					}
				}
				
				hypo_entity_ptr = make_each_unnamed_entity(entity_name);
				entity_ptr->hypothetical_entity = hypo_entity_ptr->num;
				strcpy(entity_ptr->hypothetical_name,entity_name);
				hypo_entity_ptr->real_entity = entity_ptr->num;
				
				return TRUE;
			}
		}
	}
	return FALSE;
}

void make_tag_pa_string(char *tag_pa_string,const char *ga_elem,const  char *wo_elem,const char *ni_elem,const char *pred)
{
	strcpy(tag_pa_string,ga_elem);
	strcat(tag_pa_string,":ガ;");
	strcat(tag_pa_string,wo_elem);
	strcat(tag_pa_string,":ヲ;");
	strcat(tag_pa_string,ni_elem);
	strcat(tag_pa_string,":ニ;");
	strcat(tag_pa_string,pred);
}

int bnst_to_psude_pp(BNST_DATA *child)
{
	if(check_feature(child->f,"係:ガ格"))
	{
		return  1;
	}
	else if(check_feature(child->f,"係:ヲ格"))
	{
		return 2;
	}
	else if(check_feature(child->f,"係:ニ格"))
	{
		return 3;
	}
	else
	{
		return 0;
	}
}

double get_swappable_lift_value(char *analysing_tag_pa_string,char *target_tag_pa_string,char *temp_match_string,int *sequence)
{
	double score1,score2;

	score1 = get_lift_value(analysing_tag_pa_string,target_tag_pa_string,temp_match_string);
	score2 = get_lift_value(target_tag_pa_string,analysing_tag_pa_string,temp_match_string);
	if(score1>score2)
	{
		*sequence = -1;
		return score1;
	}
	else
	{
		*sequence = 1;
		return score2;
	}
}

double get_lift_value(char *analysing_tag_pa_string,char *target_tag_pa_string,char *temp_match_string)
{
	char pa_pair_string[((WORD_LEN_MAX+4)*3)*2+3];
	char *value;
	double lift_value;
	strcpy(pa_pair_string,analysing_tag_pa_string);
	strcat(pa_pair_string,"=>");
	strcat(pa_pair_string,target_tag_pa_string);
	//printf("debug %s\n",pa_pair_string);
	value = db_get(event_db,pa_pair_string);
	if(value != NULL)
	{
		sscanf(value,"%lf",&lift_value);
		strcpy(temp_match_string,pa_pair_string);
		return  lift_value;
	}
	return 0;
}

int check_pred_pair(char *pred1,char *pred2)
{
	char pred_pair_string[(WORD_LEN_MAX+4)*2+3];
	char *value;
	int flag;
	strcpy(pred_pair_string,pred1);
	strcat(pred_pair_string,"=>");
	strcat(pred_pair_string,pred2);
	value = db_get(event_db,pred_pair_string);
	
	if(value != NULL)
	{
	
		sscanf(value,"%d",&flag);
		if(flag ==1)
		{
			return  1;
		}
	}
	return 0;
}
int check_swappable_pred_pair(char *pred1,char *pred2)
{
	int flag = -1;
	flag = check_pred_pair(pred1,pred2);
	if(flag ==1)
	{
		return 1;
	}
	flag = check_pred_pair(pred2,pred1);
	if(flag ==1)
	{
		return 1;
	}
	return 0;
}

double check_relational_event(TAG_DATA *analysing_tag,CF_TAG_MGR *ctm_ptr,int focus_pp,MENTION *target_mention_ptr,int sent_num,int tag_num,char* loc_name)
{
	char analysing_cf_entry[REPNAME_LEN_MAX] ="";
	char target_cf_entry[REPNAME_LEN_MAX] ="";
	int i,j;
	ENTITY *entity_ptr;
	MENTION_MGR *mention_mgr_ptr;
	int analysing_tag_pa_num=0;
	char analysing_tag_arg_string[ELLIPSIS_CASE_NUM][MENTIONED_MAX+2][(REPNAME_LEN_MAX+4)];
	int analysing_tag_arg_num[ELLIPSIS_CASE_NUM] = {0,0,0,0};
	int target_tag_pa_num=0;
	char target_tag_arg_string[ELLIPSIS_CASE_NUM][MENTIONED_MAX+2][(REPNAME_LEN_MAX+4)];
	int target_tag_arg_num[ELLIPSIS_CASE_NUM] = {0,0,0,0};
	int target_pp = pp_kstr_to_code(target_mention_ptr->cpp_string);
	int pred_tag_num = -1;


	int analysing_ga_mention_idx,analysing_wo_mention_idx,analysing_ni_mention_idx;
	int target_ga_mention_idx,target_wo_mention_idx,target_ni_mention_idx;
	
	int backword_flag =0;
	double score = 0;
	char match_string[((WORD_LEN_MAX+4)*3)*2+3];
	char temp_match_string[((WORD_LEN_MAX+4)*3)*2+3];
	int sequence = 0;
	char *cp;
	char key[SMALL_DATA_LEN];
	char key2[SMALL_DATA_LEN];

	int un_lex_match = 0;


	for (i=0;i<3;i++)
	{
		if(i ==focus_pp-1)
		{
			if(un_lex_match  ==1)
			{
				strcpy(analysing_tag_arg_string[i][0],"X");
				analysing_tag_arg_num[i]++;
			}
		}
		else
		{
			strcpy(analysing_tag_arg_string[i][0], "*");
			analysing_tag_arg_num[i]++;
			if(un_lex_match  ==1)
			{
			
				/* /\*Yが違っていてもマッチ*\/ */
				strcpy(analysing_tag_arg_string[i][1], "Y");
				analysing_tag_arg_num[i]++;
			}
			

		}
	}
	
	for (i = 0; i < ctm_ptr->result_num; i++)
	{
		int pp,e_num;
		e_num = ctm_ptr->cf_element_num[i];
		entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i];
		pp = ctm_ptr->cf_ptr->pp[e_num][0];
		if(pp > 3 || pp < 1)
		{
			continue;
		}
		if(un_lex_match ==1 && pp == focus_pp)
		{
			continue;
		}
		for (j = 0; j < entity_ptr->mentioned_num; j++)
		{
			if (entity_ptr->mention[j]->type != 'S' && entity_ptr->mention[j]->type != '=')
			{
				continue;
			}

			cp = get_bnst_head_canonical_rep(entity_ptr->mention[j]->tag_ptr->b_ptr, OptCaseFlag & OPT_CASE_USE_CN_CF);
				
			if(cp != NULL)
			{
				strcpy(analysing_tag_arg_string[pp-1][ analysing_tag_arg_num[pp-1] ],cp);
				analysing_tag_arg_num[pp-1]++;
			}

			if(un_lex_match == 0)
			{
				cp = check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ");
				if(cp != NULL)
				{
					while (strchr(cp, ':') && (cp = strchr(cp, ':')) || (cp = strchr(cp, ';')))
					{
						sprintf(key, "CT:%s:", ++cp);
						if (strchr(key + 3, ';')) *strchr(key + 3, ';') = ':'; /* tag = CT:組織・団体;抽象物: */
						*strchr(key + 3, ':') = '\0';
						sprintf(key2,"<%s>",key);
						
						strcpy(analysing_tag_arg_string[pp-1][ analysing_tag_arg_num[pp-1] ],key2);
						analysing_tag_arg_num[pp-1]++;
						
					}
				}
				
				cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE");
				if(cp != NULL)
				{
					strcpy(key, cp);
					*strchr(key + 3, ':') = '\0'; /* key = NE:LOCATION */
					sprintf(key2,"<%s>",key);
					
					strcpy(analysing_tag_arg_string[pp-1][ analysing_tag_arg_num[pp-1] ],key2);
					analysing_tag_arg_num[pp-1]++;
				}
			}
		}
	}
	
	
	cp = check_feature(analysing_tag->b_ptr->f,"正規化代表表記");
	if(cp)
	{
		
		strcpy(target_cf_entry,cp+strlen("正規化代表表記:"));
	}
	if (analysing_tag->voice & VOICE_SHIEKI) {
		strcat(analysing_cf_entry, ":C");
	}
	else if (analysing_tag->voice & VOICE_UKEMI ||
			 analysing_tag->voice & VOICE_UNKNOWN) {
		strcat(analysing_cf_entry, ":P");
	}
	else if (analysing_tag->voice & VOICE_SHIEKI_UKEMI) {
		strcat(analysing_cf_entry, ":PC");
	}


	mention_mgr_ptr = &(target_mention_ptr->tag_ptr->mention_mgr);
	if( target_mention_ptr->tag_ptr->b_ptr->parent && analysis_flags[target_mention_ptr->sent_num][ target_mention_ptr->tag_ptr->parent->num ] ==0 )
	{
		BNST_DATA* pred_bnst_ptr;

		
		if(!(target_mention_ptr->tag_ptr->b_ptr->parent) || !check_feature(target_mention_ptr->tag_ptr->b_ptr->parent->f,"用言"))
		{
			return 0;
		}

		target_pp = bnst_to_psude_pp(target_mention_ptr->tag_ptr->b_ptr);
		if(target_pp < 0)
		{
			return 0;
		}

		
		pred_bnst_ptr = target_mention_ptr->tag_ptr->b_ptr->parent;
		{
			
			cp = check_feature(pred_bnst_ptr->f,"正規化代表表記");
			if(cp)
			{
				
				strcpy(target_cf_entry,cp+strlen("正規化代表表記:"));
			}

			
		}
		pred_tag_num = pred_bnst_ptr->tag_ptr->num;

		if (pred_bnst_ptr->voice & VOICE_SHIEKI) {
			strcat(target_cf_entry, ":C");
		}
		else if (pred_bnst_ptr->voice & VOICE_UKEMI ||
				 pred_bnst_ptr->voice & VOICE_UNKNOWN) {
			strcat(target_cf_entry, ":P");
		}
		else if (pred_bnst_ptr->voice & VOICE_SHIEKI_UKEMI) {
			strcat(target_cf_entry, ":PC");
		}

		if(target_cf_entry == NULL || !strcmp("",target_cf_entry))
		{
			return 0;
		}
		
		
		if(check_swappable_pred_pair(analysing_cf_entry,target_cf_entry) ==0)
		{
			return 0;
		}
		
		for (i=0;i<3;i++)
		{
			if(i ==target_pp-1)
			{
				if(un_lex_match == 1)
				{
					strcpy(target_tag_arg_string[i][0], "X");
					target_tag_arg_num[i]++;
				}
			}
			else
			{
				strcpy(target_tag_arg_string[i][0],  "*");
				target_tag_arg_num[i]++;
				if(un_lex_match == 1)
				{
					/* /\*Yが違っていてもマッチ*\/ */
					strcpy(target_tag_arg_string[i][1],  "Y");
					target_tag_arg_num[i]++;
				}
			}
		}
		
		backword_flag = 1;
		
		for(i=0;pred_bnst_ptr->child[i];i++)
		{
			int pp=0;
			if(check_feature(pred_bnst_ptr->child[i]->f,"係:ガ格"))
			{
				pp = 1;
			}
			else if(check_feature(pred_bnst_ptr->child[i]->f,"係:ヲ格"))
			{
				pp = 2;
			}
			else if(check_feature(pred_bnst_ptr->child[i]->f,"係:ニ格"))
			{
				pp = 3;
			}
			if(un_lex_match ==1 && target_pp == pp)
			{
				continue;
			}
			if(pp==0)
			{
				continue;
			}
			for(j=0;j< pred_bnst_ptr->child[i]->tag_num;j++)
			{
				if(pred_bnst_ptr->child[i]->tag_ptr[j].mention_mgr.mention->entity)
				{
					entity_ptr = pred_bnst_ptr->child[i]->tag_ptr[j].mention_mgr.mention->entity;
				}
			}
			
			for (j = 0; j < entity_ptr->mentioned_num; j++)
			{
				if (entity_ptr->mention[j]->type != 'S' && entity_ptr->mention[j]->type != '=')
				{
					continue;
				}
				cp = get_bnst_head_canonical_rep(entity_ptr->mention[j]->tag_ptr->b_ptr, OptCaseFlag & OPT_CASE_USE_CN_CF);
				if(cp != NULL)
				{
					strcpy(target_tag_arg_string[pp-1][ target_tag_arg_num[pp-1] ], cp);
					target_tag_arg_num[pp-1]++;
				}
				if(un_lex_match == 0)
				{
					cp = check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ");
					if(cp != NULL)
					{
						while (strchr(cp, ':') && (cp = strchr(cp, ':')) || (cp = strchr(cp, ';')))
						{
							sprintf(key, "CT:%s:", ++cp);
							if (strchr(key + 3, ';')) *strchr(key + 3, ';') = ':'; /* tag = CT:組織・団体;抽象物: */
							*strchr(key + 3, ':') = '\0';
							sprintf(key2,"<%s>",key);
							strcpy(target_tag_arg_string[pp-1][ target_tag_arg_num[pp-1] ], key2);
							target_tag_arg_num[pp-1]++;
							
							
						}
					}
					cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE");
					if(cp != NULL)
					{
						strcpy(key, cp);
						*strchr(key + 3, ':') = '\0'; /* key = NE:LOCATION */
						sprintf(key2,"<%s>",key);
						
						strcpy(target_tag_arg_string[pp-1][ target_tag_arg_num[pp-1] ], key2);
						target_tag_arg_num[pp-1]++;
					}
				}
			}
		}
		
		
	}
	else if(!target_mention_ptr->tag_ptr->b_ptr)
	{

		return 0;
	}
	else
	{
		char *cp;
		
		if(target_pp <0)
		{
			return (0);
		}
		if(target_mention_ptr->type == 'S' || target_mention_ptr->type == '=' )
		{
			return 0;
		}
		
		{
			cp = check_feature(target_mention_ptr->tag_ptr->b_ptr->f,"正規化代表表記");
			if(cp)
			{
				
				strcpy(target_cf_entry,cp+strlen("正規化代表表記:"));

			}
		}


		if(target_cf_entry == NULL || !strcmp("",target_cf_entry))
		{
			return 0;
		}
		
		if (target_mention_ptr->tag_ptr->voice & VOICE_SHIEKI) {
			strcat(target_cf_entry, ":C");
		}
		else if (target_mention_ptr->tag_ptr->voice & VOICE_UKEMI ||
				 target_mention_ptr->tag_ptr->voice & VOICE_UNKNOWN) {
			strcat(target_cf_entry, ":P");
		}
		else if (target_mention_ptr->tag_ptr->voice & VOICE_SHIEKI_UKEMI) {
			strcat(target_cf_entry, ":PC");
		}

		
		if(check_swappable_pred_pair(analysing_cf_entry,target_cf_entry) ==0)
		{
			return 0;
		}

		for (i=0;i<3;i++)
		{
			if(i ==target_pp-1)
			{
				if(un_lex_match == 1)
				{
					strcpy(target_tag_arg_string[i][0], "X");
					target_tag_arg_num[i]++;
				}
			}
			else
			{
				strcpy(target_tag_arg_string[i][0], "*");
				target_tag_arg_num[i]++;
				/* /\*Yが違っていてもマッチ*\/ */
				if(un_lex_match ==1)
				{
					strcpy(target_tag_arg_string[i][1],  "Y");
					target_tag_arg_num[i]++;
				}


			}
		}
		for(i=1;i<mention_mgr_ptr->num;i++)
		{
			int pp;
			pp = pp_kstr_to_code(mention_mgr_ptr->mention[i].cpp_string);
			if(pp >3 || pp < 1)
			{
				continue;
			}
			if(un_lex_match == 1 && target_pp == pp)
			{
				continue;
			}
			entity_ptr = mention_mgr_ptr->mention[i].entity;
			for (j = 0; j < entity_ptr->mentioned_num; j++)
			{
				if (entity_ptr->mention[j]->type != 'S' && entity_ptr->mention[j]->type != '=')
				{
					continue;
				}
				
				cp = get_bnst_head_canonical_rep(entity_ptr->mention[j]->tag_ptr->b_ptr, OptCaseFlag & OPT_CASE_USE_CN_CF);
				if(cp != NULL)
				{
					strcpy(target_tag_arg_string[pp-1][ target_tag_arg_num[pp-1] ],cp);
					target_tag_arg_num[pp-1]++;
				}
				
				if(un_lex_match == 0)
				{
					cp = check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ");
					if(cp != NULL)
					{
						while (strchr(cp, ':') && (cp = strchr(cp, ':')) || (cp = strchr(cp, ';')))
						{
							sprintf(key, "CT:%s:", ++cp);
							if (strchr(key + 3, ';')) *strchr(key + 3, ';') = ':'; /* tag = CT:組織・団体;抽象物: */
							*strchr(key + 3, ':') = '\0';
							sprintf(key2,"<%s>",key);
							strcpy(target_tag_arg_string[pp-1][ target_tag_arg_num[pp-1] ],key2);
							target_tag_arg_num[pp-1]++;

					
						}
					}

					cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE");
					if(cp != NULL)
					{
						strcpy(key, cp);
						*strchr(key + 3, ':') = '\0'; /* key = NE:LOCATION */
						sprintf(key2,"<%s>",key);

						strcpy(target_tag_arg_string[pp-1][ target_tag_arg_num[pp-1] ], key2);
						target_tag_arg_num[pp-1]++;
					}
				}

			}
			
		}
		
	}
	

	for (analysing_ga_mention_idx =0;analysing_ga_mention_idx< analysing_tag_arg_num[0];analysing_ga_mention_idx++)
	{
		for (analysing_wo_mention_idx =0;analysing_wo_mention_idx< analysing_tag_arg_num[1];analysing_wo_mention_idx++)
		{
			for (analysing_ni_mention_idx =0;analysing_ni_mention_idx< analysing_tag_arg_num[2];analysing_ni_mention_idx++)
			{
				for (target_ga_mention_idx =0;target_ga_mention_idx< target_tag_arg_num[0];target_ga_mention_idx++)
				{
					for (target_wo_mention_idx =0;target_wo_mention_idx< target_tag_arg_num[1];target_wo_mention_idx++)
					{
						for (target_ni_mention_idx =0;target_ni_mention_idx< target_tag_arg_num[2];target_ni_mention_idx++)
						{
							char pa_pair_string[((REPNAME_LEN_MAX+4)*3)*2+3];
							double lift_value;
							char analysing_tag_pa_string[((REPNAME_LEN_MAX+4)*3)];
							char target_tag_pa_string[((REPNAME_LEN_MAX+4)*3)];
							int temp_sequence = 0;
							
							make_tag_pa_string(analysing_tag_pa_string,
											   analysing_tag_arg_string[0][analysing_ga_mention_idx],
											   analysing_tag_arg_string[1][analysing_wo_mention_idx],
											   analysing_tag_arg_string[2][analysing_ni_mention_idx],
											   analysing_cf_entry);
							make_tag_pa_string(target_tag_pa_string,
											   target_tag_arg_string[0][target_ga_mention_idx],
											   target_tag_arg_string[1][target_wo_mention_idx],
											   target_tag_arg_string[2][target_ni_mention_idx],
											   target_cf_entry);

							lift_value = get_swappable_lift_value(analysing_tag_pa_string,target_tag_pa_string,temp_match_string,&temp_sequence);
							if(lift_value > score)
							{
								strcpy(match_string,temp_match_string);
								sequence = temp_sequence;
								score = lift_value;
							}
							
							
							/*********lexicalize後はYとしなくていい*********/
							if(un_lex_match == 1)
							{
								if(strcmp(analysing_tag_arg_string[0][analysing_ga_mention_idx],"*") && strcmp(analysing_tag_arg_string[0][analysing_ga_mention_idx],"X") && !strcmp(analysing_tag_arg_string[0][analysing_ga_mention_idx],target_tag_arg_string[0][target_ga_mention_idx]))
								{
									make_tag_pa_string(analysing_tag_pa_string,
													   "Y",
													   analysing_tag_arg_string[1][analysing_wo_mention_idx],
													   analysing_tag_arg_string[2][analysing_ni_mention_idx],
													   analysing_cf_entry);
								
									make_tag_pa_string(target_tag_pa_string,
													   "Y",
													   target_tag_arg_string[1][target_wo_mention_idx],
													   target_tag_arg_string[2][target_ni_mention_idx],
													   target_cf_entry);
									lift_value = get_swappable_lift_value(analysing_tag_pa_string,target_tag_pa_string,temp_match_string,&temp_sequence);

									if(lift_value > score)
									{
										strcpy(match_string,temp_match_string);
										score = lift_value;
										sequence = temp_sequence;
									}
								
								}

								if(strcmp(analysing_tag_arg_string[1][analysing_wo_mention_idx],"*")
								   && strcmp(analysing_tag_arg_string[1][analysing_wo_mention_idx],"X")
								   && !strcmp(analysing_tag_arg_string[1][analysing_wo_mention_idx],target_tag_arg_string[1][target_wo_mention_idx]))
								{
									make_tag_pa_string(analysing_tag_pa_string,
													   analysing_tag_arg_string[0][analysing_ga_mention_idx],
													   "Y",
													   analysing_tag_arg_string[2][analysing_ni_mention_idx],
													   analysing_cf_entry);
								
									make_tag_pa_string(target_tag_pa_string,
													   target_tag_arg_string[0][target_ga_mention_idx],
													   "Y",
													   target_tag_arg_string[2][target_ni_mention_idx],
													   target_cf_entry);
									lift_value = get_swappable_lift_value(analysing_tag_pa_string,target_tag_pa_string,temp_match_string,&temp_sequence);
									if(lift_value > score)
									{
										strcpy(match_string,temp_match_string);
										score = lift_value;
										sequence = temp_sequence;
									}
								
								}

								if(strcmp(analysing_tag_arg_string[2][analysing_ni_mention_idx],"*")
								   && strcmp(analysing_tag_arg_string[2][analysing_ni_mention_idx],"X")
								   && !strcmp(analysing_tag_arg_string[2][analysing_ni_mention_idx],target_tag_arg_string[2][target_ni_mention_idx]))
								{
									make_tag_pa_string(analysing_tag_pa_string,
													   analysing_tag_arg_string[0][analysing_ga_mention_idx],
													   analysing_tag_arg_string[1][analysing_wo_mention_idx],
													   "Y",
													   analysing_cf_entry);
								
									make_tag_pa_string(target_tag_pa_string,
													   target_tag_arg_string[0][target_ga_mention_idx],
													   target_tag_arg_string[1][target_wo_mention_idx],
													   "Y",
													   target_cf_entry);
									lift_value = get_swappable_lift_value(analysing_tag_pa_string,target_tag_pa_string,temp_match_string,&temp_sequence);
									if(lift_value > score)
									{

										strcpy(match_string,temp_match_string);
										score = lift_value;
										sequence = temp_sequence;
									}
								
								

								}
							}
							
						}
					}
				}
			}
		}
	}
	
	if(score>0)
	{
		int seq;

		if(pred_tag_num ==-1)
		{
			pred_tag_num = target_mention_ptr->tag_ptr->num;
		}

		if(sent_num > target_mention_ptr->sent_num)
		{
			seq = sequence;
		}
		else if(sent_num < target_mention_ptr->sent_num)
		{
			seq = sequence*(-1);
		}
		else if(tag_num >  pred_tag_num)
		{
			seq = sequence;
		}
		else
		{
			seq = sequence*(-1);
		}
		printf("lift%d %s %s %f sen1:%d tag1:%d sen2:%d tag2:%d\n",seq,loc_name+7,match_string,score,sent_num,tag_num,target_mention_ptr->sent_num,pred_tag_num);
	}
	return score;
	
}

void set_bnst_cueue(int *analysis_bnst_cueue,SENTENCE_DATA *sp)
{
	int i,j;
	int bnst_num,tag_num;
	int check_bnst[BNST_MAX];
	int check_tag[TAG_MAX];
	int set_idx = 0;
	BNST_DATA *bnst_ptr;
	for (i=0;i<TAG_MAX;i++)
	{
		analysis_bnst_cueue[i] = -1;
	}
	for (bnst_num = sp->Bnst_num -1; bnst_num>=0;bnst_num--)
	{
		check_bnst[bnst_num] = 0;
	}

	//ハ格で係るものを優先して解析
	for (bnst_num = sp->Bnst_num -1; bnst_num>=0;bnst_num--)
	{
		int check_flag =0;
		bnst_ptr = sp->bnst_data+bnst_num; 
		for (i = 0; bnst_ptr->child[i]; i++) 
		{
			if (bnst_ptr->child[i]->para_top_p) 
			{ 
				for (j = 0; bnst_ptr->child[i]->child[j]; j++) 
				{
					/* todo::とりあえず並列の並列は無視 */		
					if (!bnst_ptr->child[i]->child[j]->para_top_p)
					{
						if(check_feature(bnst_ptr->child[i]->child[j]->f,"ハ"))
						{
							check_flag = 1;
						}
					}
				}
			}
			else 
			{
				if(check_feature(bnst_ptr->child[i]->f,"ハ"))
				{
					check_flag = 1;
				}
			}
		}
		if(check_flag == 1)
		{
			check_bnst[bnst_ptr->num] = 1;
			analysis_bnst_cueue[set_idx]=bnst_ptr->num;
			set_idx++;
		}
	}

	for (bnst_num = sp->Bnst_num -1; bnst_num>=0;bnst_num--)
	{
		bnst_ptr = sp->bnst_data+bnst_num; 
		if(check_bnst[bnst_ptr->num]==1)
		{
			continue; 
		}
		check_bnst[bnst_ptr->num] = 1;
		analysis_bnst_cueue[set_idx]=bnst_ptr->num;
		set_idx++;
			

		
	}	
}





void make_cf_entry_type(char* cf_entry_type,CASE_FRAME* cf_ptr)
{
	char cf_type;
	strcpy(cf_entry_type,cf_ptr->entry);
	cf_type = *(cf_ptr->cf_id+strlen(cf_ptr->entry)+1+strlen(cf_ptr->pred_type)+1);
	if(cf_type == 'C' || cf_type == 'P')
	{
		strncat(cf_entry_type,":",1);
		strncat(cf_entry_type,&cf_type,1);
	}
	
}

void merge_ellipsis_result_and_correct(void)
{

	int i,j,k;
	CF_TAG_MGR *temp_result_ctm;
	char correct_cf_aresult_strings[ELLIPSIS_CORRECT_MAX][REPNAME_LEN_MAX];
	int correct_flag = 1;
	int initital_ctm_flag =0;
	temp_result_ctm =(CF_TAG_MGR *)malloc_data(sizeof(CF_TAG_MGR)*ELLIPSIS_RESULT_MAX, "reordering_ellipsis_result");
	for (i=0;i<ELLIPSIS_RESULT_MAX;i++)
	{
		copy_ctm(&ellipsis_result_ctm[i],&temp_result_ctm[i]);
	}
	for (i=0;i < ELLIPSIS_CORRECT_MAX;i++)
	{
		strcpy(correct_cf_aresult_strings[i],"");
	}

	j = 0;
	for (i=0;i<ELLIPSIS_RESULT_MAX;i++)
	{
		char temp_cf_aresult_string[REPNAME_LEN_MAX];
		char aresult[REPNAME_LEN_MAX];
		
		if(correct_flag == 1)
		{

			copy_ctm(&ellipsis_correct_ctm[j],&ellipsis_result_ctm[i]);
			make_aresult_string((&ellipsis_result_ctm[i]),aresult);
			strcpy(temp_cf_aresult_string,ellipsis_result_ctm[i].cf_ptr->cf_id);
			strcat(temp_cf_aresult_string," ");
			strcat(temp_cf_aresult_string,aresult);

			strcpy(correct_cf_aresult_strings[j],temp_cf_aresult_string);
			j++;
			if(j >= ELLIPSIS_CORRECT_MAX || ellipsis_correct_ctm[j].score ==INITIAL_SCORE  )
			{
				correct_flag =0;
				j = 0;
			}
			
		}
		else if(correct_flag ==0)
		{
			int copy_flag=1;
			if(temp_result_ctm[j].score != INITIAL_SCORE)
			{
				make_aresult_string((&temp_result_ctm[j]),aresult);
				strcpy(temp_cf_aresult_string,temp_result_ctm[j].cf_ptr->cf_id);
				strcat(temp_cf_aresult_string," ");
				strcat(temp_cf_aresult_string,aresult);

				for(k=0;k<ELLIPSIS_CORRECT_MAX;k++)
				{
					if(!strcmp(temp_cf_aresult_string,correct_cf_aresult_strings[k]))
					{
						copy_flag =0;
					}
				}
			}
			if(copy_flag ==1)
			{
				copy_ctm(&temp_result_ctm[j],&ellipsis_result_ctm[i]);
				if(j == ELLIPSIS_RESULT_MAX -1)
				{
					break;
				}
			}
			else
			{
				i--;
			}
			
			j++;
		}
	}	
}

/*==================================================================*/
void make_aresult_string(CF_TAG_MGR *ctm_ptr,char *aresult)
/*==================================================================*/
{
	int pp_code,i,j,k;
	int entity_num[ENTITY_MAX];
	int mention_num;
	char cp[REPNAME_LEN_MAX];
	
	aresult[0] = '\0';
	for (pp_code = 0; pp_code<ELLIPSIS_CASE_NUM; pp_code++)
	{
		for (i =0;i<ENTITY_MAX;i++)
		{
			entity_num[i]= ENTITY_MAX;
		}
		mention_num = 0;
		for (j = 0; j < ctm_ptr->result_num; j++) {
			if(!strcmp(ELLIPSIS_CASE_LIST_VERB[pp_code],pp_code_to_kstr(ctm_ptr->cf_ptr->pp[ctm_ptr->cf_element_num[j]][0])))
			{
				if(mention_num==0)
				{
					entity_num[0] = (entity_manager.entity + ctm_ptr->entity_num[j])->num;

					mention_num++;
				}
				else
				{
					for (i=0;i<mention_num;i++)
					{
						if((entity_manager.entity + ctm_ptr->entity_num[j])->num <entity_num[i])
						{
							for(k=mention_num;k>i;k--)
							{
								entity_num[k] = entity_num[k-1];
							}

							entity_num[i] = (entity_manager.entity + ctm_ptr->entity_num[j])->num;

							mention_num++;
							break;
						}
					}
				}
			}
		}
		for (i=0;i<mention_num;i++)
		{
			if(entity_num[i] == ENTITY_MAX)
			{
				break;
			}
			if (!(OptAnaphora & OPT_UNNAMED_ENTITY)
				 && ((OptAnaphora & OPT_TRAIN) || (OptAnaphora & OPT_GS))){

				if(entity_num[i] < UNNAMED_ENTITY_NUM)
				{
					continue;
				}
			}
			sprintf(cp, " %s:%d",
					ELLIPSIS_CASE_LIST_VERB[pp_code],
					entity_num[i]
				);
			strcat(aresult, cp);
		}


	}
}



/*==================================================================*/
void make_case_assingment_string(CF_TAG_MGR *ctm_ptr,char *aresult)
/*==================================================================*/
{
	int pp_code,i,j,k;
	int entity_num[ENTITY_MAX];
	int mention_num;
	char cp[REPNAME_LEN_MAX];
	
	aresult[0] = '\0';
	for (pp_code = 0; pp_code< PP_NUMBER; pp_code++)
	{
		for (i =0;i<ENTITY_MAX;i++)
		{
			entity_num[i]= ENTITY_MAX;
		}
		mention_num = 0;
		for (j = 0; j < ctm_ptr->result_num; j++) {
			if(pp_code == ctm_ptr->cf_ptr->pp[ctm_ptr->cf_element_num[j]][0])
			{
				if(mention_num==0)
				{
					entity_num[0] = (entity_manager.entity + ctm_ptr->entity_num[j])->num;

					mention_num++;
				}
				else
				{
					for (i=0;i<mention_num;i++)
					{
						if((entity_manager.entity + ctm_ptr->entity_num[j])->num <entity_num[i])
						{
							for(k=mention_num;k>i;k--)
							{
								entity_num[k] = entity_num[k-1];
							}

							entity_num[i] = (entity_manager.entity + ctm_ptr->entity_num[j])->num;

							mention_num++;
							break;
						}
					}
				}
			}
		}
		for (i=0;i<mention_num;i++)
		{
			if(entity_num[i] == ENTITY_MAX)
			{
				break;
			}
			if (!(OptAnaphora & OPT_UNNAMED_ENTITY)
				 && ((OptAnaphora & OPT_TRAIN) || (OptAnaphora & OPT_GS))){

				if(entity_num[i] < UNNAMED_ENTITY_NUM)
				{
					continue;
				}
			}
			sprintf(cp, " %s:%d",
					pp_code_to_kstr(pp_code),
					entity_num[i]
				);
			strcat(aresult, cp);
		}


	}
}



void make_gresult_strings(TAG_DATA *tag_ptr,char *gresult)
{
	int pp_code,i,j,k;
	int entity_num[ENTITY_MAX];
	int mention_num;
	char cp[REPNAME_LEN_MAX];
	MENTION *mention_ptr;
	
	gresult[0] = '\0';
	for (pp_code = 0; pp_code<ELLIPSIS_CASE_NUM; pp_code++)
	{
		for (i =0;i<ENTITY_MAX;i++)
		{
			entity_num[i]= ENTITY_MAX;
		}
		mention_num = 0;
		
		for (j = 1; j < tag_ptr->mention_mgr.num; j++) {
			mention_ptr = tag_ptr->mention_mgr.mention + j;	    
			if (mention_ptr->type == 'O' || mention_ptr->type == 'E' || mention_ptr->type == 'C' ) {
				if(!strcmp(ELLIPSIS_CASE_LIST_VERB[pp_code],  mention_ptr->cpp_string))
				{
					if(mention_num==0)
					{
						entity_num[0] = mention_ptr->entity->num;
						mention_num++;
					}
					else
					{
						for (i=0;i<=mention_num;i++)
						{
							if(mention_ptr->entity->num < entity_num[i])
							{
								for(k=mention_num;k>i;k--)
								{
									entity_num[k] = entity_num[k-1];
								}
								
								entity_num[i] = mention_ptr->entity->num;

								mention_num++;
								break;
							}
						}
					}
				}
			}
		}
		for (i=0;i<mention_num;i++)
		{
			if(entity_num[i] == ENTITY_MAX)
			{
				break;
			}
			if (!(OptAnaphora & OPT_UNNAMED_ENTITY)
				 && ((OptAnaphora & OPT_TRAIN) || (OptAnaphora & OPT_GS))){
				
				if(entity_num[i] < UNNAMED_ENTITY_NUM)
				{
					continue;
				}
			}
			sprintf(cp, " %s:%d",
					ELLIPSIS_CASE_LIST_VERB[pp_code],
					entity_num[i]
				);
			strcat(gresult, cp);
		}

	}
}




/*==================================================================*/
void set_param(void)
/*==================================================================*/
{
	int i,j;
	def_overt_arguments_weight = 1;
	def_all_arguments_weight =1;
		
	if(OptAnaphora & OPT_TRAIN)
	{
		overt_arguments_weight = def_overt_arguments_weight;
		all_arguments_weight = def_all_arguments_weight;
	}
	else
	{
		overt_arguments_weight = learned_overt_arguments_weight;
		all_arguments_weight = learned_all_arguments_weight;
	}

	for (i=0;i<ELLIPSIS_CASE_NUM;i++)
	{
		for (j=0;j<O_FEATURE_NUM;j++)
		{

			if(j%EACH_FEARUTE_NUM==NO_ASSIGNMENT)
			{
				def_case_feature_weight[i][j] =1;
			}
			else if(j%EACH_FEARUTE_NUM<=CLS_PMI)
			{
				def_case_feature_weight[i][j] =0.5;
			}
			else if(j%EACH_FEARUTE_NUM == ASSIGNED)
			{
				if(i ==0)
				{
					def_case_feature_weight[i][j] =-1.8;
				}
				else
				{
					def_case_feature_weight[i][j] =-1.4;
				}
			}
			else
			{
				def_case_feature_weight[i][j] =0;
			}
			if(OptAnaphora & OPT_TRAIN)
			{
				case_feature_weight[i][j] = def_case_feature_weight[i][j];
			}
			else
			{
				case_feature_weight[i][j] = learned_case_feature_weight[i][j];
			}
		}
	}
	
}


/*省略解析の後で一人称とひっつける*/
void link_hypo_enity_after_analysis(SENTENCE_DATA *sp)
{
	int i;
	TAG_DATA *tag_ptr;
	MENTION_MGR *mention_mgr;
    char *cp;
	
    for (i = 0; i < sp->Tag_num; i++){ /* 解析文のタグ単位:i番目のタグについて */
		tag_ptr = substance_tag_ptr(sp->tag_data + i);
		
		mention_mgr = &(tag_ptr->mention_mgr);
		if (((OptAnaphora & OPT_UNNAMED_ENTITY) && ((OptReadFeature & OPT_AUTHOR) || (OptReadFeature & OPT_AUTHOR_AUTO) ))
			|| (OptAnaphora & OPT_TRAIN) || (OptAnaphora & OPT_GS)){
			
			if(cp = check_feature(tag_ptr->f, "格解析結果"))
			{
				link_hypothetical_entity(cp,mention_mgr->mention->entity);
			}
		}

	}	
}

/*==================================================================*/
void make_entity_from_coreference(SENTENCE_DATA *sp)
/*==================================================================*/
{
	int i;
	TAG_DATA *tag_ptr;
	MENTION_MGR *mention_mgr;
    char *cp;
	/* 省略以外のMENTIONの処理 */
    for (i = 0; i < sp->Tag_num; i++){ /* 解析文のタグ単位:i番目のタグについて */
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
		if (OptReadFeature & OPT_COREFER) {
			
			if (cp = check_feature(tag_ptr->f, "格解析結果")) {		
				for (cp = strchr(cp + strlen("格解析結果:"), ':') + 1; *cp; cp++) {
					if (*cp == ':' || *cp == ';') {
						if (read_one_annotation(sp, tag_ptr, cp + 1, TRUE))
						{
							assign_cfeature(&(tag_ptr->f), "共参照", FALSE);
						}
						if(!(OptAnaphora & OPT_TRAIN))
						{
							//search_hypo_entity(sp, tag_ptr, cp + 1);
						}
					}
				}
			}
		}
		else if (OptReadFeature & OPT_COREFER_AUTO) {
			if(cp = check_feature(tag_ptr->f,"COREFER_ID"))
			{
				link_entity_from_corefer_id(sp,tag_ptr,cp);
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
		
		/*一人称等が"="の関係の時に仮想entityと関連付け*/
		
		if (((OptAnaphora & OPT_UNNAMED_ENTITY) && (OptReadFeature & OPT_COREFER) &&  !(OptAnaphora & OPT_AUTHOR_AFTER) ) || (OptAnaphora & OPT_TRAIN) ||(OptAnaphora & OPT_GS) ) {
			if(cp = check_feature(tag_ptr->f, "格解析結果"))
			{
				link_hypothetical_entity(cp,mention_mgr->mention->entity);
			}
		}
    }

}
/*==================================================================*/
void clear_context(SENTENCE_DATA *sp, int init_flag)
/*==================================================================*/
{
	int i;

	if (OptAnaphora & OPT_PRINT_ENTITY) printf(";;\n;;CONTEXT INITIALIZED\n");
	for (i = 0; i < sp->Sen_num - 1; i++) ClearSentence(sentence_data + i);
	if (init_flag) {
		base_sentence_num = base_entity_num = 0;
		corefer_id = 0;
	}
	else {
		base_sentence_num += sp->Sen_num - 1;
		base_entity_num += entity_manager.num;
	}   
	sp->Sen_num = 1;
	entity_manager.num = 0;
}

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
int get_ellipsis_case_num(char *key, char **list)
/*==================================================================*/
{
	/* keyが省略対象格のいずれかとマッチするかどうかをチェック */
	int i;

	/* 引数がある場合はそのリストを、
	   ない場合はELLIPSIS_CASE_LISTをチェックする */
	if (!list) list = ELLIPSIS_CASE_LIST;

	for (i = 0; *list[i]; i++) {
		if (!strcmp(key, list[i])) return i;
	}
	return -1;
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
int get_location(char *loc_name, int sent_num, char *kstr, MENTION *mention, int old_flag)
/*==================================================================*/
{
	char cpp[PP_STRING_MAX];
	char  pos_name[SMALL_DATA_LEN];

	if (mention->sent_num == sent_num)
	{
		
		sprintf(pos_name,"C%d",mention->tag_ptr->b_ptr->parent?loc_category[mention->tag_ptr->b_ptr->parent->num]
				:loc_category[mention->tag_ptr->b_ptr->num]);
	}
	else if(mention->sent_num > sent_num)
	{
		sprintf(pos_name,"A%d",	( mention->sent_num -sent_num <= 3 ) ?
						mention->sent_num -sent_num : 0);
	}
	else
	{
		sprintf(pos_name,"B%d",	( sent_num - mention->sent_num <= 3 ) ?
				sent_num - mention->sent_num : 0);
	}

	if(	(mention->type == '=' || mention->type == 'S') &&
		mention->tag_ptr->parent &&
		analysis_flags[mention->sent_num][mention->tag_ptr->parent->num] ==0)
	{
		if(
			/* 格がガ格、ヲ格、ニ格、ノ格のいずれか */
			(check_feature(mention->tag_ptr->b_ptr->f, "係:ガ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ヲ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ニ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ノ格"))) {
			sprintf(cpp,"%s",
					check_feature(mention->tag_ptr->b_ptr->f, "係:ガ格") ? "ガ" :
					check_feature(mention->tag_ptr->b_ptr->f, "係:ヲ格") ? "ヲ" :
					check_feature(mention->tag_ptr->b_ptr->f, "係:ニ格") ? "ニ" : "ノ");
			sprintf(loc_name,"%s-C%s-%s",kstr,cpp,pos_name);
			return TRUE;
		}
	}
	else
	{
			sprintf(loc_name, "%s-%c%s-%s", kstr,
					(mention->type == '=') ? 'S' : 
					(mention->type == 'N') ? 'C' : mention->type, 
					old_flag ? "" : mention->cpp_string,
					pos_name);
			return TRUE;
	}


/*旧*/
	/* 同一文の場合は*/
	if (mention->sent_num == sent_num) {
		/* C[ガヲニノ]-C[247]はまだ解析していない箇所の解析結果が必要となるため
		   そのままでは出力されないのでここで強制的に生成する */
		if (!old_flag &&
			/* flagが'='または'S' */
			(mention->type == '=' || mention->type == 'S') &&
			/* 係り先が未解析 */
			mention->tag_ptr->b_ptr->parent &&
			analysis_flags[mention->tag_ptr->b_ptr->parent->num] == 0 &&
			/* 格がガ格、ヲ格、ニ格、ノ格のいずれか */
			(check_feature(mention->tag_ptr->b_ptr->f, "係:ガ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ヲ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ニ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ノ格"))) {
			sprintf(loc_name, "%s-C%s-C%d", kstr,
					check_feature(mention->tag_ptr->b_ptr->f, "係:ガ格") ? "ガ" :
					check_feature(mention->tag_ptr->b_ptr->f, "係:ヲ格") ? "ヲ" :
					check_feature(mention->tag_ptr->b_ptr->f, "係:ニ格") ? "ニ" : "ノ",
					loc_category[mention->tag_ptr->b_ptr->parent->num]);
			return TRUE;
		}
		else {
			sprintf(loc_name, "%s-%c%s-C%d", kstr,
					(mention->type == '=') ? 'S' : 
					(mention->type == 'N') ? 'C' : mention->type, 
					old_flag ? "" : mention->cpp_string,
					loc_category[mention->tag_ptr->b_ptr->num]);
			return TRUE;
		}
	}
	else if (sent_num - mention->sent_num == 1 &&
			 (check_feature(mention->tag_ptr->f, "文頭") ||
			  check_feature(mention->tag_ptr->f, "読点")) &&
			 check_feature(mention->tag_ptr->f, "ハ")) {
		sprintf(loc_name, "%s-%c%s-B1B", kstr, 
				(mention->type == '=') ? 'S' : 
				(mention->type == 'N') ? 'C' : mention->type, 
				old_flag ? "" : mention->cpp_string);
		return TRUE;
	}
	else if (sent_num - mention->sent_num == 1 &&
			 check_feature(mention->tag_ptr->f, "文末") &&
			 check_feature(mention->tag_ptr->f, "用言:判")) {
		sprintf(loc_name, "%s-%c%s-B1E", kstr,
				(mention->type == '=') ? 'S' : 
				(mention->type == 'N') ? 'C' : mention->type, 
				old_flag ? "" : mention->cpp_string);
		return TRUE;
	}
	else if (sent_num - mention->sent_num > 0) {
		sprintf(loc_name, "%s-%c%s-B%d", kstr, 
				(mention->type == '=') ? 'S' : 
				(mention->type == 'N') ? 'C' : mention->type, 
				old_flag ? "" : mention->cpp_string,
				(sent_num - mention->sent_num <= 3 ) ? 
				sent_num - mention->sent_num : 0);
		return TRUE;
	}
	else if( sent_num - mention->sent_num < 0)
	{

		if (!old_flag &&
			/* flagが'='または'S' */
			(mention->type == '=' || mention->type == 'S') &&
			/* 格がガ格、ヲ格、ニ格、ノ格のいずれか */
			(check_feature(mention->tag_ptr->b_ptr->f, "係:ガ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ヲ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ニ格") ||
			 check_feature(mention->tag_ptr->b_ptr->f, "係:ノ格"))) {
			sprintf(loc_name, "%s-C%s-A%d", kstr,
					check_feature(mention->tag_ptr->b_ptr->f, "係:ガ格") ? "ガ" :
					check_feature(mention->tag_ptr->b_ptr->f, "係:ヲ格") ? "ヲ" :
					check_feature(mention->tag_ptr->b_ptr->f, "係:ニ格") ? "ニ" : "ノ",
					( mention->sent_num -sent_num <= 3 ) ?
					mention->sent_num -sent_num : 0);
			return TRUE;
		}
		else {
			sprintf(loc_name, "%s-%c%s-A%d", kstr,
					(mention->type == '=') ? 'S' : 
					(mention->type == 'N') ? 'C' : mention->type, 
					old_flag ? "" : mention->cpp_string,
					( mention->sent_num -sent_num <= 3 ) ?
					mention->sent_num -sent_num : 0	);
			return TRUE;
		}
	}
	else {
		return FALSE;
	}
	return FALSE;
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

	/* 自分自身を越えて係る"は" */
	for (i = 0; i < bnst_ptr->num; i++) {
		if ((sp->bnst_data[i].parent)->num &&
			(sp->bnst_data[i].parent)->num > bnst_ptr->num &&
			check_feature(sp->bnst_data[i].f, "ハ")) loc_category[i] = LOC_OTHERS_THEME;
	}

	if (OptDisplay == OPT_DEBUG) {
		for (i = 0; i < sp->Bnst_num; i++)
			printf(";;LOC %d-%s target_bnst:%d-%d\n", bnst_ptr->num,
				   bnst_ptr->Jiritu_Go, i, loc_category[i]);
	}

	return;
}

/*==================================================================*/
int check_analyze_tag(TAG_DATA *tag_ptr, int demo_flag)
/*==================================================================*/
{
	/* 与えたられたtag_ptrが解析対象かどうかをチェック */
	/* demo_flagが与えられた場合は"その"に修飾されているかどうかを返す */

	/* 用言としての解析対象である場合:CF_PRED(=1)を返す */
	/* 名詞としての解析対象である場合:CF_NOUN(=2)を返す */
	/* それ以外の場合は0を返す */
	int i;
	BNST_DATA *bnst_ptr;

	/* 省略解析なし */
	if (check_feature(tag_ptr->f, "省略解析なし") ||
		check_feature(tag_ptr->f, "NE") ||
		check_feature(tag_ptr->f, "NE内") ||
		check_feature(tag_ptr->f, "同格") ||
		check_feature(tag_ptr->f, "共参照") ||
		check_feature(tag_ptr->f, "共参照内")) return 0;

	/* demo_flagがたっている場合は体言のみ対象 */
	if (demo_flag && 
		(!(OptEllipsis & OPT_REL_NOUN) || !check_feature(tag_ptr->f, "体言"))) return 0;

	/* 名詞として解析する場合 */
	if ((OptEllipsis & OPT_REL_NOUN) && check_feature(tag_ptr->f, "体言") &&
		!check_feature(tag_ptr->f, "用言一部") &&

		/* 用言の解析を行う場合はサ変は対象外 */
		!((OptEllipsis & OPT_ELLIPSIS) && check_feature(tag_ptr->f, "サ変"))) {

		/* 主辞以外は対象外 */
		if (check_feature(tag_ptr->f, "文節内")) return 0;

		/* 形副名詞は対象外 */
		if (check_feature(tag_ptr->f, "形副名詞")) return 0;

		bnst_ptr = (BNST_DATA *)substance_tag_ptr((TAG_DATA *)tag_ptr->b_ptr);

		/* 文末の連用修飾されている体言は除外 */
		if (check_feature(bnst_ptr->f, "文末") && !check_feature(bnst_ptr->f, "文頭") &&
			bnst_ptr->child[0] && check_feature(bnst_ptr->child[0]->f, "係:連用")) return 0;

		/* "その"に修飾されているかどうかを判定する場合以外 */
		/* 修飾されている句も対象(暫定的) */
		if (!demo_flag) return CF_NOUN; 

		/* "その"に修飾されているかどうかを判定する場合 */
		/* "その"以外に修飾されている場合 */
		if (bnst_ptr->child[0] && 
			strcmp(bnst_ptr->child[0]->head_ptr->Goi2, "その") &&
			(!bnst_ptr->child[1] || strcmp(bnst_ptr->child[1]->head_ptr->Goi2, "その"))) return 0;
		/* "その"に修飾されている場合 */
		if (demo_flag && bnst_ptr->child[0]) return CF_NOUN;

		if (/* 並列句の場合は並列の並列句に係る表現も確認 */
			bnst_ptr->para_type == PARA_NORMAL) {
			for (i = 0; bnst_ptr->parent->child[i]; i++) {
				if (bnst_ptr->parent->child[i]->para_type == PARA_NIL &&
					strcmp(bnst_ptr->parent->child[i]->head_ptr->Goi2, "その")) return 0;
				if (demo_flag && 
					!strcmp(bnst_ptr->parent->child[i]->head_ptr->Goi2, "その")) return 1;
			}
		}

		if (demo_flag) return 0;
		return CF_NOUN;
	}

	/* 用言として解析する場合 */
	if ((OptEllipsis & OPT_ELLIPSIS) &&	check_feature(tag_ptr->f, "用言")) {

		/* 付属語は解析しない */
		/*ルール化済み*/
		//if (check_feature(tag_ptr->mrph_ptr->f, "付属")) return 0;
		
		/* 単独の用言が『』で囲まれている場合は省略解析しない(暫定的) */
		if (check_feature(tag_ptr->f, "括弧始") &&
			check_feature(tag_ptr->f, "括弧終")) return 0;
		
		/* サ変は文節主辞のみ対象 */
		/*不要なので除外*/
		/* if (check_feature(tag_ptr->f, "文節内") &&  */
		/* 	check_feature(tag_ptr->f, "サ変")) return 0; */

		/* 判定詞の解析を行わない場合は体言は対象外 */
		if (!(OptAnaphora & OPT_ANAPHORA_COPULA) &&
			check_feature(tag_ptr->f, "体言")) return 0;
		return CF_PRED;
	}
	return 0;
}

int search_hypo_entity(SENTENCE_DATA *sp, TAG_DATA *tag_ptr, char *token )
{
	/*正解データ中にhypo_entityが出現するかを確認し、出現した場合にはskip_flagを0にする*/
	char type, rel[SMALL_DATA_LEN], *cp, loc_name[SMALL_DATA_LEN],entity_name[REPNAME_LEN_MAX],*rep;
	int i, j, tag_num, sent_num, bnst_num, diff_sen;
	TAG_DATA *parent_ptr;
	MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
	MENTION *mention_ptr = NULL;
	ENTITY *entity_ptr;
	if (!sscanf(token, "%[^/]/%c/%[^/]/%d/%d/", rel, &type,entity_name, &tag_num, &sent_num))
	{
		return FALSE;
	}

	if ((OptAnaphora & OPT_UNNAMED_ENTITY)&&(type == 'E'))
	{
		int make_mention_flag = 0;

		if (mention_mgr->num >= MENTION_MAX - 1) return;


		for(i = 0;i<UNNAMED_ENTITY_NUM;i++)
		{
			for(j = 0;j<UNNAMED_ENTITY_NAME_NUM;j++)
			{
				if(!strcmp("",unnamed_entity_name[i][j]))
				{
					break;
				}
				if(strstr(entity_name,unnamed_entity_name[i][j]))
				{
					ENTITY *hypo_entity_ptr;
					hypo_entity_ptr = &(entity_manager.entity[i]);
					hypo_entity_ptr->skip_flag = 0;
				}
			}
		}
	}
}

/*==================================================================*/
int read_one_annotation(SENTENCE_DATA *sp, TAG_DATA *tag_ptr, char *token, int co_flag)
/*==================================================================*/
{

	/* 解析結果からMENTION、ENTITYを作成する */
	/* co_flagがある場合は"="のみを処理、ない場合は"="以外を処理 */
	char type, rel[SMALL_DATA_LEN], *cp, loc_name[SMALL_DATA_LEN],entity_name[REPNAME_LEN_MAX],*rep,hypo_name[REPNAME_LEN_MAX];
	int i, j, tag_num, sent_num, bnst_num, diff_sen,hypo_flag,name_change_flag;
	TAG_DATA *parent_ptr;
	MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
	MENTION *mention_ptr = NULL;
	ENTITY *entity_ptr;
	if (!sscanf(token, "%[^/]/%c/%[^/]/%d/%d/", rel, &type,entity_name, &tag_num, &sent_num))
	{
		return FALSE;
	}
	/* 共参照関係の読み込み */
	if (co_flag && 
		(!strcmp(rel, "=") || !strcmp(rel, "=構") || !strcmp(rel, "=役"))) 
	{


		if (tag_num == -1)
		{
			return FALSE;
		}
		else
		{
			/* 複数の共参照情報が付与されている場合 */
			if (mention_mgr->mention->entity) {
				merge_two_entity(mention_mgr->mention->entity,substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity);
				substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity->link_entity = mention_mgr->mention->entity->num;
				return TRUE;
			}
			
			mention_ptr = mention_mgr->mention;
			if(mention_ptr->entity == NULL)
			{
				if(sp->Sen_num -sent_num > 0 && (sp - sent_num)->Tag_num > tag_num )
				{
					mention_ptr->entity = 
						substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;
				}
				else
				{
					return FALSE;
				}
			}

			/*共参照タグがずれている場合 暫定的な処置*/
			if(mention_ptr->entity == NULL)
			{
				return FALSE;
			}
			
			set_mention_from_coreference(tag_ptr,mention_ptr);
		}
	}
	//共参照以外の関係
	else if ((!co_flag &&
			  (type == 'N' || type == 'C' || type == 'O' || type == 'D'))
			 &&
			 (
				 (OptReadFeature & OPT_ALL_CASE) ||
				 (
					 /* 用言の場合は省略対象格のみ読み込む */
					 (check_analyze_tag(tag_ptr, FALSE) == CF_PRED && (1 || (OptReadFeature & OPT_CASE_ANALYSIS) ||match_ellipsis_case(rel, NULL)) ||
					  /* 名詞の場合は */
					  check_analyze_tag(tag_ptr, FALSE) == CF_NOUN &&
					  /* 連想照応対象格の場合はそのまま読み込み */
					  (match_ellipsis_case(rel, NULL) ||
					   /* 用言の省略対象格の場合はノ？格として読み込む */
					   match_ellipsis_case(rel, ELLIPSIS_CASE_LIST_VERB) && strcpy(rel, "ノ？"))))
				 )
		)
	{
		if (tag_num == -1) return FALSE;
		if (type == 'O' && !(OptReadFeature & OPT_ELLIPSIS))
		{
			return FALSE;
		}
		if(sp->Sen_num -sent_num <= 0)
		{
			return FALSE;
		}
		if (mention_mgr->num >= MENTION_MAX - 1) return FALSE;		
		/* 先行詞は体言のみ */

		if((OptReadFeature & OPT_ALL_CASE)|| (type != 'O' && 
				(check_feature(((sp - sent_num)->tag_data + tag_num)->f, "体言") ||
			 check_feature(((sp - sent_num)->tag_data + tag_num)->f, "形副名詞")))
		   ||
		   (type =='O' && check_feature(((sp - sent_num)->tag_data + tag_num)->f, "先行詞候補"))) {	
			
			mention_ptr = mention_mgr->mention + mention_mgr->num;
			if(substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity->link_entity ==-1)
			{ 
				mention_ptr->entity=
					substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity;
			}
			else
			{
				mention_ptr->entity=entity_manager.entity+(substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention->entity->link_entity);
			}
			
			mention_ptr->explicit_mention = (type == 'C') ?
				substance_tag_ptr((sp - sent_num)->tag_data + tag_num)->mention_mgr.mention : NULL;
			mention_ptr->salience_score = mention_ptr->entity->salience_score;
			mention_ptr->tag_num = mention_mgr->mention->tag_num;
			mention_ptr->sent_num = mention_mgr->mention->sent_num;
			mention_ptr->tag_ptr = 
				(sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
			mention_ptr->type = type;
			strcpy(mention_ptr->cpp_string, rel);
			if (type == 'C' && 
				(cp = check_feature(((sp - sent_num)->tag_data + tag_num)->f, "係"))) {
				strcpy(mention_ptr->spp_string, cp + strlen("係:"));
			} 
			else if (type == 'C' && 
					 (check_feature(((sp - sent_num)->tag_data + tag_num)->f, "文末"))) {
				strcpy(mention_ptr->spp_string, "文末");
			} 		
			else {
				strcpy(mention_ptr->spp_string, "＊");
			}
			mention_mgr->num++;
			
			/* 共参照タグを辿ると連体修飾先である場合はtypeを'C'に変更 */
			if (type == 'O' && check_feature(tag_ptr->f, "連体修飾") &&
				tag_ptr->parent->mention_mgr.mention->entity == mention_ptr->entity) {
					mention_ptr->type = type = 'C';
			}	    
				
			if (type == 'O') {
				if (check_analyze_tag(tag_ptr, FALSE) == CF_PRED)		
				{
					mention_ptr->static_salience_score = SALIENCE_ZERO;
				}
				else 
				{
					mention_ptr->static_salience_score = SALIENCE_ASSO;
				}
			}  
		}
		else/*体言以外は不特定-その他にする*//*体言以外は補文する*/
		{
			mention_ptr = mention_mgr->mention + mention_mgr->num;			
			
			mention_ptr->entity = entity_manager.entity+4;
			mention_ptr->explicit_mention = NULL;
			mention_ptr->tag_num = mention_mgr->mention->tag_num;
			mention_ptr->sent_num = mention_mgr->mention->sent_num;
			mention_ptr->tag_ptr = 
				(sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
			mention_ptr->type = 'E';
			strcpy(mention_ptr->cpp_string, rel);
			strcpy(mention_ptr->spp_string, "＊");
			mention_mgr->num++;

		}
			
	}
	else if (((OptAnaphora & OPT_UNNAMED_ENTITY) || (OptAnaphora & OPT_GS) || (OptAnaphora & OPT_TRAIN))&&(!co_flag &&type == 'E'))
	{
		
		int make_mention_flag = 0;
		if (!(OptReadFeature & OPT_ELLIPSIS))
		{
			return FALSE;
		}
		if((OptReadFeature & OPT_ALL_CASE) ||
		   check_analyze_tag(tag_ptr, FALSE) == CF_PRED && ((OptReadFeature & OPT_CASE_ANALYSIS)||match_ellipsis_case(rel, NULL)) ||
			  /* 名詞の場合は */
			  check_analyze_tag(tag_ptr, FALSE) == CF_NOUN &&
			  /* 連想照応対象格の場合はそのまま読み込み */
		   (match_ellipsis_case(rel, NULL) ||
			   /* 用言の省略対象格の場合はノ？格として読み込む */
			   match_ellipsis_case(rel, ELLIPSIS_CASE_LIST_VERB) && strcpy(rel, "ノ？")))
		{
			
		}
		else
		{
			return FALSE;
		}
		
		
		if (mention_mgr->num >= MENTION_MAX - 1) return;


		for(i = 0;i<entity_manager.num;i++)
		{
			int name_match_flag =0;
			if( make_mention_flag ==1)
			{
				break;
			}
			if(entity_manager.entity[i].hypothetical_flag !=1)
			{
				continue;
			}
			if(i<UNNAMED_ENTITY_NUM)
			{
				for(j = 0;j<UNNAMED_ENTITY_NAME_NUM;j++)
				{
					if(!strcmp("",unnamed_entity_name[i][j]))
					{
						break;
					}
					if(!strcmp(entity_name,unnamed_entity_name[i][j]))
					{
						name_match_flag = 1;
					}
				}
			}
			else
			{
				if(!strcmp(entity_manager.entity[i].name,entity_name))
				{
					name_match_flag =1;
				}
			}

			if(name_match_flag ==1)
			{
				ENTITY *hypo_entity_ptr;
				hypo_entity_ptr = &(entity_manager.entity[i]);
				for (cp =entity_name;*cp != '\0';cp++)
				{
					if(*cp == ':')
					{
						*cp = '-';
					}
				}

				make_mention_flag =1;
				if(hypo_entity_ptr->real_entity == -1)
				{
					mention_ptr = mention_mgr->mention + mention_mgr->num;			
					
					mention_ptr->entity = entity_manager.entity+i;
					mention_ptr->explicit_mention = NULL;
					mention_ptr->tag_num = mention_mgr->mention->tag_num;
					mention_ptr->sent_num = mention_mgr->mention->sent_num;
					mention_ptr->tag_ptr = 
						(sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
					mention_ptr->type = type;
					strcpy(mention_ptr->cpp_string, rel);
					strcpy(mention_ptr->spp_string, "＊");
					mention_mgr->num++;
				}
				else
				{
					mention_ptr = mention_mgr->mention + mention_mgr->num;
					mention_ptr->entity = entity_manager.entity+(hypo_entity_ptr->real_entity);
					mention_ptr->explicit_mention = NULL;
					mention_ptr->salience_score = mention_ptr->entity->salience_score;
					mention_ptr->tag_num = mention_mgr->mention->tag_num;
					mention_ptr->sent_num = mention_mgr->mention->sent_num;
					mention_ptr->tag_ptr = 
						(sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
					mention_ptr->type = 'O';
					strcpy(mention_ptr->cpp_string, rel);
					strcpy(mention_ptr->spp_string, "＊");
					mention_mgr->num++;
				}
					
			}
		}
		if(make_mention_flag ==0)
		{
			/*不特定-人nを作る*/
			char *temp;
			for (cp =entity_name;*cp != '\0';cp++)
			{
				if(*cp == ':')
				{
					*cp = '-';
				}
			}


			entity_ptr = make_each_unnamed_entity(entity_name);
			entity_ptr->salience_score =0;

			mention_ptr = mention_mgr->mention + mention_mgr->num;
			mention_ptr->entity = entity_ptr;
			mention_ptr->salience_score = 1;
			mention_ptr->tag_num = mention_mgr->mention->tag_num;
			mention_ptr->sent_num = mention_mgr->mention->sent_num;
			mention_ptr->tag_ptr = 
				(sentence_data + mention_ptr->sent_num - 1)->tag_data + mention_ptr->tag_num;
			mention_ptr->type = 'E';
			strcpy(mention_ptr->cpp_string, rel);
			strcpy(mention_ptr->spp_string, "＊");
			mention_mgr->num++;
		}
		
		
		
	}
	
	if (!mention_ptr) return FALSE;
	mention_ptr->entity->mention[mention_ptr->entity->mentioned_num] = mention_ptr;
	if (mention_ptr->entity->mentioned_num >= MENTIONED_MAX - 1) { 
		fprintf(stderr, "Entity \"%s\" mentiond too many times!\n", mention_ptr->entity->name);
	}
	else  {
		mention_ptr->entity->mentioned_num++;
	}
	
	/* 学習用情報の出力 */
	if ((OptAnaphora & OPT_TRAIN) && type == 'O' && strcmp(rel, "=")) {

		/* 位置カテゴリの出力 */
		mark_loc_category(sp, tag_ptr);
		entity_ptr = mention_ptr->entity;

		/* 何文以内にmentionを持っているかどうかのチェック */
		diff_sen = 4;
		for (i = 0; i < entity_ptr->mentioned_num; i++) {
			if (mention_ptr->sent_num == entity_ptr->mention[i]->sent_num &&
				loc_category[(entity_ptr->mention[i]->tag_ptr)->b_ptr->num] == LOC_SELF) continue;

			if (mention_ptr->sent_num - entity_ptr->mention[i]->sent_num < diff_sen)
				diff_sen = mention_ptr->sent_num - entity_ptr->mention[i]->sent_num;
		}

		for (i = 0; i < entity_ptr->mentioned_num; i++) {
			/* もっとも近くの文に出現したmentionのみ出力 */
			if (mention_ptr->sent_num - entity_ptr->mention[i]->sent_num > diff_sen)
				continue;

			if ( /* 自分自身はのぞく */
				entity_ptr->mention[i]->sent_num == mention_ptr->sent_num &&
				loc_category[(entity_ptr->mention[i]->tag_ptr)->b_ptr->num] == LOC_SELF) continue;

			if (get_location(loc_name, mention_ptr->sent_num, rel, entity_ptr->mention[i], FALSE)) {
				printf(";;LOCATION-ANT: %s\n", loc_name);
			}
		}
	}
	return TRUE;
}

/*==================================================================*/
void expand_result_to_parallel_entity(TAG_DATA *tag_ptr)
/*==================================================================*/
{
	/* 並列要素を展開する */
	int i, j, result_num;
	CF_TAG_MGR *ctm_ptr = tag_ptr->ctm_ptr; 
	TAG_DATA *t_ptr, *para_ptr;
	ENTITY *entity_ptr, *epnd_entity_ptr;
	MENTION *mention_ptr;

	result_num = ctm_ptr->result_num;
	for (i = 0; i < result_num; i++) {
		entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i];

		/* 格要素のentityの省略以外の直近の出現を探す */
		for (j = entity_ptr->mentioned_num - 1; j >= 0; j--) {
			if (entity_ptr->mention[j]->type == 'S' ||
				entity_ptr->mention[j]->type == '=') break;
		}
		/* 先行文の場合のみを対象とする */
		
		if ((OptAnaphora & OPT_UNNAMED_ENTITY)&&(entity_ptr->hypothetical_flag == 1)
			|| tag_ptr->mention_mgr.mention->sent_num < entity_ptr->mention[j]->sent_num)
		{
			continue;
		}
		
		t_ptr = entity_ptr->mention[j]->tag_ptr;

		/* 並列の要素をチェック */
		if (t_ptr->para_type == PARA_NORMAL &&
			t_ptr->parent && t_ptr->parent->para_top_p) {

			for (j = 0; t_ptr->parent->child[j]; j++) {
				para_ptr = substance_tag_ptr(t_ptr->parent->child[j]);

				if (para_ptr != t_ptr && check_feature(para_ptr->f, "体言") &&
					para_ptr->para_type == PARA_NORMAL &&
					/* 橋渡し指示には適用しない(暫定的) */
					!(ctm_ptr->type[i] == 'O' && 
					  check_analyze_tag(tag_ptr, FALSE) == CF_NOUN) &&
					/* 省略の場合は拡張先が解析対象の係り先である場合、拡張しない*/
					!(ctm_ptr->type[i] == 'O' && tag_ptr->parent == para_ptr)) {    
					if(para_ptr->mention_mgr.mention->entity->num == entity_ptr->num || para_ptr->mention_mgr.mention->entity->output_num == entity_ptr->output_num)
					{
						continue;
					}
					epnd_entity_ptr = para_ptr->mention_mgr.mention->entity;
					ctm_ptr->filled_entity[epnd_entity_ptr->num] = TRUE;
					ctm_ptr->entity_num[ctm_ptr->result_num] = epnd_entity_ptr->num;
					ctm_ptr->type[ctm_ptr->result_num] = ctm_ptr->type[i];
					ctm_ptr->cf_element_num[ctm_ptr->result_num] = ctm_ptr->cf_element_num[i];
					ctm_ptr->result_num++;

					if (OptDisplay == OPT_DEBUG)
						printf(";;EXPANDED %s : %s -> %s\n", 
							   tag_ptr->head_ptr->Goi2, 
							   entity_ptr->name, epnd_entity_ptr->name);

					if (ctm_ptr->result_num == CF_ELEMENT_MAX) return;
				}
			}
		}
	}
}

/*==================================================================*/
void anaphora_result_to_entity(TAG_DATA *tag_ptr)
/*==================================================================*/
{
	/* 照応解析結果ENTITYに関連付ける */
	int i, j;
	char *cp;
	MENTION_MGR *mention_mgr = &(tag_ptr->mention_mgr);
	MENTION *mention_ptr = NULL;
	CF_TAG_MGR *ctm_ptr = tag_ptr->ctm_ptr; 

	/* 格・省略解析結果がない場合は終了 */
	if (!ctm_ptr) return;

	for (i = 0; i < ctm_ptr->result_num; i++) {
		if (mention_mgr->num >= MENTION_MAX - 1) return;
		mention_ptr = mention_mgr->mention + mention_mgr->num;
		mention_ptr->entity = entity_manager.entity + ctm_ptr->entity_num[i];
		mention_ptr->tag_num = mention_mgr->mention->tag_num;
		mention_ptr->sent_num = mention_mgr->mention->sent_num;
		mention_ptr->type = ctm_ptr->type[i];
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
			/* 省略でない場合(expand_result_to_parallel_entityで拡張) */
			if (ctm_ptr->type[i] != 'O' &&  ctm_ptr->type[i] != 'E') 
			{
				strcpy(mention_ptr->spp_string, "Ｐ");
			}
			else 
			{
				strcpy(mention_ptr->spp_string, "Ｏ");
				if (check_analyze_tag(tag_ptr, FALSE) == CF_PRED)
				{
					mention_ptr->static_salience_score = SALIENCE_ZERO;
				}
				else
				{
					mention_ptr->static_salience_score = SALIENCE_ASSO;
				}
			}
		}
		mention_mgr->num++;

		mention_ptr->entity->mention[mention_ptr->entity->mentioned_num] = mention_ptr;
		if (mention_ptr->entity->mentioned_num >= MENTIONED_MAX - 1) { 
			fprintf(stderr, "Entity \"%s\" mentiond too many times!\n", mention_ptr->entity->name);
		}
		else {
			mention_ptr->entity->mentioned_num++;
		}
    }  
}

/*==================================================================*/
int set_tag_case_frame(SENTENCE_DATA *sp, TAG_DATA *tag_ptr, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    /* ENTITY_PRED_MGRを作成する関数
       make_data_cframeを用いて入力文の格構造を作成するため
       CF_PRED_MGRを作り、そのcfをコピーしている */
    int i;
    TAG_CASE_FRAME *tcf_ptr = tag_ptr->tcf_ptr;
    char *vtype = NULL;  

    /* 入力文側の格要素設定 */
    /* set_data_cf_type(cpm_ptr); */
    if (check_analyze_tag(tag_ptr, FALSE) == CF_PRED) {
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

    return TRUE;
}

/*==================================================================*/
int set_cf_candidate(TAG_DATA *tag_ptr, CASE_FRAME **cf_array)
/*==================================================================*/
{
    int i, l, frame_num = 0, hiragana_prefer_type = 0;
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
			check_str_type(tag_ptr->head_ptr->Goi,TYPE_HIRAGANA)) {
			if (check_feature(tag_ptr->f, "代表ひらがな")) {
				hiragana_prefer_type = 1;
			}
			else {
				hiragana_prefer_type = -1;
			}
		}

		for (l = 0; l < tag_ptr->cf_num; l++) {
			if ((tag_ptr->cf_ptr + l)->type == tag_ptr->tcf_ptr->cf.type && 
				(hiragana_prefer_type == 0 || 
				 (hiragana_prefer_type > 0 && 
				  check_str_type((tag_ptr->cf_ptr + l)->entry, TYPE_HIRAGANA)) || 
				 (hiragana_prefer_type < 0 && 
				  (!check_str_type((tag_ptr->cf_ptr + l)->entry,TYPE_HIRAGANA))))) {
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
    int i, j, e_num, debug = 1;
    double score;
    char key[SMALL_DATA_LEN];

	ctm_ptr->case_analysis_ga_entity = -1;
    /* 対象の格フレームが選択されることのスコア */
    score = get_cf_probability_for_pred(&(tcf_ptr->cf), ctm_ptr->cf_ptr);

    /* 対応付けられた要素に関するスコア(格解析結果) */
    for (i = 0; i < ctm_ptr->case_result_num; i++) {

		e_num = ctm_ptr->cf_element_num[i];
		if(ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code("ガ"))
		{
			ctm_ptr->case_analysis_ga_entity = ctm_ptr->entity_num[i];
			
		}
		score += 
			get_ex_probability_with_para(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr) +
			get_case_function_probability_for_pred(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr, TRUE);
	
		if (OptDisplay == OPT_DEBUG && debug)
			printf(";;対応あり:%s-%s:%f:%f ", ctm_ptr->elem_b_ptr[i]->head_ptr->Goi2, 
				   pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),
				   get_ex_probability_with_para(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr),
				   get_case_function_probability_for_pred(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), e_num, ctm_ptr->cf_ptr, TRUE));
    }

    /* 入力文の格要素のうち対応付けられなかった要素に関するスコア */
    for (i = 0; i < tcf_ptr->cf.element_num - ctm_ptr->case_result_num; i++) {
		if (OptDisplay == OPT_DEBUG && debug) 
			printf(";;対応なし:%s:%f ", 
				   (tcf_ptr->elem_b_ptr[ctm_ptr->non_match_element[i]])->head_ptr->Goi2, score);	
		score += FREQ0_ASSINED_SCORE + UNKNOWN_CASE_SCORE;
    }
    if (OptDisplay == OPT_DEBUG && debug) printf(";; %f ", score);	   

    /* 格フレームの格が埋まっているかどうかに関するスコア */
    for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
		if (tcf_ptr->cf.type == CF_NOUN) continue;
		score += get_case_probability(e_num, ctm_ptr->cf_ptr, ctm_ptr->filled_element[e_num], NULL);	
    }
    if (OptDisplay == OPT_DEBUG && debug) printf(";; %f\n", score);

    return score;
}



/*==================================================================*/
int convert_locname_id(char *loc_name,int *loc_num_ptr, int *simple_loc_num_ptr)
/*==================================================================*/
{
    /* 位置カテゴリをIDに変換[0-134]、その他:-1 */
    /* 出現格: S＊ + [CO][ガヲニノ]: 9  */
    /* 位置カ: C[1-9] + B[1-3] + A[1-3]	 : 15 */
    int id = 0;
	int loc_id =0;
	int case_id = 0;
	int c_type = 9;
	int b_type = 3;
	int a_type = 3;
	int num=0;
    /* ヲ-Oヲ-C8 */
    /* ニ-Oガ-B1 */
    if (strlen(loc_name) != BYTES4CHAR*2 + 5)
	{
		*loc_num_ptr = -1;
		*simple_loc_num_ptr = -1;
		
		return -1;
	}

	
    /* [SCO] */
    if (loc_name[BYTES4CHAR + 1] == 'C')
	{
		case_id = 1;
	}
	else if (loc_name[BYTES4CHAR + 1] == 'O' || loc_name[BYTES4CHAR + 1] == 'E')
	{
		case_id = 2;
	}
    else if (loc_name[BYTES4CHAR + 1] == 'S')
	{
		case_id = 0;
	}
	else
	{
		*loc_num_ptr = -1;
		*simple_loc_num_ptr = -1;
		return -1;
	}

	
	
    /* [ガヲニノ] */
    if (loc_name[BYTES4CHAR + 1] != 'S') {
		int case_type = 4;
		if (!strncmp(loc_name + BYTES4CHAR + 2, "ヲ", 2))
		{
			case_id = (case_id-1)*case_type+1+1;
		}
		else if (!strncmp(loc_name + BYTES4CHAR + 2, "ニ", 2))
		{
			case_id = (case_id-1)*case_type+2+1;
		}
		else if (!strncmp(loc_name + BYTES4CHAR + 2, "ガ", 2))
		{
			case_id = (case_id-1)*case_type+1	;

		}
		else if(!strncmp(loc_name + BYTES4CHAR + 2, "ノ", 2))
		{
			//精度が下がったのでなし
			*loc_num_ptr = -1;
			*simple_loc_num_ptr = -1;

			return -1;
			//橋渡しは精度が低いので使わない
			if(case_id ==2)
			{
				
				return -1;
			}
			case_id = (case_id-1)*case_type+3+1;
		}
		else
		{
			*loc_num_ptr = -1;
			*simple_loc_num_ptr = -1;

			return -1;
		}
		
    }
	
    /* [CBA] */
	
    if (loc_name[BYTES4CHAR*2+3] == 'B') 
	{
		loc_id = c_type;
	}
	else if(loc_name[BYTES4CHAR*2+3] == 'A')
	{
		loc_id = b_type+c_type;
	}
    else if (loc_name[BYTES4CHAR*2+3] == 'C')
	{
		loc_id = 0;
	}
	else
	{
		*loc_num_ptr = -1;
		*simple_loc_num_ptr = -1;

		return -1;
	}

    /* [1-9] */
	num = atoi(loc_name + BYTES4CHAR*2+4);
    if ( num> 0)
	{
		loc_id += num - 1;

	}
    else
	{
		*loc_num_ptr = -1;
		*simple_loc_num_ptr = -1;

		return -1;
	}
	
    *loc_num_ptr= loc_id + case_id*(c_type+b_type+a_type);
	*simple_loc_num_ptr = loc_id;

	return 1;
}

int make_ctm_from_corpus(TAG_DATA *tag_ptr,CF_TAG_MGR *ctm_ptr, int handling_idx, int r_num,int annotated_r_num,int case_analysis_flag)
{
	int j, k,l, e_num,exist_flag;
	int pre_filled_element[CF_ELEMENT_MAX], pre_filled_entity[ENTITY_MAX];
	//コーパスの結果を利用してctmを作る

	memset(ctm_ptr->filled_element, 0, sizeof(int) * CF_ELEMENT_MAX);
	for (j = 0; j < r_num; j++) {
		ctm_ptr->filled_element[ctm_ptr->cf_element_num[j]] = TRUE;
	
	}

	if(case_analysis_flag ==0)
	{
		int mention_idx = handling_idx;
		if(mention_idx < tag_ptr->mention_mgr.num)
		{
			int assinged_flag=0;
			int skip_flag =0;
			if( (case_analysis_flag ==0 && tag_ptr->mention_mgr.mention[mention_idx].type != 'C'))
			{
				
				for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {//格スロットを順番に探す
					if(ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code(tag_ptr->mention_mgr.mention[mention_idx].cpp_string) )
					{
						if (ctm_ptr->filled_element[e_num] == TRUE)
						{
							continue;
						}
						assinged_flag =1;
						ctm_ptr->cf_element_num[r_num] = e_num;
						ctm_ptr->entity_num[r_num] = tag_ptr->mention_mgr.mention[mention_idx].entity->num;
						make_ctm_from_corpus(tag_ptr,ctm_ptr,mention_idx+1,r_num+1,annotated_r_num+1,case_analysis_flag);
					}
				}
			}
			//とりあえず割り当てないバージョンも作る
			//保存時にr_numが大きい順にする
			make_ctm_from_corpus(tag_ptr,ctm_ptr,mention_idx+1,r_num,annotated_r_num,case_analysis_flag);
		}
		else//全部のmentionを処理したら
		{
			char aresult[REPNAME_LEN_MAX];
			double score;
			ctm_ptr->result_num = r_num;
			
			ctm_ptr->annotated_result_num = annotated_r_num;
			
			make_aresult_string(ctm_ptr,aresult);
			score = calc_ellipsis_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr);
			if(score != INITIAL_SCORE)
			{
				ctm_ptr->all_arguments_score=0;
				ctm_ptr->case_analysis_score = calc_score_of_case_frame_assingemnt(ctm_ptr, tag_ptr->tcf_ptr);
				ctm_ptr->score = ctm_ptr->overt_arguments_score * overt_arguments_weight;
				ctm_ptr->score += ctm_ptr->all_arguments_score * all_arguments_weight;
				for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
					for (k = 0; k < O_FEATURE_NUM; k++) {
						ctm_ptr->score += (ctm_ptr->omit_feature[j][k] == INITIAL_SCORE) ?
					0 : ctm_ptr->omit_feature[j][k] * case_feature_weight[j][k];
					}
				}
				preserve_ellipsis_gs_ctm(ctm_ptr,0,ELLIPSIS_CORRECT_MAX,ellipsis_correct_ctm);
			}

		}
	}
	else//格解析
	{
		int cf_element_idx = handling_idx;
		if (cf_element_idx < tag_ptr->tcf_ptr->cf.element_num) //まだチェックしていない要素がある場合
		{
			char annotated_tag[PP_STRING_MAX]="";
			int mention_idx;
			int annotated_flag=0;
			for(mention_idx = 1;mention_idx < tag_ptr->mention_mgr.num;mention_idx++)
			{
				if(tag_ptr->tcf_ptr->elem_b_ptr[cf_element_idx]->mention_mgr.mention->entity->num == tag_ptr->mention_mgr.mention[mention_idx].entity->num)
				{
					//タグ付けされているかを確認
					strcpy(annotated_tag,tag_ptr->mention_mgr.mention[mention_idx].cpp_string);
					annotated_flag =1;
				}
			}
			
			for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) 
			{
				if(((annotated_flag ==1) &&  ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code(annotated_tag) )
				   || ((annotated_flag ==0) && (ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code("修飾") 
												|| ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code("時間") 
												|| ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code("外の関係"))))
				{
					ctm_ptr->cf_element_num[r_num] = e_num;
					ctm_ptr->elem_b_ptr[r_num] = tag_ptr->tcf_ptr->elem_b_ptr[cf_element_idx];
					ctm_ptr->tcf_element_num[r_num] = cf_element_idx;
					ctm_ptr->type[r_num] = tag_ptr->tcf_ptr->elem_b_num[cf_element_idx] == -1 ? 'N' : 'C';
					ctm_ptr->entity_num[r_num] = tag_ptr->tcf_ptr->elem_b_ptr[cf_element_idx]->mention_mgr.mention->entity->num;
					
					make_ctm_from_corpus(tag_ptr,ctm_ptr,cf_element_idx+1,r_num+1,annotated_r_num+annotated_flag,case_analysis_flag);
					
				}
			}
			make_ctm_from_corpus(tag_ptr,ctm_ptr,cf_element_idx+1,r_num,annotated_r_num,case_analysis_flag);
		}
		else//終わったら省略のタグを読む
		{
			ctm_ptr->case_result_num = r_num;
			ctm_ptr->overt_arguments_score = calc_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr);
			make_ctm_from_corpus(tag_ptr,ctm_ptr,0,r_num,annotated_r_num,0);
		}

		
	}
	memcpy(ctm_ptr->filled_element, pre_filled_element, sizeof(int) * CF_ELEMENT_MAX);
	return TRUE;
	
}

/*==================================================================*/
double calc_ellipsis_score_of_ctm(CF_TAG_MGR *ctm_ptr, TAG_CASE_FRAME *tcf_ptr)
/*==================================================================*/
{
    /* 格フレームとの対応付けのスコアを計算する関数(省略解析の評価) */
    int i, j,k,l, loc_num, e_num, sent_num, tag_num,pp;
    double score = 0, max_score, tmp_ne_ct_score, tmp_score,  prob, penalty;
    double *of_ptr, scase_prob_cs, scase_prob, location_prob;
	char buf[SMALL_DATA_LEN];
    char *cp, key[SMALL_DATA_LEN], loc_name[SMALL_DATA_LEN];
	int modality_feature[MODALITY_NUM];
	int voice_feature[VOICE_NUM];
	int keigo_feature[KEIGO_NUM];

	char aresult[REPNAME_LEN_MAX];
	int sen_cat_base;
	int adj_flag;
	int pred_dpnd_type=0;
	int verb_situation=0; 
	int ga_filled_flag=0;
	int adjacent_flag=0;
	int oblig_flag = 1;
	double ex_case_prob;
	double ex_prob;
	int cf_ex_sum = 0;
	double cf_ga_filled_ratio=FREQ0_ASSINED_SCORE;
	int hypo_entity_fill_case[ELLIPSIS_CASE_NUM];
	int ability_feature=0;
	
	
    /* 解析対象の基本句の文番号 */
    sent_num = tcf_ptr->pred_b_ptr->mention_mgr.mention->sent_num;
    tag_num = tcf_ptr->pred_b_ptr->mention_mgr.mention->tag_num;

    /* omit_featureの初期化 */
    for (i = 0; i < ELLIPSIS_CASE_NUM; i++) {
		for (j = 0; j < O_FEATURE_NUM; j++) {
			ctm_ptr->omit_feature[i][j] = INITIAL_SCORE;
		}
    }
	for (i = 0; i < ELLIPSIS_CASE_NUM; i++) {
		hypo_entity_fill_case[i] = -1;
	}
	ctm_ptr->ga_entity = -1;

    for (i = 0; i < ctm_ptr->result_num; i++) {
		int feature_case_num;
		e_num = ctm_ptr->cf_element_num[i];
		feature_case_num = get_ellipsis_case_num(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),NULL);


		if(feature_case_num ==0)
		{
			ga_filled_flag =1;
		}
		
	}
	for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
		cf_ex_sum += ctm_ptr->cf_ptr->freq[e_num];
	}

	for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {

		if(cf_ex_sum)
		{
		
			if(ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code("ガ")&&ctm_ptr->cf_ptr->freq[e_num])
			{
				cf_ga_filled_ratio = log((double)(ctm_ptr->cf_ptr->freq[e_num])/(double)cf_ex_sum);
			}
			

			if(!ctm_ptr->filled_element[e_num]&& ctm_ptr->cf_ptr->pp[e_num][0] != pp_kstr_to_code("外の関係") && ((double)ctm_ptr->cf_ptr->freq[e_num]/cf_ex_sum > 0.5))
			{
				//oblig_flag =0;
					
			}
		
			
		}
	}


	
	
	//if(!(OptAnaphora & OPT_TRAIN) &&  (OptAnaphora & OPT_UNNAMED_ENTITY) && (ga_filled_flag ==0||oblig_flag==0))
	if((OptAnaphora & OPT_UNNAMED_ENTITY) && ((!(OptAnaphora & OPT_TRAIN) &&   ga_filled_flag ==0 ) || oblig_flag==0))
	{
		return INITIAL_SCORE;
	}

	

	
	for (i =0;i<MODALITY_NUM;i++)
	{
		char mod[DATA_LEN] = "モダリティ-";
		strcat(mod,modality[i]);
		if(check_feature(tcf_ptr->pred_b_ptr->b_ptr->f,mod))
		{
			modality_feature[i] = 1;
		}
		else
		{
			modality_feature[i] = 0;
		}
	}
	for (i=0;i<VOICE_NUM;i++)
	{
		char voice_str[DATA_LEN] ="態:";
		strcat(voice_str,voice[i]);
		if(check_feature(tcf_ptr->pred_b_ptr->b_ptr->f,voice_str))
		{
			voice_feature[i] = 1;
		}
		else
		{
			voice_feature[i] = 0;
		}
	}

	for (i =0;i<KEIGO_NUM;i++)
	{
		char kei[DATA_LEN] = "敬語:";
		strcat(kei,keigo[i]);	
		if(check_feature(tcf_ptr->pred_b_ptr->b_ptr->f,kei))
		{
			keigo_feature[i] = 1;
		}
		else
		{
			keigo_feature[i] = 0;
		}
	}
	if(check_feature(tcf_ptr->pred_b_ptr->b_ptr->f,"可能表現"))
	{
		ability_feature = 1;
	}

	if(!strcmp(ctm_ptr->cf_ptr->pred_type,"形") )
	{
		adj_flag = 1;
	}
	else
	{
		adj_flag = 0;
	}
	

	if(check_feature(tcf_ptr->pred_b_ptr->f,"係:文末"))
	{
		pred_dpnd_type = 0;
	}
	else if(check_feature(tcf_ptr->pred_b_ptr->f,"係:連格"))
	{
		pred_dpnd_type = 1;
	}
	else if(check_feature(tcf_ptr->pred_b_ptr->f,"係:連用"))
	{
		pred_dpnd_type = 2;
	}

	if(check_feature(tcf_ptr->pred_b_ptr->f,"動態述語"))
	{
		verb_situation =0;
	}
	else if(check_feature(tcf_ptr->pred_b_ptr->f,"状態述語"))
	{
		verb_situation =1;
	}


    for (i = 0; i < ctm_ptr->result_num; i++) {
		ENTITY *entity_ptr;
		int entity_num = -1;
		int hypothetical_num;
		entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i];
		hypothetical_num = entity_ptr->hypothetical_entity;
		if((OptAnaphora & OPT_UNNAMED_ENTITY)&& ((entity_ptr->num < UNNAMED_ENTITY_NUM) ||
												 (hypothetical_num !=-1 && (hypothetical_num < UNNAMED_ENTITY_NUM))||
												 hypothetical_num !=-1 && entity_ptr->hypothetical_flag ==1))
		{
			if((entity_ptr->num < UNNAMED_ENTITY_NUM))
			{
				entity_num = entity_ptr->num;
			}
			else if(hypothetical_num < UNNAMED_ENTITY_NUM)
			{
				entity_num =  hypothetical_num;
			}
			else 
			{
				for(j=0;j<UNNAMED_ENTITY_NUM;j++)
				{
					for(k=0;k<UNNAMED_ENTITY_NAME_NUM;k++)
					{
						if(!strcmp(unnamed_entity_name[j][k],""))
						{
							break;
						}
						
						if(strstr(entity_ptr->hypothetical_name,unnamed_entity_name[j][k]))
						{
							entity_num = j;
							break;
						}
					}
				}
			
			}
		}
		if(entity_num != -1)
		{
			int feature_case_num = get_ellipsis_case_num(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),NULL);
			if(feature_case_num != -1 && entity_num < UNNAMED_ENTITY_NUM)
			{
				hypo_entity_fill_case[feature_case_num] = entity_num;
			}
		}
	}

    /* 対応付けられた要素に関するスコア(省略解析結果) */
    for (i = 0; i < ctm_ptr->result_num; i++) {
		int feature_base;
		int feature_base_2=-1;
		int hypothetical_num;
		int filled_flag=0;
		int feature_case_num;
		int case_frame_exist_flag=0;
		ENTITY *entity_ptr;
		e_num = ctm_ptr->cf_element_num[i];
		entity_ptr = entity_manager.entity + ctm_ptr->entity_num[i]; /* 関連付けられたENTITY */
		hypothetical_num = entity_ptr->hypothetical_entity;
		pp = ctm_ptr->cf_ptr->pp[e_num][0]; /* "が"、"を"、"に"のcodeはそれぞれ1、2、3 */
		feature_case_num = get_ellipsis_case_num(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),NULL);
		

		if(i < ctm_ptr->case_result_num)
		{
			filled_flag =1;
			case_frame_exist_flag =1;
		}
		else
		{
			feature_base = 0;
		}
		if(pp ==1)
		{
			ctm_ptr->ga_entity = entity_ptr->num;
		}
		//ノ格(名詞)の場合は暫定的にガ格のfeatureに入れる
		//ただし、実際にはscoreしか使っていないので関係ない
		if(feature_case_num == -1)
		{
			if(filled_flag ==1)
			{
				continue;
			}
			else
			{
				of_ptr = ctm_ptr->omit_feature[0];
			}

		}
		else
		{
			of_ptr = ctm_ptr->omit_feature[feature_case_num];
		}

		
	
		/* 対応付けられた解析対象格の埋まりやすさ */
		

		
		
		/* P(弁当|食べる:動2,ヲ格)/P(弁当) (∝P(食べる:動2,ヲ格|弁当)) */
		/* type='S'または'='のmentionの中で最大となるものを使用 */	
		max_score = INITIAL_SCORE;
		

		if((OptAnaphora & OPT_UNNAMED_ENTITY)&& ((entity_ptr->num < UNNAMED_ENTITY_NUM) ||
												 (hypothetical_num !=-1 && (hypothetical_num < UNNAMED_ENTITY_NUM))||
												 hypothetical_num !=-1 && entity_ptr->hypothetical_flag ==1))
		{
			int entity_num;
			double max_pmi = INITIAL_SCORE;

			case_frame_exist_flag = 1;
			if((entity_ptr->num < UNNAMED_ENTITY_NUM))
			{
				entity_num = entity_ptr->num;
			}
			else if(hypothetical_num < UNNAMED_ENTITY_NUM)
			{
				entity_num = hypothetical_num;
			}
			else 
			{
				for(j=0;j<UNNAMED_ENTITY_NUM;j++)
				{
					for(k=0;k<UNNAMED_ENTITY_NAME_NUM;k++)
					{
						if(!strcmp(unnamed_entity_name[j][k],""))
						{
							break;
						}
						
						if(strstr(entity_ptr->hypothetical_name,unnamed_entity_name[j][k]))
						{
							entity_num = j;
							break;
						}
					}
				}
			}
			
			feature_base = UNNAMED_S+entity_num*EACH_FEARUTE_NUM;
			
			feature_base += adj_flag*ADJ_FEATURE_S;
			feature_base_2 = feature_base;
			/* 埋まったかどうか */
			of_ptr[feature_base+ASSIGNED] = 1;
			if( (OptAnaphora & OPT_AUTHOR_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
			{
				of_ptr[feature_base+AUTHOR_SCORE] = author_score;
			}
			if( (OptAnaphora & OPT_READER_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
			{

				of_ptr[feature_base+READER_SCORE] = reader_score;
			}
			

			if(ctm_ptr->cf_ptr->freq[e_num]>0)
			{
				of_ptr[feature_base+EX_ASSIGNMENT] =log((double)ctm_ptr->cf_ptr->freq[e_num]/cf_ex_sum);
			}
			else
			{
				of_ptr[feature_base+EX_ASSIGNMENT] = FREQ0_ASSINED_SCORE;
			}
			of_ptr[feature_base+ASSIGNMENT] = get_case_probability(e_num, ctm_ptr->cf_ptr, TRUE, NULL);


			of_ptr[feature_base+PRED_DPND_TYPE_S+pred_dpnd_type] = 1;
			of_ptr[feature_base+VERB_SITUATION_S+verb_situation] = 1;
			of_ptr[feature_base+YOBIKAKE_S] = yobikake_count;

			for (j =0;j <ELLIPSIS_CASE_NUM+1;j++)
			{
				for (k=0;k<UNNAMED_ENTITY_NUM;k++)
				{
					if(hypo_entity_fill_case[j] == k)
					{
						of_ptr[feature_base+UNNAMED_NUM_S+j*UNNAMED_ENTITY_NUM+k] =1;
					}
				}
			}
			//of_ptr[feature_base+CF_GA_FILLED_RATIO] = cf_ga_filled_ratio;
			
			for (j=0;j<MODALITY_NUM;j++)
			{
				of_ptr[feature_base+MODALITY_S+j] = modality_count[j];
				of_ptr[feature_base+MODALITY_S+MODALITY_NUM+j] = modality_feature[j];
				
				
			}
			for(j=0;j<VOICE_NUM;j++)
			{
				of_ptr[feature_base+VOICE_S+j] = voice_feature[j];				
			}
			for (j=0;j<KEIGO_NUM;j++)
			{
				of_ptr[feature_base+KEIGO_S+j] = keigo_count[j];
				of_ptr[feature_base+KEIGO_S+KEIGO_NUM+j] = keigo_feature[j];
			}
			if(ability_feature == 1)
			{
				of_ptr[feature_base+ABILITY] = 1;
			}

			if (OptGeneralCF & OPT_CF_CATEGORY)
			{
				for (j=0;j<UNNAMED_ENTITY_CATEGORY_NUM;j++)
				{
					if(!strcmp(unnamed_entity_category[entity_num][j],""))
					{
						break;
					}
					sprintf(key, "CT:%s:",unnamed_entity_category[entity_num][j]);
					
					prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE);
					*strchr(key + 3, ':') = '\0';
					ex_prob = get_general_probability(key, "KEY"); 
					
					if(ex_prob != FREQ0_ASSINED_SCORE)
					{
						if(prob==0)
						{
							tmp_score = FREQ0_ASSINED_SCORE;
						}
						else
						{
							tmp_score = log(prob);
						}
						ex_case_prob = tmp_score;

						tmp_score -= ex_prob;
						
						if (tmp_score > of_ptr[feature_base+CEX_PMI])
						{
							if(tmp_score > max_pmi)
							{
								max_pmi = tmp_score;
							}
							of_ptr[feature_base+CEX_PMI] = tmp_score;
							of_ptr[feature_base+CEX_CASE_PROB] = ex_case_prob;
							of_ptr[feature_base+CEX_PROB] = ex_prob;
						}
					}
					
				}
			}
			
			if (0&&(OptGeneralCF & OPT_CF_NE) )
			{
				
				for (j=0;j<UNNAMED_ENTITY_NE_NUM;j++)
				{
					
					if(!strcmp(unnamed_entity_ne[entity_num][j],""))
					{
						break;
					}
					
					sprintf(key, "NE:%s:",unnamed_entity_ne[entity_num][j]);
					prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE);
					
					
					/* P(ARTIFACT|食べる:動2,ヲ格) */
					*strchr(key + 3, ':') = '\0';
					/* /P(ARTIFACT) */
					ex_prob = get_general_probability(key, "KEY");		
					if(ex_prob != FREQ0_ASSINED_SCORE)
					{

						if(prob ==0)
						{
							tmp_score = FREQ0_ASSINED_SCORE;
						}
						else
						{
							tmp_score = log(prob);
						}
						ex_case_prob = tmp_score;
						tmp_score -= ex_prob;
						if (tmp_score > of_ptr[feature_base+NEX_PMI])
						{
							if(tmp_score > max_pmi)
							{
								max_pmi = tmp_score;
							}
							of_ptr[feature_base+NEX_PMI] = tmp_score;
							of_ptr[feature_base+NEX_CASE_PROB] = ex_case_prob;
							of_ptr[feature_base+NEX_PROB] = ex_prob;
						}
						if (tmp_score > tmp_ne_ct_score) tmp_ne_ct_score = tmp_score;
					}
					
				}
			}
			


			prob = _get_ex_probability_internal(unnamed_entity[entity_num] ,e_num ,ctm_ptr->cf_ptr );
			ex_prob = get_general_probability(unnamed_entity[entity_num],"KEY");
			if(prob !=0)
			{
				if( log(prob) > of_ptr[feature_base+UNNAMED_CASE_PROB])
				{
					of_ptr[feature_base+UNNAMED_CASE_PROB] = log(prob);
				}
			}
			else
			{
				if(of_ptr[feature_base+UNNAMED_CASE_PROB] == INITIAL_SCORE)
				{
					of_ptr[feature_base+UNNAMED_CASE_PROB] = FREQ0_ASSINED_SCORE;
				}
			}


			for (j=0;j<UNNAMED_ENTITY_REP_NUM;j++)
			{
				
				if(!strcmp(unnamed_entity_rep[entity_num][j],""))
				{
					break; 
				}
				sprintf(key,unnamed_entity_rep[entity_num][j]);
				prob = _get_ex_probability_internal(key,e_num ,ctm_ptr->cf_ptr );
				ex_prob = get_general_probability(key,"KEY");
				
				if(ex_prob != FREQ0_ASSINED_SCORE)
				{
					if(prob == 0)
					{
						tmp_score = FREQ0_ASSINED_SCORE;
					}
					else
					{
						tmp_score = log(prob);
					}
					ex_case_prob = tmp_score;
					tmp_score -= ex_prob;
					if (tmp_score > of_ptr[feature_base+EX_PMI])
					{
						if(tmp_score > max_pmi)
						{
							max_pmi = tmp_score;
						}

						of_ptr[feature_base+EX_PMI] = tmp_score;
						of_ptr[feature_base+EX_CASE_PROB] = ex_case_prob;
						of_ptr[feature_base+EX_PROB] = ex_prob;
						
					}
				}

			}
			of_ptr[feature_base+MAX_PMI] = max_pmi;
		}
		
		
		if(entity_ptr->hypothetical_flag ==0)
		{
			double max_pmi = INITIAL_SCORE;
			if(filled_flag == 0)
			{
				feature_base = REAL_S;
			}
			else
			{
				feature_base = FILLED_S;
			}
			
			feature_base+=adj_flag*ADJ_FEATURE_S;
			of_ptr[feature_base+ASSIGNED] = 1;
			if( (OptAnaphora & OPT_AUTHOR_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
			{
				of_ptr[feature_base+AUTHOR_SCORE] = author_score;
			}
			if( (OptAnaphora & OPT_READER_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
			{

				of_ptr[feature_base+READER_SCORE] = reader_score;
			}

			if(ctm_ptr->cf_ptr->freq[e_num]>0)
			{
				of_ptr[feature_base+EX_ASSIGNMENT] =log((double)ctm_ptr->cf_ptr->freq[e_num]/cf_ex_sum);
			}
			else
			{
				of_ptr[feature_base+EX_ASSIGNMENT] = FREQ0_ASSINED_SCORE;
			}
			of_ptr[feature_base+ASSIGNMENT] = get_case_probability(e_num, ctm_ptr->cf_ptr, TRUE, NULL);
			
			of_ptr[feature_base+PRED_DPND_TYPE_S+pred_dpnd_type] = 1;
			of_ptr[feature_base+VERB_SITUATION_S+verb_situation] = 1;
			of_ptr[feature_base+YOBIKAKE_S] = yobikake_count;


			for (j =0;j <ELLIPSIS_CASE_NUM+1;j++)
			{
				for (k=0;k<UNNAMED_ENTITY_NUM;k++)
				{
					if(hypo_entity_fill_case[j] == k)
					{
						of_ptr[feature_base+UNNAMED_NUM_S+j*UNNAMED_ENTITY_NUM+k] =1;
					}
				}
			}



			for (j=0;j<MODALITY_NUM;j++)
			{
				of_ptr[feature_base+MODALITY_S+j] = modality_count[j];
				of_ptr[feature_base+MODALITY_S+MODALITY_NUM+j] = modality_feature[j];
				
			}
			for(j=0;j<VOICE_NUM;j++)
			{
				of_ptr[feature_base+VOICE_S+j] = voice_feature[j];				
			}

			for (j=0;j<KEIGO_NUM;j++)
			{
				of_ptr[feature_base+KEIGO_S+j] = keigo_count[j];
				of_ptr[feature_base+KEIGO_S+KEIGO_NUM+j] = keigo_feature[j];
			}
			if(ability_feature == 1)
			{
				of_ptr[feature_base+ABILITY] = 1;
			}

			


			for (j = 0; j < entity_ptr->mentioned_num; j++) {

				if (entity_ptr->mention[j]->type != 'S' && entity_ptr->mention[j]->type != '=') continue;
				tmp_ne_ct_score = FREQ0_ASSINED_SCORE;

				/* クラスのスコアを計算 */
				if ((OptGeneralCF & OPT_CF_CLASS) && tcf_ptr->cf.type == CF_PRED) {
					
					cp = get_bnst_head_canonical_rep(entity_ptr->mention[j]->tag_ptr->b_ptr, OptCaseFlag & OPT_CASE_USE_CN_CF);

					/*cp がNULLの場合があるので確認する(class probabilityは代表表記のみ?)*/
					
					
					if ( cp != NULL&& strlen(cp) < SMALL_DATA_LEN - 4) {
						sprintf(key, "%s:CL", cp);
						prob = get_class_probability(key, e_num, ctm_ptr->cf_ptr); 
						if (prob && log(prob) > of_ptr[feature_base+CLS_PMI])
						{
							of_ptr[feature_base+CLS_PMI] = log(prob);
						}
					}
				}
				
				/* カテゴリがある場合はP(食べる:動2,ヲ格|カテゴリ:人)もチェック */
				if ((OptGeneralCF & OPT_CF_CATEGORY) && 
					(cp = check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ"))) {
					char *category_ptr;
					

					strcpy(buf,cp+strlen("カテゴリ:"));
					category_ptr = strtok(buf,";");
					while(category_ptr != NULL)
					{
						sprintf(key, "CT:%s:",category_ptr);
						prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE);
						/* /P(カテゴリ:人) */
						*strchr(key + 3, ':') = '\0';
						ex_prob = get_general_probability(key, "KEY");		
						if(ex_prob != FREQ0_ASSINED_SCORE)
						{
							/* P(カテゴリ:人|食べる:動2,ヲ格) */
							if(prob == 0)
							{
								tmp_score = FREQ0_ASSINED_SCORE;
							}
							else
							{
								tmp_score = log(prob);
							}
							ex_case_prob = tmp_score;
							
							if(ex_case_prob != FREQ0_ASSINED_SCORE || ex_prob == FREQ0_ASSINED_SCORE )
							{
								case_frame_exist_flag = 1;
							}

							tmp_score -= ex_prob;
							if (tmp_score > of_ptr[feature_base+CEX_PMI])
							{
								if(tmp_score > max_pmi)
								{
									max_pmi = tmp_score;
								}
								of_ptr[feature_base+CEX_PMI] = tmp_score;
								of_ptr[feature_base+CEX_CASE_PROB] = ex_case_prob;
								of_ptr[feature_base+CEX_PROB] = ex_prob;
								
							}
							if (tmp_score > tmp_ne_ct_score) tmp_ne_ct_score = tmp_score;
						}
 
						for(k=0;k<CATEGORY_NUM;k++)
						{
							
							if(!strcmp(category_list[k],category_ptr))
							{
								for(l=0;l<CATEGORY_NUM;l++)
								{
									if(!strcmp(alternate_category[k][l],""))
									{
										break;
									}
									sprintf(key, "CT:%s:", alternate_category[k][l]);
									prob = get_ex_ne_probability(key, e_num, ctm_ptr->cf_ptr, TRUE);
									*strchr(key + 3, ':') = '\0';
									ex_prob = get_general_probability(key, "KEY");		
									if(ex_prob != FREQ0_ASSINED_SCORE)
									{
										/* P(カテゴリ:人|食べる:動2,ヲ格) */
										if(prob == 0)
										{
											tmp_score = FREQ0_ASSINED_SCORE;
										}
										else
										{
											tmp_score = log(prob);
										}
										ex_case_prob = tmp_score;
										if(ex_case_prob != FREQ0_ASSINED_SCORE || ex_prob == FREQ0_ASSINED_SCORE )
										{
											case_frame_exist_flag = 1;
										}

										tmp_score -= ex_prob;
										if (tmp_score > of_ptr[feature_base+ALT_CT_PMI])
										{
											of_ptr[feature_base+ALT_CT_PMI] = tmp_score;
											of_ptr[feature_base+ALT_CT_CASE_PROB] = ex_case_prob;
											of_ptr[feature_base+ALT_CT_PROB] = ex_prob;
										}
									}
								}
							}
						}
						

						if(!strcmp("人",category_ptr) || !strcmp("組織・団体",category_ptr))
						{
							//of_ptr[feature_base+CF_GA_FILLED_RATIO] = cf_ga_filled_ratio;
						}
						

						category_ptr=strtok(NULL,";");

					}
					
					

				}
				
				/* 固有表現の場合はP(食べる:動2,ヲ格|ARTIFACT)もチェック */
				if ((OptGeneralCF & OPT_CF_NE) && 
					(cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE")) )
				{
					prob = get_ex_ne_probability(cp, e_num, ctm_ptr->cf_ptr, TRUE);
					/* /P(ARTIFACT) */
					strcpy(key, cp);
					*strchr(key + 3, ':') = '\0'; /* key = NE:LOCATION */
					ex_prob = get_general_probability(key, "KEY");		

					if(ex_prob != FREQ0_ASSINED_SCORE)
					{
						if(prob ==0)
						{
							tmp_score = FREQ0_ASSINED_SCORE;
						}
						else
						{
							/* P(ARTIFACT|食べる:動2,ヲ格) */
							tmp_score = log(prob);
						}
						
						ex_case_prob = tmp_score;
						if(ex_case_prob != FREQ0_ASSINED_SCORE || ex_prob == FREQ0_ASSINED_SCORE )
						{
							case_frame_exist_flag = 1;
						}
						

						tmp_score -= ex_prob;
						if (tmp_score > of_ptr[feature_base+NEX_PMI])
						{
							if(tmp_score > max_pmi)
							{
								max_pmi = tmp_score;
							}
							of_ptr[feature_base+NEX_PMI] = tmp_score;
							of_ptr[feature_base+NEX_CASE_PROB] = ex_case_prob;
							of_ptr[feature_base+NEX_PROB] = ex_prob;
						}
						if (tmp_score > tmp_ne_ct_score) tmp_ne_ct_score = tmp_score;
					}

				}
				
				/* P(弁当|食べる:動2,ヲ格) */
				tmp_score = ex_case_prob = get_ex_probability(ctm_ptr->tcf_element_num[i], &(tcf_ptr->cf), 
															  entity_ptr->mention[j]->tag_ptr, e_num, ctm_ptr->cf_ptr, FALSE);
				
				
				/* /P(弁当) */


				ex_prob = get_key_probability(entity_ptr->mention[j]->tag_ptr);
				
				tmp_score -= ex_prob;
				
				if(ex_case_prob != FREQ0_ASSINED_SCORE || ex_prob == FREQ0_ASSINED_SCORE )
				{
					case_frame_exist_flag = 1;
				}
				if(ex_prob != FREQ0_ASSINED_SCORE || ex_case_prob != FREQ0_ASSINED_SCORE)
				{
					if (tmp_score > of_ptr[feature_base+EX_PMI])
					{
						if(tmp_score > max_pmi)
						{
							max_pmi = tmp_score;
						}

						of_ptr[feature_base+EX_PMI] = tmp_score;
						of_ptr[feature_base+EX_CASE_PROB] = ex_case_prob;
						of_ptr[feature_base+EX_PROB] = ex_prob;
					}
				}
				
				

				
				of_ptr[feature_base+MAX_PMI] = max_pmi;
				
				/* 人名の場合はof_ptr[NEX_PMI]以下にはしない */
				if ((OptGeneralCF & OPT_CF_NE) && check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE:PERSON") &&
					of_ptr[feature_base+EX_PMI] < 0 && of_ptr[feature_base+EX_PMI] < of_ptr[feature_base+NEX_PMI])
				{
					of_ptr[feature_base+EX_PMI] = 0;
				}
				
				/* カテゴリ、固有表現から計算された値との平均値を使用 */
				if (ex_case_prob > FREQ0_ASSINED_SCORE && 
					tmp_ne_ct_score > FREQ0_ASSINED_SCORE)
					tmp_score = (tmp_score + tmp_ne_ct_score) / 2;
				else if (tmp_ne_ct_score > FREQ0_ASSINED_SCORE)
					tmp_score = tmp_ne_ct_score;	
				
				if (tmp_score > max_score) {
					max_score = tmp_score;
				}
			}
			
		}
		if(case_frame_exist_flag == 0)
		{
			return INITIAL_SCORE;
		}
		
		
		
		for(j=adj_flag*ADJ_FEATURE_S+REAL_S;j<adj_flag*ADJ_FEATURE_S+ADJ_FEATURE_S;j++)
		{
			if(of_ptr[j%EACH_FEARUTE_NUM+adj_flag*ADJ_FEATURE_S] < of_ptr[j])
			{
				of_ptr[j%EACH_FEARUTE_NUM+adj_flag*ADJ_FEATURE_S] = of_ptr[j];
			}
		}
		
		
		score += max_score;
	

		if(filled_flag ==0)
		{
			feature_base = adj_flag*ADJ_FEATURE_S;
		}
		else
		{
			feature_base = adj_flag*ADJ_FEATURE_S+FILLED_S;
		}
		
		/* SALIENCE_SCORE */
		//of_ptr[feature_base+SALIENCE_CHECK] = (entity_ptr->salience_score >= 1.00) ? 1 : 0;
		


		if(filled_flag==0)
		{
			int closest_appearance = 100;
			int closest_wa_appearance = 100;
			int old_wa_flag = 0;
			of_ptr[feature_base+SAME_PRED]=0;
			if(feature_base_2 != -1)
			{
				of_ptr[feature_base_2+SAME_PRED]=0;
			}
			for (j = 0; j < entity_ptr->mentioned_num; j++)
			{
				if(entity_ptr->mention[j]->type == 'C')
				{
					
					if(
						(pp_kstr_to_code(entity_ptr->mention[j]->cpp_string) == pp)&&
						
						entity_ptr->mention[j]->tag_ptr->cf_ptr != NULL &&
						entity_ptr->mention[j]->tag_ptr->cf_ptr->entry != NULL &&
						ctm_ptr->cf_ptr->entry != NULL &&
						!strcmp(entity_ptr->mention[j]->tag_ptr->cf_ptr->entry, ctm_ptr->cf_ptr->entry) )
					{
						of_ptr[feature_base+SAME_PRED]++;
						if(feature_base_2 != -1)
						{
							of_ptr[feature_base_2+SAME_PRED]++;
						}
						
					}
				}
			}
			
			/*表層での出現回数*/
			of_ptr[feature_base+OVERT_APPEARANCE]=0;
			of_ptr[feature_base+BEFORE_OVERT_APPEARANCE]=0;
			of_ptr[feature_base+AFTER_OVERT_APPEARANCE]=0;
			
			if(feature_base_2 != -1)
			{
				of_ptr[feature_base_2+OVERT_APPEARANCE]=0;
				of_ptr[feature_base_2+BEFORE_OVERT_APPEARANCE]=0;
				of_ptr[feature_base_2+AFTER_OVERT_APPEARANCE]=0;
				
				
			}
			for (j = 0; j < entity_ptr->mentioned_num; j++)
			{
				if(entity_ptr->mention[j]->type == 'S'|| entity_ptr->mention[j]->type == '=' )
				{

					of_ptr[feature_base+OVERT_APPEARANCE]++;
					if(entity_ptr->mention[j]->sent_num < sent_num)
					{
						of_ptr[feature_base+BEFORE_OVERT_APPEARANCE]++;
					}
					else if(entity_ptr->mention[j]->sent_num > sent_num)
					{
						of_ptr[feature_base+AFTER_OVERT_APPEARANCE]++;
					}
					if(entity_ptr->mention[j]->sent_num <= sent_num)
					{
						closest_appearance = sent_num - entity_ptr->mention[j]->sent_num;

					}
					if(entity_ptr->mention[j]->sent_num < sent_num)
					{
						if(	check_feature(entity_ptr->mention[j]->tag_ptr->f, "ハ"))
						{
							closest_wa_appearance = entity_ptr->mention[j]->sent_num;
						}
					}


					if(feature_base_2 != -1)
					{
						of_ptr[feature_base_2+OVERT_APPEARANCE]++;
						if(entity_ptr->mention[j]->sent_num < sent_num)
						{
						of_ptr[feature_base_2+BEFORE_OVERT_APPEARANCE]++;
						}
						else if(entity_ptr->mention[j]->sent_num > sent_num)
						{
							of_ptr[feature_base_2+AFTER_OVERT_APPEARANCE]++;
						}
						
					}
					
				}
			}
			

			if(closest_appearance > CLOSEST_APPEARANCE_NUM -1)
			{
				closest_appearance = CLOSEST_APPEARANCE_NUM -1;
			}
			of_ptr[feature_base+CLOSEST_APPEARANCE_S+closest_appearance] = 1;
			if(feature_base_2 != -1)
			{
				of_ptr[feature_base_2+CLOSEST_APPEARANCE_S+closest_appearance] = 1;					
			}

			if(closest_wa_appearance != 100)
			{
				for(j=0;j<entity_manager.num;j++)
				{
					if(j == entity_ptr->num)
					{
						continue;
					}
					for(k=0;k<entity_manager.entity[j].mentioned_num;k++)
					{
						if(entity_manager.entity[j].mention[k]->type == 'S' || 
						   entity_manager.entity[j].mention[k]->type == '=')
						{
							if(check_feature(entity_manager.entity[j].mention[k]->tag_ptr->f,"ハ"))
							{
								if(entity_manager.entity[j].mention[k]->sent_num <= sent_num)
								{
									if(closest_wa_appearance < entity_manager.entity[j].mention[k]->sent_num)
									{
										old_wa_flag = 1;
									}
								}
							}
						}
						
					}
				}
			}
			
			of_ptr[feature_base+OLD_TOPIC] =old_wa_flag;
			if(feature_base_2 != -1)
			{
				of_ptr[feature_base_2+OLD_TOPIC] =old_wa_flag;
			}

			/* mentionごとにスコアを計算 */	
			max_score = FREQ0_ASSINED_SCORE;
			
			

			for (j =0;j <CONTEXT_FEATURE_NUM;j++)
			{
				of_ptr[feature_base+CONTEXT_S+j]=  context_feature[entity_ptr->num][j] ;
				if(feature_base_2 != -1)
				{
					of_ptr[feature_base_2+CONTEXT_S+j]=  context_feature[entity_ptr->num][j] ;
				}

			}
			
			for (j = 0; j < entity_ptr->mentioned_num; j++) {
				tmp_score = 0;
				
				/* 位置カテゴリであまり考慮できない情報を追加 */
				/* if (entity_ptr->mention[j]->sent_num == sent_num && */
				/* 	check_feature(entity_ptr->mention[j]->tag_ptr->f, "ハ") && */
				/* 	(check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE:PERSON") || */
				/* 	 check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ:人") || */
				/* 	 check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE:ORIGANIZATION") || */
				/* 	 check_feature(entity_ptr->mention[j]->tag_ptr->head_ptr->f, "カテゴリ:組織・団体"))) { */
				/* 	of_ptr[feature_base+WA_IN_THE_SENT] = 1; */
				/* 	if(feature_base_2 != -1) */
				/* 	{ */
				/* 		of_ptr[feature_base_2+WA_IN_THE_SENT] = 1; */
				/* 	} */
				/* } */

				if (entity_ptr->mention[j]->sent_num == sent_num &&
					check_feature(entity_ptr->mention[j]->tag_ptr->f, "ハ") 
					) {
					of_ptr[feature_base+WA_IN_THE_SENT] = 1;
					if(feature_base_2 != -1)
					{
						of_ptr[feature_base_2+WA_IN_THE_SENT] = 1;
					}
				}
				
				if (check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE:PERSON")) {
					of_ptr[feature_base+NE_PERSON] = 1;
					if(feature_base_2 != -1)
					{
						of_ptr[feature_base_2+NE_PERSON] = 1;
					}
				}
				cp = check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE");
				
				if(cp)
				{
					
					for(k=0;k<NE_NUM;k++)
					{
						if(strstr(cp + strlen("NE:"),ne[k]))
						{
							of_ptr[feature_base+NE_FEATURE_S+k] = 1;
							if(feature_base_2 != -1)
							{
								of_ptr[feature_base_2+NE_FEATURE_S+k] = 1;
							}
							
						}
					}
				}
				if (check_feature(entity_ptr->mention[j]->tag_ptr->f, "NE")) {
					//of_ptr[feature_base+NE_CHECK] = 1;
				}
				
				
				/* 自分自身は除外 */
				if (entity_ptr->mention[j]->sent_num == sent_num &&
					!loc_category[(entity_ptr->mention[j]->tag_ptr)->b_ptr->num]) continue;
				
				/* 解析対象格以外の関係は除外 */
				/*
				  if (strcmp(entity_ptr->mention[j]->cpp_string, "＊") &&
				  !match_ellipsis_case(entity_ptr->mention[j]->cpp_string, NULL)) continue;	
				*/
				
				/* 位置カテゴリ */
				/* 省略格、type(S,=,O,N,C)ごとに位置カテゴリごとに先行詞となる確率を考慮
				   位置カテゴリは、以前の文であれば B + 何文前か(4文前以上は0)
				   同一文内であれば C + loc_category という形式(ex. ガ-O-C3、ヲ-=-B2) */
				if (tcf_ptr->cf.type == CF_PRED) {
					int simple_loc_num;
					get_location(loc_name, sent_num, pp_code_to_kstr(pp), entity_ptr->mention[j], FALSE);		

					location_prob = get_general_probability("PMI", loc_name);
					convert_locname_id(loc_name,&loc_num,&simple_loc_num);
					//printf("debug %s %d %s\n",loc_name,loc_num,entity_ptr->name);
					if (loc_num != -1 && (entity_ptr->mention[j]->tag_ptr)->cf_ptr != NULL)
					{
						char cf_entry_type[REPNAME_LEN_MAX] ="";
						char mention_cf_entry_type[REPNAME_LEN_MAX] ="";
						double event_lift;
						
						make_cf_entry_type(cf_entry_type,ctm_ptr->cf_ptr);
						strncat(cf_entry_type,":",1);
						strcat(cf_entry_type,pp_code_to_kstr(pp));
						make_cf_entry_type(mention_cf_entry_type,(entity_ptr->mention[j]->tag_ptr)->cf_ptr);
						strncat(mention_cf_entry_type,":",1);
						strcat(mention_cf_entry_type,entity_ptr->mention[j]->cpp_string);
					}
					
					if(loc_num != 1)
					{
						of_ptr[feature_base+LOCATION_S + loc_num*3] =1;
						if(feature_base_2 != -1)
						{
									of_ptr[feature_base_2+LOCATION_S + loc_num*3] =1;
						}
					}
				}
				else {
					get_location(loc_name, sent_num, pp_code_to_kstr(pp), entity_ptr->mention[j], TRUE);
					location_prob = get_general_probability("T", loc_name);
				}
				tmp_score += location_prob;
				
				if (tmp_score > max_score) {
					max_score = tmp_score;
					/* 最大のスコアとなった基本句を保存(並列への対処のため) */
					ctm_ptr->elem_b_ptr[i] = entity_ptr->mention[j]->tag_ptr;
				}
			}
		}

		score += max_score;

		
	}

	

	/* 対応付けられなかった解析対象格の埋まりにくさ */
	for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
		int feature_base = adj_flag*ADJ_FEATURE_S;
		if (!ctm_ptr->filled_element[e_num] &&
			match_ellipsis_case(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]), NULL)
			)
		{
			int feature_case_num;
			feature_case_num = get_ellipsis_case_num(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[e_num][0]),NULL);
			if(feature_case_num == -1 || (feature_case_num == 0 && ga_filled_flag == 1))
			{
				
			}
			else
			{
				of_ptr = ctm_ptr->omit_feature[feature_case_num];
				of_ptr[feature_base+NO_ASSIGNMENT] = get_case_probability(e_num, ctm_ptr->cf_ptr, FALSE, NULL);
				score += of_ptr[feature_base+NO_ASSIGNMENT];
			}
		}
	}


LAST:
	return score;
}

/*==================================================================*/
int copy_ctm(CF_TAG_MGR *source_ctm, CF_TAG_MGR *target_ctm)
/*==================================================================*/
{
	int i, j;

	target_ctm->score = source_ctm->score;
	target_ctm->score_def = source_ctm->score_def;
	target_ctm->case_analysis_score = source_ctm->case_analysis_score;
	target_ctm->cf_ptr = source_ctm->cf_ptr;
	target_ctm->result_num = source_ctm->result_num;
	target_ctm->annotated_result_num = source_ctm->annotated_result_num;
	target_ctm->ga_entity = source_ctm->ga_entity;
	target_ctm->case_analysis_ga_entity = source_ctm->case_analysis_ga_entity;
	target_ctm->case_result_num = source_ctm->case_result_num;
	for (i = 0; i < CF_ELEMENT_MAX; i++) {
		target_ctm->filled_element[i] = source_ctm->filled_element[i];
		target_ctm->non_match_element[i] = source_ctm->non_match_element[i];
		target_ctm->cf_element_num[i] = source_ctm->cf_element_num[i];
		target_ctm->tcf_element_num[i] = source_ctm->tcf_element_num[i];
		target_ctm->entity_num[i] = source_ctm->entity_num[i];
		target_ctm->elem_b_ptr[i] = source_ctm->elem_b_ptr[i];
		target_ctm->type[i] = source_ctm->type[i];
	}
	for (i=0;i < ENTITY_MAX;i++)
	{
		target_ctm->filled_entity[i] = source_ctm->filled_entity[i];
	}
	target_ctm->overt_arguments_score = source_ctm->overt_arguments_score;    
	target_ctm->all_arguments_score = source_ctm->all_arguments_score;    
	for (i = 0; i < ELLIPSIS_CASE_NUM; i++) {
		for (j = 0; j < O_FEATURE_NUM; j++) {
			target_ctm->omit_feature[i][j] = source_ctm->omit_feature[i][j];
		}
	}
}

/*==================================================================*/
int preserve_case_candidate_ctm(CF_TAG_MGR *ctm_ptr, int start, int num)
/*==================================================================*/
{
	/* start番目からnum個のcase_candidate_ctmのスコアと比較し上位ならば保存する
	   num個のcase_candidate_ctmのスコアは降順にソートされていることを仮定している
	   保存された場合は1、されなかった場合は0を返す */

	int i, j;
	int same_case_flame_num = 0;
	
    
	for (i = start; i < start + num; i++) {
		if(case_candidate_ctm[i].score != INITIAL_SCORE && !strcmp(ctm_ptr->cf_ptr->cf_id,case_candidate_ctm[i].cf_ptr->cf_id))
		{
			
			same_case_flame_num++;
			if(same_case_flame_num >=5)
			{
				return FALSE;
			}
		}
		/* work_ctmに結果を保存 */
		if (ctm_ptr->score > case_candidate_ctm[i].score) {	    
			for (j = start + num - 1; j > i; j--) {
				if (case_candidate_ctm[j - 1].score > INITIAL_SCORE) {
					copy_ctm(&case_candidate_ctm[j - 1], &case_candidate_ctm[j]);
				}
			}
			copy_ctm(ctm_ptr, &case_candidate_ctm[i]);
			return TRUE;
		}
	}
	return FALSE;
}


int reordering_ellipsis_result(CF_TAG_MGR *result_ctm)
{
	int i;
	char cp_check[ELLIPSIS_RESULT_MAX];
	int copy_count=0;
	
	int max_idx;
	CF_TAG_MGR *temp_ellipsis_result_ctm;
	
	temp_ellipsis_result_ctm = (CF_TAG_MGR *)malloc_data(sizeof(CF_TAG_MGR)*ELLIPSIS_RESULT_MAX, "reordering_ellipsis_result");
	/* CF_TAG_MGR temp_ellipsis_result_ctm[ELLIPSIS_RESULT_MAX]; */
	

	for(i=0;i<ELLIPSIS_RESULT_MAX;i++)
	{
		cp_check[i] = 0;
	}
	while(copy_count <ELLIPSIS_RESULT_MAX)
	{
		double max_score = INITIAL_SCORE -100;
		for(i=0;i<ELLIPSIS_RESULT_MAX;i++)
		{
			if(cp_check[i] ==0 && result_ctm[i].score > max_score )
			{

				max_idx = i;
				max_score = result_ctm[i].score;
			}
		}
		if(max_score < INITIAL_SCORE)
		{
			break ;
		}
		copy_ctm(&result_ctm[max_idx],&temp_ellipsis_result_ctm[copy_count]);
		copy_count++;
		cp_check[max_idx]=1;
	}
	for(i=0;i<ELLIPSIS_RESULT_MAX;i++)
	{
		
		copy_ctm(&temp_ellipsis_result_ctm[i],&result_ctm[i]);
	}
	free(temp_ellipsis_result_ctm);
	
}


/*==================================================================*/
int preserve_ellipsis_result_ctm(CF_TAG_MGR *ctm_ptr, int start, int num ,CF_TAG_MGR *result_ctm)
/*==================================================================*/
{
	/* start番目からnum個のellipsis_result_ctmのスコアと比較し上位ならば保存する
	   num個のellipsis_result_ctmのスコアは降順にソートされていることを仮定している
	   保存された場合は1、されなかった場合は0を返す */

	int i, j;
    
	for (i = start; i < start + num; i++) {
	

		if (ctm_ptr->score > result_ctm[i].score) {	    
			for (j = start + num - 1; j > i; j--) {
				if (result_ctm[j - 1].score > INITIAL_SCORE) {
					copy_ctm(&result_ctm[j - 1], &result_ctm[j]);
				}
			}
			copy_ctm(ctm_ptr, &result_ctm[i]);
			return TRUE;
		}
	}
	return FALSE;
}

/*==================================================================*/
int preserve_ellipsis_gs_ctm(CF_TAG_MGR *ctm_ptr, int start, int num ,CF_TAG_MGR *result_ctm)
/*==================================================================*/
{
	/* start番目からnum個のellipsis_result_ctmのr_numとスコアと比較し上位ならば保存する
	   num個のellipsis_result_ctmのスコアは降順にソートされていることを仮定している
	   保存された場合は1、されなかった場合は0を返す */

	int i, j;

	for (i = start; i < start + num; i++) {
	
		if (ctm_ptr->annotated_result_num > result_ctm[i].annotated_result_num || ( ctm_ptr->result_num == result_ctm[i].result_num &&ctm_ptr->score > result_ctm[i].score)) {	    

			for (j = start + num - 1; j > i; j--) {
				if (result_ctm[j - 1].score > INITIAL_SCORE) {
					copy_ctm(&result_ctm[j - 1], &result_ctm[j]);
				}
			}
			copy_ctm(ctm_ptr, &result_ctm[i]);
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
	int j, k,l, e_num;

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
		int assinged_flag=0;
		int skip_flag = 0;
		/* 入力文のi番目の格要素の取りうる格(cf.pp[i][j])を順番にチェック */
		for (j = 0; tag_ptr->tcf_ptr->cf.pp[i][j] != END_M; j++) {
			/* 入力文のi番目の格要 素を格フレームのcf.pp[i][j]格に割り当てる */

			if(tag_ptr->tcf_ptr->cf.pp[i][j] == pp_kstr_to_code("＊"))
			{
				/* 格要素を割り当てない場合 */
				/* 入力文のi番目の要素が対応付けられなかったことを記録 */
				if(skip_flag ==0)
				{
					assinged_flag = 1;
					skip_flag = 1;
					ctm_ptr->non_match_element[i - r_num] = i; 
					case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num);
				}
			}
			if(skip_flag ==0)
			{
				for (k = i+1; k< tag_ptr->tcf_ptr->cf.element_num;k++)
				{
					for(l = 0; tag_ptr->tcf_ptr->cf.pp[k][l] != END_M; l++) 
					{
						if(tag_ptr->tcf_ptr->cf.pp[i][j] == tag_ptr->tcf_ptr->cf.pp[k][l])
						{
							/*自分以外にも同じ格を埋める可能性がある候補がある場合には埋めないことを許す*/
							/*ex.祈願にお寺に訪ずれる*/
							assinged_flag = 1;
							skip_flag = 1;
							ctm_ptr->non_match_element[i - r_num] = i; 
							case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num);
							
						}
						
					}
				}
			}
			for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
				int analysed_case_flag =0;

				for (k=0;k<tag_ptr->mention_mgr.num;k++)
				{
					if(tag_ptr->tcf_ptr->elem_b_ptr[i]->mention_mgr.mention->entity->num == tag_ptr->mention_mgr.mention[k].entity->num && tag_ptr->tcf_ptr->cf.pp[i][j] == pp_kstr_to_code(tag_ptr->mention_mgr.mention[k].cpp_string))
					{
						analysed_case_flag = 1;
					}
				}
				if((OptReadFeature & OPT_CASE_ANALYSIS) && analysed_case_flag ==0)
				{
					continue;
				}
				if (tag_ptr->tcf_ptr->cf.pp[i][j] == ctm_ptr->cf_ptr->pp[e_num][0] &&
					(tag_ptr->tcf_ptr->cf.type != CF_NOUN || 
					 check_feature(tag_ptr->tcf_ptr->elem_b_ptr[i]->f, "係:ノ格"))) {
					

					/* 対象の格が既に埋まっている場合は不可 */
					if(skip_flag ==0)
					{
						if (ctm_ptr->filled_element[e_num] == TRUE)
						{
							assinged_flag = 1;
							skip_flag = 1;
							ctm_ptr->non_match_element[i - r_num] = i; 
							case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num);
							
							continue;
						}
					}

					/* 非格要素は除外 */
					
					if(skip_flag ==0)
					{
						if (check_feature(tag_ptr->tcf_ptr->elem_b_ptr[i]->f, "非格要素")) {
							assinged_flag = 1;
							skip_flag = 1;
							ctm_ptr->non_match_element[i - r_num] = i; 
							case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num);
							continue;
						}	
					}
	    		    
					/* 入力文側でヲ格かつ直前格である場合は格フレームの直前格のみに対応させる */
					if (0 && tag_ptr->tcf_ptr->cf.type != CF_NOUN &&
						check_feature(tag_ptr->tcf_ptr->elem_b_ptr[i]->f, "助詞") &&
						ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code("ヲ") &&
						tag_ptr->tcf_ptr->cf.adjacent[i] && !(ctm_ptr->cf_ptr->adjacent[e_num])) {
						continue;
					}
					assinged_flag =1;
					/* 名詞格フレームの格は"φ"となっているので表示用"ノ"に変更 */
					if (tag_ptr->tcf_ptr->cf.type == CF_NOUN) {
						ctm_ptr->cf_ptr->pp[e_num][0] = pp_kstr_to_code("ノ");
					}
					/* 対応付け結果を記録 */
					ctm_ptr->elem_b_ptr[r_num] = tag_ptr->tcf_ptr->elem_b_ptr[i];
					ctm_ptr->cf_element_num[r_num] = e_num;
					ctm_ptr->tcf_element_num[r_num] = i;
					ctm_ptr->type[r_num] = tag_ptr->tcf_ptr->elem_b_num[i] == -1 ? 'N' : 'C';

					ctm_ptr->entity_num[r_num] = ctm_ptr->elem_b_ptr[r_num]->mention_mgr.mention->entity->num;

					/* i+1番目の要素のチェックへ */
					case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num + 1);
				}
			}    
		}

		if(assinged_flag==0 && skip_flag == 1)
		{
			/*割り当てる対象がなかった場合*/
			/* 入力文のi番目の要素が対応付けられなかったことを記録 */
			ctm_ptr->non_match_element[i - r_num] = i; 
			case_analysis_for_anaphora(tag_ptr, ctm_ptr, i + 1, r_num);
		}
	}

	/* すべてのチェックが終了した場合 */
	else {

		/* この段階でr_num個が対応付けられている */
		ctm_ptr->result_num = ctm_ptr->case_result_num = r_num;
		/* スコアを計算 */
		ctm_ptr->score = ctm_ptr->overt_arguments_score = calc_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr);
		
		/* スコア上位を保存 */
		preserve_case_candidate_ctm(ctm_ptr, 0, CASE_CANDIDATE_MAX);	
	}

	return TRUE;
}

/*==================================================================*/
int ellipsis_analysis(TAG_DATA *tag_ptr, CF_TAG_MGR *ctm_ptr, int i, int r_num,char *gresult)
/*==================================================================*/
{
	/* 候補となる格フレームと格要素の対応付けについて省略解析を実行する関数
	   再帰的に呼び出す 
	   iにはELLIPSIS_CASE_LIST[]のうちチェックした数が入る
	   r_numには格フレームと関連付けられた要素の数が入る
	   (格解析の結果関連付けられたものも含む) */
	int j, k, e_num,exist_flag;
	TAG_DATA *para_ptr;
	int pre_filled_element[CF_ELEMENT_MAX], pre_filled_entity[ENTITY_MAX];
	
	/* 再帰前のfilled_element, filled_entityを保存 */
	memcpy(pre_filled_element, ctm_ptr->filled_element, sizeof(int) * CF_ELEMENT_MAX);
	memcpy(pre_filled_entity, ctm_ptr->filled_entity, sizeof(int) * ENTITY_MAX);

	/* すでに埋まっている格フレームの格をチェック */
	memset(ctm_ptr->filled_element, 0, sizeof(int) * CF_ELEMENT_MAX);
	memset(ctm_ptr->filled_entity, 0, sizeof(int) * ENTITY_MAX);

	for (j = 0; j < r_num; j++) {
		/* 埋まっている格をチェック */
		ctm_ptr->filled_element[ctm_ptr->cf_element_num[j]] = TRUE;
		/* 類似している格も埋まっているものとして扱う */
		for (k = 0; ctm_ptr->cf_ptr->samecase[k][0] != END_M; k++) {
			if (ctm_ptr->cf_ptr->samecase[k][0] == ctm_ptr->cf_element_num[j])
				ctm_ptr->filled_element[ctm_ptr->cf_ptr->samecase[k][1]] = TRUE;
			else if (ctm_ptr->cf_ptr->samecase[k][1] == ctm_ptr->cf_element_num[j])
				ctm_ptr->filled_element[ctm_ptr->cf_ptr->samecase[k][0]] = TRUE;
		}
		/* 格を埋めた要素をチェック */
		ctm_ptr->filled_entity[ctm_ptr->entity_num[j]] = TRUE;
		
		/* 並列要素もチェック */
		if (j < ctm_ptr->case_result_num && /* 格解析結果の場合 */
			check_feature(ctm_ptr->elem_b_ptr[j]->f, "体言") &&
			substance_tag_ptr(ctm_ptr->elem_b_ptr[j])->para_type == PARA_NORMAL) {
			
			for (k = 0; substance_tag_ptr(ctm_ptr->elem_b_ptr[j])->parent->child[k]; k++) {
				para_ptr = substance_tag_ptr(substance_tag_ptr(ctm_ptr->elem_b_ptr[j])->parent->child[k]);
				ctm_ptr->filled_entity[para_ptr->mention_mgr.mention->entity->num] = TRUE;
			}
		}
		
		else if ( /* 省略解析結果の場合は同一文の場合のみ考慮 */    
			(((OptAnaphora & OPT_UNNAMED_ENTITY)&&entity_manager.entity[ctm_ptr->entity_num[j]].hypothetical_flag == 0 ) )
			&&
			entity_manager.entity[ctm_ptr->entity_num[j]].mention[0]->sent_num == tag_ptr->mention_mgr.mention->sent_num &&
			entity_manager.entity[ctm_ptr->entity_num[j]].mention[0]->tag_ptr->para_type == PARA_NORMAL) {
			
			for (k = 0; entity_manager.entity[ctm_ptr->entity_num[j]].mention[0]->tag_ptr->parent->child[k]; k++) {
				para_ptr = substance_tag_ptr(entity_manager.entity[ctm_ptr->entity_num[j]].mention[0]->tag_ptr->parent->child[k]);
				ctm_ptr->filled_entity[para_ptr->mention_mgr.mention->entity->num] = TRUE;
			}
		}
	}

	/* 自分自身も不可 */
	ctm_ptr->filled_entity[tag_ptr->mention_mgr.mention->entity->num] = TRUE;
	

	if (tag_ptr->parent && !check_feature(tag_ptr->parent->f, "体言"))
	{
		ctm_ptr->filled_entity[substance_tag_ptr(tag_ptr->parent)->mention_mgr.mention->entity->num] = TRUE;
	}
  

	/* 自分と並列な要素も不可(橋渡し指示の場合) */
	if (check_analyze_tag(tag_ptr, FALSE) == CF_NOUN &&
		tag_ptr->para_type == PARA_NORMAL &&
		tag_ptr->parent && tag_ptr->parent->para_top_p) {
	
		for (j = 0; tag_ptr->parent->child[j]; j++) {
			para_ptr = substance_tag_ptr(tag_ptr->parent->child[j]);
			if (para_ptr != tag_ptr && check_feature(para_ptr->f, "体言") &&
				para_ptr->para_type == PARA_NORMAL)
				ctm_ptr->filled_entity[para_ptr->mention_mgr.mention->entity->num] = TRUE;
		}
	}

	/* まだチェックしていない省略解析対象格がある場合 */
	if (*ELLIPSIS_CASE_LIST[i]) {
		
		exist_flag = 0;
		/* すべての格スロットを調べ、格がELLIPSIS_CASE_LIST[i]と一致していれば対応付けを生成する */
		/* 格がELLIPSIS_CASE_LIST[i]と一致する格スロットを探す(e_numがelement_numより小さい場合は一致する格スロットあり) */
		for (e_num = 0; e_num < ctm_ptr->cf_ptr->element_num; e_num++) {
			/* 名詞の場合は格スロットの番号とiを一致させる */
			if (tag_ptr->tcf_ptr->cf.type == CF_NOUN) {
				ctm_ptr->cf_ptr->pp[e_num][0] = pp_kstr_to_code("ノ");
				if (e_num == i && !strcmp("ノ", ELLIPSIS_CASE_LIST[i])) break;
			}
			/* 用言の場合は格をチェック */
			else if (ctm_ptr->cf_ptr->pp[e_num][0] == pp_kstr_to_code(ELLIPSIS_CASE_LIST[i])) break;
		}
			
		
		/* 対象の格が格フレームに存在しない場合は次の格へ */
		if (e_num == ctm_ptr->cf_ptr->element_num) {	    
			ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num,gresult);
		}
		else { /* 対象の格が格フレームに存在する場合 */
			/* すでに埋まっている場合は次の格をチェックする */
			if (ctm_ptr->filled_element[e_num] == TRUE) {
				ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num,gresult);
			}
			else {/* 埋まっていない場合は候補を埋める */
				for (k = 0; k < entity_manager.num; k++) {
					/* salience_scoreがSALIENCE_THRESHOLD以下なら候補としない
					   ただし解析対象が係っている表現、
					   ノ格の場合で、同一文中でノ格で出現している要素は除く */

					if(entity_manager.entity[k].hypothetical_flag == 0 ) 
					{
						
						if( !(entity_manager.entity[k].salience_score) && 
							!(tag_ptr->parent  && substance_tag_ptr(tag_ptr->parent)->mention_mgr.mention->entity->num == entity_manager.entity[k].num)
							&& !(tag_ptr->tcf_ptr->cf.type == CF_NOUN && entity_manager.entity[k].tmp_salience_flag))
						{
							continue;
						}
						
						
						
						/* 疑問詞は先行詞候補から除外(暫定的) */
						if (check_feature(entity_manager.entity[k].mention[0]->tag_ptr->f, "疑問詞")) continue;
						
					}
					if(entity_manager.entity[k].skip_flag ==1)//skip_flagが1の場合割り当てない
					{
						continue;
					}
					
					/* 対象のENTITYがすでに対応付けられている場合は不可 */
					if (ctm_ptr->filled_entity[k]) continue;
					
					if(!(OptAnaphora & OPT_TRAIN) )
					{
						if (candidate_entities[k] == 0)
						{
							continue;
						}
					}
					/* 対応付け結果を記録
					   (基本句との対応付けは取っていないためelem_b_ptrは使用しない) */
					ctm_ptr->cf_element_num[r_num] = e_num;
					ctm_ptr->entity_num[r_num] = k;
					
					/* 次の格のチェックへ */
					ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num + 1,gresult);
				}
				/* 埋めないで次の格へ(不特定) */
				ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num,gresult);
			}
		}
		/* 対象の格が格フレームに存在しない場合は次の格へ */
		//if (!exist_flag) ellipsis_analysis(tag_ptr, ctm_ptr, i + 1, r_num,gresult);
		
    }
	/* すべてのチェックが終了した場合 */
	else {
		double score;
		/* この段階でr_num個が対応付けられている */
		ctm_ptr->result_num = r_num;
		for (j = ctm_ptr->case_result_num; j < r_num; j++)
		{
			if((OptAnaphora & OPT_UNNAMED_ENTITY) && (entity_manager.entity[ctm_ptr->entity_num[j]].hypothetical_flag == 1))
			{
				ctm_ptr->type[j] = 'E';
			}
			else
			{
				ctm_ptr->type[j] = 'O';
			}
		}
		
		/* スコアを計算(旧モデル、連想照応解析に使用) */
		if ((OptAnaphora & OPT_ANAPHORA_PROB) || tag_ptr->tcf_ptr->cf.type == CF_NOUN) {
			
			ctm_ptr->score = calc_ellipsis_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr) + ctm_ptr->overt_arguments_score;
			ctm_ptr->score_def =ctm_ptr->score;
			
		}
		/* スコアを計算(線形対数モデル) */
		else {
			int calc_flag = 1;
			if (OptAnaphora & OPT_GS)
			{
				char aresult[REPNAME_LEN_MAX];
				make_aresult_string(ctm_ptr,aresult);
			}

			if(calc_flag ==1)
			{
		
				score = calc_ellipsis_score_of_ctm(ctm_ptr, tag_ptr->tcf_ptr);
				
				
				if(score != INITIAL_SCORE)
				{
					/*省略も含めた格解析スコアを計算する*/
					ctm_ptr->all_arguments_score=0;
					ctm_ptr->case_analysis_score = calc_score_of_case_frame_assingemnt(ctm_ptr, tag_ptr->tcf_ptr);
					

					if(OptAnaphora & OPT_GS)
					{
						ctm_ptr->score = ctm_ptr->case_analysis_score;
					}
					else
					{
						ctm_ptr->score = ctm_ptr->overt_arguments_score * overt_arguments_weight;
						ctm_ptr->score += ctm_ptr->all_arguments_score * all_arguments_weight;
						for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
							for (k = 0; k < O_FEATURE_NUM; k++) {
								ctm_ptr->score += (ctm_ptr->omit_feature[j][k] == INITIAL_SCORE) ?
									0 : ctm_ptr->omit_feature[j][k] * case_feature_weight[j][k];
							}
						}
					}
					
					ctm_ptr->score_def = ctm_ptr->overt_arguments_score * def_overt_arguments_weight;
					ctm_ptr->score_def += ctm_ptr->all_arguments_score * def_all_arguments_weight;
					
					for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
						for (k = 0; k < O_FEATURE_NUM; k++) {
							ctm_ptr->score_def += (ctm_ptr->omit_feature[j][k] == INITIAL_SCORE) ?
								0 : ctm_ptr->omit_feature[j][k] * def_case_feature_weight[j][k];
						}
					}
				}
				else
				{
					ctm_ptr->score = INITIAL_SCORE;
				}
				
			}
			
		}

		/* 橋渡し指示の場合で"その"に修飾されている場合は
		   対応付けが取れなかった場合はlog(4)くらいペナルティ:todo */
		if (tag_ptr->tcf_ptr->cf.type == CF_NOUN && 
			check_analyze_tag(tag_ptr, TRUE) && r_num == 0) ctm_ptr->score += -1.3863;
		
		if(ctm_ptr->score != INITIAL_SCORE)
		{
			/* スコア上位を保存 */
			preserve_ellipsis_result_ctm(ctm_ptr, 0, ELLIPSIS_RESULT_MAX,ellipsis_result_ctm);
		}

	}   
    
	/* filled_element, filled_entityを元に戻す */

	memcpy(ctm_ptr->filled_element, pre_filled_element, sizeof(int) * CF_ELEMENT_MAX);
	memcpy(ctm_ptr->filled_entity, pre_filled_entity, sizeof(int) * ENTITY_MAX);
	return TRUE;
}

/*==================================================================*/
int ellipsis_analysis_main(TAG_DATA *tag_ptr)
/*==================================================================*/
{
	/* ある基本句を対象として省略解析を行う関数 */
	/* 格フレームごとにループを回す */
	int i, j, k, frame_num = 0, rnum_check_flag;
	char cp[REPNAME_LEN_MAX], aresult[REPNAME_LEN_MAX], gresult[REPNAME_LEN_MAX],cf_aresult[REPNAME_LEN_MAX];
	CASE_FRAME **cf_array;
	MENTION *mention_ptr;
	CF_TAG_MGR work;
	CF_TAG_MGR *ctm_ptr;
	CF_TAG_MGR *gs_ctm_ptr = NULL;
	ctm_ptr = &work;


	/* 使用する格フレームの設定 */
	cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *)*tag_ptr->cf_num, "ellipsis_analysis_main");
	frame_num = set_cf_candidate(tag_ptr, cf_array);
    //printf("debug %s %d\n",tag_ptr->jiritu_ptr->Goi2,frame_num);
	if (OptDisplay == OPT_DEBUG) printf(";;CASE FRAME NUM: %d\n", frame_num);

	/* work_ctmのスコアを初期化 */
	for (i = 0; i < CASE_CANDIDATE_MAX ; i++) 
		case_candidate_ctm[i].score = INITIAL_SCORE;
	
	for (i = 0; i <  ELLIPSIS_RESULT_MAX ; i++) 
	{
		ellipsis_result_ctm[i].score = INITIAL_SCORE;
		ellipsis_result_ctm[i].score_def = INITIAL_SCORE;
		ellipsis_result_ctm[i].case_analysis_score = INITIAL_SCORE;
	}

	for (i = 0; i <  ELLIPSIS_CORRECT_MAX ; i++) 
	{
		ellipsis_correct_ctm[i].score = INITIAL_SCORE;
		ellipsis_correct_ctm[i].score_def = INITIAL_SCORE;
		ellipsis_correct_ctm[i].case_analysis_score = INITIAL_SCORE;
		ellipsis_correct_ctm[i].result_num = -1;
		ellipsis_correct_ctm[i].annotated_result_num = -1;
	}
	

	/* 照応解析用格解析(上位CASE_CANDIDATE_MAX個の結果を保持する) */
	for (i = 0; i < frame_num; i++) {

		/* OR の格フレーム(和フレーム)を除く */
		if (((*(cf_array + i))->etcflag & CF_SUM) && frame_num != 1) {
			continue;
		}

		/* ctm_ptrの初期化 */
		ctm_ptr->score = INITIAL_SCORE;
		ctm_ptr->score_def = INITIAL_SCORE;
		ctm_ptr->case_analysis_score = INITIAL_SCORE;
		
		/* 格フレームを指定 */
		ctm_ptr->cf_ptr = *(cf_array + i);
		if (OptAnaphora & OPT_TRAIN) 
		{
			CF_TAG_MGR temp_ctm;
			copy_ctm( ctm_ptr,&temp_ctm);
			make_ctm_from_corpus(tag_ptr,&temp_ctm,0,0,0,1);
		}

		/* 格解析 */
		case_analysis_for_anaphora(tag_ptr, ctm_ptr, 0, 0);	
	}

	if (OptAnaphora & OPT_TRAIN) 
	{
	
		for(i=0;i < ELLIPSIS_CORRECT_MAX;i++)
		{
			CF_TAG_MGR temp_ctm;
			CF_TAG_MGR* ctm_ptr = &temp_ctm;
			if(ellipsis_correct_ctm[i].score == INITIAL_SCORE)
			{
				printf(";;格フレームカバレッジデータ%d-%d:%2d %.3f\n", 
					   tag_ptr->mention_mgr.mention->sent_num,
					   tag_ptr->num,
					   i+1, INITIAL_SCORE);

					continue;
			}
			copy_ctm( &ellipsis_correct_ctm[i],ctm_ptr);
			make_case_assingment_string(ctm_ptr,aresult);
			printf(";;格フレームカバレッジデータ%d-%d:%2d %.3f %s", 
				   tag_ptr->mention_mgr.mention->sent_num,
				   tag_ptr->num,
				   i+1, 
				   ctm_ptr->score, ctm_ptr->cf_ptr->cf_id);
			
			for (j = 0; j < ctm_ptr->result_num; j++) {
				printf(" %s%s:%s%d",
					   (j < ctm_ptr->case_result_num) ? "" : "*",
					   pp_code_to_kstr(ctm_ptr->cf_ptr->pp[ctm_ptr->cf_element_num[j]][0]),
					   (entity_manager.entity + ctm_ptr->entity_num[j])->name,
					   (entity_manager.entity + ctm_ptr->entity_num[j])->num);
			}
			for (j = 0; j < ctm_ptr->cf_ptr->element_num; j++) {
				if (!ctm_ptr->filled_element[j] && 
					match_ellipsis_case(pp_code_to_kstr(ctm_ptr->cf_ptr->pp[j][0]), NULL)) 
					printf(" %s:%s", pp_code_to_kstr(ctm_ptr->cf_ptr->pp[j][0]),
						   (ctm_ptr->cf_ptr->oblig[j]) ? "×" : "-");
			}	    
			printf(" (0:%.2f*%.2f", ctm_ptr->overt_arguments_score,overt_arguments_weight);
			printf(" 1:%.2f", ctm_ptr->all_arguments_score);
			for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
				printf("|%s", ELLIPSIS_CASE_LIST_VERB[j]);
				for (k = 0; k < O_FEATURE_NUM; k++) {
					if (ctm_ptr->omit_feature[j][k] != INITIAL_SCORE && ctm_ptr->omit_feature[j][k] != 0 && case_feature_weight[j][k] != 0)
					{
						char feature_name[DATA_LEN] ="";
						
						if(k%EACH_FEARUTE_NUM == AUTHOR_SCORE)
						{
							strcpy(feature_name ,"A");
						}
						if(k%EACH_FEARUTE_NUM == READER_SCORE)
						{
							strcpy(feature_name ,"R");
						}
						if(k%EACH_FEARUTE_NUM > LOCATION_S && k%EACH_FEARUTE_NUM < (LOCATION_S+LOCATION_NUM*3) )
						{
							sprintf(feature_name,"L%d",(k%EACH_FEARUTE_NUM-LOCATION_S)/3);
						}
						if(k%EACH_FEARUTE_NUM > MODALITY_S && k%EACH_FEARUTE_NUM < (MODALITY_S+MODALITY_F_NUM))
						{
							sprintf(feature_name,"M%d",(k%EACH_FEARUTE_NUM-MODALITY_S)/2);
						}
						if(k%EACH_FEARUTE_NUM > KEIGO_S && k%EACH_FEARUTE_NUM < (KEIGO_S+KEIGO_F_NUM))
						{
							sprintf(feature_name,"H%d",(k%EACH_FEARUTE_NUM-KEIGO_S)/2);
						}
						if(k%EACH_FEARUTE_NUM > CONTEXT_S && k%EACH_FEARUTE_NUM < (CONTEXT_S+CONTEXT_FEATURE_NUM))
						{
							sprintf(feature_name,"C%d",(k%EACH_FEARUTE_NUM-CONTEXT_S));
						}
						
						
						//if(!(k%EACH_FEARUTE_NUM > LOCATION_S && k%EACH_FEARUTE_NUM < SIMPLE_LOCATION_S))
						{
							printf(",%s:%d:%d:%.2f*%.2f", feature_name, k,k%EACH_FEARUTE_NUM , ctm_ptr->omit_feature[j][k],case_feature_weight[j][k]);
						}
					}
				}
			}
			printf(")");
			
			printf("\n");

		}
	}
	

	if (case_candidate_ctm[0].score == INITIAL_SCORE) return FALSE;

	if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
		for (i = 0; i < CASE_CANDIDATE_MAX; i++) {
			if(OptReadFeature & OPT_COREFER_AUTO || OptAnaphora & OPT_PRUNING)
			{

				if (case_candidate_ctm[i].score == INITIAL_SCORE ||
					(i > 0 && case_candidate_ctm[i].score < case_candidate_ctm[i-1].score - CASE_CAND_DIF_MAX/2)) break;
			}
			/*足切りしない*/
			if (case_candidate_ctm[i].score == INITIAL_SCORE) break;
			printf(";;格解析候補%d-%d:%2d %.3f %s",
				   tag_ptr->mention_mgr.mention->sent_num, tag_ptr->num,
				   i + 1, case_candidate_ctm[i].score, case_candidate_ctm[i].cf_ptr->cf_id);
			
			for (j = 0; j < case_candidate_ctm[i].result_num; j++) {
				printf(" %s%s:%s",
					   case_candidate_ctm[i].cf_ptr->adjacent[case_candidate_ctm[i].cf_element_num[j]] ? "*" : "-",
					   pp_code_to_kstr(case_candidate_ctm[i].cf_ptr->pp[case_candidate_ctm[i].cf_element_num[j]][0]),
					   case_candidate_ctm[i].elem_b_ptr[j]->head_ptr->Goi2);
			}
			for (j = 0; j < case_candidate_ctm[i].cf_ptr->element_num; j++) {
				if (!case_candidate_ctm[i].filled_element[j] && 
					match_ellipsis_case(pp_code_to_kstr(case_candidate_ctm[i].cf_ptr->pp[j][0]), NULL))
					printf(" %s:×", pp_code_to_kstr(case_candidate_ctm[i].cf_ptr->pp[j][0]));
			}
			printf("\n");
		}
	}

	if (OptAnaphora & OPT_TRAIN) 
	{
		/* 正解出力を生成 */
		make_gresult_strings(tag_ptr,gresult);    

	}
	else
	{
		gresult[0] ='\0';
	}
	/* 上記の対応付けに対して省略解析を実行する */
	for (i = 0; i < CASE_CANDIDATE_MAX; i++) {
		int e_num;

		/*足切りしない*/
		if(OptReadFeature & OPT_COREFER_AUTO|| OptAnaphora & OPT_PRUNING)
		{
			if (case_candidate_ctm[i].score == INITIAL_SCORE ||
				(i > 0 && case_candidate_ctm[i].score < case_candidate_ctm[i-1].score - CASE_CAND_DIF_MAX/2)) break;
		}


		if (i > 0 && case_candidate_ctm[i].score == INITIAL_SCORE) break;
		copy_ctm(&case_candidate_ctm[i], ctm_ptr);
				
		ellipsis_analysis(tag_ptr, ctm_ptr, 0, ctm_ptr->result_num,gresult);

	}

	if (OptAnaphora & OPT_TRAIN) {

		//merge_ellipsis_result_and_correct();


		/* すべての格対応付けがない場合は出力しない */
		rnum_check_flag = 0;
		for (i = 0; i <  ELLIPSIS_RESULT_MAX; i++) {
			if (ellipsis_result_ctm[i].score == INITIAL_SCORE) break;
			if (ellipsis_result_ctm[i].result_num - ellipsis_result_ctm[i].case_result_num > 0) {
				rnum_check_flag = 1;
				break;
			}
		}
		
		if (rnum_check_flag) {
			
			

			for (i = 0; i <  ELLIPSIS_RESULT_MAX; i++) {

				
				if (ellipsis_result_ctm[i].score == INITIAL_SCORE) break;
				
				

				/* 出力結果を生成 */
				make_aresult_string(&(ellipsis_result_ctm[i]),aresult);
				if (gs_ctm_ptr == NULL && !strcmp(aresult, gresult) )
				{
					/*正解データと一致するものを保存しておく*/
					gs_ctm_ptr = ellipsis_result_ctm+i;
				}
				/* 不正解、または、既に正解出力をした場合で
				   省略関連の全素性が0（すなわち省略解析対象格がすべて直接格要素によって埋まる）である用例は出力しない */
				if ((strcmp(aresult, gresult) || !rnum_check_flag) && (ellipsis_result_ctm[i].result_num - ellipsis_result_ctm[i].case_result_num == 0))
				{
					continue;
				}
				
				
				strcpy(cf_aresult,ellipsis_result_ctm[i].cf_ptr->cf_id);
				strcat(cf_aresult," ");
				strcat(cf_aresult,aresult);
				/* 素性出力 */
				if(svm_feaature_opt != 1)
				{
					printf(";;<%s>%d FEATURE: %d, %f, %f, ", cf_aresult, i, !strcmp(aresult, gresult) ? 1 : 0,
						   ellipsis_result_ctm[i].overt_arguments_score,ellipsis_result_ctm[i].all_arguments_score);
					for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
						for (k = 0; k < O_FEATURE_NUM; k++) {
							(ellipsis_result_ctm[i].omit_feature[j][k] == INITIAL_SCORE) ?
								printf(" 0,") : 
							(ellipsis_result_ctm[i].omit_feature[j][k] == 0.0) ?
								printf(" 0,") : 
								(ellipsis_result_ctm[i].omit_feature[j][k] == 1.0) ?
								printf(" 1,") : printf(" %f,", ellipsis_result_ctm[i].omit_feature[j][k]);
						}
					}
					printf("\n");
				}
				else
				{
					printf(";;<%s>%d FEATURE: %d,", cf_aresult, i, !strcmp(aresult, gresult) ? 1 : 0);
					printf("%d:%f %d:%f ",1,ellipsis_result_ctm[i].overt_arguments_score,2,ellipsis_result_ctm[i].all_arguments_score);
					
					for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
						for (k = 0; k < O_FEATURE_NUM; k++) {
							if(ellipsis_result_ctm[i].omit_feature[j][k] == 0.0 || ellipsis_result_ctm[i].omit_feature[j][k] == INITIAL_SCORE)
							{
								if(j == ELLIPSIS_CASE_NUM -1 && k == O_FEATURE_NUM-1)
								{
									printf(" %d:%d ",j*O_FEATURE_NUM+k+3,0);
								}
							}
							else
							{
								printf(" %d:%f ",j*O_FEATURE_NUM+k+3,ellipsis_result_ctm[i].omit_feature[j][k]);
							}
							
						}
					}
					printf("\n");

					
				}
				if (!strcmp(aresult, gresult)) rnum_check_flag = 0;
			}		
	    
			/* 候補ごとの区切りのためのダミー出力 */
			//FEATUREを増やす時はここの+2を増やす
			if(svm_feaature_opt != 1)
			{
				
				printf(";;<dummy %s> FEATURE: -1,", gresult);
				for (j = 0; j < ELLIPSIS_CASE_NUM * O_FEATURE_NUM + 2; j++) printf(" 0,");
				printf("\n");
			}
			else
			{
				printf(";;<dummy %s> FEATURE: -1,", gresult);
				printf(" %d:%d",ELLIPSIS_CASE_NUM * O_FEATURE_NUM + 2,0);
					
				printf("\n");

			}
		}
	}
	

	{
		//reordering_ellipsis_result(ellipsis_result_ctm);

	}



	if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {

		
		for (i = 0; i <  ELLIPSIS_RESULT_MAX; i++) {

			if (ellipsis_result_ctm[i].score == INITIAL_SCORE) break;
			if (ellipsis_result_ctm[i].score == 0) continue;

			make_aresult_string(&(ellipsis_result_ctm[i]),aresult);

			printf(";;省略解析候補%d-%d:%2d %.3f %s", 
				   tag_ptr->mention_mgr.mention->sent_num,
				   tag_ptr->num,
				   i+1, 
				   ellipsis_result_ctm[i].score, ellipsis_result_ctm[i].cf_ptr->cf_id);
			
			for (j = 0; j < ellipsis_result_ctm[i].result_num; j++) {
				printf(" %s%s:%s%d",
					   (j < ellipsis_result_ctm[i].case_result_num) ? "" : "*",
					   pp_code_to_kstr(ellipsis_result_ctm[i].cf_ptr->pp[ellipsis_result_ctm[i].cf_element_num[j]][0]),
					   (entity_manager.entity + ellipsis_result_ctm[i].entity_num[j])->name,
					   (entity_manager.entity + ellipsis_result_ctm[i].entity_num[j])->num);
			}
			for (j = 0; j < ellipsis_result_ctm[i].cf_ptr->element_num; j++) {
				if (!ellipsis_result_ctm[i].filled_element[j] && 
					match_ellipsis_case(pp_code_to_kstr(ellipsis_result_ctm[i].cf_ptr->pp[j][0]), NULL)) 
					printf(" %s:%s", pp_code_to_kstr(ellipsis_result_ctm[i].cf_ptr->pp[j][0]),
						   (ellipsis_result_ctm[i].cf_ptr->oblig[j]) ? "×" : "-");
			}	    
			
			printf(" (0:%.2f*%.2f", ellipsis_result_ctm[i].overt_arguments_score,overt_arguments_weight);
			printf(" 1:%.2f", ellipsis_result_ctm[i].all_arguments_score);
			for (j = 0; j < ELLIPSIS_CASE_NUM; j++) {
				printf("|%s", ELLIPSIS_CASE_LIST_VERB[j]);
				for (k = 0; k < O_FEATURE_NUM; k++) {
					if (ellipsis_result_ctm[i].omit_feature[j][k] != INITIAL_SCORE && ellipsis_result_ctm[i].omit_feature[j][k] != 0)
					{
						char feature_name[DATA_LEN] ="";
						
						if(k%EACH_FEARUTE_NUM == AUTHOR_SCORE)
						{
							strcpy(feature_name ,"A");
						}
						if(k%EACH_FEARUTE_NUM == READER_SCORE)
						{
							strcpy(feature_name ,"R");
						}
						if(k%EACH_FEARUTE_NUM > LOCATION_S && k%EACH_FEARUTE_NUM < (LOCATION_S+LOCATION_NUM*3) )
						{
							sprintf(feature_name,"L%d",(k%EACH_FEARUTE_NUM-LOCATION_S)/3);
						}
						if(k%EACH_FEARUTE_NUM > MODALITY_S && k%EACH_FEARUTE_NUM < (MODALITY_S+MODALITY_F_NUM))
						{
							sprintf(feature_name,"M%d",(k%EACH_FEARUTE_NUM-MODALITY_S)/2);
						}
						if(k%EACH_FEARUTE_NUM > KEIGO_S && k%EACH_FEARUTE_NUM < (KEIGO_S+KEIGO_F_NUM))
						{
							sprintf(feature_name,"H%d",(k%EACH_FEARUTE_NUM-KEIGO_S)/2);
						}
						if(k%EACH_FEARUTE_NUM > CONTEXT_S && k%EACH_FEARUTE_NUM < (CONTEXT_S+CONTEXT_FEATURE_NUM))
						{
							sprintf(feature_name,"C%d",(k%EACH_FEARUTE_NUM-CONTEXT_S));
						}
						
						//if(!(k%EACH_FEARUTE_NUM > LOCATION_S && k%EACH_FEARUTE_NUM < SIMPLE_LOCATION_S))
						{
							printf(",%s:%d:%d:%.2f*%.2f", feature_name, k,k%EACH_FEARUTE_NUM , ellipsis_result_ctm[i].omit_feature[j][k],case_feature_weight[j][k]);
						}
					}
				}
			}
			printf(")");
			
			printf("\n");

		}

	}

	/* BEST解を保存 */
	if (ellipsis_result_ctm[0].score == INITIAL_SCORE) return FALSE;
	if(gs_ctm_ptr == NULL)
	{
		copy_ctm(&ellipsis_result_ctm[0], tag_ptr->ctm_ptr);
		strcpy(tag_ptr->mention_mgr.cf_id, ellipsis_result_ctm[0].cf_ptr->cf_id);
		tag_ptr->mention_mgr.cf_ptr = ellipsis_result_ctm[0].cf_ptr;
	}
	else
	{
		/*正解データがあればそれを保存*/
		copy_ctm(gs_ctm_ptr, tag_ptr->ctm_ptr);
		strcpy(tag_ptr->mention_mgr.cf_id, gs_ctm_ptr->cf_ptr->cf_id);
		
		tag_ptr->mention_mgr.cf_ptr = gs_ctm_ptr->cf_ptr;
	}
	/* 格フレームを解放 */
	free(cf_array);


	return TRUE;

}




/*==================================================================*/
ENTITY *make_each_unnamed_entity(char *name)
/*==================================================================*/
{
	ENTITY *entity_ptr;
	char *temp;
	
	entity_ptr = entity_manager.entity + entity_manager.num;
	entity_ptr->num = entity_ptr->output_num = entity_manager.num;
	entity_manager.num++; 
	entity_ptr->hypothetical_flag = 1;
	entity_ptr->real_entity = -1;
	entity_ptr->link_entity = -1;
	entity_ptr->mentioned_num = 0; 
	entity_ptr->first_appearance = 0;
	strcpy(entity_ptr->named_entity,"");
	strcpy(entity_ptr->name,name); 
	entity_ptr->skip_flag=0;
	entity_ptr->salience_score=1;
	entity_ptr->corefer_id = -1;
	entity_ptr->rep_tag_num = -1;
	entity_ptr->rep_sen_num = -1;
	return entity_ptr;
}
	
void make_unnamed_entity()
{
	int i;
	for (i=0;i<UNNAMED_ENTITY_NUM;i++)
	{
		make_each_unnamed_entity(unnamed_entity[i]);
	}
}


/*==================================================================*/
int make_new_entity(TAG_DATA *tag_ptr, MENTION_MGR *mention_mgr)
/*==================================================================*/
{    
	char *cp,*rep;
	ENTITY *entity_ptr;
	int i,j;
	int corefer_id = -1;

	
	entity_ptr = entity_manager.entity + entity_manager.num;
	entity_ptr->num = entity_ptr->output_num = entity_manager.num;
	entity_manager.num++;				
	entity_ptr->mention[0] = mention_mgr->mention;
	entity_ptr->mentioned_num = 1;
	entity_ptr->hypothetical_flag=0;
	entity_ptr->real_entity = -1;
	entity_ptr->hypothetical_entity=-1;
	strcpy(entity_ptr->hypothetical_name,"");
	entity_ptr->skip_flag = 0;
	entity_ptr->link_entity = -1;
	entity_ptr->corefer_id = -1;
	entity_ptr->first_appearance = mention_mgr->mention->sent_num;
	entity_ptr->rep_sen_num = mention_mgr->mention->sent_num;
	entity_ptr->rep_tag_num = tag_ptr->num;

	/* 先行詞になりやすさ(基本的に文節主辞なら1) */
	entity_ptr->salience_score = 
		(tag_ptr->inum > 0 || /* 文節内最後の基本句でない */
		 !check_feature(tag_ptr->f, "照応詞候補") ||
		 check_feature(tag_ptr->f, "NE内")) ? 0 : 
		((check_feature(tag_ptr->f, "ハ") || check_feature(tag_ptr->f, "モ")) &&
		 !check_feature(tag_ptr->f, "括弧終") ||
		 check_feature(tag_ptr->f, "文末")) ? SALIENCE_THEMA : /* 文末 */
		(check_feature(tag_ptr->f, "読点") && tag_ptr->para_type != PARA_NORMAL ||
		 check_feature(tag_ptr->b_ptr->f, "文頭") ||
		 check_feature(tag_ptr->f, "係:ガ格") ||
		 check_feature(tag_ptr->f, "係:ヲ格")) ? SALIENCE_CANDIDATE : SALIENCE_NORMAL;
	if (check_feature(tag_ptr->f, "係:ニ格") || check_feature(tag_ptr->f, "係:ノ格"))
	{
		entity_ptr->tmp_salience_flag = 1;
	}
	entity_ptr->mention[0]->static_salience_score = entity_ptr->salience_score;
	/* ENTITYの名前 */
	if (cp = check_feature(tag_ptr->f, "NE")) {
		strcpy(entity_ptr->named_entity, cp + strlen("NE:"));
		abbreviate_NE(cp);
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
	mention_mgr->mention->type = 'S'; /* 自分自身 */   
	if (OptReadFeature & OPT_COREFER_AUTO) {
		if(cp = check_feature(tag_ptr->f,"COREFER_ID"))
		{
			sscanf(cp,"COREFER_ID:%d",&corefer_id);
			entity_ptr->corefer_id = corefer_id;

		}
	}

}

/*==================================================================*/
void print_all_location_category(TAG_DATA *tag_ptr)
/*==================================================================*/
{
	int i, j, diff_sen;
	char *cp, type, rel[SMALL_DATA_LEN], loc_name[SMALL_DATA_LEN];
	ENTITY *entity_ptr;
	MENTION *mention_ptr;

	for (i = 0; i < entity_manager.num; i++) {
		mention_ptr = substance_tag_ptr(tag_ptr)->mention_mgr.mention;
		entity_ptr = entity_manager.entity + i;
	
		if (entity_ptr->salience_score == 0) continue;
			
		/* 何文以内にmentionを持っているかどうかのチェック */
		diff_sen = 4;
		for (j = 0; j < entity_ptr->mentioned_num; j++) {
			if (mention_ptr->sent_num == entity_ptr->mention[j]->sent_num &&
				loc_category[(entity_ptr->mention[j]->tag_ptr)->b_ptr->num] == LOC_SELF) continue;
	    
			if (mention_ptr->sent_num - entity_ptr->mention[j]->sent_num < diff_sen)
				diff_sen = mention_ptr->sent_num - entity_ptr->mention[j]->sent_num;
		}
	
		for (j = 0; j < entity_ptr->mentioned_num; j++) {
			/* もっとも近くの文に出現したmentionのみ出力 */
			if (mention_ptr->sent_num - entity_ptr->mention[j]->sent_num > diff_sen)
				continue;
	    
			if ( /* 自分自身はのぞく */
				entity_ptr->mention[j]->sent_num == mention_ptr->sent_num &&
				loc_category[(entity_ptr->mention[j]->tag_ptr)->b_ptr->num] == LOC_SELF) continue;
	    
			if (get_location(loc_name, mention_ptr->sent_num, 
							 check_analyze_tag(tag_ptr, FALSE) == CF_PRED ? "動" : "名",
							 entity_ptr->mention[j], FALSE)) {
				printf(";;LOCATION-ALL: %s", loc_name);
				printf(" %s ",entity_ptr->name);
				if (cp = check_feature(tag_ptr->f, "格解析結果")) {		
					for (cp = strchr(cp + strlen("格解析結果:"), ':') + 1; *cp; cp++) {
						if (*cp == ':' || *cp == ';') {
							if (sscanf(cp + 1, "%[^/]/%c/", rel, &type) &&
								match_ellipsis_case(rel, NULL) && (type == 'C' || type == 'N')) {
								printf(" -%s", rel);
							}
						}
					}
				}
				printf("\n");
			}
		}
	}
}

/*==================================================================*/
int make_context_structure(SENTENCE_DATA *sp)
/*==================================================================*/
{
	/* 共参照解析結果を読み込み、省略解析を行い文の構造を構築する */
	int i, j, check_result;
	char *cp;
	TAG_DATA *tag_ptr;
	CF_PRED_MGR *cpm_ptr;
	
	int analysis_bnst_cueue[BNST_MAX];//文節を解析する順番を保持する
	int bnst_cueue_idx;
	/*一文毎の解析のオプションを切り分けた時に条件を付ける*/
	if(0)
	{
		make_entity_from_coreference(sp);
	}

	
	set_bnst_cueue(analysis_bnst_cueue,sp);
	
	/* 省略解析を行う場合 */
	for (bnst_cueue_idx = 0; bnst_cueue_idx < BNST_MAX;bnst_cueue_idx++)
	{
		int bnst_idx =analysis_bnst_cueue[bnst_cueue_idx];
		int tag_num;

		if(bnst_idx == -1)
		{
			continue ;
		}
		for (tag_num = (sp->bnst_data+bnst_idx)->tag_num-1;tag_num >= 0;tag_num--)
		{
			i = ((sp->bnst_data+bnst_idx)->tag_ptr+tag_num)->num;
			tag_ptr = substance_tag_ptr(sp->tag_data + i);
			check_result = check_analyze_tag(tag_ptr, FALSE);
			if (!(OptReadFeature & OPT_ALL_CASE) && !check_result) continue;	    
			/* 解析対象格の設定 */
			ELLIPSIS_CASE_LIST = (check_result == CF_PRED) ?
				ELLIPSIS_CASE_LIST_VERB : ELLIPSIS_CASE_LIST_NOUN;
			
			/* 省略のMENTIONの処理 */
			/* 入力から正解を読み込む場合 */
			if (OptAnaphora & OPT_TRAIN) {
				for (j = 0; j < entity_manager.num; j++) entity_manager.entity[j].salience_mem = 0;
			}
			if ((OptReadFeature & OPT_ALL_CASE)||check_result == CF_PRED && (OptReadFeature & OPT_ELLIPSIS) || 
				check_result == CF_PRED && (OptReadFeature & OPT_CASE_ANALYSIS) || 
				check_result == CF_NOUN && (OptReadFeature & OPT_REL_NOUN)) {

				/* この時点での各EntityのSALIENCE出力 */
				if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
					printf(";;SALIENCE-%d-%d", sp->Sen_num, i);
					for (j = 0; j < entity_manager.num; j++) {
						printf(":%.3f", (entity_manager.entity + j)->salience_score);
						if(j == 0 && (OptAnaphora & OPT_AUTHOR_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
						{
							printf(";%.3f",author_score);
						}
						if(j == 1 && (OptAnaphora & OPT_READER_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
						{
							printf(";%.3f",reader_score);
						}
						
					}
					printf("\n");
				}

				if(!(OptAnaphora & OPT_ITERATIVE) || !analysis_flag)
				{
					analysis_flags[tag_ptr->mention_mgr.mention->sent_num][tag_ptr->mention_mgr.mention->tag_num] = 1;
		
					/* featureから格解析結果を取得 */
					if (cp = check_feature(tag_ptr->f, "格解析結果")) {		
						
						/* 共参照関係にある表現は格解析結果を取得しない */
						if (check_feature(tag_ptr->f, "体言") &&
							(strstr(cp, "=/") || strstr(cp, "=構/") || strstr(cp, "=役/"))) {
							assign_cfeature(&(tag_ptr->f), "共参照", FALSE);
							continue;
						}
						
						for (cp = strchr(cp + strlen("格解析結果:"), ':') + 1; *cp; cp++) {
							if (*cp == ':' || *cp == ';') {
								read_one_annotation(sp, tag_ptr, cp + 1, FALSE);
							}
						}
					}
				}
			}	
			
			/* 省略解析を行う場合、または、素性を出力する場合 */
			if (check_result == CF_PRED && !(OptReadFeature & OPT_ELLIPSIS) ||
				check_result == CF_NOUN && !(OptReadFeature & OPT_REL_NOUN) ||
				(OptAnaphora & OPT_TRAIN) && (!(OptAnaphora & OPT_ITERATIVE) || analysis_flag)) {
				
				if (tag_ptr->cf_ptr) {
					
					assign_cfeature(&(tag_ptr->f), "Ｔ省略解析", FALSE);
					
					/* cpm_ptrの作成(基本的にはtcf_ptrを使用するが、set_tag_case_frameの呼び出し、および、
					   get_ex_probability_with_para内でtcf_ptr->cf.pred_b_ptr->cpm_ptrとして使用している) */
					cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "make_context_structure: cpm_ptr");
					init_case_frame(&(cpm_ptr->cf));
					cpm_ptr->pred_b_ptr = tag_ptr;
					
					/* tag_ptr->tcf_ptrを作成 */
					tag_ptr->tcf_ptr = (TAG_CASE_FRAME *) malloc_data(sizeof(TAG_CASE_FRAME), "make_context_structure: tcf_ptr");
					set_tag_case_frame(sp, tag_ptr, cpm_ptr);
					
					/* 位置カテゴリの生成 */	    
					mark_loc_category(sp, tag_ptr);
					if (OptAnaphora & OPT_TRAIN) { /* 存在するすべての位置カテゴリを出力 */
						print_all_location_category(tag_ptr); 
					}
					
					/* この時点での各EntityのSALIENCE出力 */
					if (OptDisplay == OPT_DEBUG || OptExpress == OPT_TABLE) {
						printf(";;SALIENCE-%d-%d", sp->Sen_num, i);
						for (j = 0; j < entity_manager.num; j++) {
							printf(":%.3f", (entity_manager.entity + j)->salience_score);
							if(j == 0 && (OptAnaphora & OPT_AUTHOR_SCORE) &&(OptAnaphora & OPT_UNNAMED_ENTITY))
							{
								printf(";%.3f",author_score);
							}
							if(j == 1 && (OptAnaphora & OPT_READER_SCORE) && (OptAnaphora & OPT_UNNAMED_ENTITY))
							{
								printf(";%.3f",reader_score);
							}
						}
						printf("\n");
					} 
					
					/* 省略解析メイン */
					tag_ptr->ctm_ptr = (CF_TAG_MGR *)malloc_data(sizeof(CF_TAG_MGR), "make_context_structure: ctm_ptr");
					tag_ptr->ctm_ptr->score = INITIAL_SCORE;
					ellipsis_analysis_main(tag_ptr);
					
					if (!(OptAnaphora & OPT_TRAIN) &&
						tag_ptr->ctm_ptr->score != INITIAL_SCORE) {
						double ellipsis_score = tag_ptr->ctm_ptr->score;
						if(!(OptReadFeature & OPT_COREFER_AUTO))
						{
							expand_result_to_parallel_entity(tag_ptr); /* 並列要素を展開する */
						}
						
						tag_ptr->score_diff = 100;
						for(j=1;j <ELLIPSIS_RESULT_MAX;j++)
						{
							if(ellipsis_result_ctm[j].score == INITIAL_SCORE)
							{
								break;
							}
							if(tag_ptr->ctm_ptr->ga_entity != ellipsis_result_ctm[j].ga_entity)
							{
								tag_ptr->ga_score_diff = ellipsis_score - ellipsis_result_ctm[j].score;
							}
						}
						tag_ptr->score_diff = ellipsis_result_ctm[0].score - ellipsis_result_ctm[1].score;
						if(!(OptAnaphora & OPT_ITERATIVE))
						{
							anaphora_result_to_entity(tag_ptr); /* 解析結果をENTITYと関連付ける */
				
							analysis_flags[tag_ptr->mention_mgr.mention->sent_num][tag_ptr->mention_mgr.mention->tag_num] = 1;
						}
						else
						{
							if(tag_ptr->score_diff > max_reliabirity && !check_feature(tag_ptr->f,"省略解析済み"))
							{
								if(max_reliabirity_tag_ptr != NULL)
								{
									free(max_reliabirity_tag_ptr->ctm_ptr);
									free(max_reliabirity_tag_ptr->tcf_ptr);
								}

								max_reliabirity = tag_ptr->score_diff;
								max_reliabirity_tag_ptr = tag_ptr;
							}
							else
							{
								free(tag_ptr->ctm_ptr);
								free(tag_ptr->tcf_ptr);

							}
						}
						
					}

					/* メモリを解放 */
					//free(tag_ptr->ctm_ptr);
					//free(tag_ptr->tcf_ptr);
					clear_case_frame(&(cpm_ptr->cf));
					free(tag_ptr->cpm_ptr);
				}
			}
		}
	}
}

/*==================================================================*/
void print_entities(int sen_idx)
/*==================================================================*/
{
	int i, j;
	char *cp;
	MENTION *mention_ptr;
	ENTITY *entity_ptr;
	FEATURE *fp;
	MRPH_DATA m;
	
	printf(";;\n;;SENTENCE %d\n", sen_idx + base_sentence_num); 
	for (i = 0; i < entity_manager.num; i++) {
		entity_ptr = entity_manager.entity + i;
		if(!(OptAnaphora & OPT_UNNAMED_ENTITY) && ((OptAnaphora & OPT_GS) || (OptAnaphora & OPT_TRAIN)))
		{
			if(i < UNNAMED_ENTITY_NUM )
			{
				continue;
			}
		}
		if(OptZeroPronoun ==1)
		{
			/*--------------------------------------------------------------*/
			//entity 全てのEntity_numを1にすることで、Zero-pronounの精度を測る
			printf(";; ENTITY %d [ %s ] %f {\n", 1, entity_ptr->name, entity_ptr->salience_score);
			/*--------------------------------------------------------------*/
		}
		else if(!(OptAnaphora & OPT_UNNAMED_ENTITY) && ((OptAnaphora & OPT_GS) || (OptAnaphora & OPT_TRAIN)))
		{
			printf(";; ENTITY %d [ %s ] %f {\n", entity_ptr->output_num + base_entity_num - UNNAMED_ENTITY_NUM, entity_ptr->name, entity_ptr->salience_score);
		}
		else
		{
			printf(";; ENTITY %d [ %s ] %f {\n", entity_ptr->output_num + base_entity_num, entity_ptr->name, entity_ptr->salience_score);
		}
		
		


		for (j = 0; j < entity_ptr->mentioned_num; j++) {
			mention_ptr = entity_ptr->mention[j];
			printf(";;\tMENTION%3d {", j);
			printf(" SEN:%3d", mention_ptr->sent_num + base_sentence_num);
			printf(" TAG:%3d", mention_ptr->tag_num);
			printf(" (%3d)", mention_ptr->tag_ptr->head_ptr->Num);
			printf(" CPP: %4s", mention_ptr->cpp_string);
			printf(" SPP: %4s", mention_ptr->spp_string);
			printf(" TYPE: %c", mention_ptr->type);
			printf(" SS: %.3f", mention_ptr->salience_score);
			printf(" WORD: %s", mention_ptr->tag_ptr->head_ptr->Goi2);

			/* 格フレームのカバレッジを調べる際に必要となる情報 */
			if (OptDisplay == OPT_DETAIL) {

				/* 用言の場合 */
				if (check_feature(mention_ptr->tag_ptr->f, "用言") &&
					(mention_ptr->type == 'E' ||mention_ptr->type == 'C' || mention_ptr->type == 'N' || mention_ptr->type == 'O')) {
					
					printf(" POS: %s", check_feature(mention_ptr->tag_ptr->f, "用言") + strlen("用言:"));
					if (OptCaseFlag & OPT_CASE_USE_CV_CF) {
						cp = make_pred_string_from_mrph(mention_ptr->tag_ptr, NULL, NULL, OptCaseFlag & OPT_CASE_USE_REP_CF, CF_PRED, FALSE);
					}
					else {
						cp = make_pred_string(mention_ptr->tag_ptr, NULL, NULL, OptCaseFlag & OPT_CASE_USE_REP_CF, CF_PRED, FALSE);
					}
					printf(" KEY: %s", cp);

					/* 代表表記が曖昧な用言の場合 */
					if (check_feature(mention_ptr->tag_ptr->head_ptr->f, "原形曖昧")) {
			
						fp = mention_ptr->tag_ptr->head_ptr->f;
						while (fp) {
							if (!strncmp(fp->cp, "ALT-", 4)) {
								sscanf(fp->cp + 4, "%[^-]-%[^-]-%[^-]-%d-%d-%d-%d-%[^\n]", 
									   m.Goi2, m.Yomi, m.Goi, 
									   &m.Hinshi, &m.Bunrui, 
									   &m.Katuyou_Kata, &m.Katuyou_Kei, m.Imi);
								if (OptCaseFlag & OPT_CASE_USE_CV_CF) {
									cp = make_pred_string_from_mrph(mention_ptr->tag_ptr, &m, NULL, OptCaseFlag & OPT_CASE_USE_REP_CF, CF_PRED, FALSE);
								}
								else {
									cp = make_pred_string(mention_ptr->tag_ptr, &m, NULL, OptCaseFlag & OPT_CASE_USE_REP_CF, CF_PRED, FALSE);
								}
								printf("-%s", cp);
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
				else if (mention_ptr->type == 'S' || mention_ptr->type == '=') {

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
			if(OptAnaphora & OPT_ITERATIVE)
			{
				if(cp = check_feature(mention_ptr->tag_ptr->f, "解析順序"))
				{
					printf(" ITE: %s",cp+strlen("解析順序:"));
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
	int i, j,k, count;
	char buf[DATA_LEN], tmp[IMI_MAX];
	MENTION *mention_ptr;
	TAG_DATA *tag_ptr;
	     
	for (i = 0; i < sp->Tag_num; i++) {
		tag_ptr = substance_tag_ptr(sp->tag_data + i);
		
		
		sprintf(buf, "EID:%d", tag_ptr->mention_mgr.mention->entity->num + base_entity_num);
		assign_cfeature(&(tag_ptr->f), buf, FALSE);
		if (!check_feature(tag_ptr->f, "Ｔ省略解析")) continue;
		if(!(tag_ptr->mention_mgr.cf_ptr)) continue;
		buf[0] = '\0';
		int ga2_write_flag = 0;
		for (j=0;j < tag_ptr->mention_mgr.cf_ptr->element_num;j++)
		{
			int filled_flag =0;
			int ga2_flag=0;
			if(ga2_write_flag == 0)
			{
				if(!MatchPP(tag_ptr->mention_mgr.cf_ptr->pp[j][0],"ガ２"))
				{
					if(j == tag_ptr->mention_mgr.cf_ptr->element_num-1)
					{
						ga2_write_flag =1;
						ga2_flag =1;
						j =-1;
					}

					continue;
				}
				else
				{
					ga2_write_flag =1;
					ga2_flag =1;
				}
			}
			else
			{
				if(MatchPP(tag_ptr->mention_mgr.cf_ptr->pp[j][0],"ガ２"))
				{
					continue;
				}
			}
			for(k=0; k <tag_ptr->mention_mgr.num;k++)
			{
				mention_ptr = tag_ptr->mention_mgr.mention + k;
				if (mention_ptr->type == 'N' || mention_ptr->type == 'C' ||
					mention_ptr->type == 'O' || mention_ptr->type == 'D' || mention_ptr->type == 'E') {

					if(pp_kstr_to_code(mention_ptr->cpp_string)==tag_ptr->mention_mgr.cf_ptr->pp[j][0])
					{
						filled_flag =1;

						if (OptDisplay == OPT_SIMPLE)
						{
							char ellipsis_flag[SMALL_DATA_LEN] ="";
							if(mention_ptr->type == 'O' || mention_ptr->type == 'E')
							{
								strcpy(ellipsis_flag,"*");
							}
							if (!buf[0]) 
							{
								sprintf(buf,"格解析結果:");
							}
							else
							{
								strcat(buf,";");
							}
							sprintf(tmp,"%s%s/%s",mention_ptr->cpp_string,ellipsis_flag,mention_ptr->entity->name);
							strcat(buf,tmp);
						}
						else
						{
							if (!buf[0]) 
							{
								sprintf(buf,"格解析結果:%s:",(OptReadFeature & OPT_ELLIPSIS) ? "?" : tag_ptr->mention_mgr.cf_id);
							}
							else
							{
								strcat(buf,";");								
							}
							sprintf(tmp,"%s/%c/%s/%d/%d/%d",mention_ptr->cpp_string,mention_ptr->type,mention_ptr->entity->name,sp->Sen_num-mention_ptr->entity->rep_sen_num,mention_ptr->entity->rep_tag_num,mention_ptr->entity->num);
							strcat(buf,tmp);
						}
					}
				}
			}
			if(filled_flag ==0 )
			{
				if(OptDisplay == OPT_SIMPLE) 
				{
					if  (tag_ptr->mention_mgr.cf_ptr->oblig[j] == TRUE && !MatchPP(tag_ptr->mention_mgr.cf_ptr->pp[j][0] ,"修飾") && !MatchPP(tag_ptr->mention_mgr.cf_ptr->pp[j][0] ,"外の関係"))
					{
						if (!buf[0]) 
						{
							sprintf(buf,"格解析結果:");
						}
						else
						{
							strcat(buf,";");
						}
						
						sprintf(tmp,"%s/-",pp_code_to_kstr(tag_ptr->mention_mgr.cf_ptr->pp[j][0]));
						strcat(buf,tmp);
					}
				}
				else
				{
					if (!buf[0]) 
					{
						sprintf(buf,"格解析結果:%s:",(OptReadFeature & OPT_ELLIPSIS) ? "?" : tag_ptr->mention_mgr.cf_id);
					}
					else
					{
						strcat(buf,";");								
					}
					
					sprintf(tmp,"%s/-/-/-/-/-",pp_code_to_kstr(tag_ptr->mention_mgr.cf_ptr->pp[j][0]));
					strcat(buf,tmp);
				}
			}

			if(ga2_flag ==1)
			{
				j =-1;
			}
		}
		if (buf[0])
		{
			assign_cfeature(&(tag_ptr->f), buf, FALSE);
			
			sprintf(buf,"省略解析信頼度:%.3f",tag_ptr->score_diff);
			assign_cfeature(&(tag_ptr->f), buf, FALSE);
			sprintf(buf,"ガ格省略解析信頼度:%.3f",tag_ptr->ga_score_diff);
			assign_cfeature(&(tag_ptr->f), buf, FALSE);
			
		}
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
		entity_manager.entity[i].tmp_salience_flag = 0;
	}
}

/*==================================================================*/
void calculate_salience_score(int sen_idx)
/*==================================================================*/
{
	/* 各ENTITYのその文でのsalience_scoreを計算する */
	int entity_num;
	int mention_num;
	int sentence_distance;

	for (entity_num = 0; entity_num < entity_manager.num; entity_num++) 
	{
		if((OptAnaphora & OPT_UNNAMED_ENTITY) &&(entity_num < UNNAMED_ENTITY_NUM) || (entity_manager.entity[entity_num].hypothetical_entity != -1 && entity_manager.entity[entity_num].hypothetical_entity < UNNAMED_ENTITY_NUM) || strcmp(entity_manager.entity[entity_num].named_entity,""))
		{
			entity_manager.entity[entity_num].salience_score = 1;
		}
		else
		{
			entity_manager.entity[entity_num].salience_score = 0;
		}
		
		for ( mention_num =0;mention_num < entity_manager.entity[entity_num].mentioned_num;mention_num++)
		{
			if(entity_manager.entity[entity_num].mention[mention_num] ==NULL)
			{
				continue; 
			}
			sentence_distance = sen_idx - entity_manager.entity[entity_num].mention[mention_num]->sent_num;
			if(sentence_distance >= 0)
			{
				entity_manager.entity[entity_num].salience_score +=  entity_manager.entity[entity_num].mention[mention_num]->static_salience_score *pow( SALIENCE_DECAY_RATE,sentence_distance);
			}
		}
		entity_manager.entity[entity_num].tmp_salience_flag = 0;
	}
}



/*==================================================================*/
void anaphora_analysis(int sen_idx)
/*==================================================================*/
{

	if(!(OptAnaphora & OPT_ONLY_ENTITY))
	{
		calculate_salience_score(sen_idx);
		set_candidate_entities(sen_idx);
		make_context_structure(sentence_data + sen_idx - 1);
	}
	assign_anaphora_result(sentence_data + sen_idx - 1);
	if(!(OptReadFeature & OPT_COREFER_AUTO) && !(OptAnaphora & OPT_ONLY_ENTITY))
	{
		if (OptAnaphora & OPT_PRINT_ENTITY) print_entities(sen_idx);
	}

}


/*==================================================================*/
void modify_weight(void)
/*==================================================================*/
{
	if (ModifyWeight[0]) {
		case_feature_weight[0][ASSIGNED] += ModifyWeight[0];
		case_feature_weight[1][ASSIGNED] += ModifyWeight[1];
		case_feature_weight[2][ASSIGNED] += ModifyWeight[2];
		ModifyWeight[0] = 0;
		/*複数回呼ばれても変化しないようにフラグ管理もかねる */  
	}
}


void count_yobikake(SENTENCE_DATA *last_sp)
{
	SENTENCE_DATA *sp;
	int sen_idx;
	int mrph_idx;

	char *cp;
	for (sen_idx =1; sen_idx < last_sp->Sen_num;sen_idx++)
	{
		sp = sentence_data + sen_idx - 1;
		for (mrph_idx = 0;mrph_idx < sp->Mrph_num;mrph_idx++)
		{
			cp=check_feature((sp->mrph_data+mrph_idx)->f,"呼掛") ;
			if(cp != NULL)
			{
				yobikake_count++;
			}
		}
	}

}

void count_modality_keigo(SENTENCE_DATA *last_sp)
{
	SENTENCE_DATA *sp;
	int sen_idx;
	int i,j;
	for (j=0;j<MODALITY_NUM;j++)
	{
		modality_count[j] =0;
	}
	for (j=0;j<KEIGO_NUM;j++)
	{
		keigo_count[j] =0;
	}
	for (sen_idx =1; sen_idx < last_sp->Sen_num;sen_idx++)
	{
		sp = sentence_data + sen_idx - 1;
		for (i = sp->Tag_num - 1; i >= 0; i--) {
			TAG_DATA *tag_ptr;

			tag_ptr = substance_tag_ptr(sp->tag_data + i);
			for (j=0;j<MODALITY_NUM;j++)
			{

				char mod[DATA_LEN] = "モダリティ-";
				char *cp;
				strcat(mod,modality[j]);
				cp = check_feature(tag_ptr->f,mod);
				if(cp != NULL)
				{
					modality_count[j]++;
				}
			}
			for (j=0;j<KEIGO_NUM;j++)
			{
				char kei[DATA_LEN] = "敬語:";
				char *cp;
				strcat(kei,keigo[j]);
				cp =check_feature(tag_ptr->f,kei);
				if(cp != NULL)
				{
					keigo_count[j]++;
				}

			}
		}
	}
		
		
}


void get_context_feature(void)
{
	int entity_idx;

	for (entity_idx =0;entity_idx<entity_manager.num;entity_idx++)
	{

		int mention_idx;
		int i;

		for (i =0;i <CONTEXT_FEATURE_NUM;i++)
		{
			context_feature[entity_manager.entity[entity_idx].num][i] = 0;
		}
		if(entity_manager.entity[entity_idx].skip_flag ==1)
		{
			continue;
		}
		/*
		  一文目での出現
		  文末 判定詞0
		  文末 体言止め1
		  文末 2
		  文頭 3
		  文頭 ハ4
		  文内 5
		  文内 ガ格6
		  文内 ヲ格7
		  文内 ニ格8
		  文内 ハ9
		*/
		
		for (mention_idx =0;mention_idx < entity_manager.entity[entity_idx].mentioned_num;mention_idx++)
		{
			MENTION *mention_ptr;
			mention_ptr = entity_manager.entity[entity_idx].mention[mention_idx];
			if(mention_ptr->sent_num == 1)
			{

				if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"文末"))
				{
					if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"用言:判"))
					{
						context_feature[entity_manager.entity[entity_idx].num][0] = 1;
					}
					if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"体言止"))
					{
						context_feature[entity_manager.entity[entity_idx].num][1] = 1;
					}
					context_feature[entity_manager.entity[entity_idx].num][2] = 1;
				}
				if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"文頭"))
				{

					context_feature[entity_manager.entity[entity_idx].num][3] = 1;
					if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"ハ"))
					{
						context_feature[entity_manager.entity[entity_idx].num][4] = 1;
					}
				}
				//context_feature[entity_manager.entity[entity_idx].num][5] = 1;
				if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"係:ガ格"))
				{
					context_feature[entity_manager.entity[entity_idx].num][6] = 1;
				}	
				if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"係:ヲ格"))
				{
					//context_feature[entity_manager.entity[entity_idx].num][7] = 1;
				}
				if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"係:ニ格"))
				{
					//context_feature[entity_manager.entity[entity_idx].num][8] = 1;
				}
				if(check_feature(mention_ptr->tag_ptr->b_ptr->f,"ハ"))
				{
					context_feature[entity_manager.entity[entity_idx].num][9] = 1;
				}
			}
		}
		
	}

	
}


void set_author_reader(void)
{
	int i;
	if( (OptAnaphora & OPT_TRAIN) || (OptReadFeature & OPT_ELLIPSIS))
	{
		for( i=0;i<UNNAMED_ENTITY_NUM;i++)
		{
			entity_manager.entity[i].skip_flag = 0;
		}

	}
	else if (OptAnaphora & OPT_NO_PSUDE)
	{
		for( i=0;i<UNNAMED_ENTITY_NUM;i++)
		{
			entity_manager.entity[i].skip_flag = 1;
		}
	}
	else
	{
		if ((OptAnaphora & OPT_NO_AUTHOR_ENTITY) || !(OptAnaphora & OPT_UNNAMED_ENTITY))
		{
			entity_manager.entity[0].skip_flag = 1;
		}
		else
		{
			entity_manager.entity[0].skip_flag = 0;			
		}
		if ((OptAnaphora & OPT_NO_READER_ENTITY) || !(OptAnaphora & OPT_UNNAMED_ENTITY))
		{
			entity_manager.entity[1].skip_flag = 1;
		}
		else
		{
			entity_manager.entity[1].skip_flag = 0;
		}
		for( i=2;i<UNNAMED_ENTITY_NUM;i++)
		{
			entity_manager.entity[i].skip_flag = 0;
		}
	}
	
}


void each_sentence_anaphora_analysis(SENTENCE_DATA *sp)
{	
	int sen_idx;
	const int  sen_max = sp->Sen_num;
	if(sen_max ==1)
	{
		set_param();

		/* if((OptAnaphora & OPT_UNNAMED_ENTITY) || (OptAnaphora & OPT_TRAIN) ||(OptAnaphora & OPT_GS) ) */
		{
			make_unnamed_entity();
			set_author_reader();
		}
		if(OptAnaphora & OPT_AUTHOR_SCORE)
		{
			author_score = author_score/(sen_max);
		}
		if(OptAnaphora & OPT_READER_SCORE)
		{
			reader_score = reader_score/(sen_max);
		}
		
	}
	count_yobikake(sp);
	count_modality_keigo(sp);
	make_entity_from_coreference(sentence_data + sen_max - 1);
	if((OptAnaphora & OPT_UNNAMED_ENTITY &&  !(OptAnaphora & OPT_AUTHOR_AFTER))|| (OptAnaphora & OPT_TRAIN) ||(OptAnaphora & OPT_GS) )
	{

		if(OptReadFeature & OPT_AUTHOR_AUTO)
		{
			merge_hypo_real_entity_auto();
		}
		else if((OptReadFeature & OPT_AUTHOR))
		{
			merge_hypo_real_entity();
		}
		else
		{
			author_detect();
		}
	}

	
	get_context_feature();	
	anaphora_analysis(sen_max);
	if (OptAnaphora & OPT_PRINT_ENTITY)
	{
		print_entities(sen_max);
	}
}


/*==================================================================*/
void all_sentence_anaphora_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
	int sen_idx;
	const int  sen_max = sp->Sen_num;
	char buf[SMALL_DATA_LEN];
	set_param();
	if(!(OptAnaphora & OPT_TRAIN))
	{
		//modify_weight();
	}
	//OptAnaphora |= OPT_ITERATIVE;
	count_yobikake(sp);
	count_modality_keigo(sp);
	
	if((OptAnaphora & OPT_UNNAMED_ENTITY) || (OptAnaphora & OPT_TRAIN) ||(OptAnaphora & OPT_GS) )
	{
		make_unnamed_entity();
	}
	for (sen_idx =1; sen_idx <sen_max;sen_idx++)
	{
		make_entity_from_coreference(sentence_data + sen_idx - 1);
	}
	if(OptAnaphora & OPT_AUTHOR_SCORE)
	{
		author_score = author_score/(sen_max-1);
	}
	if(OptAnaphora & OPT_READER_SCORE)
	{
		reader_score = reader_score/(sen_max-1);
	}
	
		
	if((OptAnaphora & OPT_UNNAMED_ENTITY &&  !(OptAnaphora & OPT_AUTHOR_AFTER))|| (OptAnaphora & OPT_TRAIN) ||(OptAnaphora & OPT_GS) )
	{
		set_author_reader();
		if(OptReadFeature & OPT_AUTHOR_AUTO)
		{
			merge_hypo_real_entity_auto();
		}
		else if((OptReadFeature & OPT_AUTHOR))
		{
			merge_hypo_real_entity();
		}
		else
		{
			author_detect();
		}
	}
	
	get_context_feature();

	analysis_flag = 0;
	ite_count =0;
	while(1)
	{
		max_reliabirity = INITIAL_SCORE;
		max_reliabirity_tag_ptr = NULL;

		for (sen_idx =1; sen_idx < sen_max;sen_idx++)
		{
			anaphora_analysis(sen_idx);
		}

		if(OptAnaphora & OPT_ITERATIVE)
		{
			if(OptAnaphora & OPT_TRAIN)
			{
				if(analysis_flag == 0)
				{
					analysis_flag =1;
				}
				else
				{
					break;
				}
			}
			else
			{
				if(max_reliabirity == INITIAL_SCORE)
				{
					break;
				}
				analysis_flags[max_reliabirity_tag_ptr->mention_mgr.mention->sent_num][max_reliabirity_tag_ptr->mention_mgr.mention->tag_num] = 1;
				anaphora_result_to_entity(max_reliabirity_tag_ptr);
				assign_cfeature((&max_reliabirity_tag_ptr->f),"省略解析済み",FALSE);
				sprintf(buf,"解析順序:%d",ite_count);
				assign_cfeature((&max_reliabirity_tag_ptr->f),buf,FALSE);
				free(max_reliabirity_tag_ptr->ctm_ptr);
				free(max_reliabirity_tag_ptr->tcf_ptr);
				ite_count++;
			}
		}
		else
		{
			break;
		}
	}


	if((OptAnaphora & OPT_AUTHOR_AFTER) &&((OptAnaphora & OPT_UNNAMED_ENTITY) || (OptAnaphora & OPT_TRAIN) ||(OptAnaphora & OPT_GS)  ))
	{
		for (sen_idx =1; sen_idx < sen_max;sen_idx++)
		{
			link_hypo_enity_after_analysis(sentence_data + sen_idx -1);
		}
		
		if(OptReadFeature & OPT_AUTHOR_AUTO)
		{
			merge_hypo_real_entity_auto();				
		}
		else if((OptReadFeature & OPT_AUTHOR))
		{
			merge_hypo_real_entity();
		}
		else
		{
			author_detect();
		}
		if (OptAnaphora & OPT_PRINT_ENTITY) print_entities(sen_max-1);
	}
}

/*====================================================================

                                 Îà»÷ÅÙ·×»»

                                                  Liu  06. 10.25
                                                 Modified Version
====================================================================*/
#include "knp.h"

char hownet_word[HOWNET_NUM][HOWNET_WORD_MAX];
char hownet_pos[HOWNET_NUM][HOWNET_POS_MAX];
char hownet_def[HOWNET_NUM][HOWNET_DEF_MAX];
char entity[ENTITY_NUM][ENTITY_MAX];

/*==================================================================*/
                       void hownet_open()
/*==================================================================*/
{
    /*read the Entity table*/
    FILE *fp,*fp2;
    char* dict_file_name;
    char* entity_file_name;
    char string_entity[ENTITY_MAX];
    int entity_flag = 0;
    int hownet_flag = 0;

    dict_file_name = check_dict_filename(HOWNET_DICT, TRUE);
    entity_file_name = check_dict_filename(HOWNET_ENTITY, TRUE);

    fp = fopen(entity_file_name,"r");
    fp2 = fopen(dict_file_name,"r");
    if(fp == NULL){
	fprintf(stderr, ";; Cannot open file (%s) !!\n", fp);
    }
    if(fp2 == NULL){
	fprintf(stderr, ";; Cannot open file (%s) !!\n", fp2);
    } 
    while((fgets(string_entity,ENTITY_MAX,fp)) != NULL && entity_flag < ENTITY_NUM){
	entity_flag++;
	strcpy(entity[entity_flag],string_entity);
    }
    while((fscanf(fp2,"%s %s %s",hownet_word[hownet_flag],hownet_pos[hownet_flag],hownet_def[hownet_flag])) && hownet_flag < HOWNET_NUM){
	hownet_flag++;
    }
    fclose (fp);
    fclose (fp2);
}

/*==================================================================*/
           float step(float a, float b, float c, float d)
/*==================================================================*/
{    
    float sim;
    sim  = a * PARA_1 + b * PARA_2 + c * PARA_3 + d * PARA_4;
    return  sim;
}

/*==================================================================*/
            int def_cmp(char *string1,char *string2)
/*==================================================================*/
{
    if((strstr(string1,string2)) == NULL && (strstr(string2,string1)) == NULL){
	return 0;
    }
    return 1;
}

/*==================================================================*/
              char *pos_modify(BNST_DATA *pos)
/*==================================================================*/
{
    if(check_feature(pos->f,"AD"))
	return "ADV";
    if(check_feature(pos->f,"AS"))
	return "SUFFIX";
    if(check_feature(pos->f,"BA"))
	return "PREP";
    if(check_feature(pos->f,"CC"))
	return "COOR";
    if(check_feature(pos->f,"CD"))
	return "NUM";
    if(check_feature(pos->f,"CS"))
	return "CONJ";
    if(check_feature(pos->f,"DEC"))
	return "STRU";
    if(check_feature(pos->f,"DEG"))
	return "STRU";
    if(check_feature(pos->f,"DER"))
	return "STRU";
    if(check_feature(pos->f,"DEV"))
	return "STRU";
    if(check_feature(pos->f,"DT"))
	return "N";
    if(check_feature(pos->f,"FW"))
	return "N";
    if(check_feature(pos->f,"ETC"))
	return "SUFFIX";
    if(check_feature(pos->f,"IJ"))
	return "ECHO";
    if(check_feature(pos->f,"JJ"))
	return "ADJ";
    if(check_feature(pos->f,"LB"))
	return "STRU";
    if(check_feature(pos->f,"LC"))
	return "P";
    if(check_feature(pos->f,"M"))
	return "CLAS";
    if(check_feature(pos->f,"MSP"))
	return "CLAS";
    if(check_feature(pos->f,"NN"))
	return "N";
    if(check_feature(pos->f,"NR"))
	return "N";
    if(check_feature(pos->f,"NT"))
	return "N";
    if(check_feature(pos->f,"NT-SHORT"))
	return "N";
    if(check_feature(pos->f,"NR-SHORT"))
	return "N";
    if(check_feature(pos->f,"NR-PN"))
	return "N";
    if(check_feature(pos->f,"NN-SBJ"))
	return "N";
    if(check_feature(pos->f,"NN-SHORT"))
	return "N";
    if(check_feature(pos->f,"OD"))
	return "NUM";
    if(check_feature(pos->f,"ON"))
	return "ECHO";
    if(check_feature(pos->f,"P"))
	return "PREP";
    if(check_feature(pos->f,"PN"))
	return "PRON";
    if(check_feature(pos->f,"PU"))
	return "PUNC";
    if(check_feature(pos->f,"SB"))
	return "STRU";
    if(check_feature(pos->f,"SP"))
	return "STRU";
    if(check_feature(pos->f,"VA"))
	return "V";
    if(check_feature(pos->f,"VC"))
	return "V";
    if(check_feature(pos->f,"VE"))
	return "V";
    if(check_feature(pos->f,"VV"))
	return "V";
    
    return NULL;
}


/*==================================================================*/
      float  similarity_chinese(BNST_DATA *ptr1,BNST_DATA *ptr2)
/*==================================================================*/
{
    char *m_input_pos_first,*m_input_pos_second;
    float p1,p2,p3,p4,similarity;
    int check_hownet_flag;
    char *def_first,*def_second;
    char def_first_copy[ENTITY_MAX],def_second_copy[ENTITY_MAX];
    char *def_first_head,*def_second_head;
    int entity_round_flag;
    char *level_first,*level_second;
    char entity_copy1[ENTITY_MAX];
    char entity_copy2[ENTITY_MAX];
    int first_level_num,second_level_num,same_level_num;
    char *level_pointer;
    char buffer_first_level[LEVEL_NUM][LEVEL_MAX];
    char buffer_second_level[LEVEL_NUM][LEVEL_MAX];
    int level_round_flag=1;
    char def_first_copy2[ENTITY_MAX],def_second_copy2[ENTITY_MAX];
    int first_def_num,second_def_num,same_def_num;
    char *def_pointer;
    char buffer_first_def[DEF_NUM][DEF_MAX];
    char buffer_second_def[DEF_NUM][DEF_MAX];
    int def_round_flag;
    int def_round_flag2;
    char word1[HOWNET_WORD_MAX];
    char word2[HOWNET_WORD_MAX];
    char pos1[HOWNET_POS_MAX];
    char pos2[HOWNET_POS_MAX];

    def_round_flag = 1;
    first_level_num = 0;
    second_level_num = 0;
    same_level_num = 0;
    first_def_num = 0;
    second_def_num = 0;
    same_def_num = 0;
    def_first = NULL;
    def_second = NULL;
    def_first_head = NULL;
    def_second_head = NULL;
    level_first = NULL;
    level_second = NULL;
    m_input_pos_first = NULL;
    m_input_pos_second = NULL;
    def_pointer = NULL;
    level_pointer = NULL;
    p1 = 0;
    p2 = 0;
    p3 = 0;
    p4 = 0;
    similarity = 0;
    strcpy(word1,ptr1->head_ptr->Goi);
    strcpy(word2,ptr2->head_ptr->Goi);
    if(pos_modify(ptr1)==NULL || pos_modify(ptr2)==NULL){
      return 0;
    }
    strcpy(pos1,pos_modify(ptr1));
    strcpy(pos2,pos_modify(ptr2));
    m_input_pos_first = pos1;
    m_input_pos_second = pos2;

    /* check the word and pos in the HowNet*/
    if(m_input_pos_first != NULL){
	for(check_hownet_flag = 0;check_hownet_flag < HOWNET_NUM;){
	    if(!strcmp(word1,hownet_word[check_hownet_flag]) && !strcmp(m_input_pos_first,hownet_pos[check_hownet_flag])){
		def_first = hownet_def[check_hownet_flag];
		break;
	    }
	    else{
		check_hownet_flag++;
	    }
	}
    }

    if(m_input_pos_second != NULL){
	for(check_hownet_flag = 0;check_hownet_flag < HOWNET_NUM;check_hownet_flag++){
	    if(!strcmp(word2,hownet_word[check_hownet_flag]) && !strcmp(m_input_pos_second,hownet_pos[check_hownet_flag])){
		def_second = hownet_def[check_hownet_flag];
		break;
	    }
	}
    }
   
    if(def_first == NULL || def_second == NULL){
	if(m_input_pos_first == NULL || m_input_pos_second == NULL){
	    similarity = 0.0;
	    return similarity;
	}
	else{
	    if(!strcmp(m_input_pos_first,m_input_pos_second)){
		similarity = 1.0 * PARA_4;
		return similarity;
	    }
	    else{
		similarity = 0.0;
		return similarity;
	    }
	}
    }
   
/*==================================================================*/
/*step 1:defination comparision                                     */
/*==================================================================*/
  
    if(def_cmp(def_first,def_second) != 0){
	p1 = 1;
    }
    else{
	p1 = 0;
    }

/*==================================================================*/
/*step 2:defination entity level computation                        */
/*==================================================================*/

    strcpy(def_first_copy,def_first);
    strcpy(def_second_copy,def_second);
    def_first_head = strtok(def_first_copy,",");
    def_second_head = strtok(def_second_copy,",");

    for(entity_round_flag = 0;entity_round_flag < ENTITY_NUM;entity_round_flag++){
	if(strstr(entity[entity_round_flag],def_first_head)){
	    strcpy(entity_copy1,entity[entity_round_flag]);
	    level_first = strtok(entity_copy1,"{");
	    break;
	}
    }
    for(entity_round_flag = 0;entity_round_flag < ENTITY_NUM;entity_round_flag++){
	if(strstr(entity[entity_round_flag],def_second_head)){
	    strcpy(entity_copy2,entity[entity_round_flag]);
	    level_second = strtok(entity_copy2,"{");
	    break;
	}
    }

    if(level_first == NULL || level_second == NULL){
    }
    else{
	level_pointer = strtok(level_first,".");
	if(strcpy(buffer_first_level[0],level_pointer)){
	    first_level_num++;
	}
	while((level_pointer = strtok(NULL,"."))){
	    strcpy(buffer_first_level[level_round_flag],level_pointer);
	    first_level_num++;
	    level_round_flag++;
	}

	level_round_flag = 1;
   
	level_pointer = strtok(level_second,".");
  
	if(strcpy(buffer_second_level[0],level_pointer)){
	    second_level_num++;
	}
	while((level_pointer = strtok(NULL,"."))){
	    strcpy(buffer_second_level[level_round_flag],level_pointer);
	    level_round_flag++;
	    second_level_num++;
	}

	if(!strcmp(buffer_first_level[0],buffer_second_level[0])){
	for(level_round_flag = 0;level_round_flag < (first_level_num > second_level_num ? second_level_num : first_level_num);level_round_flag++){
	    if(!strcmp(buffer_first_level[level_round_flag],buffer_second_level[level_round_flag]) && strcmp(buffer_first_level[level_round_flag],"")){
		same_level_num++;
	    }
	    else
		{
		    break;
		}
	}
	p2 = 1.6/(1.6 + first_level_num + second_level_num - 2 * same_level_num);
	}
	else{
	  p2 = 0;
	}
    }

/*==================================================================*/
/*step 3:defination  computation                                    */
/*==================================================================*/
  
    strcpy(def_first_copy2,def_first);
    strcpy(def_second_copy2,def_second);
   
    if(def_first_copy2 == NULL || def_second_copy2 == NULL){
    }
    else{
	def_pointer = strtok(def_first_copy2,",");
	if(strcpy(buffer_first_def[0],def_pointer)){
	    first_def_num++;
	}
	while((def_pointer = strtok(NULL,","))){
	    strcpy(buffer_first_def[def_round_flag],def_pointer);
	    first_def_num++;
	    def_round_flag++;
	}
	def_round_flag = 1;
	def_pointer = strtok(def_second_copy2,",");
	if(strcpy(buffer_second_def[0],def_pointer)){
	    second_def_num++;
	}
	while((def_pointer = strtok(NULL,","))){
	    strcpy(buffer_second_def[def_round_flag],def_pointer);
	    def_round_flag++;
	    second_def_num++;
	}

	for(def_round_flag = 0;def_round_flag < first_def_num;def_round_flag++){
	  for(def_round_flag2 = 0;def_round_flag2 < second_def_num;def_round_flag2++){
	  if(!strcmp(buffer_first_def[def_round_flag],buffer_second_def[def_round_flag2])){
	    same_def_num++;
	  }
	  else{
	    continue;
	  }
	  }
	}

	p3 = 2.0 * same_def_num / ( first_def_num + second_def_num );
	if (p3 > 1) {
	    p3 = 1;
	}
    }

/*==================================================================*/
/*step 4:POS similarity                                             */
/*==================================================================*/

    if(!strcmp(pos1,pos2)){
	p4 = 1;
    }
    else{
	p4 = 0;
    }

/*==================================================================*/
/*step 5:similarity                                                 */
/*==================================================================*/
    return step(p1,p2,p3,p4);
}

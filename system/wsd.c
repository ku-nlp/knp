/*====================================================================

                                 WSD

                                         Daisuke Kawahara 2014. 7. 25

    $Id$
====================================================================*/

#include "knp.h"

const int MAX_RELWORD = 1000;
const int MAX_SEM_NUM = 60;

char	static_buffer1[DATA_LEN];
char	static_buffer2[DATA_LEN];
DBM_FILE relword_db;
int RelWordDbExist;

typedef struct relword {
	char repids[SMALL_DATA_LEN];
	double relatedness;
} RELWORD;

/*==================================================================*/
                void repnames2id(char* cp, char* buf)
/*==================================================================*/
{
    int  boundary_continue_flag, boundary_end_flag;
    char *start_cp = cp, *rep_id;
	*buf='\0';

	for (; ; cp++) {
		boundary_continue_flag = boundary_end_flag = 0;
		if (*cp == '+' || *cp == '?')
			boundary_continue_flag = 1;
		if (*cp == '_' || *cp == '>' || *cp == '\0' )
			boundary_end_flag = 1;
		if (boundary_continue_flag || boundary_end_flag) {
			rep_id = rep2id(start_cp, cp - start_cp, &(static_buffer1[0]));
			strcat(buf, rep_id);
			if (boundary_continue_flag)
				strncat(buf, cp, 1);
			start_cp = cp + 1;
		}
		if (boundary_end_flag)
			break;
	}
}


/*==================================================================*/
 double context_sim( RELWORD* current, RELWORD* context )
/*==================================================================*/
{
	double similarity = -1.0;
	int cur_index, con_index;

	for(cur_index=0; (current+cur_index)->repids[0] ; cur_index++){
		for(con_index=0; (context+con_index)->repids[0] ; con_index++){
			//printf("%s eq? %s\n", (current+cur_index)->repids, (context+con_index)->repids);
			if(!strcmp((current+cur_index)->repids,(context+con_index)->repids) ){
				similarity += 1; //(current+cur_index)->relatedness;
				// relatedness の値がおかしかったので1に固定
				break;
			}
		}
	}
	return similarity;
}

/*==================================================================*/
int unsupervised_classify( RELWORD (*current)[MAX_RELWORD], RELWORD context[], unsigned int current_max )
/*==================================================================*/
{
	double best_sim=-1, current_sim=0;
	int best_index=-1, sem_index ;

	for(sem_index = 0; sem_index< current_max; sem_index++){
		current_sim = context_sim( current[sem_index], context) ;
		//printf("index%d, sim= %f bestsim=%f\n",sem_index, current_sim, best_sim);
		if(current_sim > best_sim){
			best_index = sem_index;
			best_sim = current_sim;
		}
	}
	
	return best_index + 1;
}

/*==================================================================*/
 void setrelword( RELWORD* rw, char * rel_repids, double rel_value )
/*==================================================================*/
{
	strcpy(rw->repids, rel_repids );
    rw->relatedness = rel_value;		

	// オーバーフローのチェック
	(rw+1)->repids[0] = '\0';
}

/*==================================================================*/
                           void init_wsd()
/*==================================================================*/
{
    init_opal(KNP_DICT "/" WSD_MODEL_DIR_NAME);
}


/*==================================================================*/
                           void init_relword_db()
/*==================================================================*/
{
	// フルパス
	char* filename = "/durian/shin/association/rel/140122-2/all-rel"; //check_dict_filename(DICT[ID2LEX_DB], TRUE);

    if (OptDisplay == OPT_DEBUG) {
		fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((relword_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	RelWordDbExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open RelWord DB <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	RelWordDbExist = TRUE;
    }
}
/*==================================================================*/
                           void close_relword_db()
/*==================================================================*/
{
    /* close the db of RelWord */
	if (RelWordDbExist == TRUE) {
		DB_close(relword_db);
	}
}


/*==================================================================*/
                           void close_wsd()
/*==================================================================*/
{
    close_opal();
}

/*==================================================================*/
char *sprint_features_for_wsd(SENTENCE_DATA *sp, int print_class_label_flag)
/*==================================================================*/
{
    int i, second_print_flag = 0;
    MRPH_DATA *m_ptr;
    char *rep_str, *rep_id, *buf = static_buffer2;
    buf[0] = '\0';
    if (print_class_label_flag)
        strcat(buf, "0 ");

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        if ((rep_str = get_mrph_rep_from_f(m_ptr, FALSE))) {
            rep_id = rep2id(rep_str, strlen(rep_str), &(static_buffer1[0]));
            if (rep_id[0]) {
                if (second_print_flag++)
                    strcat(buf, " ");
                strcat(buf, rep_id);
                strcat(buf, ":1");
            }
        }
    }
    return buf;
}

/*==================================================================*/
void get_context_word(SENTENCE_DATA *sp, RELWORD* rw, unsigned int * index)
/*==================================================================*/
{
    int i;
    MRPH_DATA *m_ptr;
    char *rep_str, *rep_id;

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        if ((rep_str = get_mrph_rep_from_f(m_ptr, FALSE))) {
            rep_id = rep2id(rep_str, strlen(rep_str), &(static_buffer1[0]));
            if (rep_id[0]) {
				setrelword( rw+(*index)++ , rep_id, 1 );
            }
        }
    }
}


/*==================================================================*/
            void print_features_for_wsd(SENTENCE_DATA *sp)
/*==================================================================*/
{
    fputs(sprint_features_for_wsd(sp, FALSE), Outfp);
    fputc('\n', Outfp);
}



/*==================================================================*/
                     void wsd_supervised(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, boundary_continue_flag, boundary_end_flag;
    MRPH_DATA *m_ptr;
    FEATURE *fp;
    char *cp, *start_cp, *orig_start_cp, *rep_id;
    char label[DATA_LEN], buf[DATA_LEN], pre_buf[DATA_LEN], feature_buf[DATA_LEN];

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        pre_buf[0] = '\0';
        fp = m_ptr->f;
        while (fp) {
            if (!strncmp(fp->cp, "LD-type=Wikipedia", strlen("LD-type=Wikipedia")) && (cp = strstr(fp->cp, "RepForm="))) {
                buf[0] = '\0';
                cp += strlen("RepForm=");
                orig_start_cp = start_cp = cp;
                for (; ; cp++) {
                    boundary_continue_flag = boundary_end_flag = 0;
                    if (*cp == '+' || *cp == '?')
                        boundary_continue_flag = 1;
                    if (*cp == '_' || *cp == '>')
                        boundary_end_flag = 1;
                    if (boundary_continue_flag || boundary_end_flag) {
                        rep_id = rep2id(start_cp, cp - start_cp, &(static_buffer1[0]));
                        strcat(buf, rep_id);
                        if (boundary_continue_flag)
                            strncat(buf, cp, 1);
                        start_cp = cp + 1;
                    }
                    if (boundary_end_flag)
                        break;
                }

                if (pre_buf[0] && strcmp(pre_buf, buf) && 
                    opal_classify(pre_buf, sprint_features_for_wsd(sp, TRUE), label)) {
                    fprintf(Outfp, "TARGET %s -> LABEL %s\n", pre_buf, label);
                    sprintf(feature_buf, "語義曖昧性解消結果:%s", label);
                    assign_cfeature(&(m_ptr->f), feature_buf, FALSE);
                }
                strcpy(pre_buf, buf);
            }
            fp = fp->next;
        }
        if (pre_buf[0] && 
            opal_classify(pre_buf, sprint_features_for_wsd(sp, TRUE), label)) {
            fprintf(Outfp, "TARGET %s -> LABEL %s\n", pre_buf, label);
            sprintf(feature_buf, "語義曖昧性解消結果:%s", label);
            assign_cfeature(&(m_ptr->f), feature_buf, FALSE);
        }
    }
}


/*==================================================================*/
                 void wsd_unsupervised(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, boundary_continue_flag, boundary_end_flag, context_word_index=0, label;
    MRPH_DATA *m_ptr;
    FEATURE *fp;
    char *cp, *repcp, *start_cp, *orig_start_cp, *rep_id, *ret; 
    char buf[DATA_LEN], repbuf[DATA_LEN], relword_buf[DATA_LEN],pre_repbuf[DATA_LEN], feature_buf[DATA_LEN];
	RELWORD current_word[MAX_SEM_NUM][MAX_RELWORD];
	RELWORD context_word[MAX_RELWORD];
	int sem_index, relword_index;

	// 本当は前の文も見る必要がある
    get_context_word(sp, context_word, &context_word_index);

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        pre_repbuf[0] = '\0';
        fp = m_ptr->f;
		sem_index = 0;
        while (fp) {
            if (!strncmp(fp->cp, "LD-type=Wikipedia", strlen("LD-type=Wikipedia")) && (repcp = strstr(fp->cp, "RepForm=")) && (cp = strstr(fp->cp, "RelWord="))) {
                buf[0] = '\0';//todo: 変数名を適切に
				sscanf(repcp,"RepForm=%[^_>]", repbuf);

				sscanf(cp,"RelWord=%[0-9]", buf);

				ret = db_get(relword_db, buf);

				if(ret){
					double rel_value;
					char *token_start, *token;
					char *relword_start, *relword, *value;
					relword_index=0;
					while ((token = strsep(&ret, ";")) != NULL) {
						sscanf(token,"%[^:]:%f", buf, &rel_value);
						repnames2id(buf, relword_buf);
						if(relword_buf[0] != '\0'){
							setrelword( &(current_word[sem_index][relword_index]), relword_buf, rel_value );
							//printf("buf %s setrelword %s ,%d \n", buf , relword_buf, rel_value);
							relword_index++;
						}
					}
				}
				sem_index++;
					
                if (pre_repbuf[0] && strcmp(pre_repbuf, repbuf)) {
					label = unsupervised_classify(current_word, context_word, sem_index);
                    fprintf(Outfp, "TARGET %s -> LABEL %d\n", pre_repbuf, label);
                    sprintf(feature_buf, "語義曖昧性解消結果:%d", label);
                    assign_cfeature(&(m_ptr->f), feature_buf, FALSE);
					//current のリセット
					sem_index = 0;
					current_word[0][0].repids[0]= '\0';
                }
                strcpy(pre_repbuf, repbuf);
            }
            fp = fp->next;
        }
        if (pre_repbuf[0] ){
			label = unsupervised_classify(current_word, context_word, sem_index);
			fprintf(Outfp, "TARGET %s -> LABEL %d\n", pre_repbuf, label);
			sprintf(feature_buf, "語義曖昧性解消結果:%d", label);
			assign_cfeature(&(m_ptr->f), feature_buf, FALSE);
			// current のリセット
			sem_index = 0;
			current_word[0][0].repids[0]= '\0';
		}
    }
}

/*==================================================================*/
                     void wsd(SENTENCE_DATA *sp)
/*==================================================================*/
{
	// とりあえず
	wsd_supervised(sp);
}


/*====================================================================
                               END
====================================================================*/

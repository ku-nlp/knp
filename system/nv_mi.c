/*====================================================================

			 名詞・動詞相互情報量

                                        Daisuke Kawahara 2008. 10. 13

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE nv_mi_db;
int NV_MI_Exist;


/*==================================================================*/
			  void init_nv_mi()
/*==================================================================*/
{
    char *filename;

    if (DICT[NV_MI_DB]) {
	filename = check_dict_filename(DICT[NV_MI_DB], TRUE);
    }
    else {
	filename = check_dict_filename(NV_MI_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((nv_mi_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	NV_MI_Exist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open NV MI db <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	NV_MI_Exist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
			  void close_nv_mi()
/*==================================================================*/
{
    if (NV_MI_Exist == TRUE) {
	DB_close(nv_mi_db);
    }
}

/*==================================================================*/
		    char *lookup_nv_mi(char *str)
/*==================================================================*/
{
    return db_get(nv_mi_db, str);
}

/*==================================================================*/
	  int check_nv_mi(TAG_DATA *n_ptr, TAG_DATA *v_ptr)
/*==================================================================*/
{
    int ret, num, rank, given_verb_length;
    char *dic_str, *token, *given_verb, *given_noun;
    double score;

    if (NV_MI_Exist == FALSE || 
	!n_ptr || !n_ptr->head_ptr || 
	!v_ptr || !v_ptr->head_ptr) {
	return INT_MAX;
    }

    /* チェックする名詞からDBを引く */
    if ((given_noun = get_mrph_rep_from_f(n_ptr->head_ptr, FALSE)) == NULL) {
	return INT_MAX;
    }
    dic_str = lookup_nv_mi(given_noun);

    if (dic_str) {
	if ((given_verb = get_mrph_rep_from_f(v_ptr->head_ptr, FALSE)) == NULL) { /* チェックする動詞 */
	    free(dic_str);
	    return INT_MAX;
	}
	given_verb_length = strlen(given_verb);

	token = strtok(dic_str, "|");
	while (token) {
	    if (!strncmp(token, given_verb, given_verb_length)) { /* 与えられた動詞とDBがマッチ */
		num = sscanf(token, "%*[^,],%d,%f", &rank, &score);
		if (num != 2) {
		    fprintf(stderr, ";;; Invalid string in NV MI db <%s>.\n", token);
		    break;
		}
		free(dic_str);
		return rank;
	    }

	    token = strtok(NULL, "|");
	}

	free(dic_str);
    }

    return INT_MAX;
}

/*==================================================================*/
int check_nv_mi_parent_and_children(TAG_DATA *v_ptr, int rank_threshold)
/*==================================================================*/
{
    int i, rank = INT_MAX;

    /* 子供の格要素をチェック */
    for (i = 0; v_ptr->child[i]; i++) {
	if (check_feature(v_ptr->child[i]->f, "格要素")) {
	    rank = check_nv_mi(v_ptr->child[i], v_ptr);
	    if (rank < rank_threshold) {
		return TRUE;
	    }
	}
    }

    /* 連体修飾の親をチェック */
    if (v_ptr->parent && check_feature(v_ptr->f, "係:連格")) {
	rank = check_nv_mi(v_ptr->parent, v_ptr);
	if (rank < rank_threshold) {
	    return TRUE;
	}
    }

    return FALSE;
}

/*====================================================================
                               END
====================================================================*/

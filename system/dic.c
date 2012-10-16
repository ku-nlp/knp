/*====================================================================

                           語彙データベース

                                         Daisuke Kawahara 2007. 3. 13

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE id2lex_db;
int Id2LexDbExist;

/*==================================================================*/
                            void init_ld()
/*==================================================================*/
{
    char *filename;

    if (DICT[REP2ID_DA]) {
	filename = check_dict_filename(DICT[REP2ID_DA], TRUE);
    }
    else {
	filename = check_dict_filename(REP2ID_DA_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if (!(init_darts(filename))) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open REP2ID Trie <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
    }
    free(filename);

    if (DICT[ID2LEX_DB]) {
	filename = check_dict_filename(DICT[ID2LEX_DB], TRUE);
    }
    else {
	filename = check_dict_filename(ID2LEX_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((id2lex_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	Id2LexDbExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open ID2LEX DB <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	Id2LexDbExist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
                           void close_ld()
/*==================================================================*/
{
    /* close the trie of rep2id */
    close_darts();

    /* close the db of id2lex */
    if (Id2LexDbExist == TRUE) {
	DB_close(id2lex_db);
    }
}

/*==================================================================*/
int assign_features_of_ld_from_id (SENTENCE_DATA *sp, unsigned int id, unsigned int start_mrph_num, unsigned int end_mrph_num)
/*==================================================================*/
{
    char *ret;
    char *buf;
    char small_buf[SMALL_DATA_LEN];
    int match_num;
    int offset;
    char fname[DATA_LEN];
  
    sprintf(small_buf, "%u", id);
    ret = db_get(id2lex_db, small_buf);
    if (ret) {
        char *token = strtok(ret, "|");
        while (token) {
            buf = (char *)malloc_data(strlen(token) + 9, "assign_features_of_ld_from_id");
            match_num = sscanf(token, "f:%d:%s", &offset, buf);
            if (match_num == 2) {
                sprintf(small_buf, ":%d-%d", start_mrph_num, end_mrph_num);
                strcat(buf, small_buf);
                assign_cfeature(&((sp->mrph_data + end_mrph_num + offset)->f), buf, FALSE);
            }
            free(buf);
            token = strtok(NULL, "|");
        }
        free(ret);
    }
}

/*==================================================================*/
            int count_plus(char *str, unsigned int length)
/*==================================================================*/
{
    int count = 0;
    int i;
    for (i = 0; i < length; i++) {
        if (str[i] == '+') {
            count++;
        }
    }
    return count;
}

/*==================================================================*/
             int assign_feature_by_ld (SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, l;
    int match_num, status, node_pos_num, new_node_pos_num;
    int entry_num, start_id, rep_num, rep_malloc_flag;
    size_t result_lengths[LD_ENTRY_MAX_NUM], result_values[LD_ENTRY_MAX_NUM], tmp_result_length, tmp_result_value;
    size_t node_pos, node_pos_list[LD_NODE_MAX_NUM], new_node_pos_list[LD_NODE_MAX_NUM], key_pos, key_length;
    unsigned int rep_length, reps_length[LD_REP_MAX_NUM];
    char *rep_start, *reps_start[LD_REP_MAX_NUM];
    MRPH_DATA m;
    FEATURE *fp;

    for (i = 0; i < sp->Mrph_num; i++) { /* 開始位置: 先頭から順番に */
        node_pos_num = 1;
        node_pos_list[0] = 0; /* root node */
        match_num = 0;
        for (j = i; j < sp->Mrph_num; j++) { /* その位置からTrieを検索 */
            /* この位置の(曖昧性込みの)代表表記を生成 */
            rep_num = 0;
            rep_start = get_mrph_rep(sp->mrph_data + j);
            rep_length = get_mrph_rep_length(rep_start);
            if (rep_length == 0) { /* 代表表記: なければ作る */
                rep_start = make_mrph_rn(sp->mrph_data + j);
                rep_length = strlen(rep_start);
                rep_malloc_flag = 1;
            }
            else
                rep_malloc_flag = 0;
            reps_start[rep_num] = (char *)malloc_data(rep_length + 2, "assign_feature_by_ld");
            reps_start[rep_num][0] = '\0';
            if (j != i) /* 初回は "+" なし */
                strcat(reps_start[rep_num], "+");
            strncat(reps_start[rep_num], rep_start, rep_length);
            reps_length[rep_num] = strlen(reps_start[rep_num]);
            rep_num++;

            fp = (sp->mrph_data + j)->f;
            while (fp) { /* ALT- から代表表記を抽出 */
                if (!strncmp(fp->cp, "ALT-", 4)) {
                    sscanf(fp->cp + 4, "%[^-]-%[^-]-%[^-]-%d-%d-%d-%d-%[^\n]", m.Goi2, m.Yomi, m.Goi, &m.Hinshi, &m.Bunrui, &m.Katuyou_Kata, &m.Katuyou_Kei, m.Imi);
                    rep_start = get_mrph_rep(&m);
                    rep_length = get_mrph_rep_length(rep_start);
                    if (rep_length > 0) {
                        reps_start[rep_num] = (char *)malloc_data(rep_length + 2, "assign_feature_by_ld");
                        reps_start[rep_num][0] = '\0';
                        if (j != i) /* 初回は "+" なし */
                            strcat(reps_start[rep_num], "+");
                        strncat(reps_start[rep_num], rep_start, rep_length);
                        reps_length[rep_num] = strlen(reps_start[rep_num]);
                        rep_num++;
                        if (rep_num >= LD_REP_MAX_NUM) /* 代表表記の保持最大数 */
                            break;
                    }
		}
                fp = fp->next;
            }

            new_node_pos_num = 0;
            for (k = 0; k < node_pos_num; k++) { /* 前回のノード位置からtraverse */
                for (l = 0; l < rep_num; l++) { /* この位置の各代表表記について */
                    node_pos = node_pos_list[k];
                    /* printf(";; T <%s>(%d) from %d ", reps_start[l], reps_length[l], node_pos); */
                    status = traverse_ld(reps_start[l], &node_pos, 0, reps_length[l], &tmp_result_length, &tmp_result_value);
                    if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
                        result_values[match_num] = tmp_result_value;
                        result_lengths[match_num] = j - i;
                        match_num++;
                        if (match_num >= LD_ENTRY_MAX_NUM)
                            goto NODE_POS_LOOP_END;
                        if (new_node_pos_num < LD_NODE_MAX_NUM) /* node_pos保持最大数 */
                            new_node_pos_list[new_node_pos_num++] = node_pos;
                        /* printf("OK (pos=%d, value=%d)", node_pos, tmp_result_value); */
                    }
                    else if (status == -1) { /* ここではマッチしていないが、続きがある場合 */
                        if (new_node_pos_num < LD_NODE_MAX_NUM)
                            new_node_pos_list[new_node_pos_num++] = node_pos;
                        /* printf("OK (pos=%d)", node_pos); */
                    }
                    /* printf("\n"); */
                }
            }
          NODE_POS_LOOP_END:
            for (l = 0; l < rep_num; l++) /* 代表表記をfree */
                if (l > 0 || rep_malloc_flag)
                    free(reps_start[l]);

            node_pos_num = new_node_pos_num;
            for (k = 0; k < new_node_pos_num; k++)
                node_pos_list[k] = new_node_pos_list[k];
        }

        /* feature付与 */
        for (j = 0; j < match_num; j++) {
            entry_num = result_values[j] & 0xff;
            start_id = result_values[j] >> 8;
            if (start_id == 20554) /* modification for する/する */
                entry_num = 3872;
            for (k = 0; k < entry_num; k++) {
                assign_features_of_ld_from_id(sp, start_id + k, i, i + result_lengths[j]);
            }
        }
    }
}

/*====================================================================
                                 END
  ====================================================================*/

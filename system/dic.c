/*====================================================================

			     自動獲得辞書

                                         Daisuke Kawahara 2007. 3. 13

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE auto_dic_db;
int AutoDicExist;

char *used_auto_dic_features[AUTO_DIC_FEATURES_MAX];
int used_auto_dic_features_num = 0;

/*==================================================================*/
			 void init_auto_dic()
/*==================================================================*/
{
    char *filename;

    if (DICT[AUTO_DIC_DB]) {
	filename = check_dict_filename(DICT[AUTO_DIC_DB], TRUE);
    }
    else {
	filename = check_dict_filename(AUTO_DIC_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((auto_dic_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	AutoDicExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open AUTO dictionary <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	AutoDicExist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
			void close_auto_dic()
/*==================================================================*/
{
    if (AutoDicExist == TRUE) {
	DB_close(auto_dic_db);
    }
}

/*==================================================================*/
		   char *lookup_auto_dic(char *str)
/*==================================================================*/
{
    return db_get(auto_dic_db, str);
}

/*==================================================================*/
  char *check_auto_dic(MRPH_DATA *m_ptr, int m_length, char *rule_value)
/*==================================================================*/
{
    int i, flag;
    char *ret = NULL, *dic_str, *rep_str, key[DATA_LEN];

    if (AutoDicExist == FALSE) {
	return NULL;
    }

    if (used_auto_dic_features_num > 0) { /* 使用する自動獲得属性が指定されているとき */
	flag = FALSE;
	for (i = 0; i < used_auto_dic_features_num; i++) {
	    if (!strcmp(used_auto_dic_features[i], rule_value)) {
		flag = TRUE;
		break;
	    }
	}
	if (flag == FALSE) { /* マッチしなかった */
	    return NULL;
	}
    }

    key[0] = '\0';
    for (i = 0; i < m_length; i++) { /* 形態素列からキーを作る */
	if (i) strcat(key, "+");
	if (rep_str = get_mrph_rep_from_f(m_ptr + i, FALSE)) {
	    if (strlen(key) + strlen(rep_str) + 2 > DATA_LEN) {
		return NULL;
	    }
	    strcat(key, rep_str);
	}
	else { /* 助詞、助動詞などは代表表記がない */
	    strcat(key, (m_ptr + i)->Goi2); /* 表記 */
	}
    }

    dic_str = lookup_auto_dic(key);

    if (dic_str) {
	int cmp_length = strlen(rule_value); /* strncmpの長さ */
	char *token = strtok(dic_str, "|"); /* 辞書項目を区切る (dict/auto/Makefile.amで指定) */
	while (token) {
	    if (!strncmp(token, rule_value, cmp_length)) { /* 辞書項目とルールから与えられた文字列がマッチ */
		ret = (char *)malloc_data(strlen(token) + 9, "check_auto_dic");
		sprintf(ret, "%s:%d-%d", token, m_ptr->num, (m_ptr + m_length - 1)->num);
		break;
	    }
	    token = strtok(NULL, "|");
	}

	free(dic_str);
	return ret;
    }

    return NULL;
}

/*====================================================================
                               END
====================================================================*/

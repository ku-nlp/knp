/*====================================================================

			     共参照解析

                                               R.SASANO 05. 9.24

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE synonym_db;

char *SynonymFile;

/*==================================================================*/
	       void init_Synonym_db()
/*==================================================================*/
{
    char *db_filename;

    if (!SynonymFile) return;

    db_filename = check_dict_filename(SynonymFile, TRUE);

    if ((synonym_db = DB_open(db_filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... failed.\n", db_filename);
	}
	fprintf(stderr, ";; Cannot open Synonym Database <%s>.\n", db_filename);
    } 
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... done.\n", db_filename);
	}
    }
    free(db_filename);
}

/*==================================================================*/
	       void close_Synonym_db()
/*==================================================================*/
{
    if (!SynonymFile) return;
    DB_close(synonym_db);
}

/*==================================================================*/
	       int get_modify_num(BNST_DATA *b_ptr)
/*==================================================================*/
{
    /* 並列を除いていくつの文節に修飾されているかを返す */
    
    int i;

    if (!b_ptr->child[0]) {
	return 0;
    }
    if ((b_ptr->child[0])->para_type) {
	b_ptr = b_ptr->child[0];
    }
    for (i = 0; b_ptr->child[i]; i++);
    return i;
}

/*==================================================================*/
	       void assign_anaphor_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 照応詞候補というfeatureを付与する */
    /* 照応詞候補とは強い複合名詞の末尾となっている語のこと */

    /* 強い複合名詞の基準 */
    /* 	固有表現(LOCATION、DATEは切る) */
    /* 	末尾の語がサ変名詞であれば切る */
    /* 	末尾の語とその前の語が名詞格フレームにある組合せの場合は切る */

    int i, j, k, tag_num, mrph_num;
    char word[WORD_LEN_MAX * 2], buf[WORD_LEN_MAX * 2], *cp;
    TAG_DATA *tag_ptr;
    
    for (i = 0; i < sp->Bnst_num; i++) {
	
	if (!check_feature((sp->bnst_data + i)->f, "体言")) continue;

	tag_num = (sp->bnst_data+i)->tag_num;
	tag_ptr = (sp->bnst_data + i)->tag_ptr;

  	for (j = tag_num - 1; j >= 0; j--) {

	    /* 固有表現内である場合は後回し */
	    if (check_feature((tag_ptr + j)->f, "NE") ||
		check_feature(((tag_ptr + j)->mrph_ptr + 
			       (tag_ptr + j)->mrph_num - 1)->f, "NE")) {
		break;
	    }

	    /* 数詞、形式名詞、副詞的名詞、時相名詞 */
	    /* および隣に係る形容詞は除外 */
	    if (((tag_ptr + j)->head_ptr)->Hinshi == 6 &&
		((tag_ptr + j)->head_ptr)->Bunrui > 7 ||
		((tag_ptr + j)->head_ptr)->Hinshi == 3 &&
		check_feature((tag_ptr + j)->f, "係:隣")) {
		continue;
	    }

 	    if (/* 主辞である */
		j == tag_num - 1 ||
		
		/* 直後の名詞がサ変名詞である */
		((tag_ptr + j + 1)->mrph_ptr)->Hinshi == 6 &&
		((tag_ptr + j + 1)->mrph_ptr)->Bunrui == 2 ||

		/* 直後が名詞性接尾辞である */
		((tag_ptr + j + 1)->mrph_ptr)->Hinshi == 14 &&
		((tag_ptr + j + 1)->mrph_ptr)->Bunrui < 5 ||

		/* 直後の名詞の格フレームの用例に存在する */
		(tag_ptr + j + 1)->cf_ptr &&
		check_examples(((tag_ptr + j)->mrph_ptr)->Goi2,
			       strlen(((tag_ptr + j)->mrph_ptr)->Goi2),
				((tag_ptr + j + 1)->cf_ptr)->ex_list[0],
				((tag_ptr + j + 1)->cf_ptr)->ex_num[0]) >= 0) {

		word[0] = '\0';
		for (k = (tag_ptr + j)->head_ptr - (sp->bnst_data + i)->mrph_ptr; 
		     k >= 0; k--) {

		    /* 先頭の名詞接頭辞・特殊は含めない */
		    if (!strncmp(word, "\0", 1) &&
			(((tag_ptr + j)->head_ptr - k)->Hinshi == 1 ||
			 ((tag_ptr + j)->head_ptr - k)->Hinshi == 13 &&
			 ((tag_ptr + j)->head_ptr - k)->Bunrui == 1))
			continue;
		    strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
		}
		if (strncmp(word, "\0", 1)) {
		    sprintf(buf, "照応詞候補:%s", word);
		    assign_cfeature(&((tag_ptr + j)->f), buf);
		}
	    } else {
		break;
	    }
	}
	
 	for (j = tag_num - 1; j >= 0; j--) {   
	    
	    /* 固有表現の主辞には付与 */
	    if ((cp = check_feature((tag_ptr + j)->f, "NE"))) {
		while (strncmp(cp, ")", 1)) cp++;
		sprintf(buf, "照応詞候補:%s", cp + 1);
		assign_cfeature(&((tag_ptr + j)->f), buf);
		continue;
	    } 
	    /* 固有表現中である場合 */
	    mrph_num = (tag_ptr + j)->mrph_num - 1;
	    if ((cp = check_feature(((tag_ptr + j)->mrph_ptr + mrph_num)->f, "NE")) &&
		/* DATEであれば時相名詞、名詞性名詞助数辞で切る */
		(!strncmp(cp + 3, "DATE", 4) && 
		 (((tag_ptr + j)->mrph_ptr + mrph_num)->Hinshi == 6 &&
		  ((tag_ptr + j)->mrph_ptr + mrph_num)->Bunrui == 10 ||
		  ((tag_ptr + j)->mrph_ptr + mrph_num)->Hinshi == 14 &&
		  ((tag_ptr + j)->mrph_ptr + mrph_num)->Bunrui == 3) || 
		 /* LOCATIONであれば名詞性特殊接尾辞で切る */
		 !strncmp(cp + 3, "LOCATION", 8) && 
		 ((tag_ptr + j)->mrph_ptr + mrph_num)->Hinshi == 14 &&
		 ((tag_ptr + j)->mrph_ptr + mrph_num)->Bunrui == 4)) {
		
		for (k = 0; !(cp = check_feature((tag_ptr + j + k)->f, "NE")); k++);
		while (strncmp(cp, ")", 1)) cp++;
		for (k = 0; 
		     strncmp(cp + k + 1, ((tag_ptr + j)->mrph_ptr + mrph_num)->Goi2, 
			     strlen(((tag_ptr + j)->mrph_ptr + mrph_num)->Goi2));
		     k++);
		word[0] = '\0';
		strncpy(word, cp + 1, k);
		strcat(word, ((tag_ptr + j)->mrph_ptr + mrph_num)->Goi2);
		sprintf(buf, "照応詞候補:%s", word);
		assign_cfeature(&((tag_ptr + j)->f), buf);	
	    }			
	}   
    }
}

/*==================================================================*/
               int compare_strings(char *antecedent, char *anaphor)
/*==================================================================*/
{
    /* 照応詞候補と先行詞候補を比較 */

    int i, j;
    char word[WORD_LEN_MAX * 4];

    /* 同表記の場合 */
    if (!strcmp(antecedent, anaphor)) return 1;

    /* 同義表現辞書が読み込めなかった場合はここで終了 */
    if (!synonym_db) return 0;

    /* そのまま同義表現辞書に登録されている場合 */
    word[0] = '\0';
    strcpy(word, anaphor);
    strcat(word, ":");
    strcat(word, antecedent);
    if (db_get(synonym_db, word)) {
	return 1;
    } 

    /* 前後から同じ表記の文字を削除して残りの文字列のペアを比較する */
    /* 「金融派生商品-取引」と「デリバティブ-取引」を認識できる */
    /* 「日本銀行」と「日銀」のように同義表現同じ文字を含む場合には未対応 */
    for (i = 0; i < strlen(anaphor); i += 2) {
	if (strncmp(antecedent + i, anaphor + i, 2)) {
	    break;
	}
    }  
    for (j = 0; j < strlen(anaphor); j += 2) {
	if (strncmp(antecedent + strlen(antecedent) - j - 2, 
		    anaphor + strlen(anaphor) - j -2, 2)) {
	    break;
	} 
    }

    memset(word, 0, sizeof(char) * WORD_LEN_MAX * 4);
    strncpy(word, anaphor + i, strlen(anaphor) - i - j);
    strcat(word, ":");
    strncat(word, antecedent + i, strlen(antecedent) - i - j);
    strcat(word, "\0");

    if (db_get(synonym_db, word)) {
	return 1;
    }   
    return 0;
}

/*==================================================================*/
               int search_antecedent(SENTENCE_DATA *sp, int i, 
				     char *anaphor, char *setubi)
/*==================================================================*/
{
    /* 入力されたタグと、共参照関係にあるタグを以前の文から検索する */
    /* setubiが与えられた場合は直後の接尾辞も含めて探索する */

    /* 共参照関係にある語が見つかった場合は結果がfeatureに付与され */
    /* 共参照関係にあるとされた照応詞文字列の先頭のタグの番号 */
    /* 見つからなかった場合は-2を返す */

    int j, k, l, m;
    char word[WORD_LEN_MAX * 2], buf[WORD_LEN_MAX * 2];
    SENTENCE_DATA *sdp;
    TAG_DATA *tag_ptr;
 
    sdp = sentence_data + sp->Sen_num - 1;
    for (j = 0; j <= sdp - sentence_data; j++) { /* 照応先が何文前か */
	
	for (k = j ? (sdp - j)->Tag_num - 1 : i - 1; k >= 0; k--) { /* 照応先のタグ */
   
	    tag_ptr = (sdp - j)->tag_data + k;	    		

	    /* setubiが与えられた場合、後続の名詞性接尾を比較 */
	    if (setubi && strcmp((tag_ptr->head_ptr + 1)->Goi2, setubi))
		continue;
		
	    for (l = tag_ptr->head_ptr - (tag_ptr->b_ptr)->mrph_ptr; l >= 0; l--) {

		word[0] = '\0';
		for (m = l; m >= 0; m--) {
		    strcat(word, (tag_ptr->head_ptr - m)->Goi2); /* 先行詞候補 */
		}	

		if (compare_strings(word, anaphor)) { /* 同義表現であれば */
		    if (j == 0) {
			sprintf(buf, "C用;【%s%s】;=;0;%d;9.99:%s(同一文):%d文節",
				word, setubi ? setubi : "", k, 
				sp->KNPSID ? sp->KNPSID + 5 : "?", k);
		    }
		    else {
			sprintf(buf, "C用;【%s%s】;=;%d;%d;9.99:%s(%d文前):%d文節",
				word, setubi ? setubi : "", j, k, 
				sp->KNPSID ? sp->KNPSID + 5 : "?", j, k);
		    }
		    assign_cfeature(&((sp->tag_data + i)->f), "共参照");
		    assign_cfeature(&((sp->tag_data + i)->f), buf);
		    
		    /* 複数のタグに関係する場合の処理 */
		    for (m = 0; m < l; i--) 
			m += (sp->tag_data + i)->mrph_num;
		    return i;
		}
	    }
	}	    
    }
    return -2;
}

/*==================================================================*/
               int search_antecedent_NE(SENTENCE_DATA *sp, int i, 
					char *anaphor)
/*==================================================================*/
{
    int j, k;
    char *word, buf[WORD_LEN_MAX * 2];
    SENTENCE_DATA *sdp;
    TAG_DATA *tag_ptr;
 
    sdp = sentence_data + sp->Sen_num - 1;
    for (j = 0; j <= sdp - sentence_data; j++) { /* 照応先が何文前か */
	for (k = j ? (sdp - j)->Tag_num - 1 : i - 1; k >= 0; k--) { /* 照応先のタグ */  
	    tag_ptr = (sdp - j)->tag_data + k;	    		
	    if (!(word = check_feature(tag_ptr->f, "NE"))) continue;

	    while (strncmp(word, ")", 1)) word++;
	    if (compare_strings(++word, anaphor)) {
		if (j == 0) {
		    sprintf(buf, "C用;【%s】;=;0;%d;9.99:%s(同一文):%d文節",
			    word, k, sp->KNPSID ? sp->KNPSID + 5 : "?", k);
		}
		else {
		    sprintf(buf, "C用;【%s】;=;%d;%d;9.99:%s(%d文前):%d文節",
			    word, j, k, sp->KNPSID ? sp->KNPSID + 5 : "?", j, k);
		}
		assign_cfeature(&((sp->tag_data + i)->f), "共参照");
		assign_cfeature(&((sp->tag_data + i)->f), buf);

		while (i > 0 &&
		       check_feature(((sp->tag_data + i)->mrph_ptr +
				      (sp->tag_data + i)->mrph_num)->f, "NE")) i--;
		return i;
	    }
	}
    }
    return -2;
}

/*==================================================================*/
	       void corefer_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, next_i;
    char *anaphor;
    MRPH_DATA *mrph_ptr;
    
    for (i = sp->Tag_num - 1; i >= 0; i--) { /* 解析文のタグ単位:i番目のタグについて */

	/* 共参照解析を行う条件 */
	/* 照応詞候補であり、固有表現中の語、または */
	/* 連体詞形態指示詞以外に修飾されていない語 */
	if (!(anaphor = check_feature((sp->tag_data + i)->f, "照応詞候補")) ||
	    !check_feature((sp->tag_data + i)->f, "NE") &&
	    !check_feature(((sp->tag_data + i)->mrph_ptr +
			    (sp->tag_data + i)->mrph_num)->f, "NE") &&
	    get_modify_num((sp->tag_data + i)->b_ptr) && /* 修飾されている */
	    !check_feature((((sp->tag_data + i)->b_ptr)->child[0])->f, "連体詞形態指示詞"))
	    continue;

	/* 指示詞の場合 */
	if (check_feature((sp->tag_data + i)->f, "指示詞")) {
	    continue;
	}

	/* 固有表現の場合 */
	if (check_feature((sp->tag_data + i)->f, "NE")) {
	    next_i = search_antecedent_NE(sp, i, anaphor+11);
	    if (next_i != -2) {
		i = next_i;
		continue;
	    }
	}
	
	/* 名詞性接尾辞が付いている場合はまず接尾辞も含めたものを調べる */
	mrph_ptr = (sp->tag_data + i)->head_ptr + 1;
	if (mrph_ptr->Hinshi == 14 && mrph_ptr->Bunrui < 5) {
	    next_i = search_antecedent(sp, i, anaphor+11, mrph_ptr->Goi2);
	    if (next_i != -2) {
		i = next_i;
		continue;
	    }
	}
	/* 一般の場合 */
	next_i = search_antecedent(sp, i, anaphor+11, NULL);
	i = (next_i == -2) ? i : next_i;
    }
}

/*====================================================================
                               END
====================================================================*/

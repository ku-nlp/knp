/*====================================================================

			     共参照解析

                                               R.SASANO 05. 9.24

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE synonym_db;

char *SynonymFile;

/* 文の要素を保持する */
typedef struct entity_cache {
    char            *key;
    int	            frequency;
    struct entity_cache *next;
} ENTITY_CACHE;

ENTITY_CACHE *entity_cache[TBLSIZE];

/*==================================================================*/
			 void init_entity_cache()
/*==================================================================*/
{
    memset(entity_cache, 0, sizeof(ENTITY_CACHE *)*TBLSIZE);
}

/*==================================================================*/
			void clear_entity_cache()
/*==================================================================*/
{
    int i;
    ENTITY_CACHE *ecp, *next;

    for (i = 0; i < TBLSIZE; i++) {
	ecp = entity_cache[i];
	while (ecp) {
	    free(ecp->key);
	    next = ecp->next;
	    free(ecp);
	    ecp = next;
	}
	entity_cache[i] = NULL;
    }
}

/*==================================================================*/
       void register_entity_cache(char *key)
/*==================================================================*/
{
    /* 文の要素を登録する */

    ENTITY_CACHE **ecpp;

    ecpp = &(entity_cache[hash(key, strlen(key))]);
    while (*ecpp && (*ecpp)->key && strcmp((*ecpp)->key, key)) {
	ecpp = &((*ecpp)->next);
    }
    if (!(*ecpp)) {
	*ecpp = (ENTITY_CACHE *)malloc_data(sizeof(ENTITY_CACHE), "register_entity_cache");
	memset(*ecpp, 0, sizeof(ENTITY_CACHE));
    }
    if (!(*ecpp)->key) {
	(*ecpp)->key = strdup(key);
	(*ecpp)->next = NULL;
    }
    (*ecpp)->frequency++;
}

/*==================================================================*/
	     int check_entity_cache(char *key)
/*==================================================================*/
{
    ENTITY_CACHE *ecp;

    ecp = entity_cache[hash(key, strlen(key))];
    if (!ecp || !ecp->key) {
	return 0;
    }
    while (ecp) {
	if (!strcmp(ecp->key, key)) {
	    return ecp->frequency;
	}
	ecp = ecp->next;
    }
    return 0;
}

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
		int get_modify_num(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* 並列を除いていくつの文節に修飾されているかを返す */
    /* ＡのＢＣとなっている場合はＡがＢに係っているかの判断も行う */

    int i;
    BNST_DATA *b_ptr;

    b_ptr = tag_ptr->b_ptr;

    /* 所属する文節が修飾されていない場合 */
    if (!b_ptr->child[0]) {
	return 0;
    }

    /* 文節の先頭のタグで主辞でない場合 */
    /* 直前の文節の主辞との間に名詞格フレームに記された関係がなければ */
    /* 直前の文節は主辞に係っていると考え、修飾されていないとみなす */
    if (tag_ptr == b_ptr->tag_ptr && 
	b_ptr->tag_num > 1 &&
	!((tag_ptr)->cf_ptr &&
	check_examples(((tag_ptr - 1)->mrph_ptr)->Goi2,
		       strlen(((tag_ptr - 1)->mrph_ptr)->Goi2),
		       (tag_ptr->cf_ptr)->ex_list[0],
		       (tag_ptr->cf_ptr)->ex_num[0]) >= 0)) {
	return 0;
    }

    /* 上記以外 */
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
    /* 複合名詞に照応詞候補というfeatureを付与する */

    /* 複合名詞 */   
    /* 	固有表現(LOCATION、DATEは分解) */
    /* 	末尾の語がサ変名詞であれば切る */
    /* 	末尾の語とその前の語が名詞格フレームにある組合せの場合は切る */

    int i, j, k, l, tag_num, mrph_num;
    char word[WORD_LEN_MAX * 2], buf[WORD_LEN_MAX * 2], *cp;
    TAG_DATA *tag_ptr;      

    /* 文節単位で文の前から処理 */
    for (i = 0; i < sp->Bnst_num; i++) {
	
	if (!check_feature((sp->bnst_data + i)->f, "体言")) continue;

	tag_num = (sp->bnst_data + i)->tag_num;
	tag_ptr = (sp->bnst_data + i)->tag_ptr;
	
	/* まずは文節末尾から複合名詞の末尾のタグに照応詞候補と付与していく */
	/* ex. 「金融派生商品取引を」という文節に対しては */
	/*      ラストのタグに<照応詞候補:金融派生商品取引>を */
	/*      その前のタグに<照応詞候補:金融派生商品>を付与する */
  	for (j = tag_num - 1; j >= 0; j--) {
	    
	    /* 固有表現内である場合は後回し */
	    if ((check_feature((tag_ptr + j)->f, "NE") ||
		 check_feature((tag_ptr + j)->f, "NE内")) &&
		/* ただしARTIFACTの場合は通常と同様に扱う(要検討) */
		!check_feature((tag_ptr + j)->f, "NE:ARTIFACT") &&
		!check_feature((tag_ptr + j)->f, "NE内:ARTIFACT"))
		break;
	    
	    /* 数詞、形式名詞、副詞的名詞 */
	    /* および隣に係る形容詞は除外 */
	    if (((tag_ptr + j)->head_ptr)->Hinshi == 6 &&
		((tag_ptr + j)->head_ptr)->Bunrui > 7 &&
		((tag_ptr + j)->head_ptr)->Bunrui != 10 ||
		((tag_ptr + j)->head_ptr)->Hinshi == 3 &&
		check_feature((tag_ptr + j)->f, "係:隣")) {
		continue;
	    }

 	    if (/* 主辞である */
		j == tag_num - 1 ||
		
		/* 「・」で終了している */
		!strcmp(((tag_ptr + j)->mrph_ptr +
			 (tag_ptr + j)->mrph_num -1)->Goi2, "・") ||

		/* 直後の名詞がサ変名詞、形容詞である */
		((tag_ptr + j + 1)->mrph_ptr)->Hinshi == 6 &&
		((tag_ptr + j + 1)->mrph_ptr)->Bunrui == 2 ||
		((tag_ptr + j + 1)->mrph_ptr)->Hinshi == 3 ||

		/* 直後が名詞性接尾辞である */
		((tag_ptr + j + 1)->mrph_ptr)->Hinshi == 14 &&
		((tag_ptr + j + 1)->mrph_ptr)->Bunrui < 5 ||

		/* 直後の名詞の格フレームの用例に存在する */
		(tag_ptr + j + 1)->cf_ptr &&
		check_examples(((tag_ptr + j)->head_ptr)->Goi2,
			       strlen(((tag_ptr + j)->head_ptr)->Goi2),
				((tag_ptr + j + 1)->cf_ptr)->ex_list[0],
				((tag_ptr + j + 1)->cf_ptr)->ex_num[0]) >= 0) {
		
		word[0] = '\0';
		for (k = (tag_ptr + j)->head_ptr - (sp->bnst_data + i)->mrph_ptr; 
		     k >= 0; k--) {
		    
		    /* 先頭の特殊、照応接頭辞は含めない */
		    if (!strncmp(word, "\0", 1) &&
			(((tag_ptr + j)->head_ptr - k)->Hinshi == 1 ||
			 check_feature(((tag_ptr + j)->head_ptr - k)->f, "照応接頭辞")))
			continue;

		    /* 「・」より前は含めない */
		    if (!strcmp(((tag_ptr + j)->head_ptr - k)->Goi2, "・")) {
			if (k > 0) word[0] = '\0';
		    }
		    else {
			strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
		    }
		}
		if (strncmp(word, "\0", 1)) {
		    sprintf(buf, "照応詞候補:%s", word);
		    assign_cfeature(&((tag_ptr + j)->f), buf);
		    register_entity_cache(word);
		}
	    } else {
		break;
	    }
	}
	
	/* 固有表現を含む文節である場合 */
 	for (j = tag_num - 1; j >= 0; j--) {   

	    /* PERSONの場合はその前方の文字列をentity_cacheに登録する */
	    if (check_feature((tag_ptr + j)->f, "NE:PERSON")) {
		word[0] = '\0';		
		for (k = (tag_ptr + j)->head_ptr - (tag_ptr + j)->mrph_ptr; k >= 0; k--) {
		    if (!check_feature(((tag_ptr + j)->head_ptr - k)->f, "NE:PERSON:head") &&
			!check_feature(((tag_ptr + j)->head_ptr - k)->f, "NE:PERSON:middle"))
			continue;		    
		    strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
		    register_entity_cache(word);
		}
	    }	    
    
	    /* 固有表現の主辞には付与 */
	    if ((cp = check_feature((tag_ptr + j)->f, "NE"))) {
		cp += 3; /* "NE:"を読み飛ばす */
		while (strncmp(cp, ":", 1)) cp++;
		register_entity_cache(cp + 1);
		sprintf(buf, "照応詞候補:%s", cp + 1);
		assign_cfeature(&((tag_ptr + j)->f), buf);
		continue;
	    } 

	    /* 固有表現中である場合(DATEまたはLOCATIONの場合) */
	    mrph_num = (tag_ptr + j)->mrph_num - 1;
	    if (/* DATEであれば時相名詞、名詞性名詞助数辞で切る */
		check_feature(((tag_ptr + j)->mrph_ptr + mrph_num)->f, "NE:DATE") &&
		(((tag_ptr + j)->mrph_ptr + mrph_num)->Hinshi == 6 &&
		 ((tag_ptr + j)->mrph_ptr + mrph_num)->Bunrui == 10 ||
		 ((tag_ptr + j)->mrph_ptr + mrph_num)->Hinshi == 14 &&
		 ((tag_ptr + j)->mrph_ptr + mrph_num)->Bunrui == 3) || 
		/* LOCATIONであれば名詞性特殊接尾辞で切る */
		check_feature(((tag_ptr + j)->mrph_ptr + mrph_num)->f, "NE:LOCATION") &&
		((tag_ptr + j)->mrph_ptr + mrph_num)->Hinshi == 14 &&
		((tag_ptr + j)->mrph_ptr + mrph_num)->Bunrui == 4) {
		
		for (k = 0; !(cp = check_feature((tag_ptr + j + k)->f, "NE")); k++);
		cp += 3; /* "NE:"を読み飛ばす */
		while (strncmp(cp, ":", 1)) cp++;
		/* cp + 1 は対象の固有表現文字列へのポインタ */
		for (k = 0; 
		     strncmp(cp + k + 1, ((tag_ptr + j)->mrph_ptr + mrph_num)->Goi2, 
			     strlen(((tag_ptr + j)->mrph_ptr + mrph_num)->Goi2));
		     k++);
		strncpy(word, cp + 1, k);
		word[k] = '\0';
		strcat(word, ((tag_ptr + j)->mrph_ptr + mrph_num)->Goi2);
		register_entity_cache(word);
		sprintf(buf, "照応詞候補:%s", word);
		assign_cfeature(&((tag_ptr + j)->f), buf);
	    }

	    /* 固有の前に来る表現(ex. 首都グロズヌイ) */
	    /* 同格を検出できないため */
	    if (j < tag_num - 1 && 
		check_feature(((tag_ptr + j + 1)->mrph_ptr + mrph_num)->f, "NE") &&
		!check_feature(((tag_ptr + j)->mrph_ptr + mrph_num)->f, "NE")) {
		word[0] = '\0';
		for (k = (tag_ptr + j)->head_ptr - (sp->bnst_data + i)->mrph_ptr; 
		     k >= 0; k--) {
		    
		    /* 先頭の特殊、照応接頭辞は含めない */
		    if (!strncmp(word, "\0", 1) &&
			(((tag_ptr + j)->head_ptr - k)->Hinshi == 1 ||
			 check_feature(((tag_ptr + j)->head_ptr - k)->f, "照応接頭辞")))
			continue;
		    strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
		}
		if (strncmp(word, "\0", 1)) {
		    register_entity_cache(word);
		    sprintf(buf, "照応詞候補:%s", word);
		    assign_cfeature(&((tag_ptr + j)->f), buf);
		}
	    }	    
	}   
	
	/* 最後に文節頭から見ていきentity_cacheに存在する表現であれば付与する */	
  	for (j = 0; j < tag_num; j++) {

 	    if (check_feature((tag_ptr + j)->f, "照応詞候補") || 
		check_feature((tag_ptr + j)->f, "NE")) break;

	    word[0] = '\0';
	    for (k = (tag_ptr + j)->head_ptr - (sp->bnst_data + i)->mrph_ptr; 
		 k >= 0; k--) {
		if (strncmp(word, "\0", 1) ||
		    ((tag_ptr + j)->head_ptr - k)->Hinshi != 1 &&
		    !check_feature(((tag_ptr + j)->head_ptr - k)->f, "照応接頭辞"))
		    strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
	    }

	    if (strncmp(word, "\0", 1) && check_entity_cache(word)) {
		register_entity_cache(word);
		sprintf(buf, "照応詞候補:%s", word);
		assign_cfeature(&((tag_ptr + j)->f), buf);
	    }
	}
    }
}

/*==================================================================*/
int compare_strings(char *antecedent, char *anaphor, char *ant_ne, char *ana_ne)
/*==================================================================*/
{
    /* 照応詞候補と先行詞候補を比較 */
    
    int i, j;
    char word[WORD_LEN_MAX * 4];

    /* 異なる種類の固有表現の場合は不可 */ 
    if (ana_ne && ant_ne && strncmp(ana_ne, ant_ne, 7)) return 0;

    /* 同表記の場合 */
    if (!strcmp(antecedent, anaphor)) return 1;

    /* 先行詞がPERSONかLOCATIONである場合は */
    /* 照応詞候補が先行詞候補の先頭に含まれていればOK */
    /* ex. 村山富市=村山、大分県=大分 */
    if (ant_ne && strlen(ant_ne) > strlen(antecedent) && /* 先行詞がNE全体である */
	!strcmp(ant_ne + strlen(ant_ne) - strlen(antecedent), antecedent) &&
	(!strncmp(ant_ne, "NE:PERSON", 7) || 
	 !strncmp(ant_ne, "NE:LOCATION", 7) && strlen(antecedent) - strlen(anaphor) == 2) &&
	!strncmp(antecedent, anaphor, strlen(anaphor))) return 1;

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
    if (strlen(anaphor) < i + j) return 0; /* 公文書公開 公開 のとき */

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
int search_antecedent(SENTENCE_DATA *sp, int i, char *anaphor, char *setubi, char *ne)
/*==================================================================*/
{
    /* 入力されたタグと、共参照関係にあるタグを以前の文から検索する */
    /* setubiが与えられた場合は直後の接尾辞も含めて探索する */

    /* 共参照関係にある語が見つかった場合は結果がfeatureに付与され */
    /* 共参照関係にあるとされた照応詞文字列の先頭のタグの番号 */
    /* 見つからなかった場合は-2を返す */

    int j, k, l, m;
    char word[WORD_LEN_MAX], buf[WORD_LEN_MAX], *cp, *ant_ne;
    SENTENCE_DATA *sdp;
    TAG_DATA *tag_ptr;
 
    sdp = sentence_data + sp->Sen_num - 1;
    for (j = 0; j <= sdp - sentence_data; j++) { /* 照応先が何文前か */
	
	for (k = j ? (sdp - j)->Tag_num - 1 : i - 1; k >= 0; k--) { /* 照応先のタグ */
   
	    tag_ptr = (sdp - j)->tag_data + k;	    		
	    ant_ne = check_feature(tag_ptr->f, "NE");
		
	    /* 固有名詞中である場合は照応詞候補である場合以外は先行詞候補としない */
	    if (!check_feature(tag_ptr->f, "照応詞候補") &&
		check_feature(tag_ptr->f, "NE内")) continue;
			
	    /* setubiが与えられた場合、後続の名詞性接尾を比較 */
	    if (setubi && strcmp((tag_ptr->head_ptr + 1)->Goi2, setubi)) continue;

	    for (l = tag_ptr->head_ptr - (tag_ptr->b_ptr)->mrph_ptr; l >= 0; l--) {

		word[0] = '\0';
		for (m = l; m >= 0; m--) {
		    strcat(word, (tag_ptr->head_ptr - m)->Goi2); /* 先行詞候補 */
		}	
		
		if (compare_strings(word, anaphor, ant_ne, ne)) {
		    /* 同義表現であれば */
		    if (j == 0) {
			sprintf(buf, "C用;【%s%s】;=;0;%d;9.99:%s(同一文):%d文節",
				word, setubi ? setubi : "", k, 
				sp->KNPSID ? sp->KNPSID + 5 : "?", k);
		    }
		    else {
			sprintf(buf, "C用;【%s%s】;=;%d;%d;9.99:%s(%d文前):%d文節",
				word, setubi ? setubi : "", j, k, 
				(sdp - j)->KNPSID ? (sdp - j)->KNPSID + 5 : "?", j, k);
		    }
		    assign_cfeature(&((sp->tag_data + i)->f), buf);
		    assign_cfeature(&((sp->tag_data + i)->f), "共参照"); 

		    /* 固有表現とcoreferの関係にある語を固有表現とみなす */
		    if (OptNE) {
			if (!check_feature((sp->tag_data + i)->f, "NE") &&
			    !check_feature((sp->tag_data + i)->f, "NE内") &&
			    (cp = check_feature(tag_ptr->f, "NE")) && !setubi) {
			    cp += 3; /* "NE:"を読み飛ばす */
			    while (strncmp(cp, ":", 1)) cp++;
			    if (!strcmp(cp + 1, word)) {
				ne_corefer(sp, i, anaphor,
					   check_feature(tag_ptr->f, "NE"));
			    }
			} 
		    }
		    return 1;
		}
	    }
	}	    
    }
    return 0;
}

/*==================================================================*/
int person_post(SENTENCE_DATA *sp, TAG_DATA *tag_ptr, char *cp, int j)
/*==================================================================*/
{
    /* PERSON + 役職 に"="タグを付与 */

    int i, flag;
    char buf[WORD_LEN_MAX];
    MRPH_DATA *mrph_ptr;

    mrph_ptr = tag_ptr->mrph_ptr;
    /* タグ末尾までNE中である場合のみ対象とする */
    if (!check_feature((mrph_ptr - 1)->f, "NE") &&
	!(check_feature((mrph_ptr - 2)->f, "NE") &&
	  (mrph_ptr - 1)->Hinshi == 1 && 
	  (mrph_ptr - 1)->Bunrui == 5)) /* 直前が記号である */
	return 0;
	
    flag = 0;
    for (i = 0;; i++) {
	if (check_feature((mrph_ptr + i)->f, "NE")) {
	    /* 基本的には、ブッシュ・アメリカ大統領 */
	    /* 武部自民党幹事長などを想定している */
	    continue;
	}
	else if (check_feature((mrph_ptr + i)->f, "人名末尾")) {
	    flag = 1;
	    continue;
	}
	else break;
    }
    if (!flag) return 0;
	
    /* 複数のタグにまたがっている場合は次のタグに進む */
    while (i >= tag_ptr->mrph_num) {
	i -= tag_ptr->mrph_num;
	tag_ptr++;
    }
    
    sprintf(buf, "C用;【%s】;=;0;%d;9.99:%s(同一文):%d文節",
	    cp, j, sp->KNPSID ? sp->KNPSID + 5 : "?", 
	    tag_ptr - sp->tag_data);
    assign_cfeature(&(tag_ptr->f), buf);
    assign_cfeature(&(tag_ptr->f), "役職");
    assign_cfeature(&(tag_ptr->f), "共参照");    
    return 1;
}

/*==================================================================*/
	       void corefer_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char *anaphor, *cp, *ne;
    MRPH_DATA *mrph_ptr;
    
    for (i = sp->Tag_num - 1; i >= 0; i--) { /* 解析文のタグ単位:i番目のタグについて */

	/* PERSON + 人名末尾 の処理 */
	if ((cp = check_feature((sp->tag_data + i)->f, "NE:PERSON"))) {
	    if (person_post(sp, sp->tag_data + i + 1, cp + 10, i)) continue;
	}

	/* 共参照解析を行う条件 */
	/* 照応詞候補であり、固有表現中の語、または */
	/* 連体詞形態指示詞以外に修飾されていない語 */
	if ((anaphor = check_feature((sp->tag_data + i)->f, "照応詞候補")) &&
	    (check_feature((sp->tag_data + i)->f, "NE") ||
	     check_feature((sp->tag_data + i)->f, "NE内") || /* DATA、LOCATIONなど一部 */
	     !get_modify_num(sp->tag_data + i) || /* 修飾されていない */
	     (((sp->tag_data + i)->mrph_ptr - 1)->Hinshi == 1 && 
	      ((sp->tag_data + i)->mrph_ptr - 1)->Bunrui == 2) || /* 直前が読点である */
	     check_feature(((sp->tag_data + i)->b_ptr->child[0])->f, "連体詞形態指示詞") ||
	     check_feature(((sp->tag_data + i)->b_ptr->child[0])->f, "照応接頭辞"))) {
	    
	    /* 指示詞の場合 */
	    if (check_feature((sp->tag_data + i)->f, "指示詞")) {
		continue; /* ここでは処理をしない */
	    }
	    
	    mrph_ptr = (sp->tag_data + i)->head_ptr + 1;
	    if (/* 名詞性接尾辞が付いた固有表現以外の語はまず接尾辞も含めたものを調べる */
		!((ne = check_feature((sp->tag_data + i)->f, "NE"))) &&
		mrph_ptr->Hinshi == 14 && mrph_ptr->Bunrui < 5 &&
		search_antecedent(sp, i, anaphor+11, mrph_ptr->Goi2, NULL) ||
		/* 一般の場合 */
		search_antecedent(sp, i, anaphor+11, NULL, ne)) {
		    continue;
	    }
	}
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			     共参照解析

                                               R.SASANO 05. 9.24

    $Id$
====================================================================*/
#include "knp.h"

int COREFER_ID = 0;
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
		int get_modify_num(TAG_DATA *tag_ptr)
/*==================================================================*/
{
    /* 並列を除いていくつの文節に修飾されているかを返す */
    /* ＡのＢＣとなっている場合はＡがＢに係っているかの判断も行う */

    int i, ret;
    BNST_DATA *b_ptr;

    b_ptr = tag_ptr->b_ptr;

    /* OptCorefer == 4の場合は修飾されているかどうかを用いない */
    if (OptCorefer == 4) return 0;

    /* 所属する文節が修飾されていない場合 */
    if (!b_ptr->child[0]) {
	return 0;
    }

    if (OptCorefer == 1) {
	/* タグが主辞でない場合 */
	/* 直前の文節の主辞との間に名詞格フレームに記された関係がなければ */
	/* 直前の文節は主辞に係っていると考え、修飾されていないとみなす */
	if (tag_ptr->head_ptr != b_ptr->head_ptr &&
	    (!(tag_ptr)->cf_ptr ||
	     check_examples((b_ptr - 1)->head_ptr->Goi2,
			    strlen((b_ptr - 1)->head_ptr->Goi2),
			    tag_ptr->cf_ptr->ex_list[0],
			    tag_ptr->cf_ptr->ex_num[0]) == -1)) {
	    return 0;
	}
    }
    else if (OptCorefer == 3) {
	/* 文節の主辞でないなら修飾されていないと判断する */
    	if (tag_ptr->head_ptr != b_ptr->head_ptr) {
	    return 0;   
	}
    }

    /* 所属する文節が修飾されていたらその数を返す */
    if ((b_ptr->child[0])->para_type) {
	b_ptr = b_ptr->child[0];
    }
    for (i = ret = 0; b_ptr->child[i]; i++) {
	if (!check_feature((b_ptr->child[i])->f, "係:カラ格") &&
	    !check_feature((b_ptr->child[i])->f, "係:同格未格") &&
	    !check_feature((b_ptr->child[i])->f, "係:同格連体") &&
	    !check_feature((b_ptr->child[i])->f, "係:同格連用"))
	    ret++; 
    }
    return ret;
}

/*==================================================================*/
	    void assign_anaphor_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 複合名詞に照応詞候補というfeatureを付与する */

    /* 複合名詞 */
    /* 固有表現は基本的にそのまま(LOCATION、DATEは分解) */
    /* それ以外は対象の語から文節先頭までを登録 */

    int i, j, k, l, tag_num, mrph_num;
    char word[WORD_LEN_MAX * 2], buf[WORD_LEN_MAX * 2], *cp;
    TAG_DATA *tag_ptr;      

    /* 文節単位で文の前から処理 */
    for (i = 0; i < sp->Bnst_num; i++) {
	
	if (!check_feature((sp->bnst_data + i)->f, "体言")) continue;

	tag_num = (sp->bnst_data + i)->tag_num;
	tag_ptr = (sp->bnst_data + i)->tag_ptr;
	
  	for (j = tag_num - 1; j >= 0; j--) {
	    
	    /* 固有表現内である場合 */
	    if (check_feature((tag_ptr + j)->f, "NE") ||
		check_feature((tag_ptr + j)->f, "NE内")) {
		
		/* 固有表現の主辞には付与 */
		if ((cp = check_feature((tag_ptr + j)->f, "NE"))) {
		    cp += 3; /* "NE:"を読み飛ばす */
		    while (strncmp(cp, ":", 1)) cp++;
		    sprintf(buf, "照応詞候補:%s", cp + 1);
		    assign_cfeature(&((tag_ptr + j)->f), buf, FALSE);
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
		    sprintf(buf, "照応詞候補:%s", word);
		    assign_cfeature(&((tag_ptr + j)->f), buf, FALSE);
		}
	    }
	    
	    else {
		/* 固有表現内の語を主辞としない場合
		   
		/* 数詞、形式名詞、副詞的名詞 */
		/* および隣に係る形容詞は除外 */
		if ((tag_ptr + j)->head_ptr->Hinshi == 6 &&
		    (tag_ptr + j)->head_ptr->Bunrui > 7 &&
		    (tag_ptr + j)->head_ptr->Bunrui != 10 ||
		    (tag_ptr + j)->head_ptr->Hinshi == 3 &&
		    check_feature((tag_ptr + j)->f, "係:隣")) {
		    continue;
		}

		word[0] = '\0';
		for (k = (tag_ptr + j)->head_ptr - (sp->bnst_data + i)->mrph_ptr; 
		     k >= 0; k--) {
		    
		    /* 先頭の特殊、照応接頭辞は含めない */
		    if (!strncmp(word, "\0", 1) &&
			(((tag_ptr + j)->head_ptr - k)->Hinshi == 1 ||
			 check_feature(((tag_ptr + j)->head_ptr - k)->f, "照応接頭辞")))
			continue;
		    
		    /* 「・」などより前は含めない */
		    if (!strcmp(((tag_ptr + j)->head_ptr - k)->Goi2, "・") ||
			check_feature(((tag_ptr + j)->head_ptr - k)->f, "括弧終")) {
			if (k > 0) word[0] = '\0';
		    }
		    else {
			strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
		    }
		}

		if (strncmp(word, "\0", 1)) {
		    sprintf(buf, "照応詞候補:%s", word);
		    assign_cfeature(&((tag_ptr + j)->f), buf, FALSE);
		}
	    }
	}
    }

    /* 括弧内のふりがなへの対応 */
    for (j = 0; j < sp->Tag_num; j++) {
	
	for (i = 0; i < (sp->tag_data + j)->mrph_num; i++) {

	    if (j + 1 < sp->Tag_num &&
		(check_feature(((sp->tag_data + j)->mrph_ptr + i + 1)->f, 
			      "ひらがな") ||
		!strcmp(((sp->tag_data + j)->mrph_ptr + i + 1)->Goi2, "・"))) break;
	    
	    word[0] = '\0';
	    for (k = 0; (sp->tag_data + j)->mrph_ptr + i - k > sp->mrph_data; k++) {
		if (!check_feature(((sp->tag_data + j)->mrph_ptr + i - k)->f, 
				   "ひらがな") &&
		    strcmp(((sp->tag_data + j)->mrph_ptr + i - k)->Goi2, "・")) break;
	    }
	    if (k > 0 && 
		check_feature(((sp->tag_data + j)->mrph_ptr + i - k)->f, "括弧始")) {
		for (k = k - 1; k >= 0; k--) 
		    strcat(word, ((sp->tag_data + j)->mrph_ptr + i - k)->Goi2);
	    }
	    
	    if (strncmp(word, "\0", 1)) {
		sprintf(buf, "照応詞候補:%s", word);
		assign_cfeature(&((sp->tag_data + j)->f), buf, FALSE);
		assign_cfeature(&((sp->tag_data + j)->f), "読み方", FALSE);
	    }
	}		
    }
}

/*==================================================================*/
int compare_strings(char *antecedent, char *anaphor, char *ant_ne, char *ana_ne, int flag)
/*==================================================================*/
{
    /* 照応詞候補と先行詞候補を比較 */
    /* 先行詞候補を絞り込んでいないためやや効率が悪い */
    
    int i, j, left, right;
    char word[WORD_LEN_MAX * 4];

    /* 読み方の場合 */
    if (flag) { 
    /* ex. 中島河太郎（なかじま・かわたろう) */
    /* 基準
       とりあえず人名の場合のみ
       前方マッチ文字数×後方マッチ文字数×2 > anaphora文字数
       である場合、読み方を表わしていると判定 
       ただしantecedentの直後が<括弧始>の場合(flag=2)の場合は連続していると考え
       マッチ文字数の少ない方に2文字ボーナス
    */
           
	if (!ant_ne || strncmp(ant_ne, "NE:PERSON", 7)) return 0;

	left = right = flag - 0;
	for (i = 0; i < strlen(anaphor); i += 2) {
	    if (strncmp(antecedent + i, anaphor + i, 2)) {
		break;
	    }
	    left++;
	}  
	for (j = 0; j < strlen(anaphor); j += 2) {
	    if (strncmp(antecedent + strlen(antecedent) - j - 2, 
			anaphor + strlen(anaphor) - j - 2, 2)) {
		break;
	    } 
	    right++;
	}
	if (flag == 2) (left > right) ? (right += 2) : (left += 2);
	if (left * right * 4 > strlen(anaphor)) return 1;
	return 0;
    }

    /* 異なる種類の固有表現の場合は不可 */ 
    if (ana_ne && ant_ne && strncmp(ana_ne, ant_ne, 7)) return 0;

    /* 同表記の場合 */
    if (!strcmp(antecedent, anaphor)) return 1;

    /* 固有表現が同表記の場合(文節をまたがる固有表現のため) */
    if (ant_ne && ana_ne && !strcmp(ant_ne, ana_ne)) {
	return 1;
    }

    /* 先行詞がPERSONである場合は照応詞候補が先行詞候補の先頭に含まれていればOK */
    /* 先行詞がLOCATIONである場合はさらに照応詞候補が1文字だけ短かい場合のみOK */
    /* ex. 村山富市=村山、大分県=大分 */
    if (ant_ne && strlen(ant_ne) > strlen(antecedent) && /* 先行詞がNE全体である */
	!strcmp(ant_ne + strlen(ant_ne) - strlen(antecedent), antecedent) &&
	(!strncmp(ant_ne, "NE:PERSON", 7) && ana_ne && !strncmp(ana_ne, "NE:PERSON", 7) || 
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
    /* 「金融派生商品-取引」と「デリバティブ-取引」は認識できる */
    /* 「日本銀行」と「日銀」のように同義表現が同じ文字を含む場合は認識できない */
    for (i = 0; i < strlen(anaphor); i += 2) {
	if (strncmp(antecedent + i, anaphor + i, 2)) {
	    break;
	}
    }  
    for (j = 0; j < strlen(anaphor); j += 2) {
	if (strncmp(antecedent + strlen(antecedent) - j - 2, 
		    anaphor + strlen(anaphor) - j - 2, 2)) {
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

    int j, k, l, m, yomi_flag;
    char word[WORD_LEN_MAX], word2[WORD_LEN_MAX], word3[WORD_LEN_MAX], buf[WORD_LEN_MAX];
    char *cp, *ant_ne, CO[WORD_LEN_MAX];
    SENTENCE_DATA *sdp;
    TAG_DATA *tag_ptr;
 
    yomi_flag = (check_feature((sp->tag_data + i)->f, "読み方")) ? 1 : 0;

    sdp = sentence_data + sp->Sen_num - 1;
    for (j = 0; j <= sdp - sentence_data; j++) { /* 照応先が何文前か */
	
	for (k = j ? (sdp - j)->Tag_num - 1 : i - 1; k >= 0; k--) { /* 照応先のタグ */
   
	    tag_ptr = (sdp - j)->tag_data + k;	    		
	    ant_ne = check_feature(tag_ptr->f, "NE");
		
	    /* 照応詞候補である場合以外は先行詞候補としない */
	    if (!check_feature(tag_ptr->f, "照応詞候補")) continue;
			
	    /* setubiが与えられた場合、後続の名詞性接尾を比較 */
	    if (setubi && strcmp((tag_ptr->head_ptr + 1)->Goi2, setubi)) continue;

	    for (l = tag_ptr->head_ptr - (tag_ptr->b_ptr)->mrph_ptr; l >= 0; l--) {

		word[0] = word2[0] = word3[0] = '\0';
		for (m = l; m >= 0; m--) {
		    /* 先頭の特殊、照応接頭辞は含めない */
		    if (!strncmp(word, "\0", 1) &&
			((tag_ptr->head_ptr - m)->Hinshi == 1 ||
			 check_feature((tag_ptr->head_ptr - m)->f, "照応接頭辞")))
			continue;
		    /* 「・」などより前は含めない */
		    if (!strcmp((tag_ptr->head_ptr - m)->Goi2, "・") ||
			!strcmp((tag_ptr->head_ptr - m)->Goi2, "＝") ||
			check_feature((tag_ptr->head_ptr - m)->f, "括弧終")) {
			word[0] = '\0';
		    }
		    else {
			strcat(word, (tag_ptr->head_ptr - m)->Goi2); /* 先行詞候補 */
		    }
		    strcat(word2, (tag_ptr->head_ptr - m)->Goi2); /* 先行詞候補2 */
		    strcat(word3, (tag_ptr->head_ptr - m)->Yomi); /* 先行詞候補3 */
		}	
		if (!strncmp(word, "\0", 1)) continue;
		if (strlen(word) > strlen(check_feature(tag_ptr->f, "照応詞候補")) - 11)
		    continue;

		if (compare_strings(word, anaphor, ant_ne, ne, 0) ||
		    compare_strings(word2, anaphor, ant_ne, ne, 0) ||
		    /* 読み方の場合 */
		    yomi_flag && j == 0 && (i - k < 10) &&
		    compare_strings(word3, anaphor, ant_ne, ne, 1) ||
		    yomi_flag && j == 0 && (i - k < 10) &&
		    check_feature((tag_ptr->head_ptr + 1)->f, "括弧始") &&
		    compare_strings(word3, anaphor, ant_ne, ne, 2) ||
		    /* 人称名詞の場合の特例 */
		    (check_feature((sp->tag_data + i)->f, "人称代名詞") &&
		     check_feature(tag_ptr->f, "NE:PERSON")) ||
		    /* 自称名詞の場合の特例 */
		    (!j && (k == i - 1) && check_feature(tag_ptr->f, "Ｔ解析格-ガ") &&
		     check_feature((sp->tag_data + i)->f, "Ｔ自称名詞") &&
		     sm_match_check(sm2code("主体"), tag_ptr->SM_code, SM_NO_EXPAND_NE)))
		    {
		    
		    /* 「・」などより前を含めた場合のみ同義表現があった場合 */
		    if (!compare_strings(word, anaphor, ant_ne, ne, 0) &&
			compare_strings(word2, anaphor, ant_ne, ne, 0)) {
			strcpy(word, word2);
		    }

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
		    assign_cfeature(&((sp->tag_data + i)->f), buf, FALSE);
		    assign_cfeature(&((sp->tag_data + i)->f), "共参照", FALSE); 

		    /* COREFER_IDを付与 */   
		    if ((cp = check_feature(tag_ptr->f, "COREFER_ID"))) {
			assign_cfeature(&((sp->tag_data + i)->f), cp, FALSE);
		    }
		    else {
			COREFER_ID++;
			sprintf(CO, "COREFER_ID:%d", COREFER_ID);
			assign_cfeature(&((sp->tag_data + i)->f), CO, FALSE);
			assign_cfeature(&(tag_ptr->f), CO, FALSE);
		    }
    
		    /* 固有表現とcoreferの関係にある語を固有表現とみなす */
		    if (OptNE) {
			if (!check_feature((sp->tag_data + i)->f, "NE") &&
			    !check_feature((sp->tag_data + i)->f, "NE内") &&
			    !check_feature((sp->tag_data + i)->f, "人称代名詞") &&
			    !check_feature((sp->tag_data + i)->f, "Ｔ自称名詞") &&
			    (cp = check_feature(tag_ptr->f, "NE")) && !setubi ||
			    yomi_flag && 
			    (cp = check_feature(tag_ptr->f, "NE:PERSON"))) {
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
    char buf[WORD_LEN_MAX], CO[WORD_LEN_MAX];
    MRPH_DATA *mrph_ptr;
    TAG_DATA *tag_ptr_cp;
    
    tag_ptr_cp = tag_ptr;
    mrph_ptr = tag_ptr->mrph_ptr;
    /* タグ末尾までNE中である場合のみ対象とする */
    if (!check_feature((mrph_ptr - 1)->f, "NE") &&
	!(check_feature((mrph_ptr - 2)->f, "NE") &&
	  (mrph_ptr - 1)->Hinshi == 1 && 
	  (mrph_ptr - 1)->Bunrui == 5)) /* 直前が記号である */
	return 0;

    flag = 0;
    for (i = 0;; i++) {
	if (check_feature((mrph_ptr + i)->f, "人名末尾")) {
	    flag = 1;
	    continue;
	}
	else if (check_feature((mrph_ptr + i)->f, "NE") ||
		 check_feature((mrph_ptr + i)->f, "固有修飾")) {
	    /* 基本的には、ブッシュ・アメリカ大統領 */
	    /* 武部自民党幹事長などを想定している */
	    /* ギングリッチ新下院議長 */
	    continue;
	}
	else break;
    }
    if (!flag) return 0;
	
    /* 複数のタグにまたがっている場合は次のタグに進む */
    while (i > tag_ptr->mrph_num) {
	i -= tag_ptr->mrph_num;
	tag_ptr++;
    }
    
    sprintf(buf, "C用;【%s】;=;0;%d;9.99:%s(同一文):%d文節",
	    cp, j, sp->KNPSID ? sp->KNPSID + 5 : "?", 
	    tag_ptr - sp->tag_data);
    assign_cfeature(&(tag_ptr->f), buf, FALSE);
    assign_cfeature(&(tag_ptr->f), "共参照(役職)", FALSE);
    
    /* COREFER_IDを付与 */   
    if (cp = check_feature(tag_ptr->f, "COREFER_ID")) {
	assign_cfeature(&((tag_ptr_cp - 1)->f), cp, FALSE);
    }
    else if (cp = check_feature((tag_ptr_cp - 1)->f, "COREFER_ID")) {
	assign_cfeature(&(tag_ptr->f), cp, FALSE);
    }
    else {
	COREFER_ID++;
	sprintf(CO, "COREFER_ID:%d", COREFER_ID);
	assign_cfeature(&(tag_ptr->f), CO, FALSE);
	assign_cfeature(&((sp->tag_data + i)->f), CO, FALSE);
    }
    
    return 1;
}

/*==================================================================*/
	       void corefer_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, person;
    char *anaphor, *cp, *ne;
    MRPH_DATA *mrph_ptr;

    for (i = 0; i < sp->Tag_num; i++) { /* 解析文のタグ単位:i番目のタグについて */

	/* 共参照解析を行う条件 */
	/* 照応詞候補であり、固有表現中の語、または */
	/* 連体詞形態指示詞以外に修飾されていない語 */
	if ((anaphor = check_feature((sp->tag_data + i)->f, "照応詞候補")) &&
	    (check_feature((sp->tag_data + i)->f, "NE") ||  
	     check_feature((sp->tag_data + i)->f, "NE内") || /* DATA、LOCATIONなど一部 */
	     check_feature((sp->tag_data + i)->f, "読み方") ||
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
	    /* 名詞性接尾辞が付いた固有表現以外の語はまず接尾辞も含めたものを調べる */
	    if (!((ne = check_feature((sp->tag_data + i)->f, "NE"))) &&
		mrph_ptr->Hinshi == 14 && mrph_ptr->Bunrui < 5)
		search_antecedent(sp, i, anaphor+11, mrph_ptr->Goi2, NULL);
    
	    /* 一般の場合 */
	    search_antecedent(sp, i, anaphor+11, NULL, ne);
	}
	/* PERSON + 人名末尾 の処理 */
	if ((cp = check_feature((sp->tag_data + i)->f, "NE:PERSON")) &&
	    i + 1 < sp->Tag_num) {
	    person_post(sp, sp->tag_data + i + 1, cp + 10, i);
	}
    }
}

/*==================================================================*/
int search_antecedent_after_br(SENTENCE_DATA *sp, TAG_DATA *tag_ptr1, int i)
/*==================================================================*/
{
    int j, k, l, tag, sent;
    char *cp, buf[WORD_LEN_MAX], CO[WORD_LEN_MAX];
    SENTENCE_DATA *sdp;
    TAG_DATA *tag_ptr, *tag_ptr2;
 
    sdp = sentence_data + sp->Sen_num - 1;
    for (j = 0; j <= sdp - sentence_data; j++) { /* 照応先が何文前か */
	
	for (k = j ? (sdp - j)->Tag_num - 1 : i - 1; k >= 0; k--) { /* 照応先のタグ */
   
	    tag_ptr = (sdp - j)->tag_data + k;	    		
		
	    /* 照応詞候補である場合以外は先行詞候補としない */
	    if (!check_feature(tag_ptr->f, "照応詞候補")) continue;

	    /* 照応詞候補と同じ表記のものしか先行詞候補としない */
	    if (strcmp((sp->tag_data + i)->head_ptr->Goi2, tag_ptr->head_ptr->Goi2))
		continue;
	    
	    /* 格解析結果がある */
	    sprintf(buf, "格解析結果:%s:名1", tag_ptr->head_ptr->Goi2);
	    cp = check_feature(tag_ptr->f, buf);
	    if (!cp) continue;
	    
	    /* <格解析結果:結果:名1:ノ/O/アンケート/0/1/?> */
	    for (l = 0; l < 3; l++) {
		while (strncmp(cp, "/", 1)) cp++;
		    cp++;
	    }
	    if (!sscanf(cp, "%d/%d/", &tag, &sent)) continue;
  
	    /* 指示先のタグへのポインタ */
	    tag_ptr2 = (sdp - j - sent)->tag_data + tag;

	    /* 指示先のタグが共参照関係にあるかを判定 */
	    if (check_feature(tag_ptr1->f, "COREFER_ID") &&
		check_feature(tag_ptr2->f, "COREFER_ID") &&
		!strcmp(check_feature(tag_ptr1->f, "COREFER_ID"),
			check_feature(tag_ptr2->f, "COREFER_ID"))) {

		cp = check_feature(tag_ptr->f, "照応詞候補");
		cp += 11;
		
		if (j == 0) {
		    sprintf(buf, "C用;【%s】;=;0;%d;9.99:%s(同一文):%d文節",
			    cp, k, sp->KNPSID ? sp->KNPSID + 5 : "?", k);
		}
		else {
		    sprintf(buf, "C用;【%s】;=;%d;%d;9.99:%s(%d文前):%d文節",
			    cp, j, k, 
			    (sdp - j)->KNPSID ? (sdp - j)->KNPSID + 5 : "?", j, k);
		}
		assign_cfeature(&((sp->tag_data + i)->f), buf, FALSE);
		assign_cfeature(&((sp->tag_data + i)->f), "共参照", FALSE); 
		
		/* COREFER_IDを付与 */   
		if ((cp = check_feature(tag_ptr->f, "COREFER_ID"))) {
		    assign_cfeature(&((sp->tag_data + i)->f), cp, FALSE);
		}
		else if ((cp = check_feature((sp->tag_data + i)->f, "COREFER_ID"))) {
		    assign_cfeature(&(tag_ptr->f), cp, FALSE);
		}
		else {
		    COREFER_ID++;
		    sprintf(CO, "COREFER_ID:%d", COREFER_ID);
		    assign_cfeature(&((sp->tag_data + i)->f), CO, FALSE);
		    assign_cfeature(&(tag_ptr->f), CO, FALSE);
		}
		return 1;
	    }    
	}
    }
}

/*==================================================================*/
	  void corefer_analysis_after_br(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, tag, sent;
    char *cp, buf[WORD_LEN_MAX];
    MRPH_DATA *mrph_ptr;
    TAG_DATA *tag_ptr;

    for (i = 0; i < sp->Tag_num; i++) {

	/* 照応詞候補である場合以外は先行詞候補としない */
	if (!check_feature((sp->tag_data + i)->f, "照応詞候補")) continue;
	/* 名詞に限定(接尾辞は対象外) */
	if ((sp->tag_data + i)->head_ptr->Hinshi != 6) continue;

	/* 共参照タグがなく、格解析結果がある */
	sprintf(buf, "格解析結果:%s:名1", (sp->tag_data + i)->head_ptr->Goi2);
	if (!check_feature((sp->tag_data + i)->f, "COREFER_ID") &&
	    (cp = check_feature((sp->tag_data + i)->f, buf))) {

	    /* <格解析結果:結果:名1:ノ/O/アンケート/0/1/?> */
	    for (j = 0; j < 3; j++) {
		while (strncmp(cp, "/", 1)) cp++;
		cp++;
	    }
	    if (sscanf(cp, "%d/%d/", &tag, &sent)) {
		/* 指示先のタグへのポインタ */
		tag_ptr = ((sentence_data + sp->Sen_num - 1 - sent)->tag_data + tag);
		search_antecedent_after_br(sp, tag_ptr, i);
	    }
	}	
    }	   
}
/*====================================================================
                               END
====================================================================*/

/*====================================================================

			     FEATURE処理

                                               S.Kurohashi 96. 7. 4

    $Id$
====================================================================*/
#include "knp.h"

/*
  FEATUREの処理には次の３種類がある

  	(1) ファイル(S式または文字列) ==コピー==> ルール構造体

	(2) ルール構造体 ==付与==> 形態素または文節構造体
        	<○:□>は<○:…>というFEATUREへの上書き (なければ新規)
                <^○>は<○:…>の削除 (なければ無視)
		<&○>は関数呼出
			&表層:付与 -- 辞書引きによる表層格付与
			&表層:削除 -- すべての表層格削除
			&表層:○格 -- ○格付与
			&表層:^○格 -- ○格削除
			&MEMO:○ -- MEMOへの書き込み
			&#num:○ -- ○とnum番目の変数による新たなFEATUREを付与

	(3) ルール構造体 <==照合==> 形態素または文節構造体
	       	<○>は<○:…>というFEATUREがあればOK
	    	<^○>は<○:…>というFEATUREがなければOK
	    	<&○>は関数呼出
			&記英数カ -- 表記が記号,英文字,数字,カタナナ (形態素)
			&漢字 -- 表記が漢字 (形態素)
	    		&表層:○格 -- ○格がある (文節)
	    		&表層:照合 -- 係の表層格が受にある (係受)
			&D:n -- 構造体間が距離n以内 (係受)
			&レベル:強 -- 受が係以上 (係受)
			&レベル:l -- 自身がl以上 (係受)
			&係側:○ -- 係に○ (係受)
			&#num:○ -- <○:…>というFEATUREがあればOK
			            <○:…>をnum番目の変数に格納

	※ プログラム内で形態素または文節構造体にFEATUREを与える
	場合は(2)のなかの assign_cfeature を用いる．

	※ プログラム内で形態素または文節構造体があるFEATUREを持つ
	かどうかを調べる場合は(3)のなかの check_feature を用いる．
*/

/*==================================================================*/
	      void print_feature(FEATURE *fp, FILE *filep)
/*==================================================================*/
{
    /* <f1><f2> ... <f3> という形式の出力 
       (ただしＴではじまるfeatureは表示しない) */

    while (fp) {
	if (fp->cp && strncmp(fp->cp, "Ｔ", 2))
	    fprintf(filep, "<%s>", fp->cp); 
	fp = fp->next;
    }
}

/*==================================================================*/
	  void print_some_feature(FEATURE *fp, FILE *filep)
/*==================================================================*/
{
    /* <f1><f2> ... <f3> という形式の出力 
       指定したものだけを表示 */

    while (fp) {
	if (fp->cp && strncmp(fp->cp, "Ｃ", 2) && !strncmp(fp->cp, "C", 1))
	    fprintf(filep, "<%s>", fp->cp); 
	fp = fp->next;
    }
}
/*==================================================================*/
	      void print_feature2(FEATURE *fp, FILE *filep)
/*==================================================================*/
{
    /* (f1 f2 ... f3) という形式の出力
       (ただしＴではじまるfeatureは表示しない) */
    if (fp) {
	fprintf(filep, "("); 
	while (fp) {
	    if (fp->cp && strncmp(fp->cp, "Ｔ", 2)) {
		fprintf(filep, "%s", fp->cp); 
		if (fp->next) fprintf(filep, " "); 		
	    }
	    fp = fp->next;
	}
	fprintf(filep, ")"); 
    } else {
	fprintf(filep, "NIL"); 
    }
}

/*==================================================================*/
		   void clear_feature(FEATURE **fpp)
/*==================================================================*/
{
    FEATURE *fp, *next;

    fp = *fpp;
    *fpp = NULL;

    while (fp) {
	next = fp->next;
	free(fp->cp);
	free(fp);
	fp = next;
    }
}

/*
 *
 *  ファイル(S式または文字列) ==コピー==> ルール構造体
 *
 */

/*==================================================================*/
	   void copy_cfeature(FEATURE **fpp, char *fname)
/*==================================================================*/
{
    while (*fpp) fpp = &((*fpp)->next);

    if (!((*fpp) = (FEATURE *)(malloc(sizeof(FEATURE)))) ||
	!((*fpp)->cp = (char *)(malloc(strlen(fname) + 1)))) {
	fprintf(stderr, "Can't allocate memory for FEATURE\n");
	exit(-1);
    }
    strcpy((*fpp)->cp, fname);
    (*fpp)->next = NULL;
}

/*==================================================================*/
	      void list2feature(CELL *cp, FEATURE **fpp)
/*==================================================================*/
{
    while (!Null(car(cp))) {
	copy_cfeature(fpp, _Atom(car(cp)));
	fpp = &((*fpp)->next);
	cp = cdr(cp);	    
    }
}

/*==================================================================*/
      void list2feature_pattern(FEATURE_PATTERN *f, CELL *cell)
/*==================================================================*/
{
    /* リスト ((文頭)(体言)(提題)) などをFEATURE_PATTERNに変換 */

    int nth = 0;

    while (!Null(car(cell))) {
	clear_feature(f->fp+nth);		/* ?? &(f->fp[nth]) */ 
	list2feature(car(cell), f->fp+nth);	/* ?? &(f->fp[nth]) */ 
	cell = cdr(cell);
	nth++;
    }
    f->fp[nth] = NULL;
}

/*==================================================================*/
      void string2feature_pattern_OLD(FEATURE_PATTERN *f, char *cp)
/*==================================================================*/
{
    /* 文字列 "文頭|体言|提題" などをFEATURE_PATTERNに変換
       本来list2feature_patternに対応するものだが,
       ORだけでANDはサポートしていない */

    int nth = 0;
    char buffer[256], *scp, *ecp;

    if (cp == NULL || cp[0] == NULL) {
	f->fp[nth] = NULL;
	return;
    }

    strcpy(buffer, cp);
    scp = ecp = buffer;
    while (*ecp) {
	if (*ecp == '|') {
	    *ecp = NULL;
	    clear_feature(f->fp+nth);		/* ?? &(f->fp[nth]) */
	    copy_cfeature(f->fp+nth, scp);	/* ?? &(f->fp[nth]) */
	    nth++;
	    scp = ecp + 1; 
	}
	ecp ++;
    }
    
    clear_feature(f->fp+nth);			/* ?? &(f->fp[nth]) */ 
    copy_cfeature(&(f->fp[nth]), scp);
    nth++;

    f->fp[nth] = NULL;
}

/*==================================================================*/
      void string2feature_pattern(FEATURE_PATTERN *f, char *cp)
/*==================================================================*/
{
    /* 文字列 "文頭|体言|提題" などをFEATURE_PATTERNに変換
       本来list2feature_patternに対応するものだが,
       ORだけでANDはサポートしていない */

    int nth;
    char buffer[256], *start_cp, *loop_cp;
    FEATURE **fpp;
    
    if (!*cp) {
	f->fp[0] = NULL;
	return;
    }

    strcpy(buffer, cp);
    nth = 0;
    clear_feature(f->fp+nth);
    fpp = f->fp+nth;
    loop_cp = buffer;
    start_cp = loop_cp;
    while (*loop_cp) {
	if (*loop_cp == '&' && *(loop_cp+1) == '&') {
	    *loop_cp = '\0';
	    copy_cfeature(fpp, start_cp);
	    fpp = &((*fpp)->next);
	    loop_cp += 2;
	    start_cp = loop_cp;
	}
	else if (*loop_cp == '|' && *(loop_cp+1) == '|') {
	    *loop_cp = '\0';
	    copy_cfeature(fpp, start_cp);
	    nth++;
	    clear_feature(f->fp+nth);
	    fpp = f->fp+nth;
	    loop_cp += 2;
	    start_cp = loop_cp;
	}
	else {
	    loop_cp ++;
	}
    }
    copy_cfeature(fpp, start_cp);

    nth++;
    f->fp[nth] = NULL;
}

/*
 *
 * ルール構造体 ==付与==> 形態素または文節構造体
 *
 */

/*==================================================================*/
	   void assign_cfeature(FEATURE **fpp, char *fname)
/*==================================================================*/
{
    char type[256];

    /* 上書きの可能性をチェック */

    sscanf(fname, "%[^:]", type);	/* ※ fnameに":"がない場合は
					   typeはfname全体になる */
    while (*fpp) {
	if (comp_feature((*fpp)->cp, type) == TRUE) {
	    free((*fpp)->cp);
	    if (!((*fpp)->cp = (char *)(malloc(strlen(fname) + 1)))) {
		fprintf(stderr, "Can't allocate memory for FEATURE\n");
		exit(-1);
	    }
	    strcpy((*fpp)->cp, fname);
	    return;	/* 上書きで終了 */
	}
	fpp = &((*fpp)->next);
    }

    /* 上書きできなければ末尾に追加 */

    if (!((*fpp) = (FEATURE *)(malloc(sizeof(FEATURE)))) ||
	!((*fpp)->cp = (char *)(malloc(strlen(fname) + 1)))) {
	fprintf(stderr, "Can't allocate memory for FEATURE\n");
	exit(-1);
    }
    strcpy((*fpp)->cp, fname);
    (*fpp)->next = NULL;
}    

/*==================================================================*/
    void assign_feature(FEATURE **fpp1, FEATURE **fpp2, void *ptr)
/*==================================================================*/
{
    /*
     *  ルールを適用の結果，ルールから構造体にFEATUREを付与する
     *  構造体自身に対する処理も可能としておく
     */

    int i, flag;
    char *cp;
    FEATURE **fpp, *next;

    while (*fpp2) {

	if (*((*fpp2)->cp) == '^') {	/* 削除の場合 */
	    
	    fpp = fpp1;
	    
	    while (*fpp) {
		if (comp_feature((*fpp)->cp, &((*fpp2)->cp[1])) == TRUE) {
		    free((*fpp)->cp);
		    next = (*fpp)->next;
		    free(*fpp);
		    *fpp = next;
		} else {
		    fpp = &((*fpp)->next);
		}
	    }
	
	} else if (*((*fpp2)->cp) == '&') {	/* 関数の場合 */

	    if (!strcmp((*fpp2)->cp, "&表層:付与")) {
		set_pred_voice((BNST_DATA *)ptr);	/* ヴォイス */
		get_scase_code((BNST_DATA *)ptr);	/* 表層格 */
	    }
	    else if (!strcmp((*fpp2)->cp, "&表層:削除")) {
		for (i = 0, cp = ((BNST_DATA *)ptr)->SCASE_code; 
		     i < SCASE_CODE_SIZE; i++, cp++) 
		    *cp = 0;		
	    }
	    else if (!strncmp((*fpp2)->cp, "&表層:^", strlen("&表層:^"))) {
		((BNST_DATA *)ptr)->
		    SCASE_code[case2num((*fpp2)->cp + strlen("&表層:^"))] = 0;
	    }
	    else if (!strncmp((*fpp2)->cp, "&表層:", strlen("&表層:"))) {
		((BNST_DATA *)ptr)->
		    SCASE_code[case2num((*fpp2)->cp + strlen("&表層:"))] = 1;
	    }
	    else if (!strncmp((*fpp2)->cp, "&MEMO:", strlen("&MEMO:"))) {
		strcat(PM_Memo, " ");
		strcat(PM_Memo, (*fpp2)->cp + strlen("&MEMO:"));
	    }
	    else if (!strncmp((*fpp2)->cp, "&#", strlen("&#"))) {
		int gnum;
		char fprefix[64], fname[64];
		sscanf((*fpp2)->cp, "&#%d:%s", &gnum, fprefix);
		sprintf(fname, "%s%s", fprefix, G_Feature[gnum]);
		assign_cfeature(fpp1, fname);
	    }
	    else if (!strncmp((*fpp2)->cp, "&品詞変更:", strlen("&品詞変更:"))) {
		change_mrph((MRPH_DATA *)ptr, *fpp2);
	    }
	    else if (!strncmp((*fpp2)->cp, "&意味素付与:", strlen("&意味素付与:"))) {
		assign_sm((BNST_DATA *)ptr, (*fpp2)->cp + strlen("&意味素付与:"));
	    }
	    /*
	    else if (!strncmp((*fpp2)->cp, "&辞書", strlen("&辞書"))) {
		assign_f_from_dic(fpp1, ((MRPH_DATA *)ptr)->Goi);
	    }
	    */
	} else {			/* 追加の場合 */
	    assign_cfeature(fpp1, (*fpp2)->cp);	
	}

	fpp2 = &((*fpp2)->next);
    }
}

/*
 *
 * ルール構造体 <==照合==> 形態素または文節構造体
 *
 */

/*==================================================================*/
	     int comp_feature(char *data, char *pattern)
/*==================================================================*/
{
    /* 
     *  完全一致 または 部分一致(patternが短く,次の文字が':')ならマッチ
     */
    
    if (data && !strcmp(data, pattern)) {
	return TRUE;
    } else if (data && !strncmp(data, pattern, strlen(pattern)) &&
	       data[strlen(pattern)] == ':') {
	return TRUE;
    } else {
	return FALSE;
    }
}

/*==================================================================*/
	     int comp_feature_NE(char *data, char *pattern)
/*==================================================================*/
{
    char decision[9];

    decision[0] = '\0';
    sscanf(data, "%*[^:]:%*[^:]:%s", decision);

    if (decision[0] && !strcmp(decision, pattern))
	return TRUE;
    else
	return FALSE;
}

/*==================================================================*/
	    char *check_feature(FEATURE *fp, char *fname)
/*==================================================================*/
{
    while (fp) {
	if (comp_feature(fp->cp, fname) == TRUE) {
	    return fp->cp;
	}
	fp = fp->next;
    }
    return NULL;
}

/*==================================================================*/
	    char *check_feature_NE(FEATURE *fp, char *fname)
/*==================================================================*/
{
    while (fp) {
	if (comp_feature_NE(fp->cp, fname) == TRUE) {
	    return fp->cp;
	}
	fp = fp->next;
    }
    return NULL;
}

/*==================================================================*/
	      int whether_corpus_compare(BNST_DATA *bp)
/*==================================================================*/
{
    /* マッチすれば、FALSE をかえし、
       統計的情報を用いた述語の係り受け解析を行わない。
       */
    char *type;

    /* 文節ポインタが NULL? */
    if (bp == NULL)
	return FALSE;

    type = (char *)check_feature(bp->f, "ID");

    if (type) {
	type +=3;
	if (!strcmp(type, "〜と（引用）") || 
	    !strcmp(type, "（弱連用）") || 
	    !strcmp(type, "（区切）") || 
	    !strcmp(type, "（複合辞連用）") || 
	    !strcmp(type, "〜の〜"))
	    return FALSE;
    }
    
    return TRUE;
}

/*==================================================================*/
	int compare_threshold(int value, int threshold, char *eq)
/*==================================================================*/
{
    if (str_eq(eq, "lt")) {
	if (value < threshold)
	    return TRUE;
	else
	    return FALSE;
    }
    else if (str_eq(eq, "le")) {
	if (value <= threshold)
	    return TRUE;
	else
	    return FALSE;
    }
    else if (str_eq(eq, "gt")) {
	if (value > threshold)
	    return TRUE;
	else
	    return FALSE;
    }
    else if (str_eq(eq, "ge")) {
	if (value >= threshold)
	    return TRUE;
	else
	    return FALSE;
    }
    return FALSE;
}

/*==================================================================*/
	int _check_function_NE(struct _pos_s *p, char *class)
/*==================================================================*/
{
    if (str_eq(class, "地名")) {
	return p->Location;
    }
    else if (str_eq(class, "人名")) {
	return p->Person;
    }
    else if (str_eq(class, "組織名")) {
	return p->Organization;
    }
    else if (str_eq(class, "固有名詞")) {
	return p->Artifact;
    }
    else if (str_eq(class, "その他")) {
	return p->Others;
    }
    return FALSE;
}

/*==================================================================*/
      int check_Bunrui_others(MRPH_DATA *mp, int flag)
/*==================================================================*/
{
    if (mp->Bunrui != 3 && /* 固有名詞 */
	mp->Bunrui != 4 && /* 地名 */
	mp->Bunrui != 5 && /* 人名 */
	mp->Bunrui != 6) /* 組織名 */
	return flag;

    if (check_feature(mp->f, "品曖-その他"))
	return flag;

    return 1-flag;
}

/*==================================================================*/
      int check_Bunrui(MRPH_DATA *mp, char *class, int flag)
/*==================================================================*/
{
    char string[14];

    if (str_eq(Class[6][mp->Bunrui].id, class))
	return flag;

    sprintf(string, "品曖-%s", class);
    if (check_feature(mp->f, string))
	return flag;

    return 1-flag;
}

/*==================================================================*/
      int check_function_NE(char *rule, void *ptr1, void *ptr2)
/*==================================================================*/
{
    char category[7], cp1[128], cp2[128], cp3[128], cp4[128];
    int n, threshold;

   /* 5 XB:かな漢字:人名:le:33
      4 単語:人名:gt:3
      3 細:地名:1
      2 文字種:カタカナ
      4 文字種:地名:le:4
      3 頻度:gt:3 */

    n = sscanf(rule, "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]", category, cp1, cp2, cp3, cp4);

    if (n == 2) {
	if (str_eq(category, "文字種")) {
	    if (strcmp((char *)check_class((MRPH_DATA *)ptr2), cp1))
		return FALSE;
	    else
		return TRUE;
	}

	fprintf(stderr, "Invalid rule! (%s).\n", rule);
	return FALSE;
    }
    else if (n == 3) {
	if (str_eq(category, "細")) {
	    threshold = atoi(cp2);
	    if (str_eq(cp1, "その他"))
		return check_Bunrui_others((MRPH_DATA *)ptr2, threshold);
	    else
		return check_Bunrui((MRPH_DATA *)ptr2, cp1, threshold);
	}
	else if (str_eq(category, "頻度")) {
	    threshold = atoi(cp2);
	    return compare_threshold(((MRPH_DATA *)ptr2)->eNE.self.Count, 
				     threshold, cp1);
	}

	fprintf(stderr, "Invalid rule! (%s).\n", rule);
	return FALSE;
    }
    else if (n == 4) {
	threshold = atoi(cp3);

	if (str_eq(category, "単語"))
	    return compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.self, cp1), 
				     threshold, cp2);
	else if (str_eq(category, "文字種"))
	    return compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.selfSM, cp1), 
				     threshold, cp2);
	else if (str_eq(category, "格"))
	    return compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.Case, cp1), 
				     threshold, cp2);

	fprintf(stderr, "Invalid rule! (%s).\n", rule);
	return FALSE;
    }
    else if (n == 5) {
	threshold = atoi(cp4);

	if (str_eq(category, "XB"))
	    return (compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.XB, cp2), 
				     threshold, cp3) && 
		    str_eq(((MRPH_DATA *)ptr2)->eNE.XB.Type, cp1));
	else if (str_eq(category, "AX"))
	    return (compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.AX, cp2), 
				     threshold, cp3) && 
		    str_eq(((MRPH_DATA *)ptr2)->eNE.XB.Type, cp1));
	else if (str_eq(category, "AのX"))
	    return (compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.AnoX, cp2), 
				     threshold, cp3) && 
		    str_eq(((MRPH_DATA *)ptr2)->eNE.XB.Type, cp1));
	else if (str_eq(category, "XのB"))
	    return (compare_threshold(_check_function_NE(&((MRPH_DATA *)ptr2)->eNE.XnoB, cp2), 
				     threshold, cp3) && 
		    str_eq(((MRPH_DATA *)ptr2)->eNE.XB.Type, cp1));

	fprintf(stderr, "Invalid rule! (%s).\n", rule);
	return FALSE;
    }

    fprintf(stderr, "Invalid rule! (%s).\n", rule);
    return FALSE;
}

/*==================================================================*/
		    int check_char_type(int code)
/*==================================================================*/
{
    /* カタカナと "ー" */
    if ((0xa5a0 < code && code < 0xa6a0) || code == 0xa1bc) {
	return TYPE_KATAKANA;
    }
    /* ひらがな */
    else if (0xa4a0 < code && code < 0xa5a0) {
	return TYPE_HIRAGANA;
    }
    /* 漢字 */
    else if (0xb0a0 < code || code == 0xa1b9) {
	return TYPE_KANJI;
    }
    /* 数字と "・", "．" */
    else if ((0xa3af < code && code < 0xa3ba) || code == 0xa1a5 || code == 0xa1a6) {
	return TYPE_SUUJI;
    }
    /* アルファベット */
    else if (0xa3c0 < code && code < 0xa3fb) {
	return TYPE_EIGO;
    }
    /* 記号 */
    else {
	return TYPE_KIGOU;
    }
}

/*==================================================================*/
 int check_function(char *rule, FEATURE *fd, void *ptr1, void *ptr2)
/*==================================================================*/
{
    /* rule : ルール
       fd : データ側のFEATURE
       p1 : ルール側の構造体(MRPH_DATA,BNST_DATAなど)
       p2 : データ側の構造体(MRPH_DATA,BNST_DATAなど)
    */

    int i, code, type, pretype, flag;
    char *cp;
    unsigned char *ucp; 
    static BNST_DATA *pre1 = NULL, *pre2 = NULL;
    static char *prerule = NULL;

    /* &記英数カ : 記英数カ チェック (句読点以外) (形態素レベル) */

    if (!strcmp(rule, "&記英数カ")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (!(0xa1a5 < code && code < 0xa4a0) && /* 記号の範囲 */
		!(0xa5a0 < code && code < 0xb0a0))
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &漢字 : 漢字 チェック (形態素レベル) */

    else if (!strcmp(rule, "&漢字")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (code >= 0xb0a0 ||	/* 漢字の範囲 */
		code == 0xa1b9 || 	/* 々 */
		(code == 0xa4ab && ucp == ((MRPH_DATA *)ptr2)->Goi) ||	/* か */
		(code == 0xa5ab && ucp == ((MRPH_DATA *)ptr2)->Goi) ||	/* カ */
		(code == 0xa5f6 && ucp == ((MRPH_DATA *)ptr2)->Goi))	/* ヶ */
	      ;
	    else 
	      return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &かな漢字 : かな漢字チェック (形態素レベル) */

    else if (!strcmp(rule, "&かな漢字")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    code = check_char_type(code);
	    if (!(code == TYPE_KANJI || code == TYPE_HIRAGANA))
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &ひらがな : ひらがな チェック (形態素レベル) */

    else if (!strcmp(rule, "&ひらがな")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (check_char_type(code) != TYPE_HIRAGANA)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &末尾ひらがな : 末尾の一文字がひらがなか チェック (形態素レベル) */

    else if (!strcmp(rule, "&末尾ひらがな")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi2;	/* 表記をチェック */
	ucp += strlen(ucp)-2;
	code = (*ucp)*0x100+*(ucp+1);
	if (check_char_type(code) != TYPE_HIRAGANA)
	    return FALSE;
	return TRUE;
    }

    /* &カタカナ : カタカナ チェック (形態素レベル) */

    else if (!strcmp(rule, "&カタカナ")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (check_char_type(code) != TYPE_KATAKANA)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &数字 : 数字 チェック (形態素レベル) */

    else if (!strcmp(rule, "&数字")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (check_char_type(code) != TYPE_SUUJI)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &英記号 : 英記号 チェック (形態素レベル) */

    else if (!strcmp(rule, "&英記号")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    type = check_char_type(code);
	    if (type != TYPE_EIGO && type != TYPE_KIGOU)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &記号 : 記号 チェック (形態素レベル) */

    else if (!strcmp(rule, "&記号")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    type = check_char_type(code);
	    if (type != TYPE_KIGOU)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &混合 : 混合 (漢字+...) チェック (形態素レベル) */

    else if (!strcmp(rule, "&混合")) {
	ucp = ((MRPH_DATA *)ptr2)->Goi;
	pretype = 0;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    type = check_char_type(code);
	    if (pretype && pretype != type)
		return TRUE;
	    pretype = type;
	    ucp += 2;
	}
	return FALSE;
    }

    /* &一文字 : 文字数 チェック (形態素レベル) */

    else if (!strcmp(rule, "&一文字")) {
	if (strlen(((MRPH_DATA *)ptr2)->Goi) == 2)
	    return TRUE;
	else 
	    return FALSE;
    }

    /* &固有: 固有名詞 Feature チェック (形態素レベル) */

    else if (!strncmp(rule, "&固有:", strlen("&固有:")))
	return check_function_NE(rule + strlen("&固有:"), ptr1, ptr2);

    /* &固有C: 固有名詞 クラスチェック */

    else if (!strncmp(rule, "&固有C:", strlen("&固有C:"))) {
	if (check_feature_NE(((MRPH_DATA *)ptr2)->f, rule + strlen("&固有C:")))
	    return TRUE;
	else
	    return FALSE;
    }

    /* &意味素: 意味素チェック */

    else if (!strncmp(rule, "&意味素:", strlen("&意味素:"))) {
	if (((MRPH_DATA *)ptr2)->SM == NULL) {
	    return FALSE;
	}

	cp = rule + strlen("&意味素:");
	/* 漢字だったら意味属性名, それ以外ならコードそのまま */
	if (*cp & 0x80) {
	    if (SM2CODEExist == TRUE)
		cp = (char *)sm2code(cp);
	    else
		cp = NULL;
	    flag = SM_NO_EXPAND_NE;
	}
	else {
	    flag = SM_CHECK_FULL;
	}

	if (cp) {
	    for (i = 0; ((MRPH_DATA *)ptr2)->SM[i]; i+=SM_CODE_SIZE) {
		if (_sm_match_score(cp, 
				    &(((MRPH_DATA *)ptr2)->SM[i]), flag))
		    return TRUE;
	    }
	}
	return FALSE;
    }

    else if (!strncmp(rule, "&文節意味素:", strlen("&文節意味素:"))) {
	cp = rule + strlen("&文節意味素:");
	/* 漢字だったら意味属性名, それ以外ならコードそのまま */
	if (*cp & 0x80) {
	    if (SM2CODEExist == TRUE)
		cp = (char *)sm2code(cp);
	    else
		cp = NULL;
	    flag = SM_NO_EXPAND_NE;
	}
	else {
	    flag = SM_CHECK_FULL;
	}

	if (cp) {
	    for (i = 0; ((BNST_DATA *)ptr2)->SM_code[i]; i+=SM_CODE_SIZE) {
		if (_sm_match_score(cp, &(((BNST_DATA *)ptr2)->SM_code[i]), flag))
		    return TRUE;
	    }
	}
	return FALSE;
    }

    /* 形態素の長さ */
    
    else if (!strncmp(rule, "&形態素長:", strlen("&形態素長:"))) {
	cp = rule + strlen("&形態素長:");
	if (cp)
	    code = atoi(cp);
	else
	    code = 0;
	if (strlen(((MRPH_DATA *)ptr2)->Goi) >= code*2) {
	    return TRUE;
	}
	return FALSE;
    }

    else if (!strncmp(rule, "&形態素末尾:", strlen("&形態素末尾:"))) {
	cp = rule + strlen("&形態素末尾:");
	i = strlen(((MRPH_DATA *)ptr2)->Goi) - strlen(cp);
	if (*cp && i >= 0 && !strcmp((((MRPH_DATA *)ptr2)->Goi)+i, cp)) {
	    return TRUE;
	}
	return FALSE;
    }

    /* &固照応 固有名詞照応チェック */

    else if (!strncmp(rule, "&固照応:", strlen("&固照応:")))
	return check_correspond_NE((MRPH_DATA *)ptr2, rule + strlen("&固照応:"));

    /* &表層: 表層格チェック (文節レベル,係受レベル) */

    else if (!strncmp(rule, "&表層:", strlen("&表層:"))) {
	if (!strcmp(rule + strlen("&表層:"), "照合")) {
	    if ((cp = check_feature(((BNST_DATA *)ptr1)->f, "係")) == NULL) {
		return FALSE;
	    }
	    if (((BNST_DATA *)ptr2)->
		SCASE_code[case2num(cp + strlen("係:"))]) {
		return TRUE;
	    } else {
		return FALSE;
	    }
	}
	else if (((BNST_DATA *)ptr2)->
	    	SCASE_code[case2num(rule + strlen("&表層:"))]) {
	    return TRUE;
	} else {
	    return FALSE;
 	}
    }

    /* &D : 距離比較 (係受レベル) */

    else if (!strncmp(rule, "&D:", strlen("&D:"))) {
	if (((BNST_DATA *)ptr2 - (BNST_DATA *)ptr1)
	    <= atoi(rule + strlen("&D:"))) {
	    return TRUE;
	} else {
	    return FALSE;
	}
    }

    /* &レベル : 用言のレベル比較 (係受レベル) */

    else if (!strncmp(rule, "&レベル:", strlen("&レベル:"))) {
	if ((OptInhibit & OPT_INHIBIT_CLAUSE) || (whether_corpus_compare((BNST_DATA *)ptr1) == FALSE)) {
	    if (!strcmp(rule + strlen("&レベル:"), "強"))
		/* 述語間の強弱の比較 */
		return subordinate_level_comp((BNST_DATA *)ptr1, 
					      (BNST_DATA *)ptr2);
	    else
		/* 述語は係り受け可能か */
		return subordinate_level_check(rule + strlen("&レベル:"),
					       (BNST_DATA *)ptr2);
	}
	else {
	    /* &レベル が連続してチェックされるのは無駄だね */
	    if (prerule && 
		!strncmp(prerule, "&レベル:", strlen("&レベル:")) && 
		ptr1 == pre1 && 
		ptr2 == pre2) {
		return TRUE;
	    }

	    prerule = rule;
	    pre1 = ptr1;
	    pre2 = ptr2;

	    return corpus_clause_comp((BNST_DATA *)ptr1, 
				      (BNST_DATA *)ptr2, 
				      TRUE);
	}
    }

    /* &レベル禁止 : 用言のレベルチェック (係受レベル) --  tentative */

    else if (!strncmp(rule, "&レベル禁止:", strlen("&レベル禁止:"))) {
	/* 述語は係り受け可能か */
	return subordinate_level_forbid(rule + strlen("&レベル禁止:"),
					       (BNST_DATA *)ptr2);
    }

    /* &節境界 : 節間の壁チェック */

    else if (!strncmp(rule, "&節境界:", strlen("&節境界:"))) {
	if ((OptInhibit & OPT_INHIBIT_CLAUSE))
	    /* 
	       1. ルールに書いてあるレベルより強いことをチェック
	       2. 係り側より受け側のレベルが強いことをチェック
	    */
	    return (subordinate_level_check(rule + strlen("&節境界:"),
					    (BNST_DATA *)ptr2) && 
		    subordinate_level_comp((BNST_DATA *)ptr1, 
				       (BNST_DATA *)ptr2));
	else
	    return corpus_clause_barrier_check((BNST_DATA *)ptr1, 
					       (BNST_DATA *)ptr2);
    }


    /* &格述 : 格と述語の嗜好性チェック */

    else if (!strncmp(rule, "&格述:", strlen("&格述:"))) {
	if (OptInhibit & OPT_INHIBIT_CASE_PREDICATE)
	    return subordinate_level_check(rule + strlen("&格述:"),
					   (BNST_DATA *)ptr2);
	else
	    return corpus_case_predicate_check((BNST_DATA *)ptr1, 
					       (BNST_DATA *)ptr2);
    }

    /* &境界 : 格と述語の壁チェック */

    else if (!strncmp(rule, "&境界:", strlen("&境界:"))) {
	if (OptInhibit & OPT_INHIBIT_BARRIER)
	    return subordinate_level_check(rule + strlen("&境界:"),
					   (BNST_DATA *)ptr2);
	else
	    return corpus_barrier_check((BNST_DATA *)ptr1, 
					(BNST_DATA *)ptr2);
    }

    /* &境界連用 : 格と述語の壁チェック (連用) */

    else if (!strncmp(rule, "&境界連用:", strlen("&境界連用:"))) {
	if (OptInhibit & OPT_INHIBIT_BARRIER)
	    return subordinate_level_check(rule + strlen("&境界連用:"),
					   (BNST_DATA *)ptr2);
	else
	    /* レベルの処理 */
	    return subordinate_level_check(rule + strlen("&境界連用:"),
					   (BNST_DATA *)ptr2);
	    /* 統計的に処理しない */
	    /* return FALSE; */
	    /* 統計的に処理する */
	    /* return corpus_barrier_check((BNST_DATA *)ptr1, 
					(BNST_DATA *)ptr2); */
    }

    /* &境界特別 : 格と述語の壁チェック (事例, 臨時) */

    else if (!strncmp(rule, "&境界特別:", strlen("&境界特別:"))) {
	/* ルールによる壁 */
	if (OptInhibit & OPT_INHIBIT_BARRIER) {
	    /* 事例を使うとき */
	    if (!(OptInhibit & OPT_INHIBIT_OPTIONAL_CASE)) {
		return subordinate_level_check_special(rule + strlen("&境界特別:"),
						       (BNST_DATA *)ptr2);
	    }
	    else {
		return subordinate_level_check(rule + strlen("&境界特別:"),
					       (BNST_DATA *)ptr2);
	    }
	}
	else{
	    return corpus_barrier_check((BNST_DATA *)ptr1, 
					(BNST_DATA *)ptr2);
	}
    }

    /* &係側 : 係側のFEATUREチェック (係受レベル) */
    
    else if (!strncmp(rule, "&係側:", strlen("&係側:"))) {
	cp = rule + strlen("&係側:");
	if ((*cp != '^' && check_feature(((BNST_DATA *)ptr1)->f, cp)) ||
	    (*cp == '^' && !check_feature(((BNST_DATA *)ptr1)->f, cp))) {
	    return TRUE;
	} else {
	    return FALSE;
	}
    }

    /* &#? : FEATUREチェックと変数への格納 */
    
    else if (!strncmp(rule, "&#", strlen("&#"))) {
	int gnum;
	char fname[64];
	sscanf(rule, "&#%d:%s", &gnum, fname);
	if (cp = check_feature(fd, fname)) {
	    strcpy(G_Feature[gnum], cp);
	    return TRUE;
	} else {
	    return FALSE;
	}
    }

    /* &自立語一致 : 自立語が同じかどうか */
    
    else if (!strncmp(rule, "&自立語一致", strlen("&自立語一致"))) {
	if (!strcmp(((BNST_DATA *)ptr1)->Jiritu_Go, 
		    ((BNST_DATA *)ptr2)->Jiritu_Go)) {
	    return TRUE;
	} else {
	    return FALSE;
	}
    }


    /* &文字列照合 : 原形との文字列部分マッチ by kuro 00/12/28 */
    
    else if (!strncmp(rule, "&文字列照合:", strlen("&文字列照合:"))) {
      	cp = rule + strlen("&文字列照合:");
	if (strstr(((MRPH_DATA *)ptr2)->Goi, cp)) {
	    return TRUE;
	} else {
	    return FALSE;
	}
    }

    /* &ST : 並列構造解析での類似度の閾値 (ここでは無視) */
    
    else if (!strncmp(rule, "&ST", strlen("&ST"))) {
	return TRUE;
    }

    else if (!strncmp(rule, "&時間", strlen("&時間"))) {
	if (sm_all_match(((BNST_DATA *)ptr2)->SM_code, "1128********")) {
	    return TRUE;
	}
	else {
	    return FALSE;
	}
    }

    else {
#ifdef DEBUG
	fprintf(stderr, "Invalid Feature-Function (%s)\n", rule);
#endif
	return TRUE;
    }
}

/*==================================================================*/
 int feature_AND_match(FEATURE *fp, FEATURE *fd, void *p1, void *p2)
/*==================================================================*/
{
    int value;

    while (fp) {
	if (fp->cp[0] == '^' && fp->cp[1] == '&') {
	    value = check_function(fp->cp+1, fd, p1, p2);
	    if (value == TRUE) {
		return FALSE;
	    }
	} else if (fp->cp[0] == '&') {
	    value = check_function(fp->cp, fd, p1, p2);
	    if (value == FALSE) {
		return FALSE;
	    }
	} else if (fp->cp[0] == '^') {
	    if (check_feature(fd, fp->cp+1)) {
		return FALSE;
	    }
	} else {
	    if (!check_feature(fd, fp->cp)) {
		return FALSE;
	    }
	}
	fp = fp->next;
    }
    return TRUE;
}

/*==================================================================*/
int feature_pattern_match(FEATURE_PATTERN *fr, FEATURE *fd,
			  void *p1, void *p2)
/*==================================================================*/
{
    /* fr : ルール側のFEATURE_PATTERN,
       fd : データ側のFEATURE
       p1 : ルール側の構造体(MRPH_DATA,BNST_DATAなど)
       p2 : データ側の構造体(MRPH_DATA,BNST_DATAなど)
    */

    int i, value;

    /* PATTERNがなければマッチ */
    if (fr->fp[0] == NULL) return TRUE;

    /* ORの各条件を調べる */
    for (i = 0; fr->fp[i]; i++) {
	value = feature_AND_match(fr->fp[i], fd, p1, p2);
	if (value == TRUE) 
	    return TRUE;
    }
    return FALSE;
}

/*====================================================================
                               END
====================================================================*/

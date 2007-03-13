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

	(3) ルール構造体 <==照合==> 形態素または文節構造体
	       	<○>は<○:…>というFEATUREがあればOK
	    	<^○>は<○:…>というFEATUREがなければOK
	    	<&○>は関数呼出
			&記英数カ -- 表記が記号,英文字,数字,カタカナ (形態素)
			&漢字 -- 表記が漢字 (形態素)
	    		&表層:○格 -- ○格がある (文節)
	    		&表層:照合 -- 係の表層格が受にある (係受)
			&D:n -- 構造体間が距離n以内 (係受)
			&レベル:強 -- 受が係以上 (係受)
			&レベル:l -- 自身がl以上 (係受)
			&係側:○ -- 係に○ (係受)

	※ プログラム内で形態素または文節構造体にFEATUREを与える
	場合は(2)のなかの assign_cfeature を用いる．

	※ プログラム内で形態素または文節構造体があるFEATUREを持つ
	かどうかを調べる場合は(3)のなかの check_feature を用いる．
*/

/*==================================================================*/
	    void print_one_feature(char *cp, FILE *filep)
/*==================================================================*/
{
    if (!strncmp(cp, "仮付与:", strlen("仮付与:"))) { /* 仮付与したものを表示するとき用(-nbest) */
	if (OptExpress == OPT_TABLE)
	    fprintf(filep, "＜%s＞", cp + strlen("仮付与:")); 
	else
	    fprintf(filep, "<%s>", cp + strlen("仮付与:")); 
    }
    else {
	if (OptExpress == OPT_TABLE)
	    fprintf(filep, "＜%s＞", cp);
	else
	    fprintf(filep, "<%s>", cp);
    }
}

/*==================================================================*/
	      void print_feature(FEATURE *fp, FILE *filep)
/*==================================================================*/
{
    /* <f1><f2> ... <f3> という形式の出力 
       (ただしＴではじまるfeatureは表示しない) */

    while (fp) {
	if (fp->cp && 
	    (strncmp(fp->cp, "Ｔ", strlen("Ｔ")) ||
	     OptDisplay == OPT_DEBUG))
	    print_one_feature(fp->cp, filep);
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
	if (fp->cp && strncmp(fp->cp, "Ｃ", strlen("Ｃ")) && !strncmp(fp->cp, "C", 1))
	    print_one_feature(fp->cp, filep);
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
	    if (fp->cp && strncmp(fp->cp, "Ｔ", strlen("Ｔ"))) {
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

/*==================================================================*/
	   void delete_cfeature(FEATURE **fpp, char *type)
/*==================================================================*/
{
    FEATURE *prep = NULL;

    while (*fpp) {
	if (comp_feature((*fpp)->cp, type) == TRUE) {
	    FEATURE *next;
	    free((*fpp)->cp);
	    if (prep == NULL) {
		next = (*fpp)->next;
		free(*fpp);
		*fpp = next;
	    }
	    else {
		next = (*fpp)->next;
		free(*fpp);
		prep->next = next;
	    }
	    return;
	}
	prep = *fpp;
	fpp = &(prep->next);
    }
}

/*==================================================================*/
	       void delete_temp_feature(FEATURE **fpp)
/*==================================================================*/
{
    /* 仮付与したfeatureを削除 */

    FEATURE *prep = NULL;

    while (*fpp) {
	if (comp_feature((*fpp)->cp, "仮付与") == TRUE) {
	    FEATURE *next;
	    free((*fpp)->cp);
	    if (prep == NULL) {
		next = (*fpp)->next;
		free(*fpp);
		*fpp = next;
	    }
	    else {
		next = (*fpp)->next;
		free(*fpp);
		prep->next = next;
	    }
	    fpp = &(prep->next);
	    continue;
	}
	prep = *fpp;
	fpp = &(prep->next);
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

    if (cp == NULL || cp[0] == '\0') {
	f->fp[nth] = NULL;
	return;
    }

    strcpy(buffer, cp);
    scp = ecp = buffer;
    while (*ecp) {
	if (*ecp == '|') {
	    *ecp = '\0';
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
	   void append_feature(FEATURE **fpp, FEATURE *afp)
/*==================================================================*/
{
    while (*fpp) {
	fpp = &((*fpp)->next);
    }
    *fpp = afp;
}    

/*==================================================================*/
void assign_cfeature(FEATURE **fpp, char *fname, int temp_assign_flag)
/*==================================================================*/
{
    /* temp_assign_flag: TRUEのとき「仮付与」を頭につける */

    char type[256];

    /* 上書きの可能性をチェック */

    sscanf(fname, "%[^:]", type);	/* ※ fnameに":"がない場合は
					   typeはfname全体になる */

    /* quote('"')中の":"で切っていれば、もとに戻す */
    if (strcmp(type, fname)) {
	int i, count = 0;

	for (i = 0; i < strlen(type); i++) {
	    if (type[i] == '"') {
		count++;
	    }
	}
	if (count % 2 == 1) { /* '"'が奇数 */
	    strcpy(type, fname);
	}
    }

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
	!((*fpp)->cp = (char *)(malloc(strlen(fname) + 8)))) {
	fprintf(stderr, "Can't allocate memory for FEATURE\n");
	exit(-1);
    }
    if (temp_assign_flag) {
	strcpy((*fpp)->cp, "仮付与:");
	strcat((*fpp)->cp, fname);
    }
    else {
	strcpy((*fpp)->cp, fname);
    }
    (*fpp)->next = NULL;
}    

/*==================================================================*/
void assign_feature(FEATURE **fpp1, FEATURE **fpp2, void *ptr, int offset, int length, int temp_assign_flag)
/*==================================================================*/
{
    /*
     *  ルールを適用の結果，ルールから構造体にFEATUREを付与する
     *  構造体自身に対する処理も可能としておく
     */

    int i;
    char *cp, *pat;
    char buffer[256];
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
		set_pred_voice((BNST_DATA *)ptr + offset);	/* ヴォイス */
		get_scase_code((BNST_DATA *)ptr + offset);	/* 表層格 */
	    }
	    else if (!strcmp((*fpp2)->cp, "&表層:削除")) {
		for (i = 0, cp = ((BNST_DATA *)ptr + offset)->SCASE_code; 
		     i < SCASE_CODE_SIZE; i++, cp++) 
		    *cp = 0;		
	    }
	    else if (!strncmp((*fpp2)->cp, "&表層:^", strlen("&表層:^"))) {
		((BNST_DATA *)ptr + offset)->
		    SCASE_code[case2num((*fpp2)->cp + strlen("&表層:^"))] = 0;
	    }
	    else if (!strncmp((*fpp2)->cp, "&表層:", strlen("&表層:"))) {
		((BNST_DATA *)ptr + offset)->
		    SCASE_code[case2num((*fpp2)->cp + strlen("&表層:"))] = 1;
	    }
	    else if (!strncmp((*fpp2)->cp, "&MEMO:", strlen("&MEMO:"))) {
		strcat(PM_Memo, " ");
		strcat(PM_Memo, (*fpp2)->cp + strlen("&MEMO:"));
	    }
	    else if (!strncmp((*fpp2)->cp, "&品詞変更:", strlen("&品詞変更:"))) {
		change_mrph((MRPH_DATA *)ptr + offset, *fpp2);
	    }
	    else if (!strncmp((*fpp2)->cp, "&意味素付与:", strlen("&意味素付与:"))) {
		assign_sm((BNST_DATA *)ptr + offset, (*fpp2)->cp + strlen("&意味素付与:"));
	    }
	    else if (!strncmp((*fpp2)->cp, "&複合辞格解析", strlen("&複合辞格解析"))) {
		cp = make_fukugoji_string((TAG_DATA *)ptr + offset + 1);
		if (cp) {
		    assign_cfeature(&(((TAG_DATA *)ptr + offset)->f), cp, temp_assign_flag);
		}
	    }
	    else if (!strncmp((*fpp2)->cp, "&記憶語彙付与:", strlen("&記憶語彙付与:"))) {
		sprintf(buffer, "%s:%s", 
			(*fpp2)->cp + strlen("&記憶語彙付与:"), 
			((MRPH_DATA *)matched_ptr)->Goi);
		assign_cfeature(&(((BNST_DATA *)ptr + offset)->f), buffer, temp_assign_flag);
	    }
	    /* &伝搬:n:FEATURE : FEATUREの伝搬  */
	    else if (!strncmp((*fpp2)->cp, "&伝搬:", strlen("&伝搬:"))) {
		pat = (*fpp2)->cp + strlen("&伝搬:");
		sscanf(pat, "%d", &i);
		pat = strchr(pat, ':');
		pat++;
		if ((cp = check_feature(((TAG_DATA *)ptr + offset)->f, pat))) {
		    assign_cfeature(&(((TAG_DATA *)ptr + offset + i)->f), cp, temp_assign_flag);
		}
		else { /* ないなら、もとからあるものを削除 */
		    delete_cfeature(&(((TAG_DATA *)ptr + offset + i)->f), pat);
		}
		if (((TAG_DATA *)ptr + offset)->bnum >= 0) { /* 文節区切りでもあるとき */
		    if ((cp = check_feature((((TAG_DATA *)ptr + offset)->b_ptr)->f, pat))) {
			assign_cfeature(&((((TAG_DATA *)ptr + offset)->b_ptr + i)->f), cp, temp_assign_flag);
		    }
		    else {
			delete_cfeature(&((((TAG_DATA *)ptr + offset)->b_ptr + i)->f), pat);
		    }
		}
	    }
	    /* 形態素付属化 : 属する形態素列をすべて<付属>にする */
	    else if (!strncmp((*fpp2)->cp, "&形態素付属化", strlen("&形態素付属化"))) {
		for (i = 0; i < ((TAG_DATA *)ptr + offset)->mrph_num; i++) {
		    delete_cfeature(&((((TAG_DATA *)ptr + offset)->mrph_ptr + i)->f), "自立");
		    delete_cfeature(&((((TAG_DATA *)ptr + offset)->mrph_ptr + i)->f), "意味有");
		    assign_cfeature(&((((TAG_DATA *)ptr + offset)->mrph_ptr + i)->f), "付属", temp_assign_flag);
		}
	    }
	    /* 自動辞書 : 自動獲得した辞書をチェック (マッチ部分全体) */
	    else if (!strncmp((*fpp2)->cp, "&自動辞書:", strlen("&自動辞書:"))) {
		if (offset == 0 && check_auto_dic((MRPH_DATA *)ptr, length, (*fpp2)->cp + strlen("&自動辞書:"))) {
		    for (i = 0; i < length; i ++) {
			assign_cfeature(&(((MRPH_DATA *)ptr + i)->f), (*fpp2)->cp + strlen("&自動辞書:"), temp_assign_flag);
		    }
		}
	    }
	} else {			/* 追加の場合 */
	    assign_cfeature(fpp1, (*fpp2)->cp, temp_assign_flag);	
	}

	fpp2 = &((*fpp2)->next);
    }
}

/*==================================================================*/
	void copy_feature(FEATURE **dst_fpp, FEATURE *src_fp)
/*==================================================================*/
{
    while (src_fp) {
	assign_cfeature(dst_fpp, src_fp->cp, FALSE);
	src_fp = src_fp->next;
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
		int check_str_type(unsigned char *ucp)
/*==================================================================*/
{
    int code = 0, precode = 0;

    while (*ucp) {
	code = (*ucp << 8) + *(ucp + 1);
	code = check_char_type(code);
	if (precode && precode != code) {
	    return 0;
	}
	precode = code;
	ucp += 2;
    }

    return code;
}

/*==================================================================*/
 int check_function(char *rule, FEATURE *fd, void *ptr1, void *ptr2)
/*==================================================================*/
{
    /* rule : ルール
       fd : データ側のFEATURE
       p1 : 係り受けの場合，係り側の構造体(MRPH_DATA,BNST_DATAなど)
       p2 : データの構造体(MRPH_DATA,BNST_DATAなど)
    */

    int i, code, type, pretype, flag;
    char *cp;
    unsigned char *ucp; 

    /* &記英数カ : 記英数カ チェック (句読点以外) (形態素レベル) */

    if (!strcmp(rule, "&記英数カ")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
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

    else if (!strcmp(rule, "&漢字")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (code >= 0xb0a0 ||	/* 漢字の範囲 */
		code == 0xa1b9 || 	/* 々 */
		(code == 0xa4ab && ucp == (unsigned char *)((MRPH_DATA *)ptr2)->Goi2) ||	/* か */
		(code == 0xa5ab && ucp == (unsigned char *)((MRPH_DATA *)ptr2)->Goi2) ||	/* カ */
		(code == 0xa5f6 && ucp == (unsigned char *)((MRPH_DATA *)ptr2)->Goi2))		/* ヶ */
	      ;
	    else 
	      return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &かな漢字 : かな漢字チェック (形態素レベル) */

    else if (!strcmp(rule, "&かな漢字")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
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

    else if (!strcmp(rule, "&ひらがな")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (check_char_type(code) != TYPE_HIRAGANA)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &末尾ひらがな : 末尾の一文字がひらがなか チェック (形態素レベル) */

    else if (!strcmp(rule, "&末尾ひらがな")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;	/* 表記をチェック */
	ucp += strlen(ucp) - BYTES4CHAR;
	code = (*ucp)*0x100+*(ucp+1);
	if (check_char_type(code) != TYPE_HIRAGANA)
	    return FALSE;
	return TRUE;
    }

    /* &末尾文字列 : 末尾の文字列を チェック (形態素レベル) */

    else if (!strncmp(rule, "&末尾文字列:", strlen("&末尾文字列:"))) {
	cp = rule + strlen("&末尾文字列:");

	/* パターンの方が大きければFALSE */
	if (strlen(cp) > strlen(((MRPH_DATA *)ptr2)->Goi2))
	    return FALSE;

	ucp = ((MRPH_DATA *)ptr2)->Goi2;	/* 表記をチェック */
	ucp += strlen(ucp)-strlen(cp);
	if (strcmp(ucp, cp))
	    return FALSE;
	return TRUE;
    }

    /* &カタカナ : カタカナ チェック (形態素レベル) */

    else if (!strcmp(rule, "&カタカナ")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (check_char_type(code) != TYPE_KATAKANA)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &数字 : 数字 チェック (形態素レベル) */

    else if (!strcmp(rule, "&数字")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
	while (*ucp) {
	    code = (*ucp)*0x100+*(ucp+1);
	    if (check_char_type(code) != TYPE_SUUJI)
		return FALSE;
	    ucp += 2;
	}	    
	return TRUE;
    }

    /* &英記号 : 英記号 チェック (形態素レベル) */

    else if (!strcmp(rule, "&英記号")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
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

    else if (!strcmp(rule, "&記号")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
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

    else if (!strcmp(rule, "&混合")) { /* euc-jp */
	ucp = ((MRPH_DATA *)ptr2)->Goi2;
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
	if (strlen(((MRPH_DATA *)ptr2)->Goi2) == BYTES4CHAR)
	    return TRUE;
	else 
	    return FALSE;
    }

    /* &意味素: 意味素チェック (形態素) */

    else if (!strncmp(rule, "&意味素:", strlen("&意味素:"))) {
	if (Thesaurus != USE_NTT || ((MRPH_DATA *)ptr2)->SM == NULL) {
	    return FALSE;
	}

	cp = rule + strlen("&意味素:");
	/* 漢字だったら意味属性名, それ以外ならコードそのまま */
	if (*cp & 0x80) { /* euc-jp */
	    if (SM2CODEExist == TRUE)
		cp = sm2code(cp);
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

    /* &文節意味素: 意味素チェック (文節) */

    else if (!strncmp(rule, "&文節意味素:", strlen("&文節意味素:"))) {
	if (Thesaurus != USE_NTT) {
	    return FALSE;
	}

	cp = rule + strlen("&文節意味素:");
	/* 漢字だったら意味属性名, それ以外ならコードそのまま */
	if (*cp & 0x80) { /* euc-jp */
	    if (SM2CODEExist == TRUE)
		cp = sm2code(cp);
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

    /* &文節全意味素: 文節のすべての意味素が指定意味素以下にあるかどうか */

    else if (!strncmp(rule, "&文節全意味素:", strlen("&文節全意味素:"))) {
	if (Thesaurus != USE_NTT) {
	    return FALSE;
	}

	cp = rule + strlen("&文節全意味素:");
	/* 漢字だったら意味属性名, それ以外ならコードそのまま */
	if (*cp & 0x80) { /* euc-jp */
	    if (SM2CODEExist == TRUE)
		cp = sm2code(cp);
	    else
		cp = NULL;
	}

	if (cp && ((BNST_DATA *)ptr2)->SM_code[0] && 
	    sm_all_match(((BNST_DATA *)ptr2)->SM_code, cp)) {
		return TRUE;
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
	if (strlen(((MRPH_DATA *)ptr2)->Goi2) >= code * BYTES4CHAR) {
	    return TRUE;
	}
	return FALSE;
    }

    else if (!strncmp(rule, "&形態素末尾:", strlen("&形態素末尾:"))) {
	cp = rule + strlen("&形態素末尾:");
	i = strlen(((MRPH_DATA *)ptr2)->Goi2) - strlen(cp);
	if (*cp && i >= 0 && !strcmp((((MRPH_DATA *)ptr2)->Goi2)+i, cp)) {
	    return TRUE;
	}
	return FALSE;
    }

    /* &表層: 表層格チェック (文節レベル,係受レベル) */

    else if (!strncmp(rule, "&表層:", strlen("&表層:"))) {
	if (!strcmp(rule + strlen("&表層:"), "照合")) {
	    if ((cp = check_feature(((BNST_DATA *)ptr1)->f, "係")) == NULL) {
		return FALSE;
	    }
	    if (((BNST_DATA *)ptr2)->
		SCASE_code[case2num(cp + strlen("係:"))]) {
		return TRUE;
	    }
	    else {
		return FALSE;
	    }
	}
	else if (((BNST_DATA *)ptr2)->
		 SCASE_code[case2num(rule + strlen("&表層:"))]) {
	    return TRUE;
	}
	else {
	    return FALSE;
 	}
    }

    /* &D : 距離比較 (係受レベル) */

    else if (!strncmp(rule, "&D:", strlen("&D:"))) {
	if (((BNST_DATA *)ptr2 - (BNST_DATA *)ptr1)
	    <= atoi(rule + strlen("&D:"))) {
	    return TRUE;
	}
	else {
	    return FALSE;
	}
    }

    /* &レベル:強 : 用言のレベル比較 (係受レベル) */

    else if (!strcmp(rule, "&レベル:強")) {
	return subordinate_level_comp((BNST_DATA *)ptr1, 
				      (BNST_DATA *)ptr2);
    }

    /* &レベル:X : 用言がレベルX以上であるかどうか */

    else if (!strncmp(rule, "&レベル:", strlen("&レベル:"))) {
	return subordinate_level_check(rule + strlen("&レベル:"), fd);
	/* (BNST_DATA *)ptr2); */
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

    /* &係側チェック : 係側のFEATUREチェック (文節ルール) */
    
    else if (!strncmp(rule, "&係側チェック:", strlen("&係側チェック:"))) {
	cp = rule + strlen("&係側チェック:");
	for (i = 0; ((BNST_DATA *)ptr2)->child[i]; i++) {
	    if (check_feature(((BNST_DATA *)ptr2)->child[i]->f, cp)) {
		return TRUE;
	    }
	}
	return FALSE;
    }

    /* &受側チェック : 受側のFEATUREチェック (文節ルール) */
    
    else if (!strncmp(rule, "&受側チェック:", strlen("&受側チェック:"))) {
	cp = rule + strlen("&受側チェック:");
	if (((BNST_DATA *)ptr2)->parent &&
	    check_feature(((BNST_DATA *)ptr2)->parent->f, cp)) {
	    return TRUE;
	} else {
	    return FALSE;
	}
    }

    /* &自立語一致 : 自立語が同じかどうか */
    
    else if (!strncmp(rule, "&自立語一致", strlen("&自立語一致"))) {
	/* if (!strcmp(((BNST_DATA *)ptr1)->head_ptr->Goi, 
	   ((BNST_DATA *)ptr2)->head_ptr->Goi)) { */
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

    /* &OPTCHECK : オプションのチェック */
    
    else if (!strncmp(rule, "&OptCheck:", strlen("&OptCheck:"))) {
	char **opt;

	cp = rule + strlen("&OptCheck:");
	if (*cp == '-') { /* '-'を含んでいたら飛ばす */
	    cp++;
	}

	for (opt = Options; *opt != NULL; opt++) {
	    if (!strcasecmp(cp, *opt)) {
		return TRUE;	    
	    }
	}
	return FALSE;
    }

    /*
    else if (!strncmp(rule, "&時間", strlen("&時間"))) {
	if (sm_all_match(((BNST_DATA *)ptr2)->SM_code, "1128********")) {
	    return TRUE;
	}
	else {
	    return FALSE;
	}
    } */

    /* &態 : 態をチェック */

    else if (!strncmp(rule, "&態:", strlen("&態:"))) {
	cp = rule + strlen("&態:");
	if ((!strcmp(cp, "能動") && ((BNST_DATA *)ptr2)->voice == 0) || 
	    (!strcmp(cp, "受動") && (((BNST_DATA *)ptr2)->voice & VOICE_UKEMI || 
				     ((BNST_DATA *)ptr2)->voice & VOICE_SHIEKI_UKEMI)) || 
	    (!strcmp(cp, "使役") && (((BNST_DATA *)ptr2)->voice & VOICE_SHIEKI || 
				     ((BNST_DATA *)ptr2)->voice & VOICE_SHIEKI_UKEMI))) {
	    return TRUE;
	}
	else {
	    return FALSE;
	}
    }

    /* &記憶 : 形態素または文節のポインタを記憶 */

    else if (!strcmp(rule, "&記憶")) {
	matched_ptr = ptr2;
	return TRUE;
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
       p1 : 係り受けの場合，係り側の構造体(MRPH_DATA,BNST_DATAなど)
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

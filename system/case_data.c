/*====================================================================

			  格構造解析: 入力側

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int SOTO_SCORE = 7;

char fukugoji_string[SMALL_DATA_LEN];

char *FukugojiTable[] = {"を除く", "をのぞく", 
			 "を通じる", "をつうじる", 
			 "を通ずる", "をつうじる", 
			 "を通す", "をつうじる", 
			 "を含める", "をふくめる", 
			 "を始める", "をはじめる", 
			 "に絡む", "にからむ", 
			 "に沿う", "にそう", 
			 "に向ける", "にむける", 
			 "に伴う", "にともなう", 
			 "に基づく", "にもとづく", 
			 "に対する", "にたいする", 
			 "に関する", "にかんする", 
			 "に代わる", "にかわる", 
			 "に加える", "にくわえる", 
			 "に限る", "にかぎる", 
			 "に続く", "につづく", 
			 "に合わせる", "にあわせる", 
			 "に比べる", "にくらべる", 
			 "に並ぶ", "にならぶ", 
			 "に限るぬ", "にかぎるぬ", 
			 ""};

/*==================================================================*/
	     char *make_fukugoji_string(TAG_DATA *b_ptr)
/*==================================================================*/
{
    int i, fc;
    char buf[SMALL_DATA_LEN];
    TAG_DATA *pre_b_ptr = b_ptr - 1;

    /* 付属語がないとき */
    if (b_ptr->num < 1 || pre_b_ptr->fuzoku_num == 0) {
	return NULL;
    }

    buf[0] = '\0';

    /* 前の文節の助詞 */
    strcat(buf, (pre_b_ptr->fuzoku_ptr + pre_b_ptr->fuzoku_num - 1)->Goi);

    /* この文節 */
    for (i = 0; i < b_ptr->jiritu_num; i++) {
	if (!strcmp(Class[(b_ptr->jiritu_ptr + i)->Hinshi][0].id, "特殊")) /* 特殊以外 */
	    continue;
	strcat(buf, 
	       (b_ptr->jiritu_ptr + i)->Goi);
    }

    /* 原形の読みに統一 */
    for (i = 0; *(FukugojiTable[i]); i += 2) {
	if (str_eq(buf, FukugojiTable[i])) {
	    strcpy(buf, FukugojiTable[i + 1]);
	    break;
	}
    }

    fc = pp_hstr_to_code(buf);
    if (fc != END_M) {
	sprintf(fukugoji_string, "Ｔ解析格-%s", pp_code_to_kstr(fc));
	return fukugoji_string;
    }
    return NULL;
}

/*==================================================================*/
int check_cc_relation(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr, char *pp_str)
/*==================================================================*/
{
    int i;

    if (!cpm_ptr || !cpm_ptr->cmm[0].cf_ptr) {
	return 0;
    }

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_ptr[i] && 
	    cpm_ptr->elem_b_ptr[i]->num == b_ptr->num && 
	    MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[cpm_ptr->cmm[0].result_lists_d[0].flag[i]][0], 
		    pp_str)) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
int _make_data_from_feature_to_pp(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr, 
				  int *pp_num, char *fcp)
/*==================================================================*/
{
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);
    int cc;

    /* 用言の項となるもの */
    if (cpm_ptr->cf.type == CF_PRED) {
	if (!strncmp(fcp, "Ｔ解析格-", strlen("Ｔ解析格-"))) {
	    cc = pp_kstr_to_code(fcp+strlen("Ｔ解析格-"));
	    if (cc == END_M) {
		fprintf(stderr, ";; case <%s> in a rule is unknown!\n", fcp+9);
		exit(1);
	    }
	    c_ptr->pp[c_ptr->element_num][(*pp_num)++] = cc;
	    if (*pp_num >= PP_ELEMENT_MAX) {
		fprintf(stderr, ";; not enough pp_num (%d)!\n", PP_ELEMENT_MAX);
		exit(1);
	    }
	}
	else if (!strcmp(fcp, "Ｔ必須格")) {
	    c_ptr->oblig[c_ptr->element_num] = TRUE;
	}
	else if (!strcmp(fcp, "Ｔ用言同文節")) {	/* 「〜を〜に」のとき */
	    if (cpm_ptr->pred_b_ptr->num != b_ptr->num) {
		return FALSE;
	    }
	}
    }
    /* 名詞の項となるもの */
    else {
	if (!strcmp(fcp, "Ｔ名詞項")) {
	    /* 条件: 同格ではない 
	             連体修飾節の場合はその関係が外の関係 */
	    if (b_ptr->dpnd_type != 'A' &&
		(!check_feature(b_ptr->f, "係:連格") || 
		 check_cc_relation(b_ptr->cpm_ptr, cpm_ptr->pred_b_ptr, "外の関係"))) {
		c_ptr->pp[c_ptr->element_num][(*pp_num)++] = 0;
		c_ptr->pp_str[c_ptr->element_num] = NULL;
	    }
	}
    }

    return TRUE;
}

/*==================================================================*/
TAG_DATA *_make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr, int flag)
/*==================================================================*/
{
    int pp_num = 0, cc, not_flag;
    char *buffer, *start_cp, *loop_cp;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);
    FEATURE *fp;

    /* flag == TRUE:  格要素
       flag == FALSE: 被連体修飾詞 */

    /* 格要素 */
    if (flag == TRUE) {
	if (b_ptr->num > 0 && /* 複合辞などはひとつ前の基本句をみる */
	    check_feature(b_ptr->f, "格要素表記直前参照")) {
	    b_ptr--;
	}

	/* 「〜のNだ。」禁止 (★=>ルールへ) */
	if (cpm_ptr->cf.type == CF_PRED && 
	    check_feature(b_ptr->f, "係:ノ格") && 
	    check_feature(cpm_ptr->pred_b_ptr->f, "用言:判")) {
	    return NULL;
	}

	c_ptr->oblig[c_ptr->element_num] = FALSE;

	/* 係り先をみる場合 */
	if (start_cp = check_feature(b_ptr->f, "係チ")) {
	    buffer = strdup(start_cp+strlen("係チ:"));
	    start_cp = buffer;
	    loop_cp = start_cp;
	    flag = 1; /* 2: OK, 1: 未定, 0: NG */
	    not_flag = 0;
	    while (*loop_cp) {
		if (flag == 1 && *loop_cp == '&' && *(loop_cp+1) == '&') {
		    *loop_cp = '\0';
		    if ((!not_flag && !check_feature(cpm_ptr->pred_b_ptr->f, start_cp)) || 
			(not_flag && check_feature(cpm_ptr->pred_b_ptr->f, start_cp))) {
			flag = 0; /* NG */
		    }
		    loop_cp += 2;
		    start_cp = loop_cp;
		    not_flag = 0;
		}
		else if (flag < 2 && *loop_cp == '|' && *(loop_cp+1) == '|') {
		    if (flag == 1) {
			*loop_cp = '\0';
			if ((!not_flag && check_feature(cpm_ptr->pred_b_ptr->f, start_cp)) || 
			    (not_flag && !check_feature(cpm_ptr->pred_b_ptr->f, start_cp))) {
			    flag = 2; /* OK */
			}
		    }
		    else {
			flag = 1; /* 0 -> 1 */
		    }
		    loop_cp += 2;
		    start_cp = loop_cp;
		    not_flag = 0;
		}
		else if (*loop_cp == ':') {
		    *loop_cp = '\0';
		    if (flag == 2 || (flag == 1 && 
			(!not_flag && check_feature(cpm_ptr->pred_b_ptr->f, start_cp)) || 
			(not_flag && !check_feature(cpm_ptr->pred_b_ptr->f, start_cp)))) {
			if (_make_data_from_feature_to_pp(cpm_ptr, b_ptr, &pp_num, loop_cp+1) == FALSE) {
			    free(buffer);
			    return NULL;
			}
		    }
		    break;
		}
		else {
		    if (*loop_cp == '^') {
			not_flag = 1;
		    }
		    loop_cp++;
		}
	    }
	    free(buffer);
	}

	if (check_feature(b_ptr->f, "Ｔ格直後参照")) { /* 「〜の(方)」などの格は「方」の方の格をみる */
	    fp = (b_ptr + 1)->f;
	}
	else {
	    fp = b_ptr->f;
	}

	/* featureから格へ */
	while (fp) {
	    if (_make_data_from_feature_to_pp(cpm_ptr, b_ptr, &pp_num, fp->cp) == FALSE) {
		return NULL;
	    }
	    fp = fp->next;
	}

	if (pp_num) {
	    c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	    return b_ptr;
	}
	else {
	    return NULL;
	}
    }
    /* 被連体修飾詞 (とりあえず用言のときのみ) */
    else if (cpm_ptr->cf.type == CF_PRED) {
	fp = b_ptr->f;
	c_ptr->oblig[c_ptr->element_num] = FALSE;

	while (fp) {
	    if (!strncmp(fp->cp, "Ｔ解析連格-", strlen("Ｔ解析連格-"))) {
		cc = pp_kstr_to_code(fp->cp+strlen("Ｔ解析連格-"));
		if (cc == END_M) {
		    fprintf(stderr, ";; case <%s> in a rule is unknown!\n", fp->cp+11);
		    exit(1);
		}
		c_ptr->pp[c_ptr->element_num][pp_num++] = cc;
		if (pp_num >= PP_ELEMENT_MAX) {
		    fprintf(stderr, ";; not enough pp_num (%d)!\n", PP_ELEMENT_MAX);
		    exit(1);
		}
	    }
	    fp = fp->next;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }
    return NULL;
}

/*==================================================================*/
   void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr)
/*==================================================================*/
{
    int sm_num = 0, size;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (Thesaurus == USE_NTT) {
	size = SM_CODE_SIZE;
    }
    else if (Thesaurus == USE_BGH) {
	size = BGH_CODE_SIZE;
    }

    /* 格要素 -- 文 */
    if (check_feature(b_ptr->f, "補文")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
	       sm2code("補文"));
	sm_num++;
    }
    /* 修飾 *
    else if (check_feature(b_ptr->f, "修飾")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
	       sm2code("修飾"));
	sm_num++;
	} */
    else {
	if (check_feature(b_ptr->f, "時間")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
		   sm2code("時間"));
	    sm_num++;
	}
	if (check_feature(b_ptr->f, "数量")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
		   sm2code("数量"));
	    sm_num++;
	}
	/* 固有名詞 => 主体 */
	if (check_feature(b_ptr->f, "人名") || 
	    check_feature(b_ptr->f, "組織名")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
		   sm2code("主体"));
	    sm_num++;
	}
	
	/* 主体 */
	if (Thesaurus == USE_NTT) {
	    /* いろいろ使えるので意味素すべてコピー */
	    strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
		   b_ptr->SM_code);
	    sm_num += strlen(b_ptr->SM_code)/size;	    
	}
	else if (Thesaurus == USE_BGH) {
	    if (bgh_match_check(sm2code("主体"), b_ptr->BGH_code)) {
		strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
		       sm2code("主体"));
		sm_num++;
	    }
	}
    }

    *(c_ptr->sm[c_ptr->element_num]+size*sm_num) = '\0';
}

/*==================================================================*/
   void _make_data_cframe_ex(CF_PRED_MGR *cpm_ptr, TAG_DATA *b_ptr)
/*==================================================================*/
{
    int i = 1;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);
    char *cp;

    i = 2;

    if (Thesaurus == USE_BGH) {
	strcpy(c_ptr->ex[c_ptr->element_num], b_ptr->BGH_code);
    }
    else if (Thesaurus == USE_NTT) {
	strcpy(c_ptr->ex[c_ptr->element_num], b_ptr->SM_code);
    }

    if ((OptCaseFlag & OPT_CASE_USE_REP_CF) && 
	(cp = get_mrph_rep_from_f(b_ptr->head_ptr, FALSE))) {
	strcpy(c_ptr->ex_list[c_ptr->element_num][0], cp);
    }
    else {
	strcpy(c_ptr->ex_list[c_ptr->element_num][0], b_ptr->head_ptr->Goi);
    }
    c_ptr->ex_num[c_ptr->element_num] = 1;
    c_ptr->ex_freq[c_ptr->element_num][0] = 1;
}

/*==================================================================*/
	      void set_data_cf_type(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    TAG_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    char *vtype = NULL;

    cpm_ptr->cf.type = CF_PRED;
    cpm_ptr->cf.type_flag = 0;
    cpm_ptr->cf.voice = b_ptr->voice;

    if ((vtype = check_feature(b_ptr->f, "用言"))) {
	vtype += strlen("用言:");
	strcpy(cpm_ptr->cf.pred_type, vtype);
    }
    else if ((vtype = check_feature(b_ptr->f, "非用言格解析"))) {
	vtype += strlen("非用言格解析:");
	strcpy(cpm_ptr->cf.pred_type, vtype);
    }
    else if (check_feature(b_ptr->f, "準用言")) {
	strcpy(cpm_ptr->cf.pred_type, "準");
    }
    else if (check_feature(b_ptr->f, "体言")) {
	strcpy(cpm_ptr->cf.pred_type, "名");
	cpm_ptr->cf.type = CF_NOUN;
    }
    else {
	cpm_ptr->cf.pred_type[0] = '\0';
    }

    if (cpm_ptr->cf.type == CF_PRED &&
	(check_feature(b_ptr->f, "サ変") || 
	 check_feature(b_ptr->f, "用言:判"))) {
	cpm_ptr->cf.type_flag = 1;
    }
}

/*==================================================================*/
int make_data_cframe_child(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, TAG_DATA *child_ptr, int child_num, int closest_flag)
/*==================================================================*/
{
    TAG_DATA *cel_b_ptr;

    if ((cel_b_ptr = _make_data_cframe_pp(cpm_ptr, child_ptr, TRUE))) {
	/* 「みかん三個を食べる」 ひとつ前の名詞を格要素とするとき
	   「みかんを三個食べる」 の場合はそのまま両方格要素になる
	*/

	if (check_feature(cel_b_ptr->f, "数量") && 
	    (check_feature(cel_b_ptr->f, "係:ガ格") || check_feature(cel_b_ptr->f, "係:ヲ格")) && 
	    cel_b_ptr->num > 0 && 
	    (check_feature((sp->tag_data + cel_b_ptr->num - 1)->f, "係:隣") || 
	     check_feature((sp->tag_data + cel_b_ptr->num - 1)->f, "係:同格未格")) && 
	    !check_feature((sp->tag_data + cel_b_ptr->num - 1)->f, "数量") && 
	    !check_feature((sp->tag_data + cel_b_ptr->num - 1)->f, "時間")) {
	    _make_data_cframe_sm(cpm_ptr, sp->tag_data + cel_b_ptr->num - 1);
	    _make_data_cframe_ex(cpm_ptr, sp->tag_data + cel_b_ptr->num - 1);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = sp->tag_data + cel_b_ptr->num - 1;
	    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	}
	else {
	    /* 直前格のマーク (厳しい版: 完全に直前のみ) */
	    if (closest_flag) {
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
	    }
	    else {
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	    }
	    _make_data_cframe_sm(cpm_ptr, cel_b_ptr);
	    _make_data_cframe_ex(cpm_ptr, cel_b_ptr);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
	}

	cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num]->next = NULL; /* 並列要素格納用 */

	/* 格が明示されていないことをマーク */
	if (check_feature(cel_b_ptr->f, "係:未格") || 
	    check_feature(cel_b_ptr->f, "係:ノ格") || 
	    cel_b_ptr->inum > 0) {
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	}
	else {
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = child_num;
	}

	cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	cpm_ptr->cf.element_num++;
	if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num = 0;
	}
	return TRUE;
    }

    return FALSE;
}

/*==================================================================*/
 int make_data_cframe_rentai(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    TAG_DATA *b_ptr = cpm_ptr->pred_b_ptr, *cel_b_ptr = NULL;
    int renkaku_exception_p = 0;

    if (check_feature(b_ptr->f, "格要素指定:2")) {
	renkaku_exception_p = 1;
    }

    /* 被連体修飾詞 */
    if ((check_feature(b_ptr->f, "係:連格") && 
	 (b_ptr->para_type != PARA_NORMAL || 
	  b_ptr->num == b_ptr->parent->num)) || /* 用言並列なら、「Vした」と<PARA>が同じときのみ */
	(b_ptr->para_type == PARA_NORMAL && /* 並列の連体修飾 */
	 check_feature(b_ptr->f, "係:連用") && /* ★単純な連用形に限定すべき★ */
	 b_ptr->parent->para_top_p && 
	 check_feature(b_ptr->parent->child[0]->f, "係:連格")) || 
	renkaku_exception_p) {

	/* para_type == PARA_NORMAL は「Vし,Vした PARA N」のとき
	   このときは親(PARA)の親(N)を格要素とする．

	   親がpara_top_pかどうかをみても「VしたNとN PARA」の
	   時と区別ができない
        */

	/* 用言が並列ではないとき */
	if (b_ptr->para_type != PARA_NORMAL) {
	    if (b_ptr->parent) {
		/* 〜のは */
		if (renkaku_exception_p && 
		    b_ptr->parent->parent) {
		    if (check_feature(b_ptr->parent->parent->f, "体言")) {
			cel_b_ptr = b_ptr->parent->parent;
			_make_data_cframe_pp(cpm_ptr, b_ptr, FALSE);
		    }
		}
		else {
		    cel_b_ptr = b_ptr->parent;
		    _make_data_cframe_pp(cpm_ptr, b_ptr, FALSE);
		}

		if (cel_b_ptr) {
		    _make_data_cframe_sm(cpm_ptr, cel_b_ptr);
		    _make_data_cframe_ex(cpm_ptr, cel_b_ptr);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		    cpm_ptr->cf.element_num++;
		}
	    }
	}
	/* 用言が並列のとき */
	else {
	    cel_b_ptr = b_ptr;
	    while (cel_b_ptr->parent->para_type == PARA_NORMAL) {
		cel_b_ptr = cel_b_ptr->parent;
	    }

	    if (cel_b_ptr->parent && 
		cel_b_ptr->parent->parent) {
		_make_data_cframe_pp(cpm_ptr, cel_b_ptr->parent, FALSE);
		_make_data_cframe_sm(cpm_ptr, cel_b_ptr->parent->parent);
		_make_data_cframe_ex(cpm_ptr, cel_b_ptr->parent->parent);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr->parent->parent;
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 2; /* ★不正確★ */
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		cpm_ptr->cf.element_num++;
	    }
	}
    }

    return TRUE;
}

/*==================================================================*/
    int make_data_cframe(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    TAG_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    TAG_DATA *cel_b_ptr = NULL;
    int i, child_num, first, closest, orig_child_num = -1, renkaku_exception_p;

    cpm_ptr->cf.samecase[0][0] = END_M;
    cpm_ptr->cf.samecase[0][1] = END_M;

    cpm_ptr->cf.pred_b_ptr = b_ptr;
    b_ptr->cpm_ptr = cpm_ptr;

    /* 表層格 etc. の設定 */

    cpm_ptr->cf.element_num = 0;

    /* 連体修飾 */
    make_data_cframe_rentai(sp, cpm_ptr);

    for (child_num = 0; b_ptr->child[child_num]; child_num++);

    /* 自分(用言)が複合名詞内 */
    if (b_ptr->inum > 0) {
	TAG_DATA *t_ptr;

	/* 自分(用言)が複合名詞内のときの親 : 被連体修飾詞扱い
	   ※ 連格のとき(「〜したのは」)はすでに扱っている */
	if (cpm_ptr->cf.type == CF_PRED && /* とりあえずサ変のときのみ */
	    !check_feature(b_ptr->f, "係:連格")) {
	    if (!check_feature(b_ptr->parent->f, "外の関係") || 
		check_feature(b_ptr->f, "用言:形")) {
		_make_data_cframe_pp(cpm_ptr, b_ptr, FALSE);
	    }
	    else {
		cpm_ptr->cf.pp[cpm_ptr->cf.element_num][0] = pp_hstr_to_code("外の関係");
		cpm_ptr->cf.pp[cpm_ptr->cf.element_num][1] = END_M;
		cpm_ptr->cf.oblig[cpm_ptr->cf.element_num] = FALSE;
	    }
	    _make_data_cframe_sm(cpm_ptr, b_ptr->parent);
	    _make_data_cframe_ex(cpm_ptr, b_ptr->parent);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent;
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	    cpm_ptr->cf.element_num++;
	}

	/* 文節のheadに係る名詞の取り扱い *

	t_ptr = b_ptr->parent;
	while (1) {
	    if (t_ptr->cpm_ptr) { * 別の格解析対象 *
		t_ptr = NULL;
		break;
	    }
	    if (t_ptr->inum == 0) {
		break;
	    }
	    t_ptr = t_ptr->parent;
	}

	* ... n3 n2 n1
	   複合名詞内の(うしろからみて)最初の用言(格解析対象)に対して
	   n1の子供をとってくる *
	if (t_ptr) {
	    orig_child_num = child_num;
	    for (i = 0; t_ptr->child[i]; i++) {
		* 文節内部以外 *
		if (t_ptr->child[i]->inum == 0) {
		    b_ptr->child[child_num++] = t_ptr->child[i];
		}
	    }
	}
	*/
    }

    /* 子供を格要素に */
    for (i = child_num - 1; i >= 0; i--) {
	if (make_data_cframe_child(sp, cpm_ptr, b_ptr->child[i], i, 
				   i == 0 && b_ptr->num == b_ptr->child[i]->num + 1 && 
				   !check_feature(b_ptr->f, "Ｔ用言同文節") ? TRUE : FALSE)) {
	    if (cpm_ptr->cf.element_num == 0) { /* 子供が作れるはずなのに、作れなかった */
		return -1;
	    }
	}
    }

    /* 複合名詞: 子供をもとにもどす */
    if (orig_child_num >= 0) {
	b_ptr->child[orig_child_num] = NULL;
    }

    /* 用言文節が「（〜を）〜に」のとき 
       「する」の格フレームに対してニ格(同文節)を設定
       ヲ格は子供の処理で扱われる */
    if (check_feature(b_ptr->f, "Ｔ用言同文節")) {
	if (_make_data_cframe_pp(cpm_ptr, b_ptr, TRUE)) {
	    _make_data_cframe_sm(cpm_ptr, b_ptr);
	    _make_data_cframe_ex(cpm_ptr, b_ptr);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr;
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = child_num;
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
	    cpm_ptr->cf.element_num++;
	}
    }

    /* 用言が並列のとき、格要素を expand する */
    if (b_ptr->para_type == PARA_NORMAL && 
	b_ptr->parent && 
	b_ptr->parent->para_top_p) {
	child_num = 0;

	/* <PARA>に係る子供をチェック */
	for (i = 0; b_ptr->parent->child[i]; i++) {
	    if (b_ptr->parent->child[i]->para_type == PARA_NORMAL) {
		child_num++;
	    }
	}
	for (i = 0; b_ptr->parent->child[i]; i++) {
	    if (b_ptr->parent->child[i]->para_type == PARA_NIL && 
		b_ptr->parent->child[i]->num < b_ptr->num) {
		if ((cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->parent->child[i], TRUE))) {
		    _make_data_cframe_sm(cpm_ptr, cel_b_ptr);
		    _make_data_cframe_ex(cpm_ptr, cel_b_ptr);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;

		    /* 格が明示されていないことをマーク */
		    if (check_feature(cel_b_ptr->f, "係:未格") || 
			check_feature(cel_b_ptr->f, "係:ノ格")) {
			cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		    }
		    else {
			cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = i;
		    }

		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = child_num;
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		    cpm_ptr->cf.element_num++;
		}
		if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
		    cpm_ptr->cf.element_num = 0;
		    return -1;
		}
	    }
	}
    }

    /* 直前格要素の取得 */
    closest = get_closest_case_component(sp, cpm_ptr);

    /* 直前格要素のひとつ手前のノ格
       ※ <数量>以外: 一五％の株式を V
          <時間>以外: */
    if (OptCaseFlag & OPT_CASE_NO && 
	closest > -1 && 
	cpm_ptr->elem_b_ptr[closest]->num > 0 && 
	!check_feature((sp->tag_data + cpm_ptr->elem_b_ptr[closest]->num - 1)->f, "数量") && 
	!check_feature((sp->tag_data + cpm_ptr->elem_b_ptr[closest]->num - 1)->f, "時間") && 
	check_feature((sp->tag_data + cpm_ptr->elem_b_ptr[closest]->num - 1)->f, "係:ノ格")) {
	TAG_DATA *bp;
	bp = sp->tag_data + cpm_ptr->elem_b_ptr[closest]->num - 1;

	/* 割り当てる格は格フレームによって動的に変わる */
	cpm_ptr->cf.pp[cpm_ptr->cf.element_num][0] = pp_hstr_to_code("未");
	cpm_ptr->cf.pp[cpm_ptr->cf.element_num][1] = END_M;
	cpm_ptr->cf.sp[cpm_ptr->cf.element_num] = pp_hstr_to_code("の"); /* 表層格 */
	cpm_ptr->cf.oblig[cpm_ptr->cf.element_num] = FALSE;
	_make_data_cframe_sm(cpm_ptr, bp);
	_make_data_cframe_ex(cpm_ptr, bp);
	cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = bp;
	cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	if (cpm_ptr->cf.element_num < CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num++;
	}
    }

    return closest; /* 以下は削除する予定 */

    /* 格要素がひとつで時間格のみの場合、格要素なしと同じように扱う
       ★直前の時間格でもとの格が強い格であるときは普通に扱う★ (現在はニ格のみ) */

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (!MatchPP(cpm_ptr->cf.pp[i][0], "時間")) {
	    return closest;
	}
	/* 直前格でニ格で <時間> のときは削除しない */
	else if (cpm_ptr->pred_b_ptr->num == cpm_ptr->elem_b_ptr[i]->num + 1 && 
		 check_feature(cpm_ptr->elem_b_ptr[i]->f, "係:ニ格")) {
	    return closest;
	}
    }
    cpm_ptr->cf.element_num = 0;
    return -1;
}

/*==================================================================*/
		 void set_pred_voice(BNST_DATA *ptr)
/*==================================================================*/
{
    /* ヴォイスの設定 */

    char *cp;

    ptr->voice = 0;

    if (cp = check_feature(ptr->f, "態")) {
	char *token, *str;

	str = strdup(cp + strlen("態:"));
	token = strtok(str, "|");
	while (token) {
	    if (!strcmp(token, "受動")) {
		ptr->voice |= VOICE_UKEMI;
	    }
	    else if (!strcmp(token, "使役")) {
		ptr->voice |= VOICE_SHIEKI;
	    }
	    else if (!strcmp(token, "もらう")) {
		ptr->voice |= VOICE_MORAU;
	    }
	    else if (!strcmp(token, "ほしい")) {
		ptr->voice |= VOICE_HOSHII;
	    }
	    else if (!strcmp(token, "使役&受動")) {
		ptr->voice |= VOICE_SHIEKI_UKEMI;
	    }
	    /* 「可能」は未扱い */

	    token = strtok(NULL, "|");
	}
	free(str);
    }
}

/*====================================================================
                               END
====================================================================*/

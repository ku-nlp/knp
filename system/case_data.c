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
	     char *make_fukugoji_string(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, fc;
    char buf[SMALL_DATA_LEN];

    /* 付属語がないとき */
    if (b_ptr->num < 1 || (b_ptr-1)->fuzoku_num == 0) {
	return NULL;
    }

    buf[0] = '\0';

    /* 前の文節の助詞 */
    strcat(buf, ((b_ptr-1)->fuzoku_ptr+(b_ptr-1)->fuzoku_num-1)->Goi);

    /* この文節 */
    for (i = 0; i < b_ptr->jiritu_num; i++) {
	if (!strcmp(Class[(b_ptr->jiritu_ptr+i)->Hinshi][0].id, "特殊")) /* 特殊以外 */
	    continue;
	strcat(buf, 
	       (b_ptr->jiritu_ptr+i)->Goi);
    }

    /* 原形の読みに統一 */
    for (i = 0; *(FukugojiTable[i]); i+=2) {
	if (str_eq(buf, FukugojiTable[i])) {
	    strcpy(buf, FukugojiTable[i+1]);
	    break;
	}
    }

    fc = pp_hstr_to_code(buf);
    if (fc != END_M) {
	sprintf(fukugoji_string, "解析格-%s", pp_code_to_kstr(fc));
	return fukugoji_string;
    }
    return NULL;
}

/*==================================================================*/
BNST_DATA *_make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr, int flag)
/*==================================================================*/
{
    int pp_num = 0, cc, jiritsu_num = 0, closest = FALSE;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);
    FEATURE *fp;

    if (b_ptr) {
	/* 自立語の数 */
	if (!check_feature(b_ptr->f, "数量")) {
	    jiritsu_num = b_ptr->jiritu_num;
	}

	/* ★複合名詞のときは? */
	closest = cpm_ptr->pred_b_ptr->num == b_ptr->num+1 ? TRUE : FALSE;
    }

    /* 格要素 */
    if (flag == TRUE) {
	if (b_ptr->num > 0 && 
	    check_feature(b_ptr->f, "係:連用") && 
	    check_feature(b_ptr->f, "複合辞")) {
	    b_ptr--;
	}

	fp = b_ptr->f;
	c_ptr->oblig[c_ptr->element_num] = FALSE;

	while (fp) {
	    if (!strncmp(fp->cp, "解析格-", 7)) {
		cc = pp_kstr_to_code(fp->cp+7);
		if (cc == END_M) {
		    fprintf(stderr, ";; case <%s> in a rule is unknown!\n", fp->cp+7);
		    exit(1);
		}
		c_ptr->pp[c_ptr->element_num][pp_num++] = cc;
		if (pp_num >= PP_ELEMENT_MAX) {
		    fprintf(stderr, ";; not enough pp_num (%d)!\n", PP_ELEMENT_MAX);
		    exit(1);
		}
	    }
	    else if (!strcmp(fp->cp, "必須格")) {
		c_ptr->oblig[c_ptr->element_num] = TRUE;
	    }
	    else if (!strcmp(fp->cp, "Ｔ用言同文節")) {	/* 「〜を〜に」のとき */
		if (cpm_ptr->pred_b_ptr->num != b_ptr->num) {
		    return NULL;
		}
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
    /* 被連体修飾詞 */
    else {
	fp = b_ptr->f;
	c_ptr->oblig[c_ptr->element_num] = FALSE;

	while (fp) {
	    if (!strncmp(fp->cp, "解析連格-", 9)) {
		cc = pp_kstr_to_code(fp->cp+9);
		if (cc == END_M) {
		    fprintf(stderr, ";; case <%s> in a rule is unknown!\n", fp->cp+7);
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
   void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
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
    /* 修飾 */
    else if (check_feature(b_ptr->f, "修飾")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+size*sm_num, 
	       sm2code("修飾"));
	sm_num++;
    }
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
}

/*==================================================================*/
   void _make_data_cframe_ex(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (Thesaurus == USE_BGH) {
	strcpy(c_ptr->ex[c_ptr->element_num], b_ptr->BGH_code);
    }
    else if (Thesaurus == USE_NTT) {
	strcpy(c_ptr->ex[c_ptr->element_num], b_ptr->SM_code);
    }
    strcpy(c_ptr->ex_list[c_ptr->element_num][0], b_ptr->Jiritu_Go);
}

/*==================================================================*/
    void make_data_cframe(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    BNST_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    BNST_DATA *cel_b_ptr;
    int i, child_num, first, closest, orig_child_num = -1;
    char *vtype = NULL;

    cpm_ptr->cf.voice = b_ptr->voice;

    if ((vtype = check_feature(b_ptr->f, "用言"))) {
	vtype += 5;
	strcpy(cpm_ptr->cf.pred_type, vtype);
    }
    else if (check_feature(b_ptr->f, "サ変")) {
	strcpy(cpm_ptr->cf.pred_type, "動");
    }
    else if (check_feature(b_ptr->f, "名詞的形容詞語幹")) {
	strcpy(cpm_ptr->cf.pred_type, "形");
    }
    else if (check_feature(b_ptr->f, "準用言")) {
	strcpy(cpm_ptr->cf.pred_type, "準");
    }
    else {
	cpm_ptr->cf.pred_type[0] = '\0';
    }

    cpm_ptr->cf.samecase[0][0] = END_M;
    cpm_ptr->cf.samecase[0][1] = END_M;

    cpm_ptr->cf.pred_b_ptr = b_ptr;
    b_ptr->cpm_ptr = cpm_ptr;

    /* 表層格 etc. の設定 */

    cpm_ptr->cf.element_num = 0;

    /* 用言文節が「（〜を）〜に」のとき 
       「する」の格フレームに対してニ格(同文節)を設定 */
    if (check_feature(b_ptr->f, "ID:（〜を）〜に")) {
	if (_make_data_cframe_pp(cpm_ptr, b_ptr, TRUE)) {
	    _make_data_cframe_sm(cpm_ptr, b_ptr);
	    _make_data_cframe_ex(cpm_ptr, b_ptr);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr;
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
	    cpm_ptr->cf.element_num ++;
	}
    }

    if (check_feature(b_ptr->f, "係:連格")) {

	/* para_type == PARA_NORMAL は「Vし,Vした PARA N」のとき
	   このときは親(PARA)の親(N)を格要素とする．

	   親がpara_top_pかどうかをみても「VしたNとN PARA」の
	   時と区別ができない
        */

	/* 用言が並列ではないとき */
	if (b_ptr->para_type != PARA_NORMAL) {
	    if (b_ptr->parent) {
		/* 外の関係以外のときは格要素に (外の関係でも形容詞のときは格要素にする) */
		if (!(check_feature(b_ptr->parent->f, "外の関係") || 
		      check_feature(b_ptr->parent->f, "外の関係可能性")) || 
		    check_feature(b_ptr->f, "用言:形")) {
		    _make_data_cframe_pp(cpm_ptr, b_ptr, FALSE);
		    _make_data_cframe_sm(cpm_ptr, b_ptr->parent);
		    _make_data_cframe_ex(cpm_ptr, b_ptr->parent);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		    cpm_ptr->cf.element_num ++;
		}
		else {
		    cpm_ptr->default_score = SOTO_SCORE;
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
		if (!(check_feature(cel_b_ptr->parent->parent->f, "外の関係") || 
		      check_feature(b_ptr->parent->parent->f, "外の関係可能性")) || 
		    check_feature(b_ptr->f, "用言:形")) {
		    _make_data_cframe_pp(cpm_ptr, cel_b_ptr->parent, FALSE);
		    _make_data_cframe_sm(cpm_ptr, cel_b_ptr->parent->parent);
		    _make_data_cframe_ex(cpm_ptr, cel_b_ptr->parent->parent);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr->parent->parent;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		    cpm_ptr->cf.element_num ++;
		}
		else {
		    cpm_ptr->default_score = SOTO_SCORE;
		}
	    }
	}
    }

    /* 自分(用言)が複合名詞内のときの親 */
    if (b_ptr->num == -1) {
	if (b_ptr->parent && b_ptr->parent->num == -1) {
	    _make_data_cframe_pp(cpm_ptr, b_ptr, FALSE);
	    _make_data_cframe_sm(cpm_ptr, b_ptr->parent);
	    _make_data_cframe_ex(cpm_ptr, b_ptr->parent);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent;
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	    cpm_ptr->cf.element_num ++;
	}
	/* 主辞 */
	else {
	    for (child_num = 0; b_ptr->child[child_num]; child_num++);
	    orig_child_num = child_num;
	    for (i = 0; b_ptr->parent->child[i]; i++) {
		b_ptr->child[child_num+i] = b_ptr->parent->child[i];
	    }
	}
    }

    /* 子供を格要素に */
    for (child_num=0; b_ptr->child[child_num]; child_num++);
    for (i = child_num - 1; i >= 0; i--) {
	if ((cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->child[i], TRUE))) {
	    /* 「みかん三個を食べる」 ひとつ前の名詞を格要素とするとき
	       「みかんを三個食べる」 の場合はそのまま両方格要素になる
	     */
	    if (check_feature(cel_b_ptr->f, "数量") && 
		(check_feature(cel_b_ptr->f, "係:ガ格") || check_feature(cel_b_ptr->f, "係:ヲ格")) && 
		cel_b_ptr->num > 0 && 
		(check_feature((sp->bnst_data+cel_b_ptr->num-1)->f, "係:隣接") || 
		 check_feature((sp->bnst_data+cel_b_ptr->num-1)->f, "係:同格未格")) && 
		!check_feature((sp->bnst_data+cel_b_ptr->num-1)->f, "数量") && 
		!check_feature((sp->bnst_data+cel_b_ptr->num-1)->f, "時間")) {
		_make_data_cframe_sm(cpm_ptr, sp->bnst_data+cel_b_ptr->num-1);
		_make_data_cframe_ex(cpm_ptr, sp->bnst_data+cel_b_ptr->num-1);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = sp->bnst_data+cel_b_ptr->num-1;
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	    }
	    else {
		/* 直前格のマーク (厳しい版: 完全に直前のみ) */
		if (i == 0 && b_ptr->num == b_ptr->child[i]->num+1 && 
		    !check_feature(b_ptr->f, "ID:（〜を）〜に")) {
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
		}
		else {
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		}
		_make_data_cframe_sm(cpm_ptr, cel_b_ptr);
		_make_data_cframe_ex(cpm_ptr, cel_b_ptr);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
	    }
	    if (check_feature(cel_b_ptr->f, "係:未格")) {
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	    }
	    else {
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = i;
	    }
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.element_num ++;
	}
	if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num = 0;
	    return;
	}
    }

    /* 複合名詞: 子供をもとにもどす */
    if (orig_child_num >= 0) {
	b_ptr->child[orig_child_num] = NULL;
    }

    /* 用言が並列のとき、格要素を expand する */
    if (b_ptr->para_type == PARA_NORMAL && 
	b_ptr->parent && 
	b_ptr->parent->para_top_p) {
	child_num = 0;
	for (i = 0; b_ptr->parent->child[i]; i++) {
	    if (b_ptr->parent->child[i]->para_type == PARA_NORMAL) {
		child_num++;
	    }
	}
	for (i = 0; b_ptr->parent->child[i]; i++) {
	    if (b_ptr->parent->child[i]->para_type == PARA_NIL) {
		if ((cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->parent->child[i], TRUE))) {
		    _make_data_cframe_sm(cpm_ptr, cel_b_ptr);
		    _make_data_cframe_ex(cpm_ptr, cel_b_ptr);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = i;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = child_num;
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		    cpm_ptr->cf.element_num ++;
		}
		if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
		    cpm_ptr->cf.element_num = 0;
		    return;
		}
	    }
	}
    }

    closest = get_closest_case_component(sp, cpm_ptr);

    /* 直前格要素のひとつ手前のノ格
       ※ <数量>以外: 一五％の株式を V
          <時間>以外: */
    if (OptCaseFlag & OPT_CASE_NO && 
	closest > -1 && 
	cpm_ptr->elem_b_ptr[closest]->num > 0 && 
	!check_feature((sp->bnst_data+cpm_ptr->elem_b_ptr[closest]->num-1)->f, "数量") && 
	!check_feature((sp->bnst_data+cpm_ptr->elem_b_ptr[closest]->num-1)->f, "時間") && 
	check_feature((sp->bnst_data+cpm_ptr->elem_b_ptr[closest]->num-1)->f, "係:ノ格")
	/* !check_feature(cpm_ptr->elem_b_ptr[closest]->f, "数量") && 
	!check_feature(cpm_ptr->elem_b_ptr[closest]->f, "時間") */
	) {
	BNST_DATA *bp;
	bp = sp->bnst_data+cpm_ptr->elem_b_ptr[closest]->num-1;

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

    /* 格要素がひとつで時間格のみの場合、格要素なしと同じように扱う
       ★直前の時間格でもとの格が強い格であるときは普通に扱う★ (現在はニ格のみ) */

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (!MatchPP(cpm_ptr->cf.pp[i][0], "時間")) {
	    return;
	}
	/* 直前格でニ格で <時間> のときは削除しない */
	else if (cpm_ptr->pred_b_ptr->num == cpm_ptr->elem_b_ptr[i]->num+1 && 
	    check_feature(cpm_ptr->elem_b_ptr[i]->f, "係:ニ格")) {
	    return;
	}
    }
    cpm_ptr->cf.element_num = 0;
    return;
}

/*==================================================================*/
		void set_pred_voice(BNST_DATA *b_ptr)
/*==================================================================*/
{
    /* ヴォイスの設定 */ /* ★★★ 修正必要 ★★★ */

    if (check_feature(b_ptr->f, "〜せる") ||
	check_feature(b_ptr->f, "〜させる")) {
	b_ptr->voice = VOICE_SHIEKI;
    } else if (check_feature(b_ptr->f, "〜れる") ||
	       check_feature(b_ptr->f, "〜られる")) {
	b_ptr->voice = VOICE_UKEMI;
    } else if (check_feature(b_ptr->f, "〜もらう")) {
	b_ptr->voice = VOICE_MORAU;
    }
}

/*====================================================================
                               END
====================================================================*/

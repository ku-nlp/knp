/*====================================================================

			  格構造解析: 入力側

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int SOTO_SCORE = 7;

char fukugoji_string[64];

char *FukugojiTable[] = {"を除く", "をのぞく", 
			 "を通じる", "をつうじる", 
			 "を通ずる", "をつうずる", 
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
    int i;

    fukugoji_string[0] = '\0';

    /* 前の文節の助詞 */
    strcat(fukugoji_string, 
	   ((b_ptr-1)->fuzoku_ptr+(b_ptr-1)->fuzoku_num-1)->Goi);

    /* この文節 */
    for (i = 0; i < b_ptr->jiritu_num; i++) {
	if ((b_ptr->jiritu_ptr+i)->Hinshi == 1)	/* 特殊以外 */
	    continue;
	strcat(fukugoji_string, 
	       (b_ptr->jiritu_ptr+i)->Goi);
    }

    /* 原形の読みに統一 */
    for (i = 0; *(FukugojiTable[i]); i+=2) {
	if (str_eq(fukugoji_string, FukugojiTable[i])) {
	    strcpy(fukugoji_string, FukugojiTable[i+1]);
	}
    }

    return fukugoji_string;
}

/*==================================================================*/
BNST_DATA *_make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, num, pp_num = 0, jiritsu_num = 0;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (b_ptr && !check_feature(b_ptr->f, "数量")) {
	jiritsu_num = b_ptr->jiritu_num;
    }

    if (b_ptr == NULL) {	/* 埋め込み文の被修飾詞 */
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("＊");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "修飾")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("修飾");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ガ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("が");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヲ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("を");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヘ格")) {
	/* ヘ格 or ニ格 */
	c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("へ");
	c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ノ格") && 
	     !check_feature(cpm_ptr->pred_b_ptr->f, "用言:判") && 
	     !check_feature(cpm_ptr->pred_b_ptr->f, "準用言")) {
	/* 隣接しない場合は? */
	/* 類似度に閾値を設けて、単なる修飾を区別する必要がある */

	/* <時間> なら時間格のみにする */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	}
	/* else if (check_feature(b_ptr->f, "複固:地名")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("で");
	} */
	else if (check_feature(b_ptr->f, "数量")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("φ");
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("が");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("を");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ニ格")) {
	/* ニ格で時間なら時間格 */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	    c_ptr->oblig[c_ptr->element_num] = TRUE;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヨリ格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("より");
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("より");
	    c_ptr->oblig[c_ptr->element_num] = TRUE;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }

    else if (check_feature(b_ptr->f, "係:デ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("で");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	if (check_feature(b_ptr->f, "デモ") || 
	    check_feature(b_ptr->f, "デハ")) {
	    c_ptr->pp[c_ptr->element_num][1] = -1;
	    c_ptr->pp[c_ptr->element_num][2] = END_M;
	}
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:カラ格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("から");
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("から");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ト格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("と");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:マデ格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("まで");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:無格")) {
	/* 無格で時間なら時間格 */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("φ");
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("φ");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:未格") || 
	     check_feature(b_ptr->f, "係:同格未格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	    }
	}
	else {
	    /* 提題ではないときにする?
	    if (check_feature(b_ptr->f, "提題")) {
		return NULL;
	    } */
	    c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }
    /* 係:連用, 複合辞 */
    else if (check_feature(b_ptr->f, "複合辞") && 
	     check_feature(b_ptr->f, "係:連用") && 
	     b_ptr->child[0]) {
	int fc = pp_hstr_to_code(make_fukugoji_string(b_ptr));
	if (fc == END_M) {
	    return NULL;
	}
	c_ptr->pp[c_ptr->element_num][0] = fc;
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr->child[0];
    }
    else if (check_feature(b_ptr->f, "係:連体") &&
	     !check_feature(cpm_ptr->pred_b_ptr->f, "用言:判")) {
	/* 「〜からの」: カラ格
	   「発展途上国からの批判」: ガ格 扱えない */
	if (check_feature(b_ptr->f, "カラ")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("から");
	    /* 時間格もあるかも */
	}
	/* 「〜との」: ト格 */
	else if (check_feature(b_ptr->f, "ト")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("と");
	}
	/* 「〜での」: デ格 */
	else if (check_feature(b_ptr->f, "デ")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("で");
	}
	/* 「〜までの」: マデ格 */
	else if (check_feature(b_ptr->f, "マデ")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("まで");
	    /* 時間格もあるかも */
	}
	/* 「〜への」: ヘ格, ニ格 */
	else if (check_feature(b_ptr->f, "ヘ")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("へ");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	}
	else {
	    return NULL;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    /* 複合名詞 */
    else if (check_feature(b_ptr->f, "係:文節内")) {
	/* 時間なら時間格のみにする
	   ★ 「新旧交代」: ガ格 */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	}
	/* 地名ならとりあえずガ格/デ格にする */
	else if (check_feature(b_ptr->f, "複固:地名")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("が");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("で");
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("が");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("を");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:隣接") && check_feature(b_ptr->f, "裸名詞")) {
	/* 「１階級上げた」 */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	}
	else if (check_feature(b_ptr->f, "数量")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("φ");
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("が");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("を");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    /* 「〜小池さんらで、」 のような表現 -> ガ格
    else if (check_feature(b_ptr->f, "複固:人名") && 
	     check_feature(b_ptr->f, "ID:〜で") && 
	     check_feature(b_ptr->f, "用言:判")) {
	c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("が");
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    } */
    else {
	return NULL;
    }
}

/*==================================================================*/
   void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, sm_num = 0, qua_flag = FALSE, tim_flag = FALSE;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    /* 格要素 -- 文 */
    if (check_feature(b_ptr->f, "補文")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
	       (char *)sm2code("補文"));
	sm_num++;
    }
    /* 修飾 */
    else if (check_feature(b_ptr->f, "修飾")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
	       (char *)sm2code("修飾"));
	sm_num++;
    }
    else {
	if (check_feature(b_ptr->f, "時間")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("時間"));
	    sm_num++;
	}
	if (check_feature(b_ptr->f, "数量")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("数量"));
	    sm_num++;
	}
	if (check_feature(b_ptr->f, "人名") || 
	    check_feature(b_ptr->f, "組織名")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("主体"));
	    sm_num++;
	}
	
	/* for (i = 0; i < b_ptr->SM_num; i++) */
	strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
	       b_ptr->SM_code);
	sm_num += strlen(b_ptr->SM_code)/SM_CODE_SIZE;
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
	strcpy(c_ptr->ex2[c_ptr->element_num], b_ptr->SM_code);
    }
}

/*==================================================================*/
    void make_data_cframe(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    BNST_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    BNST_DATA *cel_b_ptr;
    int i, j, k, child_num;
    char *vtype = NULL;

    if (vtype = (char *)check_feature(b_ptr->f, "用言")) {
	vtype += 5;
	strcpy(cpm_ptr->cf.ipal_id, vtype);
    }
    else {
	cpm_ptr->cf.ipal_id[0] = '\0';
    }

    cpm_ptr->cf.pred_b_ptr = b_ptr;
    b_ptr->cpm_ptr = cpm_ptr;

    /* 表層格 etc. の設定 */

    cpm_ptr->cf.element_num = 0;
    if (check_feature(b_ptr->f, "係:連格")) {

	/* para_type == PARA_NORMAL は「Vし,Vした PARA N」のとき
	   このときは親(PARA)の親(N)を格要素とする．

	   親がpara_top_pかどうかをみても「VしたNとN PARA」の
	   時と区別ができない
        */

	if (b_ptr->para_type != PARA_NORMAL) {
	    if (b_ptr->parent) {
		/* 外の関係以外のときは格要素に (外の関係でも形容詞のときは格要素にする) */
		if (!(check_feature(b_ptr->parent->f, "外の関係") || 
		      check_feature(b_ptr->parent->f, "外の関係可能性")) || 
		    check_feature(b_ptr->f, "用言:形")) {
		    _make_data_cframe_pp(cpm_ptr, NULL);
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
	} else {
	    cel_b_ptr = b_ptr;
	    while (cel_b_ptr->parent->para_type == PARA_NORMAL) {
		cel_b_ptr = cel_b_ptr->parent;
	    }

	    if (cel_b_ptr->parent && 
		cel_b_ptr->parent->parent) {
		if (!check_feature(cel_b_ptr->parent->parent->f, "外の関係") || 
		    check_feature(b_ptr->f, "用言:形")) {
		    _make_data_cframe_pp(cpm_ptr, NULL);
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

    for (child_num=0; b_ptr->child[child_num]; child_num++);
    for (i = child_num - 1; i >= 0; i--) {
	if (cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->child[i])) {
	    /* 「みかん三個を食べる」 ひとつ前の名詞を格要素とするとき
	       「みかんを三個食べる」 の場合はそのまま両方格要素になる
	     */
	    if (check_feature(cel_b_ptr->f, "数量") && 
		(check_feature(cel_b_ptr->f, "係:ガ格") || check_feature(cel_b_ptr->f, "係:ヲ格")) && 
		cel_b_ptr->num > 0 && check_feature((sp->bnst_data+cel_b_ptr->num-1)->f, "係:隣接") && 
		!check_feature((sp->bnst_data+cel_b_ptr->num-1)->f, "数量")) {
		_make_data_cframe_sm(cpm_ptr, sp->bnst_data+cel_b_ptr->num-1);
		_make_data_cframe_ex(cpm_ptr, sp->bnst_data+cel_b_ptr->num-1);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = sp->bnst_data+cel_b_ptr->num-1;
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	    }
	    else {
		/* 直前格のマーク (厳しい版: 完全に直前のみ) */
		if (i == 0 && b_ptr->num == b_ptr->child[i]->num+1) {
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
		}
		else {
		    cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
		}
		_make_data_cframe_sm(cpm_ptr, cel_b_ptr);
		_make_data_cframe_ex(cpm_ptr, cel_b_ptr);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
	    }
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = i;
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.element_num ++;
	}
	if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num = 0;
	    return;
	}
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
		if (cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->parent->child[i])) {
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

    /* 複合名詞のとき */
    if (b_ptr->internal_num && !check_feature(b_ptr->f, "名詞+接尾辞")) {
	/* とりあえず後から 2 つめの形態素を扱う */
	_make_data_cframe_pp(cpm_ptr, b_ptr->internal+b_ptr->internal_num-1);
	_make_data_cframe_sm(cpm_ptr, b_ptr->internal+b_ptr->internal_num-1);
	_make_data_cframe_ex(cpm_ptr, b_ptr->internal+b_ptr->internal_num-1);
	cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->internal+b_ptr->internal_num-1;
	cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
	cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = FALSE;
	cpm_ptr->cf.element_num ++;
	if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num = 0;
	    return;
	}
    }

    /* 格要素がひとつで時間格のみの場合、格要素なしと同じように扱う
       ★直前の時間格は普通に扱ってもよい★ */
    /* if (cpm_ptr->cf.element_num == 1 && 
	MatchPP(cpm_ptr->cf.pp[0][0], "時間") && cpm_ptr->cf.pp[0][1] == END_M) {
	cpm_ptr->cf.element_num = 0;
    } */
    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (!MatchPP(cpm_ptr->cf.pp[i][0], "時間")) {
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

    int i;

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

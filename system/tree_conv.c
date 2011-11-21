/*====================================================================

			      木構造処理

                                               S.Kurohashi 92.10.17
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
	   void init_bnst_tree_property(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < BNST_MAX; i++) {
	sp->bnst_data[i].parent = NULL;
	sp->bnst_data[i].child[0] = NULL;
	sp->bnst_data[i].para_top_p = FALSE;
	sp->bnst_data[i].para_type = PARA_NIL;
	sp->bnst_data[i].to_para_p = FALSE;
    }
}

/*==================================================================*/
	    void init_tag_tree_property(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < TAG_MAX; i++) {
	sp->tag_data[i].parent = NULL;
	sp->tag_data[i].child[0] = NULL;
	sp->tag_data[i].para_top_p = FALSE;
	sp->tag_data[i].para_type = PARA_NIL;
	sp->tag_data[i].to_para_p = FALSE;
    }
}

/*==================================================================*/
	   void init_mrph_tree_property(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < MRPH_MAX; i++) {
	sp->mrph_data[i].parent = NULL;
	sp->mrph_data[i].child[0] = NULL;
	sp->mrph_data[i].para_top_p = FALSE;
	sp->mrph_data[i].para_type = PARA_NIL;
	sp->mrph_data[i].to_para_p = FALSE;
    }
}

/*==================================================================*/
BNST_DATA *t_add_node(BNST_DATA *parent, BNST_DATA *child, int pos)
/*==================================================================*/
{
    int i, child_num;

    for (child_num = 0; parent->child[child_num]; child_num++)
	;

    if (pos == -1) {
	parent->child[child_num] = child;
	parent->child[child_num + 1] = NULL;
    }
    else {
	for (i = child_num; i >= pos; i--)
	    parent->child[i + 1] = parent->child[i];
	parent->child[pos] = child;
    }

    return parent;
}

/*==================================================================*/
BNST_DATA *t_attach_node(BNST_DATA *parent, BNST_DATA *child, int pos)
/*==================================================================*/
{
    child->parent = parent;

    return t_add_node(parent, child, pos);
}
/*==================================================================*/
    BNST_DATA *t_del_node(BNST_DATA *parent, BNST_DATA *child)
/*==================================================================*/
{
    int i, j;
    
    for (i = 0; parent->child[i]; i++) {
	if (parent->child[i] == child) {
	    for (j = i; parent->child[j]; j++)
	      parent->child[j] = parent->child[j + 1];
	    break;
	}
    }

    child->parent = NULL;
    
    return child;
}

/*==================================================================*/
	       int make_simple_tree(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, child_num, pre_node_child_num;
    int buffer[BNST_MAX];
    BNST_DATA *tmp_b_ptr;

    /* dpnd.head[i]をbuffer[i]にコピーし，3つ以上からなる並列構造では
       係先を隣のheadから末尾のheadに変更する．
       また部分並列の係り先を末尾のheadに変更する．*/

    for (i = 0; i < sp->Bnst_num - 1; i++)
	buffer[i] = sp->bnst_data[i].dpnd_head;

    for (i = 0; i < sp->Para_M_num; i++) {
	for (j = 0; j < sp->para_manager[i].part_num - 1; j++) {
	    buffer[sp->para_manager[i].end[j]] = 
		sp->para_manager[i].end[sp->para_manager[i].part_num - 1];

	    /*
	    printf(">>> (%d,%d) %d -> %d\n", i, j, sp->para_manager[i].end[j],
		   sp->para_manager[i].end[sp->para_manager[i].part_num - 1]);
	    */

	    for (k = sp->para_manager[i].start[j];
		 k <= sp->para_manager[i].end[j]; k++)
		if (Mask_matrix[k][sp->para_manager[i].end[j]] == 3) 
		    buffer[k] = 
			sp->para_manager[i].end[sp->para_manager[i].part_num - 1];
	}
    }

    /* 依存構造木構造リンク付け */

    pre_node_child_num = 0;
    for (j = sp->Bnst_num - 1; j >= 0; j--) { /* 受け側 */
	if (pre_node_child_num != 0) {
	    child_num = pre_node_child_num;
	    pre_node_child_num = 0;
	}
	else {
	    child_num = 0;
	}
	for (i = j - 1; i >= 0; i--) { /* 係り側 */
	    if (sp->bnst_data[i].num == -1) {
		continue; /* 後処理でマージされたノード */
	    }
	    if (buffer[i] == j) { /* i -> j */
		if (sp->bnst_data[j].num == -1) { /* 後処理でマージされたノード */
		    if (j - i == 1) { /* マージ側 -> マージされた側: スキップ */
			continue;
		    }
		    else { /* マージされたノードに係るノード (直前以外) */
			sp->bnst_data[j - 1].child[pre_node_child_num++] = sp->bnst_data + i;
		    }
		}
		else {
		    sp->bnst_data[j].child[child_num++] = sp->bnst_data + i;
		}
		if (child_num >= PARA_PART_MAX) {
		    child_num = PARA_PART_MAX-1;
		    break;
		}
		sp->bnst_data[i].parent = sp->bnst_data[j].num == -1 ? sp->bnst_data + j - 1 : sp->bnst_data + j; /* 後処理でマージされたノードならば -1 */
		if (Mask_matrix[i][j] == 3) {
		    sp->bnst_data[i].para_type = PARA_INCOMP;
		}
		/* PARA_NORMALは展開時にセット */
	    }
	}
	sp->bnst_data[j].child[child_num] = NULL;
    }

    /* 子供をsort */
    for (j = sp->Bnst_num - 1; j >= 0; j--) {
	for (child_num = 0; sp->bnst_data[j].child[child_num]; child_num++) {
	    ;
	}
	if (child_num < 2) { /* 2個以上のみ */
	    continue;
	}
	for (i = 0; i < child_num - 1; i++) {
	    for (k = i + 1; k < child_num; k++) {
		if (sp->bnst_data[j].child[i]->num < sp->bnst_data[j].child[k]->num) {
		    tmp_b_ptr = sp->bnst_data[j].child[i];
		    sp->bnst_data[j].child[i] = sp->bnst_data[j].child[k];
		    sp->bnst_data[j].child[k] = tmp_b_ptr;
		}
	    }
	}
    }

    return TRUE;
}

/*==================================================================*/
BNST_DATA *strong_corr_node(SENTENCE_DATA *sp, PARA_DATA *p_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i;

    for (i = p_ptr->jend_pos - p_ptr->key_pos - 1; i >= 0; i--) {
	if (sp->bnst_data + p_ptr->max_path[i] == b_ptr)
	  return sp->bnst_data + p_ptr->key_pos + i + 1;
    }
    return NULL;
}

/*==================================================================*/;
   void strong_para_expand(SENTENCE_DATA *sp, PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    int i, j, k;
    PARA_DATA *p_ptr, *pp_ptr;
    BNST_DATA *start_b_ptr, *b_ptr, *bb_ptr;

    /* 強並列内に係る文節を展開 : コピー無 */

    for (i = 0; i < m_ptr->child_num; i++)
      strong_para_expand(sp, m_ptr->child[i]);


    p_ptr = sp->para_data + m_ptr->para_data_num[0];
    if (p_ptr->status == 's') {
	start_b_ptr = sp->bnst_data + m_ptr->start[0];
	for (i = m_ptr->start[0], b_ptr = start_b_ptr; i < m_ptr->end[0]; 
	     i++, b_ptr++)
	    for (j = 0; b_ptr->child[j]; j++)
		if (b_ptr->child[j] < start_b_ptr) {
		    b_ptr->child[j]->to_para_p = TRUE;
		    bb_ptr = b_ptr;
		    for (k = 0, pp_ptr = p_ptr; k < m_ptr->para_num; k++, pp_ptr++)
			if ((bb_ptr = strong_corr_node(sp, pp_ptr, bb_ptr)))
			    t_add_node(bb_ptr, b_ptr->child[j], -1);
		}
    }
}

/*==================================================================*/
  int get_correct_postprocessed_bnst_num(SENTENCE_DATA *sp, int num)
/*==================================================================*/
{
    int i;

    for (i = num; i >= 0; i--) {
	if ((sp->bnst_data + i)->num != -1) {
	    return i;
	}
    }
    return i;
}

/*==================================================================*/
     int para_top_expand(SENTENCE_DATA *sp, PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    int i;
    BNST_DATA *new_ptr, *end_ptr, *pre_end_ptr;

    /* 並列をまとめるノードの挿入

       B  B<P> B  B<P>(end_ptr) B  B ｜ .... B(new_ptr)
                ↑                  (↑ここまでが通常の文節)
                ｜
                B(new_ptr) をここに挿入し B<P>(end_ptr)の内容をコピー
		B<P>(end_ptr) は PARA(並列をまとめるノード)となる
    */

    for (i = 0; i < m_ptr->child_num; i++) {
	if (para_top_expand(sp, m_ptr->child[i]) == FALSE) {
	    return FALSE;
	}
    }

    end_ptr = sp->bnst_data + get_correct_postprocessed_bnst_num(sp, m_ptr->end[m_ptr->part_num - 1]);
    pre_end_ptr = sp->bnst_data + get_correct_postprocessed_bnst_num(sp, m_ptr->end[m_ptr->part_num - 2]);
	
    new_ptr = sp->bnst_data + sp->Bnst_num + sp->New_Bnst_num;
    sp->New_Bnst_num++;
    if ((sp->Bnst_num + sp->New_Bnst_num) > BNST_MAX) {
	fprintf(stderr, ";; Too many nodes in expanding para top .\n");
	return FALSE;
    }
    if (sp->Max_New_Bnst_num < sp->New_Bnst_num) {
	sp->Max_New_Bnst_num = sp->New_Bnst_num;
    }
    *new_ptr = *end_ptr;	/* コピー */

    /*
      new_ptr に end_ptr をコピーすると双方のf(featureへのポインタ)から
      fの実体がポイントされ，freeする際に問題となる．当初は,

      	end_ptr->f = NULL;
	
      として対処していたがこれでは並列末尾の文節がその後の用言の格解析で
      格要素とみなされない，また何度かmake_treeを行うfがなくなってしまう
      などの問題があったので，clear_featureを文ごとの解析のループの
      先頭で行うように修正した (98/02/07)
    */

    /* 子ノードの整理 */

    new_ptr->child[0] = NULL;
    t_attach_node(end_ptr, new_ptr, 0);
    while (pre_end_ptr < end_ptr->child[1] && 
	   end_ptr->child[1]->para_type != PARA_INCOMP)
	t_attach_node(new_ptr, t_del_node(end_ptr, end_ptr->child[1]), -1);

    /* フラグ(PARA,<P>)の整理 */

    end_ptr->para_type = PARA_NIL;
    end_ptr->para_top_p = TRUE;
    new_ptr->para_type = PARA_NORMAL;
    for (i = 0; i < m_ptr->part_num - 1; i++)
	(sp->bnst_data + get_correct_postprocessed_bnst_num(sp, m_ptr->end[i]))->para_type = PARA_NORMAL;

    return TRUE;
}

/*==================================================================*/
	      void para_modifier_expand(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, j, k;
    
    /* PARA に係っているノードを <P> に係ける : コピー無 */

    if (b_ptr->para_top_p == TRUE) {
	for ( i=0; b_ptr->child[i]; i++ ) {
	    if (b_ptr->child[i]->para_type == PARA_NIL && 
		!check_feature(b_ptr->child[i]->f, "係:連用")) {

		/* b_ptr->child[i] 修飾文節 */

		b_ptr->child[i]->to_para_p = TRUE;

		for ( j=0; b_ptr->child[j]; j++ ) {
		    if (b_ptr->child[j]->para_type == PARA_NORMAL) {

			/* b_ptr->child[j] <P>文節 */
			
			for ( k=0; b_ptr->child[j]->child[k]; k++ );
			b_ptr->child[j]->child[k] = b_ptr->child[i];
			b_ptr->child[j]->child[k+1] = NULL;
		    }
		}
		b_ptr->child[i] = NULL;
	    }
	}
    }

    for ( i=0; b_ptr->child[i]; i++ )
	para_modifier_expand(b_ptr->child[i]);
}

/*==================================================================*/
   void incomplete_para_expand(SENTENCE_DATA *sp, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, j, para_pos;
    int new_num, child_num;
    BNST_DATA *para_ptr, *new_ptr;
    BNST_DATA *pre_childs[10], *pos_childs[10];

    /* 部分並列の展開 : コピー有(述語が新データ，もとの述語はPARAに) */

    para_pos = -1;

    for ( i=0; b_ptr->child[i]; i++ )
	if (b_ptr->child[i]->para_top_p == TRUE)
	    for ( j=0; b_ptr->child[i]->child[j]; j++ )
		if (b_ptr->child[i]->child[j]->para_type == PARA_INCOMP) {
		    para_pos = i;
		    break;
		}

    if (para_pos != -1) {

	/* もとの修飾要素をストック */

	for ( i=0; b_ptr->child[i] && i < para_pos; i++ )
	    pre_childs[i] = b_ptr->child[i];
	pre_childs[i] = NULL;
	for ( i=para_pos+1, j = 0; b_ptr->child[i]; i++, j++ )
	    pos_childs[j] = b_ptr->child[i];
	pos_childs[j] = NULL;

	para_ptr = b_ptr->child[para_pos];

	new_num = 0;
	for ( i=0; para_ptr->child[i]; i++ )
	    if (para_ptr->child[i]->para_type == PARA_NORMAL) {
	      
		new_ptr = sp->bnst_data + sp->Bnst_num + sp->New_Bnst_num;
		sp->New_Bnst_num++;
		if ((sp->Bnst_num + sp->New_Bnst_num) > BNST_MAX) {
		    fprintf(stderr, 
			    ";; Too many nodes in expanding incomplete para .\n");
		    exit(1);
		}
		*new_ptr = *b_ptr;		/* コピー */
		new_ptr->f = NULL;		/* 注意！！ こうしないと後でSF */

		new_ptr->parent = b_ptr;		/* 新ノードの親(自分自身) */

		b_ptr->child[new_num] = new_ptr; 	/* 元ノードは PARA */

		/* 新しいノードの子を設定
		   (後ろの修飾ノード，<P>，<I>, 前の修飾ノード) */

		child_num = 0;
		for (j=0; pre_childs[j]; j++)
		    new_ptr->child[child_num++] = pre_childs[j];
		new_ptr->child[child_num++] = para_ptr->child[i];
		while (para_ptr->child[i+1] &&
		       para_ptr->child[i+1]->para_type == PARA_INCOMP) {
		    new_ptr->child[child_num++] = para_ptr->child[i+1];
		    i++;
		}
		for (j=0; pos_childs[j]; j++)
		    new_ptr->child[child_num++] = pos_childs[j];
		new_ptr->child[child_num++] = NULL;

		new_ptr++;
		new_num++;
	    }
	b_ptr->child[new_num] = NULL;
	b_ptr->para_top_p = TRUE;
    }

    for ( i=0; b_ptr->child[i]; i++ )
      incomplete_para_expand(sp, b_ptr->child[i]);
}

/*==================================================================*/
		int make_dpnd_tree(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    init_bnst_tree_property(sp);

    sp->New_Bnst_num = 0;    				/* 初期化 */

    if (make_simple_tree(sp) == FALSE) {		/* リンク付け */
	return FALSE;
    }
	
    if (OptExpandP == TRUE) {
	for (i = 0; i < sp->Para_M_num; i++) {		/* 強並列の展開 */
	    if (sp->para_manager[i].parent == NULL) {
		strong_para_expand(sp, sp->para_manager + i);
	    }
	}
    }

    for (i = 0; i < sp->Para_M_num; i++) {		/* PARAの展開 */
	if (sp->para_manager[i].parent == NULL) {
	    if (para_top_expand(sp, sp->para_manager + i) == FALSE) {
		return FALSE;
	    }
	}
    }

    if (OptExpandP == TRUE) {
	para_modifier_expand(sp->bnst_data + sp->Bnst_num - 1);	/* PARA修飾の展開 */
    }

    /*
    incomplete_para_expand(sp->bnst_data + sp->Bnst_num - 1);*/	/* 部分並列の展開 */

    return TRUE;
}

/*==================================================================*/
	  void para_info_to_tag(BNST_DATA *bp, TAG_DATA *tp)
/*==================================================================*/
{
    tp->para_num = bp->para_num;
    tp->para_key_type = bp->para_key_type;
    tp->para_top_p = bp->para_top_p;
    tp->para_type = bp->para_type;
    tp->to_para_p = bp->to_para_p;
}

/*==================================================================*/
	 void para_info_to_mrph(BNST_DATA *bp, MRPH_DATA *mp)
/*==================================================================*/
{
    mp->para_num = bp->para_num;
    mp->para_key_type = bp->para_key_type;
    mp->para_top_p = bp->para_top_p;
    mp->para_type = bp->para_type;
    mp->to_para_p = bp->to_para_p;
}

/*==================================================================*/
    int find_head_tag_from_bnst(BNST_DATA *bp, int target_offset)
/*==================================================================*/
{
    int offset = 0, gov;
    char *cp, *cp2;

    if ((cp = check_feature(bp->f, "タグ単位受")) ||
	(cp = check_feature(bp->f, "直前タグ受"))) {
	if ((cp2 = strchr(cp, ':'))) {
	    offset = atoi(cp2 + 1);
	    if (offset > 0 || bp->tag_num <= -1 * offset) {
		offset = 0;
	    }
	}
    }

    for (gov = bp->tag_num - 1 + offset; gov >= 0; gov--) {
	if ((bp->tag_ptr + gov)->num != -1) {
	    if (target_offset <= 0) {
		break;
	    }
	    else {
		target_offset--;
	    }
	}
    }
    return gov;
}

/*==================================================================*/
	   int find_head_tag_from_dpnd_bnst(BNST_DATA *bp)
/*==================================================================*/
{
    int offset = 0, gov;
    char *cp, *cp2;

    /* 「タグ単位受無視」のときは係り先を最後のタグ単位とする */
    if (!check_feature(bp->f, "タグ単位受無視") && 
	((cp = check_feature(bp->parent->f, "タグ単位受")) ||
	 (cp = check_feature(bp->parent->f, "直前タグ受")))) {
	if ((cp2 = strchr(cp, ':'))) {
	    offset = atoi(cp2 + 1);
	    if (offset > 0 || bp->parent->tag_num <= -1 * offset) {
		offset = 0;
	    }
	}
    }

    for (gov = bp->parent->tag_num - 1 + offset; gov >= 0; gov--) {
	if ((bp->parent->tag_ptr + gov)->num != -1) {
	    break;
	}
    }
    return gov;
}

/*==================================================================*/
MRPH_DATA *find_head_mrph_from_dpnd_bnst(BNST_DATA *dep_ptr, BNST_DATA *gov_ptr)
/*==================================================================*/
{
    BNST_DATA *bp;

    /* 係り先に判定詞があり、係り元が連用なら、係り先形態素を主辞名詞ではなく判定詞にする */
    if (dep_ptr && 
	gov_ptr->head_ptr + 1 <= gov_ptr->mrph_ptr + gov_ptr->mrph_num - 1 && /* 主辞形態素の次の形態素が存在 */
	check_feature(gov_ptr->f, "用言:判") && /* 判定詞文節 */
	!strcmp(Class[(gov_ptr->head_ptr + 1)->Hinshi][0].id, "判定詞") && /* 次の形態素が判定詞 */
	!(check_feature(dep_ptr->f, "連体修飾") || 
	  check_feature(dep_ptr->f, "係:隣") || 
	  check_feature(dep_ptr->f, "係:文節内") || 
	  (dep_ptr->dpnd_type == 'P' && check_feature(dep_ptr->f, "並キ:名")))) {
//	!(dep_ptr->para_type == PARA_NIL || /* 並列のときは最後から2番目の要素のみ修正 */
//	  ((bp = (BNST_DATA *)search_nearest_para_child((TAG_DATA *)dep_ptr->parent)) && dep_ptr->num == bp->num))) {
	return gov_ptr->head_ptr + 1;
    }
    /* 係り元が裸の数量で、係り先にカウンタがあるなら、係り先形態素を主辞名詞ではなくその前の数詞にする 
     「１〜３個」など */
    else if (dep_ptr && 
             gov_ptr->head_ptr - 1 >= gov_ptr->mrph_ptr && /* 主辞形態素の一つ前の形態素が存在 */
             check_feature(dep_ptr->f, "係:文節内") && 
             check_feature(dep_ptr->f, "数量") && 
             !check_feature(dep_ptr->f, "カウンタ") && 
             check_feature(gov_ptr->f, "カウンタ") && 
             check_feature((gov_ptr->head_ptr - 1)->f, "数字")) {
        return gov_ptr->head_ptr - 1;
    }
    else {
	return gov_ptr->head_ptr;
    }
}

/*==================================================================*/
	       int bnst_to_tag_tree(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, offset, last_b_flag = 1, gov, head, gov_head, pre_bp_num;
    char *cp;
    BNST_DATA *bp;
    TAG_DATA *tp;

    /* 文節の木構造からタグ単位の木構造へ変換 */

    init_tag_tree_property(sp);
    sp->New_Tag_num = 0;

    /* new bnst -> tag */
    for (i = sp->New_Bnst_num - 1; i >= 0; i--) { /* <PARA>(1)-<PARA>(2) のときのために後からする */
	bp = sp->bnst_data + sp->Bnst_num + i;

	/* new領域にcopy */

	if ((head = find_head_tag_from_bnst(bp, 0)) < 0) { /* 主辞基本句 */
	    head = bp->tag_num - 1;
	}
	*(sp->tag_data + sp->Tag_num + sp->New_Tag_num) = *(bp->tag_ptr + head);
	sp->New_Tag_num++;

	tp = sp->tag_data + sp->Tag_num + sp->New_Tag_num - 1; /* New領域にコピーした主辞基本句へのポインタ */

	para_info_to_tag(bp, tp);
	tp->child[0] = NULL;

	/* <PARA>のときはheadのみ */
	if (bp->para_top_p == FALSE) {
	    /* 文節内の主辞基本句より前側 */
	    if (head > 0 && (pre_bp_num = find_head_tag_from_bnst(bp, 1)) >= 0) {
		/* 文節内タグ単位の親が <P>(-<PARA>) のとき */
		(bp->tag_ptr + pre_bp_num)->parent = tp; /* 主辞のひとつ前 -> 主辞 */
		t_add_node((BNST_DATA *)tp, 
			   (BNST_DATA *)(bp->tag_ptr + pre_bp_num), -1);

		/* 文節内 */
		for (j = 0; j < pre_bp_num; j++) {
		    for (gov = j + 1; gov <= pre_bp_num; gov++) {
			if ((bp->tag_ptr + gov)->num != -1) {
			    break;
			}
		    }
		    if (gov > pre_bp_num || /* 後処理で係り先がなくなった基本句 */
			(bp->tag_ptr + j)->num == -1) { /* 後処理でマージされた基本句 */
			continue;
		    }
		    (bp->tag_ptr + j)->parent = bp->tag_ptr + gov;
		    t_add_node((BNST_DATA *)(bp->tag_ptr + gov), 
			       (BNST_DATA *)(bp->tag_ptr + j), -1);
		}
		/* 主辞基本句は bp->tag_ptr からはたどれない (Newの方) */
	    }
	}

	/* 親と子のリンクつけ (new) */
	gov_head = find_head_tag_from_dpnd_bnst(bp); /* 係り先の主辞基本句 */
	tp->parent = bp->parent->tag_ptr + gov_head; /* PARAへ */
	t_add_node((BNST_DATA *)(bp->parent->tag_ptr + gov_head), 
		   (BNST_DATA *)tp, -1);

	/* 文節内の主辞基本句より後 (PARAから残りの基本句へ) */
	if (bp->parent < sp->bnst_data + sp->Bnst_num) { /* 親がNewのときはすでに設定している */
	    tp = bp->parent->tag_ptr + gov_head;
	    for (j = head + 1; j < bp->tag_num; j++) {
		if ((bp->tag_ptr + j)->num == -1) {
		    continue;
		}
		tp->parent = bp->tag_ptr + j;
		t_add_node((BNST_DATA *)(bp->tag_ptr + j), 
			   (BNST_DATA *)tp, -1);
		tp = bp->tag_ptr + j;
	    }
	    tp->parent = NULL; /* 係り先未定のマーク */
	}

	/* PARAまたは基本句1つのときは、tag_ptrをNew側にしておく */
	if (1 || bp->para_top_p == TRUE || bp->tag_num == 1) {
	    bp->tag_ptr = sp->tag_data + sp->Tag_num + sp->New_Tag_num - 1;
	    bp->tag_num = 1;
	}
    }

    /* orig */
    for (i = sp->Bnst_num - 1; i >= 0; i--) {
	bp = sp->bnst_data + i;
	if (bp->num == -1) { /* 後処理でマージされた文節 */
	    continue;
	}

	if ((head = find_head_tag_from_bnst(bp, 0)) < 0) { /* 主辞基本句 */
	    head = bp->tag_num - 1;
	}
	para_info_to_tag(bp, bp->tag_ptr + head);

	/* <PARA>のときはheadのみだが、tag_ptr, tag_numの変更はしない */
	if (bp->para_top_p == FALSE) {
	    /* 文節内 */
	    for (j = 0; j < bp->tag_num - 1; j++) {
		for (gov = j + 1; gov < bp->tag_num; gov++) {
		    if ((bp->tag_ptr + gov)->num != -1) {
			break;
		    }
		}
		if (gov >= bp->tag_num || /* 後処理で係り先がなくなった基本句 */
		    (bp->tag_ptr + j)->num == -1) { /* 後処理でマージされた基本句 */
		    continue;
		}
		(bp->tag_ptr + j)->parent = bp->tag_ptr + gov;
		t_add_node((BNST_DATA *)(bp->tag_ptr + gov), 
			   (BNST_DATA *)(bp->tag_ptr + j), -1);
	    }
	}

	if (last_b_flag) { /* 最後の文節 (後処理があるので i == Bnst_num - 1 とは限らない) */
	    last_b_flag = 0;
	    continue;
	}

	/* 親と子 */
	if (bp->parent) {
	    for (head = bp->tag_num - 1; head >= 0; head--) { /* 最後の基本句をさがす */
		if ((bp->tag_ptr + head)->num != -1) {
		    break;
		}
	    }
	    tp = bp->tag_ptr + head;
	    if (bp->para_top_p == TRUE) { /* PARAの場合はnewの方で少し処理している場合がある */
		while (tp->parent) {
		    tp = tp->parent;
		}
	    }

	    offset = find_head_tag_from_dpnd_bnst(bp); /* タグ単位内の係り先をルールから得る */
	    tp->parent = bp->parent->tag_ptr + offset;
	    t_add_node((BNST_DATA *)(bp->parent->tag_ptr + offset), 
		       (BNST_DATA *)tp, -1);
	}
	else {
	    if (Language != CHINESE) {
		fprintf(stderr, ";; %s(%d)'s parent doesn't exist!\n", bp->Jiritu_Go, i);
	    }
	}
    }
}

/*==================================================================*/
	       int bnst_to_mrph_tree(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, offset, last_b_flag = 1, gov, head, gov_head, pre_bp_num;
    char *cp;
    BNST_DATA *bp;
    MRPH_DATA *mp, *tmp_mp, *head_ptr;

    /* 文節の木構造から形態素の木構造へ変換 */

    init_mrph_tree_property(sp);
    sp->New_Mrph_num = 0;

    /* new bnst -> tag */
    for (i = sp->New_Bnst_num - 1; i >= 0; i--) { /* <PARA>(1)-<PARA>(2) のときのために後からする */
	bp = sp->bnst_data + sp->Bnst_num + i;
	// head_ptr = bp->mrph_ptr + bp->mrph_num - 1; // bp->head_ptr; /* ★主辞形態素★ */
	head_ptr = find_head_mrph_from_dpnd_bnst(NULL, bp); /* 主辞形態素 */

	/* new領域にcopy */

	*(sp->mrph_data + sp->Mrph_num + sp->New_Mrph_num) = *head_ptr; /* 主辞形態素 */
	sp->New_Mrph_num++;
	mp = sp->mrph_data + sp->Mrph_num + sp->New_Mrph_num - 1; /* New領域にコピーした主辞形態素へのポインタ */

	para_info_to_mrph(bp, mp);
	mp->child[0] = NULL;

	/* <PARA>のときはheadのみ */
	if (bp->para_top_p == FALSE) {
	    /* 文節内の主辞形態素より前側 */
	    if (head_ptr > bp->mrph_ptr) {
		/* 文節内形態素の親が <P>(-<PARA>) のとき */
		(head_ptr - 1)->parent = (BNST_DATA *)mp; /* 主辞のひとつ前 -> 主辞 */
		t_add_node((BNST_DATA *)mp, 
			   (BNST_DATA *)(head_ptr - 1), -1);

		/* 文節内 */
		for (tmp_mp = head_ptr - 2; tmp_mp >= bp->mrph_ptr; tmp_mp--) {
		    tmp_mp->parent = (BNST_DATA *)(tmp_mp + 1);
		    t_add_node((BNST_DATA *)(tmp_mp + 1), 
			       (BNST_DATA *)tmp_mp, -1);
		}
	    }
	}

	/* 親と子のリンクつけ (new) */
	mp->parent = (BNST_DATA *)find_head_mrph_from_dpnd_bnst(bp, bp->parent);  /* 係り先の主辞形態素 (PARAへ) */
	t_add_node((BNST_DATA *)(mp->parent), 
		   (BNST_DATA *)mp, -1);

	/* 文節内の主辞形態素より後 (PARAから残りの基本句へ) */
	if (bp->parent < sp->bnst_data + sp->Bnst_num) { /* 親がNewのときはすでに設定している */
	    mp = (MRPH_DATA *)mp->parent; /* PARA */
	    for (tmp_mp = head_ptr + 1; tmp_mp < bp->mrph_ptr + bp->mrph_num; tmp_mp++) {
		mp->parent = (BNST_DATA *)tmp_mp;
		t_add_node((BNST_DATA *)(tmp_mp), 
			   (BNST_DATA *)mp, -1);
		mp = tmp_mp;
	    }
	    mp->parent = NULL; /* 係り先未定のマーク */
	}

	/* mrph_ptrをNew側にしておく */
	bp->mrph_ptr = sp->mrph_data + sp->Mrph_num + sp->New_Mrph_num - 1;
	bp->head_ptr = bp->mrph_ptr;
	bp->mrph_num = 1;
    }

    /* orig */
    for (i = sp->Bnst_num - 1; i >= 0; i--) {
	bp = sp->bnst_data + i;
	if (bp->num == -1) { /* 後処理でマージされた文節 */
	    continue;
	}

	if (bp->para_type != PARA_NIL) {
	    head_ptr = bp->mrph_ptr + bp->mrph_num - 1;
	}
	else {
	    head_ptr = find_head_mrph_from_dpnd_bnst(NULL, bp); /* 主辞形態素 */
	}
	para_info_to_mrph(bp, head_ptr);

	/* <PARA>のときはheadのみだが、tag_ptr, tag_numの変更はしない */
	if (bp->para_top_p == FALSE) {
	    /* 文節内 */
	    for (tmp_mp = bp->mrph_ptr + bp->mrph_num - 2; tmp_mp >= bp->mrph_ptr; tmp_mp--) { /* 最終形態素の1つ前以前 */
		tmp_mp->parent = (BNST_DATA *)(tmp_mp + 1);
		t_add_node((BNST_DATA *)(tmp_mp + 1), 
			   (BNST_DATA *)tmp_mp, -1);
	    }
	}

	if (last_b_flag) { /* 最後の文節 (後処理があるので i == Bnst_num - 1 とは限らない) */
	    last_b_flag = 0;
	    continue;
	}

	/* 親と子 */
	if (bp->parent) {
	    mp = bp->mrph_ptr + bp->mrph_num - 1; /* 係り元: 最終形態素 */
	    if (bp->para_top_p == TRUE) { /* PARAの場合はnewの方で少し処理している場合がある */
		while (mp->parent) {
		    mp = (MRPH_DATA*)(mp->parent);
		}
	    }

	    mp->parent = (BNST_DATA *)find_head_mrph_from_dpnd_bnst(bp, bp->parent); /* タグ単位内の係り先をルールから得る */
	    t_add_node((BNST_DATA *)(mp->parent), 
		       (BNST_DATA *)mp, -1);
	}
	else {
	    if (Language != CHINESE) {
		fprintf(stderr, ";; %s(%d)'s parent doesn't exist!\n", bp->Jiritu_Go, i);
	    }
	}
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			      木構造処理

                                               S.Kurohashi 92.10.17
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
		    void init_bnst_tree_property()
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
			int make_simple_tree()
/*==================================================================*/
{
    int i, j, k, child_num;
    int buffer[BNST_MAX];

    /* dpnd.head[i]をbuffer[i]にコピーし，3つ以上からなる並列構造では
       係先を隣のheadから末尾のheadに変更する．
       また部分並列の係り先を末尾のheadに変更する．*/

    for (i = 0; i < sp->Bnst_num - 1; i++)
	buffer[i] = sp->bnst_data[i].dpnd_head;

    for (i = 0; i < Para_M_num; i++) {
	for (j = 0; j < sp->para_manager[i].part_num - 1; j++) {
	    buffer[sp->para_manager[i].end[j]] = 
		sp->para_manager[i].end[sp->para_manager[i].part_num - 1];
	    for (k = sp->para_manager[i].start[j];
		 k <= sp->para_manager[i].end[j]; k++)
		if (Mask_matrix[k][sp->para_manager[i].end[j]] == 3) 
		    buffer[k] = 
			sp->para_manager[i].end[sp->para_manager[i].part_num - 1];
	}
    }

    /* 依存構造木構造リンク付け */

    for (j = sp->Bnst_num - 1; j >= 0; j--) {
	child_num = 0;
	for (i = j - 1; i >= 0; i--)
	  if (buffer[i] == j) {
	      sp->bnst_data[j].child[child_num++] = sp->bnst_data + i;
	      if (child_num > T_CHILD_MAX) return FALSE;
	      sp->bnst_data[i].parent = sp->bnst_data + j;
	      if (Mask_matrix[i][j] == 3) {
		  sp->bnst_data[i].para_type = PARA_INCOMP;
	      }
	      /* PARA_NORMALは展開時にセット */
	  }
	sp->bnst_data[j].child[child_num] = NULL;
    }
    
    return TRUE;
}

/*==================================================================*/
  BNST_DATA *strong_corr_node(PARA_DATA *p_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i;

    for (i = p_ptr->R - p_ptr->L_B - 1; i >= 0; i--) {
	if (sp->bnst_data + p_ptr->max_path[i] == b_ptr)
	  return sp->bnst_data + p_ptr->L_B + i + 1;
    }
    return NULL;
}

/*==================================================================*/;
		void strong_para_expand(PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    int i, j, k;
    PARA_DATA *p_ptr, *pp_ptr;
    BNST_DATA *start_b_ptr, *b_ptr, *bb_ptr;

    /* 強並列内に係る文節を展開 : コピー無 */

    for (i = 0; i < m_ptr->child_num; i++)
      strong_para_expand(m_ptr->child[i]);


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
		  if ((bb_ptr = strong_corr_node(pp_ptr, bb_ptr)))
		    t_add_node(bb_ptr, b_ptr->child[j], -1);
	    }
    }
}

/*==================================================================*/;
		void para_top_expand(PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    int i, j;
    BNST_DATA *new_ptr, *end_ptr, *pre_end_ptr;

    /* 並列をまとめるノードの挿入

       B  B<P> B  B<P>(end_ptr) B  B ｜ .... B(new_ptr)
                ↑                  (↑ここまでが通常の文節)
                ｜
                B(new_ptr) をここに挿入し B<P>(end_ptr)の内容をコピー
		B<P>(end_ptr) は PARA(並列をまとめるノード)となる
    */

    for (i = 0; i < m_ptr->child_num; i++)
      para_top_expand(m_ptr->child[i]);

    end_ptr = sp->bnst_data + m_ptr->end[m_ptr->part_num - 1];
    pre_end_ptr = sp->bnst_data + m_ptr->end[m_ptr->part_num - 2];
	
    new_ptr = sp->bnst_data + sp->Bnst_num + sp->New_Bnst_num;
    sp->New_Bnst_num++;
    if ((sp->Bnst_num + sp->New_Bnst_num) > BNST_MAX) {
	fprintf(stderr, "Too many nodes in expanding para top .\n");
	exit(1);
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
      sp->bnst_data[m_ptr->end[i]].para_type = PARA_NORMAL;
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
	     void incomplete_para_expand(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, j, para_pos;
    int new_num, child_num;
    int find_type = FALSE, incomp_type = -1;
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
			  "Too many nodes in expanding incomplete para .\n");
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
      incomplete_para_expand(b_ptr->child[i]);
}

/*==================================================================*/
			 int make_dpnd_tree()
/*==================================================================*/
{
    int i;

    init_bnst_tree_property();

    sp->New_Bnst_num = 0;    				/* 初期化 */

    if (make_simple_tree() == FALSE)			/* リンク付け */
	return FALSE;
	
    if (OptExpandP == TRUE) 
	for (i = 0; i < Para_M_num; i++)		/* 強並列の展開 */
	    if (sp->para_manager[i].parent == NULL)
		strong_para_expand(sp->para_manager + i);    
    for (i = 0; i < Para_M_num; i++) 			/* PARAの展開 */
      if (sp->para_manager[i].parent == NULL)
	para_top_expand(sp->para_manager + i);    
    if (OptExpandP == TRUE) 
      para_modifier_expand(sp->bnst_data + sp->Bnst_num - 1);	/* PARA修飾の展開 */
    /*
    incomplete_para_expand(sp->bnst_data + sp->Bnst_num - 1);*/	/* 部分並列の展開 */

    return TRUE;
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			   並列構造間の関係

                                               S.Kurohashi 91.10.17
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int 	para_rel_matrix[PARA_MAX][PARA_MAX];

static char *RESULT[] = {
    "重なりなし", "少し重なる", "前で重なる", "後で重なる",  "重複",
    "前部の修正", "含まれる前", "含まれる後", "誤り"};

static int rel_matrix_normal[4][4] = {
    {REL_BIT, REL_POS, REL_BAD, REL_IN2},
    {REL_PRE, REL_PAR, REL_BAD, REL_IN2},
    {REL_REV, REL_REV, REL_BAD, REL_BAD},
    {REL_IN1, REL_IN1, REL_BAD, REL_BAD}
};
static int rel_matrix_strong[4][4] = {
    {REL_BAD, REL_POS, REL_BAD, REL_IN2}, /* (0,0) -> BAD */
    {REL_PRE, REL_PAR, REL_BAD, REL_IN2},
    {REL_REV, REL_REV, REL_BAD, REL_BAD},
    {REL_IN1, REL_IN1, REL_BAD, REL_BAD}
};

/* 
   strongの(0,1)はBADにしていたが，POSでよい例文があったので修正した．

   例文) ラジさん自身は英国籍を持つが、インド系の二万五千人のうち約五、
   六千人はインド国籍も英国籍もなく、香港住民としての資格しかない。
*/

/*==================================================================*/
	   void print_two_para_relation(int p_num1, int p_num2)
/*==================================================================*/
{
    /* 並列構造間の関係の表示 */

    int a1, a2, a3, b1, b2, b3;
    PARA_DATA *ptr1, *ptr2;
    
    ptr1 = &(para_data[p_num1]);
    a1 = ptr1->max_path[0];
    a2 = ptr1->L_B;
    a3 = ptr1->R;

    ptr2 = &(para_data[p_num2]);
    b1 = ptr2->max_path[0];
    b2 = ptr2->L_B;
    b3 = ptr2->R;

    fprintf(stdout, "%-10s ==> ", RESULT[para_rel_matrix[p_num1][p_num2]]);

    if (a1 != a2)
      print_bnst(&(bnst_data[a1]), NULL);
    fputc('(', stdout);
    print_bnst(&(bnst_data[a2]), NULL);
    fputc(')', stdout);
    print_bnst(&(bnst_data[a3]), NULL);

    fprintf(stdout, " <=> ");

    if (b1 != b2)
      print_bnst(&(bnst_data[b1]), NULL);
    fputc('(', stdout);
    print_bnst(&(bnst_data[b2]), NULL);
    fputc(')', stdout);
    print_bnst(&(bnst_data[b3]), NULL);
    fputc('\n', stdout);
}

/*==================================================================*/
		       void init_para_manager()
/*==================================================================*/
{
    int i, j;

    Para_M_num = 0;

    for (i = 0; i <Para_num; i++) {
	para_manager[i].para_num = 0;
	para_manager[i].part_num = 0;
	para_manager[i].parent = NULL;
	para_manager[i].child_num = 0;

	para_data[i].manager_ptr = NULL;
    }
}

/*==================================================================*/
	     int para_location(int pre_num, int pos_num)
/*==================================================================*/
{
    /* 並列構造間の関係の決定 */

    int a1, a2, a3, b1, b2, b3;
    int rel_pre, rel_pos;

    a1 = para_data[pre_num].max_path[0];
    a2 = para_data[pre_num].L_B;
    a3 = para_data[pre_num].R;
    b1 = para_data[pos_num].max_path[0];
    b2 = para_data[pos_num].L_B;
    b3 = para_data[pos_num].R;

    if (a3 < b1) return REL_NOT;
    
    if ((a2+1) < b1)		rel_pre = 0;
    else if ((a2+1) == b1) 	rel_pre = 1;
    else if (a1 < b1)  		rel_pre = 2;
    else                 	rel_pre = 3;

    if (a3 < b2)       		rel_pos = 0;
    else if (a3 == b2) 		rel_pos = 1;
    else if (a3 < b3)  		rel_pos = 2;
    else                 	rel_pos = 3;

    if (para_data[pos_num].status == 's') 
      return rel_matrix_strong[rel_pre][rel_pos];
    else
      return rel_matrix_normal[rel_pre][rel_pos];
}

/*==================================================================*/
	     int para_brother_p(int pre_num, int pos_num)
/*==================================================================*/
{
    /* REL_POS -> REL_PAR に変換する条件 */

    int pre_length, pos_length;

    pre_length = para_data[pre_num].R - para_data[pre_num].L_B;
    pos_length = para_data[pos_num].L_B - para_data[pos_num].max_path[0] + 1;
    
    if (pre_length * 3 > pos_length * 4) return FALSE;
    else return TRUE;
}

/*==================================================================*/
 void delete_child(PARA_MANAGER *parent_ptr, PARA_MANAGER *child_ptr)
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < parent_ptr->child_num; i++)
      if (parent_ptr->child[i] == child_ptr) {
	  for (j = i; j < parent_ptr->child_num - 1; j++)
	    parent_ptr->child[j] = parent_ptr->child[j+1];
	  parent_ptr->child_num -= 1;
	  break;
      }
}

/*==================================================================*/
  void set_parent(PARA_MANAGER *parent_ptr, PARA_MANAGER *child_ptr)
/*==================================================================*/
{
    int i, j, i_num, j_num;

    if (child_ptr->parent) {
	if (child_ptr->parent == parent_ptr) return;

	for (i = 0; i < child_ptr->parent->para_num; i++) {
	    i_num = child_ptr->parent->para_data_num[i];
	    for (j = 0; j < parent_ptr->para_num; j++) {
		j_num = parent_ptr->para_data_num[j];

		/* 元の親が直接の親 */

		if ((i_num < j_num &&
		     (para_rel_matrix[i_num][j_num] == REL_BIT ||
		      para_rel_matrix[i_num][j_num] == REL_PRE ||
		      para_rel_matrix[i_num][j_num] == REL_REV ||
		      para_rel_matrix[i_num][j_num] == REL_IN1)) ||
		    (j_num < i_num &&
		     (para_rel_matrix[j_num][i_num] == REL_POS ||
		      para_rel_matrix[j_num][i_num] == REL_IN2)))
		  return;

		/* 新しい親が直接の親 */

		else if ((i_num < j_num &&
			  (para_rel_matrix[i_num][j_num] == REL_POS ||
			   para_rel_matrix[i_num][j_num] == REL_IN2)) ||
			 (j_num < i_num &&
			  (para_rel_matrix[j_num][i_num] == REL_BIT ||
			   para_rel_matrix[j_num][i_num] == REL_PRE ||
			   para_rel_matrix[j_num][i_num] == REL_REV ||
			   para_rel_matrix[j_num][i_num] == REL_IN1))) {
		    delete_child(child_ptr->parent, child_ptr);
		    child_ptr->parent = parent_ptr;
		    parent_ptr->child[parent_ptr->child_num++] = child_ptr;
		    if (parent_ptr->child_num >= PARA_PART_MAX) {
			fprintf(stderr, "Too many para (%s)!\n", Comment);
			exit(1);
		    }
		    return;
		}
	    }
	}

	/* 元の親と新しい親に関係がない */
	fprintf(stderr, "Invalid relation !!\n");
	
    } else {
	child_ptr->parent = parent_ptr;
	parent_ptr->child[parent_ptr->child_num++] = child_ptr;
	if (parent_ptr->child_num >= PARA_PART_MAX) {
	    fprintf(stderr, "Too many para (%s)!\n", Comment);
	    exit(1);
	}
    }
}

/*==================================================================*/
	      void para_revice_scope(PARA_MANAGER *ptr)
/*==================================================================*/
{
    int i;
    PARA_MANAGER *child_ptr;

    if (ptr->child_num) {

	/* 子供の処理 */

	for (i = 0; i < ptr->child_num; i++)
	  para_revice_scope(ptr->child[i]);


	/* 左側の修正 */

	if (ptr->child[0]->start[0] < ptr->start[0])
	    ptr->start[0] = ptr->child[0]->start[0];
	

	/* 右側の修正 */
	
	child_ptr = ptr->child[ptr->child_num-1];
	if (ptr->end[ptr->part_num-1] < child_ptr->end[child_ptr->part_num-1])
	  ptr->end[ptr->part_num-1] = child_ptr->end[child_ptr->part_num-1];
    }
}

/*==================================================================*/
		      int detect_para_relation()
/*==================================================================*/
{
    /* 並列構造間の関係の整理 */

    int i, j, flag;
    int a1, a2, a3, b1, b2, b3;
    PARA_MANAGER *m_ptr, *m_ptr1, *m_ptr2;

    /* 位置関係の決定，誤りの修正 */

    for (i = 0; i < Para_num; i++) {
	if (para_data[i].status == 'x') continue;
        for (j = i+1; j < Para_num; j++) {
	    if (para_data[j].status == 'x') continue;
	    if ((para_rel_matrix[i][j] = para_location(i, j)) == REL_BAD) {
		if (OptDisplay == OPT_DEBUG)
		  print_two_para_relation(i, j);
		revise_para_rel(i, j);
		return FALSE;
	    }
	}
    }

    init_para_manager();

    /* 兄弟関係のまとめ，MANAGERによる管理 */

    for (i = 0; i < Para_num; i++) {
	if (para_data[i].status == 'x') continue;
	if (para_data[i].manager_ptr) {
	    m_ptr = para_data[i].manager_ptr;
	} else {
	    m_ptr = &para_manager[Para_M_num++];
	    para_data[i].manager_ptr = m_ptr;
	    m_ptr->para_data_num[m_ptr->para_num++] = i;
	    if (m_ptr->para_num >= PARA_PART_MAX) {
		fprintf(stderr, "Too many para (%s)!\n", Comment);
		exit(1);
	    }
	    m_ptr->start[m_ptr->part_num] = para_data[i].max_path[0];
	    m_ptr->end[m_ptr->part_num++] = para_data[i].L_B;
	    m_ptr->start[m_ptr->part_num] = para_data[i].L_B+1;
	    m_ptr->end[m_ptr->part_num++] = para_data[i].R;
	}	  
        for (j = i+1; j < Para_num; j++) {
	    if (para_data[j].status == 'x') continue;
	    switch (para_rel_matrix[i][j]) {
	      case REL_PAR:
		para_data[j].manager_ptr = m_ptr;
		m_ptr->para_data_num[m_ptr->para_num++] = j;
		if (m_ptr->para_num >= PARA_PART_MAX) {
		    fprintf(stderr, "Too many para (%s)!\n", Comment);
		    exit(1);
		}
		m_ptr->start[m_ptr->part_num] = para_data[j].L_B+1;
		m_ptr->end[m_ptr->part_num++] = para_data[j].R;
		break;
	      case REL_POS: 
		if (para_brother_p(i, j) == TRUE) {
 		    para_rel_matrix[i][j] = REL_PAR;
		    para_data[j].manager_ptr = m_ptr;
		    m_ptr->para_data_num[m_ptr->para_num++] = j;
		    if (m_ptr->para_num >= PARA_PART_MAX) {
			fprintf(stderr, "Too many para (%s)!\n", Comment);
			exit(1);
		    }
		    m_ptr->start[m_ptr->part_num] = para_data[j].L_B+1;
		    m_ptr->end[m_ptr->part_num++] = para_data[j].R;
		}
		break;
	      default:
		break;
	    }
	}
    }
    
    /* 親子関係のまとめ m_ptr1が子，m_ptr2が親の時に処理 */

    for (i = 0; i < Para_num; i++) {
	if (para_data[i].status == 'x') continue;
	m_ptr1 = para_data[i].manager_ptr;
        for (j = 0; j < Para_num; j++) {
	    if (para_data[j].status == 'x') continue;
	    m_ptr2 = para_data[j].manager_ptr;
	    if ((i < j &&
		 (para_rel_matrix[i][j] == REL_BIT ||
		  para_rel_matrix[i][j] == REL_PRE ||
		  para_rel_matrix[i][j] == REL_REV ||
		  para_rel_matrix[i][j] == REL_IN1)) ||
		(j < i &&
		 (para_rel_matrix[j][i] == REL_POS ||
		  para_rel_matrix[j][i] == REL_IN2)))
	      set_parent(m_ptr2, m_ptr1);
	}
    }

    /* 範囲の修正 */

    for (i = 0; i < Para_M_num; i++)
      if (para_manager[i].parent == NULL)
	para_revice_scope(&para_manager[i]);    

    /* 強並列のマーク */

    for (i = 0; i < Para_M_num; i++) {
	flag = TRUE;
	for (j = 0; j < para_manager[i].para_num; j++) {
	    if (para_data[para_manager[i].para_data_num[j]].status != 's') {
		flag = FALSE;
		break;
	    }
	}
	para_manager[i].status = (flag == TRUE) ? 's' : 'w';
    }

    return TRUE;
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			     出力ルーチン

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

#define PREFIX_MARK "T_P"
#define CONWRD_MARK "T_C"
#define SUFFIX_MARK "T_S"

extern char *check_feature();

char pos2symbol(char *hinshi, char *bunrui)
{
    if (!strcmp(hinshi, "特殊")) return ' ';
    else if (!strcmp(hinshi, "動詞")) return 'v';
    else if (!strcmp(hinshi, "形容詞")) return 'j';
    else if (!strcmp(hinshi, "判定詞")) return 'c';
    else if (!strcmp(hinshi, "助動詞")) return 'x';
    else if (!strcmp(hinshi, "名詞") &&
	     !strcmp(bunrui, "固有名詞")) return 'N';
    else if (!strcmp(hinshi, "名詞") &&
	     !strcmp(bunrui, "人名")) return 'J';
    else if (!strcmp(hinshi, "名詞") &&
	     !strcmp(bunrui, "地名")) return 'C';
    else if (!strcmp(hinshi, "名詞")) return 'n';
    else if (!strcmp(hinshi, "指示詞")) return 'd';
    else if (!strcmp(hinshi, "副詞")) return 'a';
    else if (!strcmp(hinshi, "助詞")) return 'p';
    else if (!strcmp(hinshi, "接続詞")) return 'c';
    else if (!strcmp(hinshi, "連体詞")) return 'm';
    else if (!strcmp(hinshi, "感動詞")) return '!';
    else if (!strcmp(hinshi, "接頭辞")) return 'p';
    else if (!strcmp(hinshi, "接尾辞")) return 's';

    return '?';
}

/*==================================================================*/
		  void print_mrph(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    fprintf(stdout, "%s %s %s ", m_ptr->Goi2, m_ptr->Yomi, m_ptr->Goi);
    
    fprintf(stdout, "%s ", Class[m_ptr->Hinshi][0].id);
    fprintf(stdout, "%d ", m_ptr->Hinshi);
	
    if (m_ptr->Bunrui) 
	fprintf(stdout, "%s ", Class[m_ptr->Hinshi][m_ptr->Bunrui].id);
    else
	fprintf(stdout, "* ");
    fprintf(stdout, "%d ", m_ptr->Bunrui);
	
    if (m_ptr->Katuyou_Kata) 
	fprintf(stdout, "%s ", Type[m_ptr->Katuyou_Kata].name);
    else                    
	fprintf(stdout, "* ");
    fprintf(stdout, "%d ", m_ptr->Katuyou_Kata);
    
    if (m_ptr->Katuyou_Kei) 
	fprintf(stdout, "%s ", 
		Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].name);
    else 
	fprintf(stdout, "* ");
    fprintf(stdout, "%d ", m_ptr->Katuyou_Kei);
    
    fprintf(stdout, "%s", m_ptr->Imi);
}

/*==================================================================*/
		  void print_mrph_f(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char yomi_buffer[256];

    sprintf(yomi_buffer, "(%s)", m_ptr->Yomi);
    fprintf(stdout, "%-16.16s%-18.18s %-14.14s",
	    m_ptr->Goi2, yomi_buffer, 
	    Class[m_ptr->Hinshi][m_ptr->Bunrui].id);
    if (m_ptr->Katuyou_Kata)
	fprintf(stdout, " %-14.14s %-12.12s",
		Type[m_ptr->Katuyou_Kata].name,
		Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].name);
}

/*==================================================================*/
		      void print_mrphs(int flag)
/*==================================================================*/
{
    int		i, j;
    MRPH_DATA	*m_ptr;
    BNST_DATA	*b_ptr;

    for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++) {
	if (flag == 1) {
	    fprintf(stdout, "* %d%c", b_ptr->dpnd_head, b_ptr->dpnd_type);
	    if (b_ptr->f) {
		fprintf(stdout, " ");
		print_feature(b_ptr->f, stdout);
	    }
	    fprintf(stdout, "\n");
	}
	else {
	    fprintf(stdout, "*\n");
	}

	for (j = 0, m_ptr = b_ptr->mrph_ptr; j < b_ptr->mrph_num; j++, m_ptr++) {
	    print_mrph(m_ptr);
	    if (m_ptr->f) {
		fprintf(stdout, " ");
		print_feature(m_ptr->f, stdout);
	    }
	    /* print_mrph_f(m_ptr); */
	    fprintf(stdout, "\n");
	}
    }
    fprintf(stdout, "EOS\n");
}

/*==================================================================*/
		   void _print_bnst(BNST_DATA *ptr)
/*==================================================================*/
{
    int i;
    for (i = 0; i < ptr->mrph_num; i++)
	fprintf(stdout, "%s", (ptr->mrph_ptr + i)->Goi2);
}

/*==================================================================*/
		void print_bnst(BNST_DATA *ptr, char *cp)
/*==================================================================*/
{
    int i;

    if ((int)cp > 0 && ptr) {
	if ( ptr->para_type == PARA_NORMAL ) strcpy(cp, "<P>");
	else if ( ptr->para_type == PARA_INCOMP ) strcpy(cp, "<I>");
	else			     cp[0] = '\0';
	if ( ptr->para_top_p == TRUE )
	  strcat(cp, "PARA");
	else {
	    strcpy(cp, ptr->mrph_ptr->Goi2);
	    for (i = 1; i < ptr->mrph_num; i++) 
	      strcat(cp, (ptr->mrph_ptr + i)->Goi2);
	}
    } else if (cp == NULL && ptr) {
	if ( ptr->para_top_p == TRUE ) {
	    fprintf(stdout, "PARA");
	} else {
	    for (i = 0; i < ptr->mrph_num; i++) {
		if (OptDisplay == OPT_NORMAL) {
		    fprintf(stdout, "%s", (ptr->mrph_ptr + i)->Goi2);
		} else { 
		    fprintf(stdout, "%s%c", (ptr->mrph_ptr + i)->Goi2, 
			    pos2symbol(Class[(ptr->mrph_ptr + i)->Hinshi]
				       [0].id,
				       Class[(ptr->mrph_ptr + i)->Hinshi]
				       [(ptr->mrph_ptr + i)->Bunrui].id));
		}
	    }
	}

	if ( ptr->para_type == PARA_NORMAL ) fprintf(stdout, "<P>");
	else if ( ptr->para_type == PARA_INCOMP ) fprintf(stdout, "<I>");
	if ( ptr->to_para_p == TRUE ) fprintf(stdout, "(D)");
    }
}

/*==================================================================*/
  void print_data2ipal_corr(BNST_DATA *b_ptr, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, j, elem_num = 0;
    int offset;
    int flag;

    switch (cpm_ptr->cmm[0].cf_ptr->voice) {
      case FRAME_PASSIVE_I:
      case FRAME_CAUSATIVE_WO_NI:
      case FRAME_CAUSATIVE_WO:
      case FRAME_CAUSATIVE_NI:
	offset = 0;
	break;
      default:
	offset = 1;
	break;
    }

    flag = FALSE;
    if (cpm_ptr->elem_b_num[0] == -1) {
	elem_num = 0;
	flag = TRUE;
    }
    if (flag == TRUE) {
	flag = FALSE;
	for (j = 0; j < cpm_ptr->cmm[0].cf_ptr->element_num; j++)
	  if (cpm_ptr->cmm[0].result_lists_p[0].flag[j] == elem_num) {
	      fprintf(stdout, " N%d", offset + j);
	      flag = TRUE;
	  }
    }
    if (flag == FALSE)
      fprintf(stdout, " *");

    for (i = 0; b_ptr->child[i]; i++) {
	flag = FALSE;
	for (j = 0; j < cpm_ptr->cf.element_num; j++)
	  if (cpm_ptr->elem_b_num[j] == i) {
	      elem_num = j;
	      flag = TRUE;
	      break;
	  }
	if (flag == TRUE) {
	    flag = FALSE;
	    for (j = 0; j < cpm_ptr->cmm[0].cf_ptr->element_num; j++)
	      if (cpm_ptr->cmm[0].result_lists_p[0].flag[j] == elem_num) {
		  fprintf(stdout, " N%d", offset + j);
		  flag = TRUE;
	      }
	}
	if (flag == FALSE)
	  fprintf(stdout, " *");
    }
}

/*==================================================================*/
		void print_bnst_detail(BNST_DATA *ptr)
/*==================================================================*/
{
    int i, j;
    MRPH_DATA *m_ptr;
    IPAL_FRAME Ipal_frame;
    IPAL_FRAME *i_ptr = &Ipal_frame;
    char *cp;
     
    fputc('(', stdout);	/* 文節始り */

    if ( ptr->para_top_p == TRUE ) {
	if (ptr->child[1] && 
	    ptr->child[1]->para_key_type == PARA_KEY_N)
	  fprintf(stdout, "noun_para"); 
	else
	  fprintf(stdout, "pred_para");
    }
    else {
	fprintf(stdout, "%d ", ptr->num);

	/* 係り受け情報の表示 (追加:97/10/29) */

	fprintf(stdout, "(type:%c int:%s ext:%s) ",
		ptr->dpnd_type, ptr->dpnd_int, ptr->dpnd_ext);

	fputc('(', stdout);
	for (i=0, m_ptr=ptr->mrph_ptr; i < ptr->mrph_num; i++, m_ptr++) {
	    fputc('(', stdout);
	    print_mrph(m_ptr);
	    fprintf(stdout, " ");
	    print_feature2(m_ptr->f, stdout);
	    fputc(')', stdout);
	}
	fputc(')', stdout);

	fprintf(stdout, " ");
	print_feature2(ptr->f, stdout);

	if (OptAnalysis = OPT_DPND ||
	    !check_feature(ptr->f, "用言") ||	/* 用言でない場合 */
	    ptr->cpm_ptr == NULL) { 		/* 解析前 */
	    fprintf(stdout, " NIL");
	}
	else {
	    fprintf(stdout, " (");
	    
	    if (ptr->cpm_ptr->cmm[0].cf_ptr == NULL)
		fprintf(stdout, "-2");	/* IPALにENTRYなし */
	    else if ((ptr->cpm_ptr->cmm[0].cf_ptr)->ipal_address == -1)
		fprintf(stdout, "-1");	/* 格要素なし */
	    else {
		fprintf(stdout, "%s", 
			(ptr->cpm_ptr->cmm[0].cf_ptr)->ipal_id);
		switch (ptr->cpm_ptr->cmm[0].cf_ptr->voice) {
		case FRAME_ACTIVE:
		    fprintf(stdout, " 能動"); break;
		case FRAME_PASSIVE_I:
		    fprintf(stdout, " 間受"); break;
		case FRAME_PASSIVE_1:
		    fprintf(stdout, " 直受１"); break;
		case FRAME_PASSIVE_2:
		    fprintf(stdout, " 直受２"); break;
		case FRAME_CAUSATIVE_WO_NI:
		    fprintf(stdout, " 使役ヲニ"); break;
		case FRAME_CAUSATIVE_WO:
		    fprintf(stdout, " 使役ヲ"); break;
		case FRAME_CAUSATIVE_NI:
		    fprintf(stdout, " 使役ニ"); break;
		case FRAME_POSSIBLE:
		    fprintf(stdout, " 可能"); break;
		case FRAME_POLITE:
		    fprintf(stdout, " 尊敬"); break;
		case FRAME_SPONTANE:
		    fprintf(stdout, " 自発"); break;
		default: break;
		}
		fprintf(stdout, " (");
		print_data2ipal_corr(ptr, ptr->cpm_ptr);
		fprintf(stdout, ")");
	    }
	    fprintf(stdout, ")");

	    /* ------------変更:述語素, 格形式を出力-----------------
	    if (ptr->cpm_ptr != NULL &&
		ptr->cpm_ptr->cmm[0].cf_ptr != NULL &&
		(ptr->cpm_ptr->cmm[0].cf_ptr)->ipal_address != -1) {
		get_ipal_frame(i_ptr, 
			       (ptr->cpm_ptr->cmm[0].cf_ptr)->ipal_address);
		if (i_ptr->DATA[i_ptr->jyutugoso]) {
		    fprintf(stdout, " 述語素 %s", 
			    i_ptr->DATA+i_ptr->jyutugoso);
		} else {
		    fprintf(stdout, " 述語素 nil");
		}
		fprintf(stdout, " 格形式 (");
		for (j=0; *((i_ptr->DATA)+(i_ptr->kaku_keishiki[j])) 
			       != NULL; j++){
		    fprintf(stdout, " %s", 
			    i_ptr->DATA+i_ptr->kaku_keishiki[j]);
		}
		fprintf(stdout, ")");
	    }
	    ------------------------------------------------------- */
	}
    }
    fputc(')', stdout);	/* 文節終り */    
}

/*==================================================================*/
		    void print_sentence_slim(void)
/*==================================================================*/
{
    int i;

    init_bnst_tree_property();

    fputc('(', stdout);
    for ( i=0; i<Bnst_num; i++ )
      print_bnst(&(bnst_data[i]), NULL);
    fputc(')', stdout);
    fputc('\n', stdout);
}

/*====================================================================
			       行列表示
====================================================================*/

/*==================================================================*/
    void print_M_bnst(int b_num, int max_length, int *para_char)
/*==================================================================*/
{
    BNST_DATA *ptr = &(bnst_data[b_num]);
    int i, len, space, comma_p;
    char tmp[BNST_LENGTH_MAX], *cp = tmp;

    if ( ptr->mrph_num == 1 ) {
	strcpy(tmp, ptr->mrph_ptr->Goi2);
	comma_p = FALSE;
    } else {
	strcpy(tmp, ptr->mrph_ptr->Goi2);
	for (i = 1; i < (ptr->mrph_num - 1); i++) 
	  strcat(tmp, (ptr->mrph_ptr + i)->Goi2);

	if (!strcmp(Class[(ptr->mrph_ptr + ptr->mrph_num - 1)->Hinshi][0].id,
		    "特殊") &&
	    !strcmp(Class[(ptr->mrph_ptr + ptr->mrph_num - 1)->Hinshi]
		    [(ptr->mrph_ptr + ptr->mrph_num - 1)->Bunrui].id, 
		    "読点")) {
	    strcat(tmp, ",");
	    comma_p = TRUE;
	} else {
	    strcat(tmp, (ptr->mrph_ptr + ptr->mrph_num - 1)->Goi2);
	    comma_p = FALSE;	      
	}
    }

    space = ptr->para_key_type ?
      max_length-(Bnst_num-b_num-1)*3-2 : max_length-(Bnst_num-b_num-1)*3;
    len = comma_p ? 
      ptr->length - 1 : ptr->length;

    if ( len > space ) {
	if ( (space%2) != (len%2) ) {
	    cp += len + 1 - space;
	    fputc(' ', stdout);
	} else
	  cp += len - space;
    } else
      for ( i=0; i<space-len; i++ ) fputc(' ', stdout);

    if ( ptr->para_key_type ) {
	fprintf(stdout, "%c>", 'a'+ (*para_char));
	(*para_char)++;
    }
    fprintf(stdout, "%s", cp);
}

/*==================================================================*/
		void print_line(int length, int flag)
/*==================================================================*/
{
    int i;
    for ( i=0; i<(length-1); i++ ) fputc('-', stdout);
    flag ? fputc(')', stdout) : fputc('-', stdout);
    fputc('\n', stdout);
}

/*==================================================================*/
		 void print_matrix(int type, int L_B)
/*==================================================================*/
{
    int i, j, space, length;
    int over_flag = 0;
    int max_length = 0;
    int para_char = 0;     /* para_key の表示用 */
    PARA_DATA *ptr;

    for ( i=0; i<Bnst_num; i++ )
      for ( j=0; j<Bnst_num; j++ )
	path_matrix[i][j] = 0;
    
    /* パスのマーク付け(PARA) */

    if (type == PRINT_PARA) {
	for ( i=0; i<Para_num; i++ ) {
	    ptr = &para_data[i];
	    for ( j=ptr->L_B+1; j<=ptr->R; j++ )
	      path_matrix[ptr->max_path[j-ptr->L_B-1]][j] =
		path_matrix[ptr->max_path[j-ptr->L_B-1]][j] ?
		  -1 : 'a' + i;
	}
    }

    /* 長さの計算 */

    for ( i=0; i<Bnst_num; i++ ) {
	length = bnst_data[i].length + (Bnst_num-i-1)*3;
        if ( bnst_data[i].para_key_type ) length += 2;
	if ( max_length < length )    max_length = length;
    }
    
    /* 印刷用の処理 */

    if ( 0 ) {
	if ( PRINT_WIDTH < Bnst_num*3 ) {
	    over_flag = 1;
	    Bnst_num = PRINT_WIDTH/3;
	    max_length = PRINT_WIDTH;
	} else if ( PRINT_WIDTH < max_length ) {
	    max_length = PRINT_WIDTH;
	}
    }

    if (type == PRINT_PARA)
      fprintf(stdout, "<< PARA MATRIX >>\n");
    else if (type == PRINT_DPND)
      fprintf(stdout, "<< DPND MATRIX >>\n");
    else if (type == PRINT_MASK)
      fprintf(stdout, "<< MASK MATRIX >>\n");
    else if (type == PRINT_QUOTE)
      fprintf(stdout, "<< QUOTE MATRIX >>\n");
    else if (type == PRINT_RSTR)
      fprintf(stdout, "<< RESTRICT MATRIX for PARA RELATION>>\n");
    else if (type == PRINT_RSTD)
      fprintf(stdout, "<< RESTRICT MATRIX for DEPENDENCY STRUCTURE>>\n");
    else if (type == PRINT_RSTQ)
      fprintf(stdout, "<< RESTRICT MATRIX for QUOTE SCOPE>>\n");

    print_line(max_length, over_flag);
    for ( i=0; i<(max_length-Bnst_num*3); i++ ) fputc(' ', stdout);
    for ( i=0; i<Bnst_num; i++ ) fprintf(stdout, "%2d ", i);
    fputc('\n', stdout);
    print_line(max_length, over_flag);

    for ( i=0; i<Bnst_num; i++ ) {
	print_M_bnst(i, max_length, &para_char);
	for ( j=i+1; j<Bnst_num; j++ ) {

	    if (type == PRINT_PARA) {
		fprintf(stdout, "%2d", match_matrix[i][j]);
	    } else if (type == PRINT_DPND) {
		if (Dpnd_matrix[i][j] == 0)
		    fprintf(stdout, " -");
		else
		    fprintf(stdout, " %c", (char)Dpnd_matrix[i][j]);
		
	    } else if (type == PRINT_MASK) {
		fprintf(stdout, "%2d", Mask_matrix[i][j]);

	    } else if (type == PRINT_QUOTE) {
		fprintf(stdout, "%2d", Quote_matrix[i][j]);

	    } else if (type == PRINT_RSTR || 
		       type == PRINT_RSTD ||
		       type == PRINT_RSTQ) {
		if (j <= L_B) 
		    fprintf(stdout, "--");
		else if (L_B < i)
		    fprintf(stdout, " |");
		else
		    fprintf(stdout, "%2d", restrict_matrix[i][j]);
	    }

	    switch(path_matrix[i][j]) {
	      case  0:	fputc(' ', stdout); break;
	      case -1:	fputc('*', stdout); break;
	      default:	fputc(path_matrix[i][j], stdout); break;
	    }
	}
	fputc('\n', stdout);
    }

    print_line(max_length, over_flag);
    
    if (type == PRINT_PARA) {
	for (i = 0; i < Para_num; i++) {
	    fprintf(stdout, "%c(%c):%4.1f(%4.1f) ", 
		    para_data[i].para_char, 
		    para_data[i].status, 
		    para_data[i].max_score,
		    para_data[i].pure_score);
	}
	fputc('\n', stdout);
    }
}

/*====================================================================
	                並列構造間の関係表示
====================================================================*/

/*==================================================================*/
	     void print_para_manager(PARA_MANAGER *m_ptr, int level)
/*==================================================================*/
{
    int i, j;
    
    for (i = 0; i < level * 5; i++)
      fputc(' ', stdout);

    for (i = 0; i < m_ptr->para_num; i++)
      fprintf(stdout, " %c", para_data[m_ptr->para_data_num[i]].para_char);
    fputc(':', stdout);

    for (i = 0; i < m_ptr->part_num; i++) {
	if (m_ptr->start[i] == m_ptr->end[i]) {
	    fputc('(', stdout);
	    print_bnst(&bnst_data[m_ptr->start[i]], NULL);
	    fputc(')', stdout);
	} else {
	    fputc('(', stdout);
	    print_bnst(&bnst_data[m_ptr->start[i]], NULL);
	    fputc('-', stdout);
	    print_bnst(&bnst_data[m_ptr->end[i]], NULL);
	    fputc(')', stdout);
	}
    }
    fputc('\n', stdout);

    for (i = 0; i < m_ptr->child_num; i++)
      print_para_manager(m_ptr->child[i], level+1);
}

/*==================================================================*/
		      void print_para_relation()
/*==================================================================*/
{
    int i;
    
    for (i = 0; i < Para_M_num; i++)
      if (para_manager[i].parent == NULL)
	print_para_manager(&para_manager[i], 0);
}

/*====================================================================
	                木構造表示(from JK)
====================================================================*/
static int max_width;			/* 木の最大幅 */

/*==================================================================*/
			   int mylog(int n)
/*==================================================================*/
{
    int i, num = 1;
    for (i=0; i<n; i++)
      num = num*2;
    return(num);
}

/*==================================================================*/
       void calc_self_space(BNST_DATA *ptr, int depth2)
/*==================================================================*/
{
    if (ptr->para_top_p == TRUE) 
	ptr->space = 4;
    else if (OptDisplay == OPT_NORMAL)
	ptr->space = ptr->length;
    else
	ptr->space = ptr->length + ptr->mrph_num; /* *4 */

    if (ptr->para_type == PARA_NORMAL || 
	ptr->para_type == PARA_INCOMP ||
	ptr->to_para_p == TRUE)
      ptr->space += 1;
    ptr->space += (depth2-1)*8;
}

/*==================================================================*/
       void calc_tree_width(BNST_DATA *ptr, int depth2)
/*==================================================================*/
{
    int i;
    
    calc_self_space(ptr, depth2);

    if ( ptr->space > max_width )
      max_width = ptr->space;

    if ( ptr->child[0] )
      for ( i=0; ptr->child[i]; i++ )
	calc_tree_width(ptr->child[i], depth2+1);
}

/*==================================================================*/
    void show_link(int n1, int n2, char para_type, char to_para_p)
/*==================================================================*/
{
    int i;
    
    if ( n2 != 1 ) {
	if (para_type == PARA_NORMAL || 
	    para_type == PARA_INCOMP ||
	    to_para_p == TRUE)
	  fprintf(stdout, "─");
	else 
	  fprintf(stdout, "──");
	if ( n1%2 ) fprintf(stdout, "┤");
	else fprintf(stdout, "┐");
	fprintf(stdout, "　");
	for ( i=2; i<n2; i++ ) {
	    fprintf(stdout, "　　");
	    if ( (n1%(mylog(i)))/mylog(i-1) ) fprintf(stdout, "│");
	    else fprintf(stdout, "　");
	    fprintf(stdout, "　");
	}
    }
}

/*==================================================================*/
  void show_self(BNST_DATA *ptr, int depth1, int depth2, int flag)
/*==================================================================*/
{
    int i, j, comb_count = 0, c_count = 0;
    BNST_DATA *ptr_buffer[10], *child_buffer[10];

    if ( ptr->child[0] ) {
	for ( i=0; ptr->child[i]; i++ );
	show_self(ptr->child[i-1], depth1*2, depth2+1, 0);
	if ( i > 1 ) {
	    for ( j=i-2; j>0; j-- )
	      show_self(ptr->child[j], depth1*2+1, depth2+1, 0);

	    /* flag: 1: ─PARA 2: -<P>PARA */

	    if (ptr->para_top_p == TRUE && 
		ptr->para_type == PARA_NIL &&
		ptr->to_para_p == FALSE)
	      show_self(ptr->child[0], depth1*2+1, depth2+1, 1);
	    else if (ptr->para_top_p == TRUE)
	      show_self(ptr->child[0], depth1*2+1, depth2+1, 2);
	    else
	      show_self(ptr->child[0], depth1*2+1, depth2+1, 0);
	}
    }

    calc_self_space(ptr, depth2);
    if ( ptr->para_top_p != TRUE ) {
	for (i = 0; i < max_width - ptr->space; i++) 
	  fputc(' ', stdout);
    }
    print_bnst(ptr, NULL);
    
    if ( flag == 0 ) {
	show_link(depth1, depth2, ptr->para_type, ptr->to_para_p);
	if (OptExpress == OPT_TREEF) {
	    fputc(';', stdout);
	    print_some_feature(ptr->f, stdout);
	}
	fputc('\n', stdout);
    } else if ( flag == 1 ) {
	fprintf(stdout, "─");
    } else if ( flag == 2 ) {
	fprintf(stdout, "-");
    }



}

/*==================================================================*/
	 void show_sexp(BNST_DATA *ptr, int depth, int pars)
/*==================================================================*/
{
    int i, j, comb_count = 0, c_count = 0;
    BNST_DATA *ptr_buffer[10], *child_buffer[10];

    for (i = 0; i < depth; i++) fputc(' ', stdout);
    fprintf(stdout, "(");

    if ( ptr->para_top_p == TRUE ) {
	if (ptr->child[1] && 
	    ptr->child[1]->para_key_type == PARA_KEY_N)
	  fprintf(stdout, "(noun_para"); 
	else
	  fprintf(stdout, "(pred_para");

	if (ptr->child[0]) {
	    fputc('\n', stdout);
	    i = 0;
	    while (ptr->child[i+1] && ptr->child[i+1]->para_type != PARA_NIL) {
		/* <P>の最後以外 */
		/* UCHI fputc(',', stdout); */
		show_sexp(ptr->child[i], depth + 3, 0);	i ++;
	    }
	    if (ptr->child[i+1]) { /* その他がある場合 */
		/* <P>の最後 */
		/* UCHI fputc(',', stdout); */
		show_sexp(ptr->child[i], depth + 3, 1);	i ++;
		/* その他の最後以外 */
		while (ptr->child[i+1]) {
		    /* UCHI fputc(',', stdout); */
		    show_sexp(ptr->child[i], depth + 3, 0); i ++;
		}
		/* その他の最後 */
		/* UCHI fputc(',', stdout); */
		show_sexp(ptr->child[i], depth + 3, pars + 1);
	    }
	    else {
		/* <P>の最後 */
		/* UCHI fputc(',', stdout); */
		show_sexp(ptr->child[i], depth + 3, pars + 1 + 1);
	    }
	}
    }
    else {
	
	print_bnst_detail(ptr);

	if (ptr->child[0]) {
	    fputc('\n', stdout);
	    for ( i=0; ptr->child[i+1]; i++ ) {
		/* UCHI fputc(',', stdout); */
		show_sexp(ptr->child[i], depth + 3, 0);
	    }
	    /* UCHI fputc(',', stdout); */
	    show_sexp(ptr->child[i], depth + 3, pars + 1);
	} else {
	    for (i = 0; i < pars + 1; i++) fputc(')', stdout);
	    fputc('\n', stdout);
	}
    }
}

/*==================================================================*/
			 void print_kakari()
/*==================================================================*/
{
    /* 依存構造木の表示 */

    if (OptExpress == OPT_TREE || OptExpress == OPT_TREEF) {
	max_width = 0;
	calc_tree_width((bnst_data + Bnst_num -1), 1);
	show_self((bnst_data + Bnst_num -1), 1, 1, 0);
    }
    else if (OptExpress == OPT_SEXP) {
	show_sexp((bnst_data + Bnst_num -1), 0, 0);
    }

    fprintf(stdout, "EOS\n");
}


/*====================================================================
			      チェック用
====================================================================*/

/*==================================================================*/
                         void check_bnst()
/*==================================================================*/
{
    int 	i, j;
    BNST_DATA 	*ptr;
    char b_buffer[256];

    for (i = 0; i < Bnst_num; i++) {
	ptr = &bnst_data[i];
	
	b_buffer[0] = '\0';
	if (ptr->settou_ptr) {
	    for (j = 0; j < ptr->settou_num; j++ ) {
		strcat(b_buffer, (ptr->settou_ptr + j)->Goi2);
		strcat(b_buffer, " ");
	    } 
	    strcat(b_buffer, ": ");
	}
	for (j = 0; j < ptr->jiritu_num; j++ ) {
	    strcat(b_buffer, (ptr->jiritu_ptr + j)->Goi2);
	    strcat(b_buffer, " ");
	}
	if (ptr->fuzoku_ptr) {
	    strcat(b_buffer, ": ");
	    for (j = 0; j < ptr->fuzoku_num; j++ ) {
		strcat(b_buffer, (ptr->fuzoku_ptr + j)->Goi2);
		strcat(b_buffer, " ");
	    }
	}
	fprintf(stdout, "%-20s", b_buffer);

	print_feature(ptr->f, stdout);

	if (check_feature(ptr->f, "用言:強") ||
	    check_feature(ptr->f, "用言:弱")) {

	    fprintf(stdout, " <表層格:");
	    if (ptr->SCASE_code[case2num("ガ格")])
	      fprintf(stdout, "ガ,");
	    if (ptr->SCASE_code[case2num("ヲ格")])
	      fprintf(stdout, "ヲ,");
	    if (ptr->SCASE_code[case2num("ニ格")])
	      fprintf(stdout, "ニ,");
	    if (ptr->SCASE_code[case2num("デ格")])
	      fprintf(stdout, "デ,");
	    if (ptr->SCASE_code[case2num("カラ格")])
	      fprintf(stdout, "カラ,");
	    if (ptr->SCASE_code[case2num("ト格")])
	      fprintf(stdout, "ト,");
	    if (ptr->SCASE_code[case2num("ヨリ格")])
	      fprintf(stdout, "ヨリ,");
	    if (ptr->SCASE_code[case2num("ヘ格")])
	      fprintf(stdout, "ヘ,");
	    if (ptr->SCASE_code[case2num("マデ格")])
	      fprintf(stdout, "マデ,");
	    if (ptr->SCASE_code[case2num("ノ格")])
	      fprintf(stdout, "ノ,");
	    if (ptr->SCASE_code[case2num("ガ２")])
	      fprintf(stdout, "ガ２,");
	    fprintf(stdout, ">");
	}

	fputc('\n', stdout);
    }
}

/*==================================================================*/
			 void print_result()
/*==================================================================*/
{
    int i, j, k;
    char *date_p;

    TOTAL_MGR *tm = &Best_mgr;
    
    /* PS出力の場合
       dpnd_info_to_bnst(&(tm->dpnd));
       make_dpnd_tree();
       print_kakari2ps();
       return;
    */ 

    /* 既解析へのパターンマッチで, マッチがなければ出力しない
       if (OptAnalysis == OPT_PM && !PM_Memo[0]) return;
    */

    if (!(OptInhibit & OPT_INHIBIT_BARRIER))
	print_barrier(Bnst_num);

    /* ヘッダの出力 */

    if (Comment[0]) {
	fprintf(stdout, "%s", Comment);
    } else {
	fprintf(stdout, "# S-ID:%d", Sen_num);
    }

    if (OptAnalysis != OPT_PM && (date_p = (char *)getenv("DATE"))) {
	fprintf(stdout, " KNP:%s", date_p);
    }

    if (PM_Memo[0]) {
	if (strstr(Comment, "MEMO")) {
	    fprintf(stdout, "%s", PM_Memo);
	} else {
	    fprintf(stdout, " MEMO:%s", PM_Memo);
	}	
    }
    fprintf(stdout, "\n");

    /* チェック用 */
    if (OptCheck == TRUE)
	for (i = 0; i < Bnst_num; i++)
	    if (tm->dpnd.check[i].num != -1) {
		fprintf(stdout, ";;;(check) %2d %2d %d (%d)", i, tm->dpnd.head[i], tm->dpnd.check[i].num, tm->dpnd.check[i].def);
		for (j = 0; j < tm->dpnd.check[i].num; j++)
		    fprintf(stdout, " %d", tm->dpnd.check[i].pos[j]);
		fprintf(stdout, "\n");
	    }

    /* 解析結果のメインの出力 */

    dpnd_info_to_bnst(&(tm->dpnd)); /* 係り受け情報を bnst 構造体に記憶 */

    if (OptExpress == OPT_TAB) {
	print_mrphs(1);
    } else {
	make_dpnd_tree();
	print_kakari();
    }

    /* 格解析を行なった場合の出力 */

    if ((OptAnalysis == OPT_CASE || 
	 OptAnalysis == OPT_CASE2 ||
	 OptAnalysis == OPT_DISC) &&
	(OptDisplay == OPT_DETAIL || 
	 OptDisplay == OPT_DEBUG)) {

	fprintf(stdout, "■ %d Score:%d, Dflt:%d, Possibilty:%d/%d ■\n", 
		Sen_num, tm->score, tm->dflt, tm->pssb+1, 1);

	/* 上記出力の最後の引数(依存構造の数)は1にしている．
	   ちゃんと扱ってない */
	
	for (i = 0; i < tm->pred_num; i++) {
	    print_data_cframe(&(tm->cpm[i]));
	    for (j = 0; j < tm->cpm[i].result_num; j++)
		print_crrspnd(&(tm->cpm[i]), &(tm->cpm[i].cmm[j]));
	}
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================
			   出力のPSへの変換

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

#define HEAD_FILE "/home/kuro/work/nl/E/knpe/src/head.dat"
#define TAIL_FILE "/home/kuro/work/nl/E/knpe/src/tail.dat"
#define X 	197
#define Y 	757
#define X2 	497
#define Y2 	757

/* #define FOR_GLOSS 訳挿入用 */

void write_head(FILE *fp)
{
    FILE *fpr;
    int c;

    fpr = fopen(HEAD_FILE, "r");
    while((c = fgetc(fpr)) != EOF)
      fputc(c, fp);
    fclose(fpr);
}

void write_tail(FILE *fp)
{
    FILE *fpr;
    int c;

    fpr = fopen(TAIL_FILE, "r");
    while((c = fgetc(fpr)) != EOF)
      fputc(c, fp);
    fclose(fpr);
} 

void write_text10(FILE *fp, char *cp, int x, int y)
{
    fprintf(fp, "Begin %%I Text\n");
    fprintf(fp, "%%I cfg Black\n");
    fprintf(fp, "0 0 0 SetCFg\n");
    fprintf(fp, "%%I f *-times-medium-r-*-100-*\n");
    fprintf(fp, "/Times-Roman 10 SetF\n");
    fprintf(fp, "%%I t\n");
    fprintf(fp, "[ 1 0 0 1 %d %d ] concat\n", x, y-3);
    fprintf(fp, "%%I\n");
    fprintf(fp, "[\n");
    fprintf(fp, "(%s)\n", cp);
    fprintf(fp, "] WhiteBg Text\n");
    fprintf(fp, "End\n");
    fprintf(fp, "\n");
}
    
void write_text14(FILE *fp, char *cp, int x, int y)
{
    fprintf(fp, "Begin %%I Text\n");
    fprintf(fp, "%%I cfg Black\n");
    fprintf(fp, "0 0 0 SetCFg\n");
    fprintf(fp, "%%I f *-times-medium-r-*-140-*\n");
    fprintf(fp, "/Times-Roman 14 SetF\n");
    fprintf(fp, "%%I t\n");
    fprintf(fp, "[ 1 0 0 1 %d %d ] concat\n", x, y-3);
    fprintf(fp, "%%I\n");
    fprintf(fp, "[\n");
    fprintf(fp, "(%s)\n", cp);
    fprintf(fp, "] WhiteBg Text\n");
    fprintf(fp, "End\n");
    fprintf(fp, "\n");
}

void write_kanji(FILE *fp, char *cp, int x, int y)
{
    fprintf(fp, "Begin %%I KText\n");
    fprintf(fp, "%%I cfg Black\n");
    fprintf(fp, "0 0 0 SetCFg\n");
    fprintf(fp, "%%I k k14\n");
    fprintf(fp, "/Ryumin-Light-H 14 SetF\n");
    fprintf(fp, "%%I t\n");
    fprintf(fp, "[ 1 0 0 1 %d %d ] concat\n", x, y);
    fprintf(fp, "%%I\n");
    fprintf(fp, "[\n");
    fprintf(fp, "<");
    while (*cp) {
	fprintf(fp, "%02x", *cp+128);
	cp++;
    }
    fprintf(fp, ">\n");
    fprintf(fp, "] WhiteBg Text\n");
    fprintf(fp, "End\n");
    fprintf(fp, "\n");
}

static char tmp[64];
/*==================================================================*/
            void print_matrix2ps(int jlen, int type)
/*==================================================================*/
{
    int i, j, comma_p;
    int para_char = 0;     /* para_key の表示用 */
    char point_B[10];
    PARA_DATA *ptr;
    BNST_DATA *b_ptr;

    /* パスのマーク付け */

    for (i = 0; i < sp->Bnst_num; i++)
      for (j = 0; j < sp->Bnst_num; j++)
	path_matrix[i][j] = 0;

    if (type == PRINT_PARA) {
	for (i = 0; i < Para_num; i++) {
	    ptr = &sp->para_data[i];
	    for (j = ptr->L_B+1; j <= ptr->R; j++)
	      path_matrix[ptr->max_path[j-ptr->L_B-1]][j] =
		path_matrix[ptr->max_path[j-ptr->L_B-1]][j] ?
		  -1 : 'a' + i;
	}
    }

    /* ＰＳの出力 */

    write_head(stdout);

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {

	if ( b_ptr->mrph_num == 1 ) {
	    strcpy(tmp, b_ptr->mrph_ptr->Goi2);
	    comma_p = FALSE;
	} else {
	    strcpy(tmp, b_ptr->mrph_ptr->Goi2);
	    for (j = 1; j < (b_ptr->mrph_num - 1); j++) 
	      strcat(tmp, (b_ptr->mrph_ptr + j)->Goi2);
	    if (!strcmp(Class[(b_ptr->mrph_ptr + b_ptr->mrph_num - 1)->Hinshi][0].id,
			"特殊") &&
		!strcmp(Class[(b_ptr->mrph_ptr + b_ptr->mrph_num - 1)->Hinshi]
			[(b_ptr->mrph_ptr + b_ptr->mrph_num - 1)->Bunrui].id, 
			"読点")) {
		comma_p = TRUE;
	    } else {
		strcat(tmp, (b_ptr->mrph_ptr + b_ptr->mrph_num - 1)->Goi2);
		comma_p = FALSE;	      
	    }
	}

	if (sp->bnst_data[i].para_key_type) {
	    sprintf(point_B, "%c>", 'a'+ para_char);
	    para_char++;
	    if (comma_p)
	      write_text14(stdout,point_B,X+i*20+15-strlen(tmp)*7-14-7,Y-i*20);
	    else
	      write_text14(stdout,point_B,X+i*20+15-strlen(tmp)*7-14, Y-i*20);
	}

	if (comma_p) {
	    write_kanji(stdout, tmp, X+i*20+15-strlen(tmp)*7-7, Y-i*20);
	    write_text14(stdout, ",", X+i*20+15-7, Y-i*20);
	} else {
	    write_kanji(stdout, tmp, X+i*20+15-strlen(tmp)*7, Y-i*20);
	}
	
	for (j = i+1; j < sp->Bnst_num; j++) {
	    if (type == PRINT_PARA)
	      sprintf(point_B, "%2d", match_matrix[i][j]);
	    else if (type == PRINT_DPND)
	      sprintf(point_B, "%2d", Dpnd_matrix[i][j]);
	    switch(path_matrix[i][j]) {
	      case  0:	point_B[2] = ' '; break;
	      case -1:	point_B[2] = '*'; break;
	      default:	point_B[2] = path_matrix[i][j]; break;
	    }
	    point_B[3] = '\0';
	    write_text14(stdout, point_B, X+j*20, Y-i*20);	
	}
    }
    
    if (type == PRINT_PARA) {
	sprintf(point_B, "\\(%d", jlen);
	write_text14(stdout, point_B, 
		     X+(sp->Bnst_num-1)*20+15-(strlen(point_B)-1)*7-35, 
		     Y-sp->Bnst_num*20);
	write_kanji(stdout, "文字", X+(sp->Bnst_num-1)*20+15-35, Y-sp->Bnst_num*20);
	write_text14(stdout, "\\)", X+(sp->Bnst_num-1)*20+15-7, Y-sp->Bnst_num*20);
    }
    write_tail(stdout);
}

static int X_pos, Y_pos, Wid, Hig;
/*==================================================================*/
	    void print_bnst2ps(BNST_DATA *ptr)
/*==================================================================*/
{
    int i;

    if (ptr) {
	if (ptr->para_top_p == TRUE) {
	    write_text10(stdout, "PARA", X_pos, Y_pos); X_pos += Wid*4;
	} else {
	    strcpy(tmp, ptr->mrph_ptr->Goi2);
	    for (i = 1; i < ptr->mrph_num; i++) 
	      strcat(tmp, (ptr->mrph_ptr + i)->Goi2);
	    write_kanji(stdout, tmp, X_pos, Y_pos); X_pos += Wid*strlen(tmp);
	}

	if (ptr->para_type == PARA_NORMAL) {
	    write_text10(stdout, "<P>", X_pos, Y_pos); X_pos += Wid*3;
	} else if (ptr->para_type == PARA_INCOMP) {
	    write_text10(stdout, "<I>", X_pos, Y_pos); X_pos += Wid*3;
	}
	if (ptr->to_para_p == TRUE) {
	    write_text10(stdout, "\\(D\\)", X_pos, Y_pos); X_pos += Wid*3;
	}
    }
}

/*==================================================================*/
       void calc_tree_width2ps(BNST_DATA *ptr, int depth2)
/*==================================================================*/
{
    int i;
    
    if (ptr->para_top_p == TRUE) ptr->space = 4;
    else      			    ptr->space = ptr->length;
    if (ptr->para_type == PARA_NORMAL || ptr->para_type == PARA_INCOMP)
      ptr->space += 1;
    if (ptr->to_para_p == TRUE)
      ptr->space += 3;
    ptr->space += (depth2-1)*8;

    if (ptr->child[0])
      for (i = 0; ptr->child[i]; i++)
	calc_tree_width2ps(ptr->child[i], depth2+1);
}

/*==================================================================*/
void show_link2ps(int depth, char *ans_flag, int para_flag, int x_pos)
/*==================================================================*/
{
    int i;
    
    if (depth != 1) {

	if (para_flag == PARA_NORMAL || para_flag == PARA_INCOMP) {
	    write_kanji(stdout, "─", X_pos, Y_pos); X_pos += Wid*2;
	} else {
	    write_kanji(stdout, "──", X_pos, Y_pos); X_pos += Wid*4;
	}

	if (ans_flag[depth-1] == '1') {
	    write_kanji(stdout, "┤", X_pos, Y_pos); X_pos += Wid*2;
	} else {
	    write_kanji(stdout, "┐", X_pos, Y_pos); X_pos += Wid*2;
	}

	X_pos += Wid*2;

	for (i = depth - 1; i > 1; i--) {
	    X_pos += Wid*4;
	    if (ans_flag[i-1] == '1') {
		write_kanji(stdout, "│", X_pos, Y_pos); X_pos += Wid*2;
	    } else {
		X_pos += Wid*2;
	    }
	    X_pos += Wid*2;
	}

#ifdef FOR_GLOSS /* 訳挿入用 */
	Y_pos -= Hig;
	X_pos = x_pos;

	if (para_flag == PARA_NORMAL || para_flag == PARA_INCOMP) {
            X_pos += Wid*2;
        } else {
            X_pos += Wid*4;
        }

	write_kanji(stdout, "│", X_pos, Y_pos); X_pos += Wid*2;

	X_pos += Wid*2;

	for (i = depth - 1; i > 1; i--) {
	    X_pos += Wid*4;
	    if (ans_flag[i-1] == '1') {
		write_kanji(stdout, "│", X_pos, Y_pos); X_pos += Wid*2;
	    } else {
		X_pos += Wid*2;
	    }
	    X_pos += Wid*2;
	}
#endif
	Y_pos -= Hig;
	X_pos = X2;
    }
}

/*==================================================================*/
void show_self2ps(BNST_DATA *ptr, int depth, char *ans_flag_p, int flag)
/*==================================================================*/
{
    int i, j, comb_count = 0, c_count = 0;
    BNST_DATA *ptr_buffer[10], *child_buffer[10];
    char ans_flag[BNST_MAX];

    if (ans_flag_p) {
	strncpy(ans_flag, ans_flag_p, BNST_MAX);
    } else {
	ans_flag[0] = '0';	/* 最初に呼ばれるとき */
    }

    if (ptr->child[0]) {
	for (i = 0; ptr->child[i]; i++);

	/* 最後の子は ans_flag を 0 に */ 
	ans_flag[depth] = '0';
	show_self2ps(ptr->child[i-1], depth+1, ans_flag, 0);

	if (i > 1) {
	    /* 他の子は ans_flag を 1 に */ 
	    ans_flag[depth] = '1';
	    for (j = i - 2; j > 0; j--) {
		show_self2ps(ptr->child[j], depth+1, ans_flag, 0);
	    }

	    /* flag: 1: ─PARA 2: -<P>PARA */
	    if (ptr->para_top_p == TRUE && ptr->para_type == PARA_NIL)
	      show_self2ps(ptr->child[0], depth+1, ans_flag, 1);
	    else if (ptr->para_top_p == TRUE && ptr->para_type != PARA_NIL)
	      show_self2ps(ptr->child[0], depth+1, ans_flag, 2);
	    else
	      show_self2ps(ptr->child[0], depth+1, ans_flag, 0);
	}
    }

    if (ptr->para_top_p != TRUE)
	X_pos -= ptr->space*7;

    print_bnst2ps(ptr);
    
    if (flag == 0) {
	show_link2ps(depth, ans_flag, ptr->para_type, X_pos);
    } else if (flag == 1) {
	write_kanji(stdout, "─", X_pos, Y_pos); X_pos += Wid*2;
    } else if (flag == 2) {
	write_text10(stdout, "-", X_pos, Y_pos); X_pos += Wid;
    }
}

/*==================================================================*/
			 void print_kakari2ps()
/*==================================================================*/
{
    /* 依存構造木の表示 */

    calc_tree_width2ps((sp->bnst_data + sp->Bnst_num - 1), 1);

    X_pos = X2; Y_pos = Y2;
    Wid = 7;   Hig = 14;

    /* ＰＳの出力 */

    write_head(stdout);

    show_self2ps((sp->bnst_data + sp->Bnst_num - 1), 1, NULL, 0);

    write_tail(stdout);
}
/*====================================================================
                               END
====================================================================*/

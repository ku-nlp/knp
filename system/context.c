/*====================================================================

			       文脈解析

                                               S.Kurohashi 98. 9. 8

    $Id$
====================================================================*/
#include "knp.h"

SENTENCE_DATA sentence_data[256];

/*==================================================================*/
			 void copy_sentence()
/*==================================================================*/
{
    SENTENCE_DATA *sp;
    int i;

    /* 文解析結果の保持 */

    sp = sentence_data + Sen_num;
    sp->sentence_num = Sen_num;

    sp->Mrph_num = Mrph_num;
    sp->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA)*Mrph_num, 
					     "MRPH DATA");
    for (i = 0; i < Mrph_num; i++)
	sp->mrph_data[i] = mrph_data[i];
	
    sp->Bnst_num = Bnst_num;
    sp->bnst_data = 
	(BNST_DATA *)malloc_data(sizeof(BNST_DATA)*(Bnst_num + New_Bnst_num), 
				 "BNST DATA");
    for (i = 0; i < Bnst_num + New_Bnst_num; i++)
	sp->bnst_data[i] = bnst_data[i];

    sp->para_data = (PARA_DATA *)malloc_data(sizeof(PARA_DATA)*Para_num, 
				 "PARA DATA");
    for (i = 0; i < Para_num; i++)
	sp->para_data[i] = para_data[i];

    sp->para_manager = 
	(PARA_MANAGER *)malloc_data(sizeof(PARA_MANAGER)*Para_M_num, 
				    "PARA MANAGER");
    for (i = 0; i < Para_M_num; i++)
	sp->para_manager[i] = para_manager[i];
}

/*==================================================================*/
	 void assign_bnst_feature2(BnstRule *r_ptr, int size)
/*==================================================================*/
{
    int 	i, j;
    BnstRule	*loop_ptr; 
    BNST_DATA	*b_ptr;

    for (i = 0, b_ptr = bnst_data + Bnst_num - 1; i < Bnst_num; 
	 i++, b_ptr--)
	for (j = 0, loop_ptr = r_ptr; j < size; j++, loop_ptr++)
	    if (regexpbnstrule_match(loop_ptr, b_ptr) == TRUE ) {
		assign_feature(&(b_ptr->f), &(loop_ptr->f), b_ptr);
	    }
}

/*==================================================================*/
			      void ttt()
/*==================================================================*/
{
    int 	i, j, k;
    BNST_DATA	*b_ptr, *b_ptr2;
    char 	fname[256];
    
    for (i = Bnst_num - 1; i >= 0; i--) {
	b_ptr = bnst_data + i;

	if (!check_feature(b_ptr->f, "C:対象")) continue;

	if (check_feature(b_ptr->f, "CTYPE:時間")) {

	    for (j = i - 1; j >= 0; j--) {
		b_ptr2 = bnst_data + j;
		if (check_feature(b_ptr2->f, "CTYPE:時間")) {
		    sprintf(fname, "CMATCH:時間%d-%d(%s)", 
			    Sen_num, j, b_ptr2->Jiritu_Go);
		    assign_cfeature(&(b_ptr->f), fname);
		}
	    }
	    for (k = Sen_num - 1; k >= 1; k--) {
		for (j = sentence_data[k].Bnst_num - 1; j >= 0; j--) {
		    b_ptr2 = &(sentence_data[k].bnst_data[j]);
		    if (check_feature(b_ptr2->f, "CTYPE:時間")) {
			sprintf(fname, "CMATCH時間%d-%d(%s)", 
				k, j, b_ptr2->Jiritu_Go);
			assign_cfeature(&(b_ptr->f), fname);
		    }
		}
	    }
	}
	else if (check_feature(b_ptr->f, "CTYPE:場所")) {
	    ;
	}
    }
}

/*==================================================================*/
		      void discourse_analysis()
/*==================================================================*/
{
    copy_sentence();
    assign_bnst_feature(ContRuleArray, ContRuleSize);
    ttt();
    print_result();
}

/*====================================================================
                               END
====================================================================*/

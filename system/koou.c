/*====================================================================

			      呼応の処理

                                               S.Kurohashi 1995. 7. 4
                                               S.Ozaki     1995. 2. 8

    $Id$
====================================================================*/
#include "knp.h"

int 	Koou_matrix[BNST_MAX][BNST_MAX];
int	Koou_dpnd_matrix[BNST_MAX][BNST_MAX];
int	koou_m_p[BNST_MAX];

/*==================================================================*/
		  void init_koou(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i ,j;

    for (i = 0; i < sp->Bnst_num; i++) {
	koou_m_p[i] = FALSE;
	for (j = 0; j < sp->Bnst_num; j++) {
	    Koou_matrix[i][j] = 0;
	    Koou_dpnd_matrix[i][j] = 0;
	}
    }
}

/*==================================================================*/
		  int check_koou(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int		i, j, k, l, flag;
    int         pu_flag;
    BNST_DATA   *b_ptr, *c_ptr;
    KoouRule 	*r_ptr;

    flag = FALSE;

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	for (j = 0, r_ptr = KoouRuleArray; j < CurKoouRuleSize; j++, r_ptr++) {
	    if (_regexpbnst_match(r_ptr->start_pattern, b_ptr) != -1 &&
		(Language != CHINESE ||
		 (Language == CHINESE && check_feature((sp->bnst_data+i)->f, "P")))) {
		if (OptDisplay == OPT_DEBUG) 
		    fprintf(stderr, "Start (%d) %d\n", j, i);
		for (k = i, c_ptr = b_ptr; k < sp->Bnst_num; k++, c_ptr++) {
		    if (Language == CHINESE) {
			pu_flag = 0;
			for (l = i; l < k; l++) {
			    if (check_feature((sp->bnst_data+l)->f, "PU")) {
				pu_flag = 1;
				break;
			    }
			}
		    }
		    else {
			pu_flag = 0;
		    }
		
		    if (_regexpbnst_match(r_ptr->end_pattern, c_ptr) != -1 && 
			(Language != CHINESE || 
			 (Language == CHINESE && !pu_flag && 
			  (check_feature((sp->bnst_data+k)->f, "LC") || check_feature((sp->bnst_data+k)->f, "NN"))))) {
			koou_m_p[i] = TRUE;
			flag = TRUE;
			Koou_matrix[i][k] = 1;
			Koou_dpnd_matrix[i][k] = (int)r_ptr->dpnd_type;
			if (OptDisplay == OPT_DEBUG) 
			    fprintf(Outfp, "  End %d\n", k);
		    }
		    else if (r_ptr->uke_pattern &&
			     _regexpbnst_match(r_ptr->uke_pattern, c_ptr) != -1 && 
			     (Language != CHINESE || 
			      (Language == CHINESE && !pu_flag && check_feature((sp->bnst_data+k)->f, "LC")))) {
			Koou_matrix[i][k] = 2;
			Koou_dpnd_matrix[i][k] = (int)r_ptr->dpnd_type; /* 今のところ記述できない */
			if (OptDisplay == OPT_DEBUG) 
			    fprintf(Outfp, "  Uke %d\n", k);
		    }
		}
	    }
	}
    }

    return flag;
}

/*==================================================================*/
     void mask_for(SENTENCE_DATA *sp, int si, int start, int end)
/*==================================================================*/
{
    int i, j;

    for (i = si + 1; i < start; i++) {
	for (j = end + 1; j < sp->Bnst_num;j++) {
	    Dpnd_matrix[i][j] = 0;
	}
    }
}

/*==================================================================*/
		   void mask_back(int si, int start)
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < si; i++) {
	for (j = si +1 ; j < start ; j++) {
	    Dpnd_matrix[i][j] = 0;
	}
    }
}

/*==================================================================*/
		void change_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, l, f_start, f_end;

    if (Language == CHINESE) {
	for (i = 0; i < sp->Bnst_num; i++){
	    if (koou_m_p[i] == TRUE) {
		f_start = -1;
		f_end = -1;
		for (j = i; j < sp->Bnst_num; j++){
		    if (Koou_matrix[i][j] > 0) {
			Dpnd_matrix[i][j] = Koou_dpnd_matrix[i][j];
			if (Koou_matrix[i][j] == 1) {
			    f_end = j;
			    if (f_start < 0)
				f_start = j;
			}
			if (Koou_dpnd_matrix[i][j] == 'L') {
			    for (k = j + 1; k < sp->Bnst_num; k++) {
				Dpnd_matrix[j][k] = 0;
			    }
			    for (k = 0; k < i; k ++) {
				Dpnd_matrix[k][j] = 0;
			    }
			    for (k = i + 1; k < j; k ++) {
				for (l = 0; l < i; l++) {
				    Dpnd_matrix[l][k] = 0;
				}
				for (l = j + 1; l < sp->Bnst_num; l++) {
				    Dpnd_matrix[k][l] = 0;
				}
			    }
			}
			if (Koou_dpnd_matrix[i][j] == 'R') {
			    for (k = i + 1; k < sp->Bnst_num; k++) {
				if (k != j) {
				    Dpnd_matrix[i][k] = 0;
				}
			    }
			    for (k = 0; k < i; k ++) {
				Dpnd_matrix[i][k] = 0;
			    }
			    for (k = i + 1; k < j; k ++) {
				for (l = 0; l < i; l++) {
				    Dpnd_matrix[l][k] = 0;
				}
				for (l = j + 1; l < sp->Bnst_num; l++) {
				    Dpnd_matrix[k][l] = 0;
				}
			    }
			}
		    }
		}
	    }
	}
    }
    else {
	for (i = 0; i < sp->Bnst_num; i++){
	    if (koou_m_p[i] == TRUE) {
		f_start = -1;
		f_end = -1;
		for (j = i; j < sp->Bnst_num; j++){
		    if (Koou_matrix[i][j] > 0) {
			Dpnd_matrix[i][j] = Koou_dpnd_matrix[i][j];
			if (Koou_matrix[i][j] == 1) {
			    f_end = j;
			    if (f_start < 0)
				f_start = j;
			}
		    }
		    else {
			Dpnd_matrix[i][j] = 0;
		    }
		}

		mask_for(sp, i, f_start, f_end);
		mask_back(i, f_start);
	    }
	}
    }
}

/*==================================================================*/
		     int koou(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int flag;

    init_koou(sp);

    flag = (check_koou(sp) == TRUE) ? TRUE: FALSE;

    /* 行列の書き換え */
    change_matrix(sp); 

    return flag;
}

/*====================================================================
                               END
====================================================================*/

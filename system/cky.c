/*====================================================================

				 CKY

    $Id$
====================================================================*/

#include "knp.h"

typedef struct _CKY *CKYptr;
typedef struct _CKY {
    int		i;
    int		j;
    char	cp;
    double	score;		/* score at this point */
    double	para_score;	/* coordination score */
    int		para_flag;	/* coordination flag */
    char	dpnd_type;	/* type of dependency (D or P) */
    int		direction;	/* direction of dependency */
    BNST_DATA	*b_ptr;
    int 	scase_check[SCASE_CODE_SIZE];
    int		un_count;
    CF_PRED_MGR cpm;		/* case components */
    CKYptr	left;		/* pointer to the left child */
    CKYptr	right;		/* pointer to the right child */
    CKYptr	next;		/* pointer to the next CKY data at this point */
} CKY;

#define	CKY_TABLE_MAX	50000
CKY *cky_matrix[BNST_MAX][BNST_MAX];/* CKY����γư��֤κǽ��CKY�ǡ����ؤΥݥ��� */
CKY cky_table[CKY_TABLE_MAX];	  /* an array of CKY data */
int cpm_allocated_cky_num = -1;

/* verb num, only for Chinese */
int verb_num;

/* add a clausal modifiee to CPM */
void make_data_cframe_rentai_simple(CF_PRED_MGR *pre_cpm_ptr, TAG_DATA *d_ptr, TAG_DATA *t_ptr) {

    /* ���δط��ʳ��ΤȤ��ϳ����Ǥ� (���δط��Ǥ���ƻ�ΤȤ��ϳ����Ǥˤ���) */
    if (!check_feature(t_ptr->f, "���δط�") || check_feature(d_ptr->f, "�Ѹ�:��")) {
	_make_data_cframe_pp(pre_cpm_ptr, d_ptr, FALSE);
    }
    /* ��դ˳��δط��ˤ��� */
    else {
	pre_cpm_ptr->cf.pp[pre_cpm_ptr->cf.element_num][0] = pp_hstr_to_code("���δط�");
	pre_cpm_ptr->cf.pp[pre_cpm_ptr->cf.element_num][1] = END_M;
	pre_cpm_ptr->cf.oblig[pre_cpm_ptr->cf.element_num] = FALSE;
    }

    _make_data_cframe_sm(pre_cpm_ptr, t_ptr);
    _make_data_cframe_ex(pre_cpm_ptr, t_ptr);
    pre_cpm_ptr->elem_b_ptr[pre_cpm_ptr->cf.element_num] = t_ptr;
    pre_cpm_ptr->elem_b_num[pre_cpm_ptr->cf.element_num] = -1;
    pre_cpm_ptr->cf.weight[pre_cpm_ptr->cf.element_num] = 0;
    pre_cpm_ptr->cf.adjacent[pre_cpm_ptr->cf.element_num] = FALSE;
    pre_cpm_ptr->cf.element_num++;
}

int convert_to_dpnd(TOTAL_MGR *Best_mgr, CKY *cky_ptr) {
    int i;
    char *cp;

    /* case analysis result */
    if (OptAnalysis == OPT_CASE && cky_ptr->cpm.pred_b_ptr && 
	Best_mgr->cpm[cky_ptr->cpm.pred_b_ptr->pred_num].pred_b_ptr == NULL) {
	if (check_feature(cky_ptr->cpm.pred_b_ptr->f, "��:Ϣ��")) { /* clausal modifiee */
	    /* make_data_cframe_rentai_simple(&(cky_ptr->cpm), d_ptr, g_ptr); ��fix me!�� left��Ϣ�ʤΤȤ� */
	    cky_ptr->cpm.cf.element_num++;
	}
	copy_cpm(&(Best_mgr->cpm[cky_ptr->cpm.pred_b_ptr->pred_num]), &(cky_ptr->cpm), 0);
	cky_ptr->cpm.pred_b_ptr->cpm_ptr = &(Best_mgr->cpm[cky_ptr->cpm.pred_b_ptr->pred_num]);
    }

    if (cky_ptr->left && cky_ptr->right) {
	if (OptDisplay == OPT_DEBUG) {
	    printf("(%d, %d): (%d, %d) (%d, %d)\n", cky_ptr->i, cky_ptr->j, cky_ptr->left->i, cky_ptr->left->j, cky_ptr->right->i, cky_ptr->right->j);
	}

	if (cky_ptr->para_flag == 0) {
	    if (cky_ptr->dpnd_type != 'P' && 
		(cp = check_feature(cky_ptr->left->b_ptr->f, "��:̵�ʽ�°")) != NULL) {
		sscanf(cp, "%*[^:]:%*[^:]:%d", &(Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num]));
		Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = 
		    Dpnd_matrix[cky_ptr->left->b_ptr->num][cky_ptr->right->b_ptr->num];
	    }
	    else {
		if (cky_ptr->direction == RtoL) { /* <- */
		    Best_mgr->dpnd.head[cky_ptr->right->b_ptr->num] = cky_ptr->left->b_ptr->num;
		    Best_mgr->dpnd.type[cky_ptr->right->b_ptr->num] = cky_ptr->dpnd_type;
		}
		else { /* -> */
		    Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num] = cky_ptr->right->b_ptr->num;
		    Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = cky_ptr->dpnd_type;
		}
	    }
	}

	convert_to_dpnd(Best_mgr, cky_ptr->left);
	convert_to_dpnd(Best_mgr, cky_ptr->right);
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    printf("(%d, %d)\n", cky_ptr->i, cky_ptr->j);
	}
    }
}

int check_scase (BNST_DATA *g_ptr, int *scase_check, int rentai, int un_count) {
    int vacant_slot_num = 0;

    /* �����Ƥ��륬��,���,�˳�,���� */
    if ((g_ptr->SCASE_code[case2num("����")]
	 - scase_check[case2num("����")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("���")]
	 - scase_check[case2num("���")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("�˳�")]
	 - scase_check[case2num("�˳�")]) == 1 &&
	rentai == 1 &&
	check_feature(g_ptr->f, "�Ѹ�:ư")) {
	vacant_slot_num++;
	/* �˳ʤ�ư���Ϣ�ν����ξ�������θ���Ĥޤ�Ϣ��
	   �����˳�����Ƥ�����ǡ�̤�ʤΥ����åȤȤϤ��ʤ� */
    }
    if ((g_ptr->SCASE_code[case2num("����")]
	 - scase_check[case2num("����")]) == 1) {
	vacant_slot_num++;
    }

    /* ���������å�ʬ����Ϣ�ν�����̤�ʤ˥�������Ϳ���� */
    if ((rentai + un_count) <= vacant_slot_num) {
	return (rentai + un_count) * 10;
    }
    else {
	return vacant_slot_num * 10;
    }
}

char check_dpnd_possibility (SENTENCE_DATA *sp, int dep, int gov, int begin, int relax_flag) {
    if ((OptParaFix == 0 && 
	 begin >= 0 && 
	 (sp->bnst_data + dep)->para_num != -1 && 
	 Para_matrix[(sp->bnst_data + dep)->para_num][begin][gov] >= 0) || /* para score is not minus */
	(OptParaFix == 1 && 
	 Mask_matrix[dep][gov] == 2)) {   /* ����P */
	return 'P';
    }
    else if (OptParaFix == 1 && 
	     Mask_matrix[dep][gov] == 3) { /* ����I */
	return 'I';
    }
    else if (Dpnd_matrix[dep][gov] && Quote_matrix[dep][gov] && 
	     (OptParaFix == 0 || Mask_matrix[dep][gov] == 1)) {
	return Dpnd_matrix[dep][gov];
    }
    else if ((Dpnd_matrix[dep][gov] == 'R' || relax_flag) && Language != CHINESE) { /* relax */
	return 'R';
    }
    return '\0';
}

/* conventional scoring function */
double calc_score(SENTENCE_DATA *sp, CKY *cky_ptr) {
    CKY *right_ptr = cky_ptr->right, *tmp_cky_ptr = cky_ptr;
    BNST_DATA *g_ptr = cky_ptr->b_ptr, *d_ptr;
    int i, k, pred_p = 0, topic_score = 0;
    int ha_check = 0, *un_count;
    int rentai, vacant_slot_num, *scase_check;
    int count, pos, default_pos;
    double one_score = 0;
    char *cp, *cp2;

    /* �оݤ��Ѹ��ʳ��Υ������򽸤�� (right�򤿤ɤ�ʤ���left�Υ�������­��) */
    while (tmp_cky_ptr) {
	if (tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left : tmp_cky_ptr->right) {
	    one_score += tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left->score : tmp_cky_ptr->right->score;
	}
	tmp_cky_ptr = tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->right : tmp_cky_ptr->left;
    }
    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f=>", one_score);
    }

    if (check_feature(g_ptr->f, "�Ѹ�") ||
	check_feature(g_ptr->f, "���Ѹ�")) {
	pred_p = 1;
	for (k = 0; k < SCASE_CODE_SIZE; k++) cky_ptr->scase_check[k] = 0;
	scase_check = &(cky_ptr->scase_check[0]);
	cky_ptr->un_count = 0;
	un_count = &(cky_ptr->un_count);
    }

    /* ���٤ƤλҶ��ˤĤ��� */
    while (cky_ptr) {
	if (cky_ptr->direction == LtoR ? cky_ptr->left : cky_ptr->right) {
	    d_ptr = cky_ptr->direction == LtoR ? cky_ptr->left->b_ptr : cky_ptr->right->b_ptr;

	    if (Mask_matrix[d_ptr->num][g_ptr->num] == 2 || /* ����P */
		Mask_matrix[d_ptr->num][g_ptr->num] == 3) { /* ����I */
		;
	    }
	    else {
		/* ��Υ�����Ȥ�׻����뤿��ˡ��ޤ�������θ����Ĵ�٤� */
		count = 0;
		pos = 0;
		for (i = 0; i < sp->Bnst_num; i++) {
		    if (Language != CHINESE && i < d_ptr->num + 1) {
			continue;
		    }
		    if ((i > d_ptr->num && check_dpnd_possibility(sp, d_ptr->num, i, cky_ptr->i, ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) || 
			(i <= d_ptr->num && check_dpnd_possibility(sp, i ,d_ptr->num, cky_ptr->i, FALSE))) {
			if (i == g_ptr->num) {
			    pos = count;
			}
			count++;
		    }
		}

		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;

		/* �������DEFAULT�ΰ��֤Ȥκ���ڥʥ�ƥ���
		   �� �����C,B'����Ʊ󤯤˷��뤳�Ȥ����뤬�����줬
		   ¾�η�����˱ƶ����ʤ��褦,�ڥʥ�ƥ��˺���Ĥ��� */
		if (check_feature(d_ptr->f, "����")) {
		    one_score -= abs(default_pos - 1 - pos);
		}
		else {
		    one_score -= abs(default_pos - 1 - pos) * 2;
		}

		/* �������Ĥ�Τ��٤ˤ����뤳�Ȥ��ɤ� */
		if (d_ptr->num + 1 == g_ptr->num && 
		    abs(default_pos - 1 - pos) > 0 && 
		    (check_feature(d_ptr->f, "����"))) {
		    one_score -= 5;
		}
	    }
	    
	    if (pred_p && (cp = check_feature(d_ptr->f, "��")) != NULL) {
		    
		/* ̤�� ����(�֡��ϡ�)�ΰ��� */
		if (check_feature(d_ptr->f, "����") && !strcmp(cp, "��:̤��")) {

		    /* ʸ��, �֡����פʤ�, ������, C, B'�˷��뤳�Ȥ�ͥ�� */
		    if ((cp2 = check_feature(g_ptr->f, "�����")) != NULL) {
			sscanf(cp2, "%*[^:]:%d", &topic_score);
			one_score += topic_score;
		    }

		    /* ��Ĥ������ˤ�������Ϳ���� (����,���̤���)
		       �� ʣ�������꤬Ʊ��Ҹ�˷��뤳�Ȥ��ɤ� */
		    if (check_feature(d_ptr->f, "����") ||
			check_feature(d_ptr->f, "����")) {
			one_score += 10;
		    }
		    else if (ha_check == 0){
			one_score += 10;
			ha_check = 1;
		    }
		}

		k = case2num(cp + 3);

		/* �����ǰ��̤ΰ��� */

		/* ̤�� : �����Ƥ�������Ƕ������åȤ�Ĵ�٤� (����,���̤���) */
		if (!strcmp(cp, "��:̤��")) {
		    if (check_feature(d_ptr->f, "����") ||
			check_feature(d_ptr->f, "����")) {
			one_score += 10;
		    }
		    else {
			(*un_count)++;
		    }
		}

		/* �γ� : �θ��ʳ��ʤ� break 
		   �� ���������γ����Ǥˤ�����Ϳ���ʤ���
		   �� �γʤ�������Ф��������γʤϤ�����ʤ�

		   �� ���θ��פȤ����Τ�Ƚ���Τ��ȡ�������
		   ʸ���ʤɤǤ��Ѹ�:ư�ȤʤäƤ��뤳�Ȥ�
		   ����Τǡ����θ��פǥ����å� */
		else if (!strcmp(cp, "��:�γ�")) {
		    if (!check_feature(g_ptr->f, "�θ�")) {
			/* one_score += 10;
			   break; */
			if (g_ptr->SCASE_code[case2num("����")] &&
			    scase_check[case2num("����")] == 0) {
			    one_score += 10;
			    scase_check[case2num("����")] = 1;
			}
		    }
		} 

		/* ���� : ������ʸ������ΤǾ���ʣ�� */
		else if (!strcmp(cp, "��:����")) {
		    if (g_ptr->SCASE_code[case2num("����")] &&
			scase_check[case2num("����")] == 0) {
			one_score += 10;
			scase_check[case2num("����")] = 1;
		    }
		    else if (g_ptr->SCASE_code[case2num("����")] &&
			     scase_check[case2num("����")] == 0) {
			one_score += 10;
			scase_check[case2num("����")] = 1;
		    }
		}

		/* ¾�γ� : �Ƴ�1�Ĥ������򤢤�����
		   �� �˳ʤξ�硤���֤Ȥ���ʳ��϶��̤��������������⡩ */
		else if (k != -1) {
		    if (scase_check[k] == 0) {
			scase_check[k] = 1;
			one_score += 10;
		    } 
		}

		/* �֡�����Τϡ����פ˥ܡ��ʥ� 01/01/11
		   �ۤȤ�ɤξ�������

		   ������)
		   �ֹ��Ĥ����Τ� Ǥ���� ���ݤ���� ��ͳ�� ��Ĥ餷����

		   �ֻȤ��Τ� �������� ���Ȥ�����
		   �ֱ�������� �ʤ뤫�ɤ����� ��̯�� �Ȥ���������
		   �� ��������ϡ֤���/�Ȥ������פ˷���Ȱ���

		   ��¾�ͤ� ������Τ� �����ˤʤ� ������Ǥ���
		   �� �������ۣ�������ʸ̮��������

		   ��������)
		   �֤��줬 �֣ͣФ� ʬ����ʤ� ���Ǥ��礦��
		   �֡� ����ʤ� ���� ��������
		   �֥ӥ��� ���Τ� ���Ѥ� ���塣��
		   ���Ȥ� ��ޤ�Τ� �򤱤�줽���ˤʤ� ���Ԥ�������
		   �֤��ޤ� ��Ω�ĤȤ� �פ��ʤ� ����������
		   �֤ɤ� �ޤ�礦���� ����뤵��Ƥ��� ˡ������
		   ��ǧ����뤫�ɤ����� ���줿 ��Ƚ�ǡ�

		   �����ꢨ
		   �֤�������פ��� �Τ褦�ʾ����Ѹ��Ȥߤʤ����Τ�����
		*/

		if (check_feature(d_ptr->f, "�Ѹ�") &&
		    (check_feature(d_ptr->f, "��:̤��") ||
		     check_feature(d_ptr->f, "��:����")) &&
		    check_feature(g_ptr->f, "�Ѹ�:Ƚ")) {
		    one_score += 3;
		}
	    }

	    /* Ϣ�ν����ξ�硤���褬
	       ������̾��,����Ū̾��
	       ����ͽ���,�ָ����ߡפʤ�
	       �Ǥʤ���а�Ĥγ����Ǥȹͤ��� */
	    if (check_feature(d_ptr->f, "��:Ϣ��")) {
		if (check_feature(g_ptr->f, "���δط�") || 
		    check_feature(g_ptr->f, "�롼�볰�δط�")) {
		    one_score += 10;	/* ���δط��ʤ餳���ǲ��� */
		}
		else {
		    /* ����ʳ��ʤ���������åȤ�����å� (Ϣ�ν������դ����Ȥ��Υ������κ�ʬ) */
		    one_score += check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 1, cky_ptr->left->un_count) - 
			check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 0, cky_ptr->left->un_count);
		}
	    }

	    /* calc score for Chinese */
	    if (Language == CHINESE) {
		/* add score for dpnd depend on verb */
		if (check_feature(g_ptr->f, "VV") ||
		     check_feature(g_ptr->f, "VC") ||
		     check_feature(g_ptr->f, "VE") ||
		     check_feature(g_ptr->f, "VA")) {
		    one_score += verb_num == 1 ? 10 : 5;
		}
		/* add score for stable dpnd */
		if (cky_ptr->direction == LtoR) {	    
		    if (d_ptr->num + 1 == g_ptr->num &&
			(((check_feature(d_ptr->f, "AD")) &&
			  (check_feature(g_ptr->f, "CD") ||
			   check_feature(g_ptr->f, "DEV") ||
			   check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "JJ"))) ||
			 
			 ((check_feature(d_ptr->f, "CC")) &&
			  (check_feature(g_ptr->f, "CD") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "JJ"))) ||
			 
			 ((check_feature(d_ptr->f, "CD")) &&
			  (check_feature(g_ptr->f, "LC") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "M"))) ||
			 
			 ((check_feature(d_ptr->f, "DEG")) &&
			  (check_feature(g_ptr->f, "NR"))) ||

			 ((check_feature(d_ptr->f, "JJ")) &&
			  (check_feature(g_ptr->f, "NN"))) ||
			 
			 ((check_feature(d_ptr->f, "DEV")) &&
			   check_feature(g_ptr->f, "VV")) ||
			 
			 ((check_feature(d_ptr->f, "JJ")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "NR"))) ||
			 
			 ((check_feature(d_ptr->f, "LC")) &&
			  (check_feature(g_ptr->f, "DEG"))) ||
			 
			 ((check_feature(d_ptr->f, "M")) &&
			  (check_feature(g_ptr->f, "LC"))) ||
			 
			 ((check_feature(d_ptr->f, "NN")) &&
			  (check_feature(g_ptr->f, "DEV") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "LC"))) ||
			 
			 ((check_feature(d_ptr->f, "NR-SHORT")) &&
			  (check_feature(g_ptr->f, "NR"))) ||
			 
			 ((check_feature(d_ptr->f, "NR")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "LC"))) ||

			 ((check_feature(d_ptr->f, "NT")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "VV") ||
			   check_feature(g_ptr->f, "NT"))) ||

			 ((check_feature(d_ptr->f, "OD")) &&
			  (check_feature(g_ptr->f, "JJ") ||
			   check_feature(g_ptr->f, "M"))) ||

			 ((check_feature(d_ptr->f, "PN")) &&
			  (check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "DEG"))) ||

			 ((check_feature(d_ptr->f, "SB")) &&
			  (check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "VA")) &&
			  (check_feature(g_ptr->f, "DEV"))))) {
			one_score += 20;
		    }
		    else if (d_ptr->num < g_ptr->num && 
			     (((check_feature(d_ptr->f, "CC")) &&
			       (check_feature(g_ptr->f, "M"))))) {
			one_score += 10;
		    }
		}
		else if (cky_ptr->direction == RtoL) {
		    if (g_ptr->num + 1 == d_ptr->num && 
			(((check_feature(d_ptr->f, "AS")) &&
			  (check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "CD")) &&
			  (check_feature(g_ptr->f, "DT"))) ||

			 ((check_feature(d_ptr->f, "PN")) && 
			  (check_feature(g_ptr->f, "P"))) ||

			 ((check_feature(d_ptr->f, "DEC")) &&
			  (check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "DER")) &&
			  (check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "VC") ||
			   check_feature(d_ptr->f, "VA") ||
			   check_feature(d_ptr->f, "VE") ||
			   check_feature(d_ptr->f, "VV")) && 
			  (check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "ETC")) &&
			  (check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR"))))) {
			one_score += 20;
		    }
		    else if (g_ptr->num < d_ptr->num && 
			     (((check_feature(d_ptr->f, "DEC")) &&
			       (check_feature(g_ptr->f, "VC") ||
				check_feature(g_ptr->f, "VA") ||
				check_feature(g_ptr->f, "VE") ||
				check_feature(g_ptr->f, "VV")))||

			      ((check_feature(d_ptr->f, "NN") ||
				check_feature(d_ptr->f, "LC")) &&
			       (check_feature(g_ptr->f, "P"))) ||

			      ((check_feature(d_ptr->f, "VC") ||
				check_feature(d_ptr->f, "VA") ||
				check_feature(d_ptr->f, "VE") ||
				check_feature(d_ptr->f, "VV")) && 
			       (check_feature(g_ptr->f, "VC") ||
				check_feature(g_ptr->f, "VA") ||
				check_feature(g_ptr->f, "VE") ||
				check_feature(g_ptr->f, "VV"))))) {
			one_score += 10;
		    }
		}
	    }
	}

	cky_ptr = cky_ptr->direction == LtoR ? cky_ptr->right : cky_ptr->left;
    }

    /* �Ѹ��ξ�硤�ǽ�Ū��̤��,����,���,�˳�,Ϣ�ν������Ф���
       ����,���,�˳ʤΥ����å�ʬ����������Ϳ���� */
    if (pred_p) {
	one_score += check_scase(g_ptr, scase_check, 0, *un_count);
    }

    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f\n", one_score);
    }

    return one_score;
}

/* make an array of dependency possibilities */
void make_work_mgr_dpnd_check(SENTENCE_DATA *sp, CKY *cky_ptr, BNST_DATA *d_ptr) {
    int i, count = 0, start;

/*    if (cky_ptr->right && cky_ptr->right->dpnd_type == 'P')
	start = cky_ptr->right->j;
	else */
	start = d_ptr->num + 1;

    for (i = start; i < sp->Bnst_num; i++) {
	if (check_dpnd_possibility(sp, d_ptr->num, i, -1, ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
	    Work_mgr.dpnd.check[d_ptr->num].pos[count] = i;
	    count++;
	}
    }

    Work_mgr.dpnd.check[d_ptr->num].num = count;
}

/* count dependency possibilities */
int count_distance(SENTENCE_DATA *sp, CKY *cky_ptr, BNST_DATA *g_ptr, int *pos) {
    int i, count = 0;
    *pos = 0;

    for (i = cky_ptr->left->b_ptr->num + 1; i < sp->Bnst_num; i++) {
	if (check_dpnd_possibility(sp, cky_ptr->left->b_ptr->num, i, cky_ptr->i, 
				   ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
	    if (i == g_ptr->num) {
		*pos = count;
	    }
	    count++;
	}
    }

    return count;
}

/* scoring function based on case structure probabilities */
double calc_case_probability(SENTENCE_DATA *sp, CKY *cky_ptr, TOTAL_MGR *Best_mgr) {
    CKY *right_ptr = cky_ptr->right, *orig_cky_ptr = cky_ptr;
    BNST_DATA *g_ptr = cky_ptr->b_ptr, *d_ptr;
    TAG_DATA *t_ptr;
    CF_PRED_MGR *cpm_ptr, *pre_cpm_ptr;
    int i, pred_p = 0, count, pos, default_pos, child_num = 0;
    int renyou_modifying_num = 0, adverb_modifying_num = 0, noun_modifying_num = 0, flag;
    double one_score = 0, orig_score;
    char *para_key;

    /* �оݤ��Ѹ��ʳ��Υ������򽸤�� (right�򤿤ɤ�ʤ���left�Υ�������­��) */
    while (cky_ptr) {
	if (cky_ptr->left) {
	    one_score += cky_ptr->left->score;
	}
	cky_ptr = cky_ptr->right;
    }
    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f=>", one_score);
    }

    cky_ptr = orig_cky_ptr;

    if (check_feature(g_ptr->f, "����ñ�̼�:-1") && g_ptr->tag_num > 1) { /* ���Τ� */
	t_ptr = g_ptr->tag_ptr + g_ptr->tag_num - 2;
    }
    else {
	t_ptr = g_ptr->tag_ptr + g_ptr->tag_num - 1;
    }

    if (t_ptr->cf_num > 0) { /* predicate or something which has case frames */
	pred_p = 1;
	cpm_ptr = &(cky_ptr->cpm);
	cpm_ptr->pred_b_ptr = t_ptr;
	cpm_ptr->score = -1;
	cpm_ptr->result_num = 0;
	cpm_ptr->tie_num = 0;
	cpm_ptr->cmm[0].cf_ptr = NULL;
	cpm_ptr->decided = CF_UNDECIDED;

	cpm_ptr->cf.pred_b_ptr = t_ptr;
	t_ptr->cpm_ptr = cpm_ptr;
	cpm_ptr->cf.element_num = 0;

	set_data_cf_type(cpm_ptr); /* set predicate type */
    }
    else {
	cky_ptr->cpm.pred_b_ptr = NULL;
    }

    /* check each child */
    while (cky_ptr) {
	if (cky_ptr->left && cky_ptr->para_flag == 0) {
	    d_ptr = cky_ptr->left->b_ptr;
	    flag = 0;

	    /* relax penalty */
	    if (cky_ptr->dpnd_type == 'R') {
		one_score += -1000;
	    }

	    /* coordination */
	    if (OptParaFix == 0) {
		if (d_ptr->para_num != -1 && (para_key = check_feature(d_ptr->f, "�¥�"))) {
		    if (cky_ptr->para_score >= 0) {
			one_score += get_para_exist_probability(para_key, cky_ptr->para_score, TRUE);
			one_score += get_para_ex_probability(para_key, d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
			flag++;
		    }
		    else {
			one_score += get_para_exist_probability(para_key, sp->para_data[d_ptr->para_num].max_score, FALSE);
		    }
		}
	    }

	    /* case component */
	    if (cky_ptr->dpnd_type != 'P' && pred_p) {
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		if (make_data_cframe_child(sp, cpm_ptr, d_ptr->tag_ptr + d_ptr->tag_num - 1, child_num, t_ptr->num == d_ptr->num + 1 ? TRUE : FALSE)) {
		    child_num++;
		    flag++;
		}

		if ((check_feature(d_ptr->f, "��:Ϣ��") && 
		     (!check_feature(d_ptr->f, "�Ѹ�") || !check_feature(d_ptr->f, "ʣ�缭"))) || 
		    check_feature(d_ptr->f, "����")) {
		    flag++;
		}
	    }

	    /* clausal modifiee */
	    if (check_feature(d_ptr->f, "��:Ϣ��")) {
		pre_cpm_ptr = &(cky_ptr->left->cpm);
		pre_cpm_ptr->pred_b_ptr->cpm_ptr = pre_cpm_ptr;
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		make_data_cframe_rentai_simple(pre_cpm_ptr, pre_cpm_ptr->pred_b_ptr, t_ptr);

		orig_score = pre_cpm_ptr->score;
		one_score -= orig_score;
		one_score += find_best_cf(sp, pre_cpm_ptr, get_closest_case_component(sp, pre_cpm_ptr), 1);
		pre_cpm_ptr->score = orig_score;
		pre_cpm_ptr->cf.element_num--;
		flag++;
	    }

	    if (flag == 0) { /* ̾��ʥե졼��� */
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		one_score += get_noun_co_ex_probability(d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
		noun_modifying_num++;

		/* 
		one_score += FREQ0_ASSINED_SCORE;
		count = count_distance(sp, cky_ptr, g_ptr, &pos);
		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;
		one_score -= abs(default_pos - 1 - pos) * 5;
		*/
	    }

	    /* penalty of adverb etc. (tentative) */
	    if (check_feature(d_ptr->f, "��:Ϣ��") && !check_feature(d_ptr->f, "�Ѹ�")) {
		count = count_distance(sp, cky_ptr, g_ptr, &pos);
		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;
		one_score -= abs(default_pos - 1 - pos) * 5;
	    }
	}
	cky_ptr = cky_ptr->right;
    }

    /* one_score += get_noun_co_num_probability(t_ptr, noun_modifying_num); */

    if (pred_p) {
	t_ptr->cpm_ptr = cpm_ptr;

	/* �Ѹ�ʸ�᤬�֡ʡ���ˡ��ˡפΤȤ� 
	   �֤���פγʥե졼����Ф��ƥ˳�(Ʊʸ��)������
	   ��ʤϻҶ��ν����ǰ����� */
	if (check_feature(t_ptr->f, "���Ѹ�Ʊʸ��")) {
	    if (_make_data_cframe_pp(cpm_ptr, t_ptr, TRUE)) {
		_make_data_cframe_sm(cpm_ptr, t_ptr);
		_make_data_cframe_ex(cpm_ptr, t_ptr);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = t_ptr;
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = child_num;
		cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
		cpm_ptr->cf.element_num++;
	    }
	}

	/* call case structure analysis */
	one_score += find_best_cf(sp, cpm_ptr, get_closest_case_component(sp, cpm_ptr), 1);

	/* for each child */
	cky_ptr = orig_cky_ptr;
	while (cky_ptr) {
	    if (cky_ptr->left) {
		d_ptr = cky_ptr->left->b_ptr;
		if (cky_ptr->dpnd_type != 'P') {
		    /* modifying predicate */
		    if (check_feature(d_ptr->f, "��:Ϣ��") && check_feature(d_ptr->f, "�Ѹ�") && 
			!check_feature(d_ptr->f, "ʣ�缭")) {
			make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
			one_score += calc_vp_modifying_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, 
								   d_ptr->tag_ptr + d_ptr->tag_num - 1, 
								   cky_ptr->left->cpm.cmm[0].cf_ptr);
			renyou_modifying_num++;
		    }

		    /* modifying adverb */
		    if ((check_feature(d_ptr->f, "��:Ϣ��") && !check_feature(d_ptr->f, "�Ѹ�")) || 
			check_feature(d_ptr->f, "����")) {
			make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
			one_score += calc_adv_modifying_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, 
								    d_ptr->tag_ptr + d_ptr->tag_num - 1);
			adverb_modifying_num++;
		    }
		}
	    }
	    cky_ptr = cky_ptr->right;
	}

	one_score += calc_vp_modifying_num_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, renyou_modifying_num);
	one_score += calc_adv_modifying_num_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, adverb_modifying_num);
    }

    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f\n", one_score);
    }

    return one_score;
}

int relax_barrier_for_P(CKY *cky_ptr, int dep, int gov, int *dep_check) {
    while (cky_ptr) {
	if (cky_ptr->left && 
	    cky_ptr->dpnd_type == 'P' && 
	    *(dep_check + dep) >= cky_ptr->left->i) {
	    return TRUE;
	}
	cky_ptr = cky_ptr->right;
    }

    return FALSE;
}

int relax_dpnd_for_P(CKY *cky_ptr, int dep, int gov) {
    int i;

    while (cky_ptr) {
	if (cky_ptr->left && 
	    cky_ptr->dpnd_type == 'P') {
	    for (i = cky_ptr->left->i; i <= cky_ptr->left->j; i++) {
		if (Dpnd_matrix[dep][i] && Quote_matrix[dep][i]) {
		    return TRUE;
		}
	    }
	}
	cky_ptr = cky_ptr->right;
    }

    return FALSE;
}

void fix_predicate_coordination(SENTENCE_DATA* sp) {
    int i, j, k;

    for (i = 0; i < sp->Para_num; i++) {
	if (sp->para_data[i].type == PARA_KEY_P) { /* predicate coordination */
	    if (sp->para_data[i].status == 'x') {
		sp->para_data[i].max_score = -1;
	    }

	    /* modify Para_matrix */
	    for (j = 0; j < sp->Bnst_num; j++) {
		for (k = 0; k < sp->Bnst_num; k++) {
		    /* preserve the best coordination */
		    if (sp->para_data[i].status == 'x' || 
			j != sp->para_data[i].max_path[0] || k != sp->para_data[i].jend_pos) { 
			Para_matrix[i][j][k] = -1;
		    }
		}
	    }

	    /* modify Dpnd_matrix */
	    if (sp->para_data[i].status != 'x') {
		for (j = sp->para_data[i].key_pos + 1; j < sp->Bnst_num; j++) {
		    if (j == sp->para_data[i].jend_pos) {
			Dpnd_matrix[sp->para_data[i].key_pos][j] = 'R';
		    }
		    else {
			Dpnd_matrix[sp->para_data[i].key_pos][j] = 0;
		    }
		}
	    }
	}
    }
}


void discard_bad_coordination(SENTENCE_DATA* sp) {
    int i, j, k;

    for (i = 0; i < sp->Para_num; i++) {
	if (sp->para_data[i].status == 'x') {
	    for (j = 0; j < sp->Bnst_num; j++) {
		for (k = 0; k < sp->Bnst_num; k++) {
		    Para_matrix[i][j][k] = -1;
		}
	    }
	}
    }
}

void handle_incomplete_coordination(SENTENCE_DATA* sp) {
    int i, j;

    for (i = 0; i < sp->Bnst_num; i++) {
	for (j = 0; j < sp->Bnst_num; j++) {
	    if (Mask_matrix[i][j] == 3 && 
		Dpnd_matrix[i][j] == 0) {
		Dpnd_matrix[i][j] = (int)'I';
	    }
	}
    }
}

void extend_para_matrix(SENTENCE_DATA* sp) {
    int i, j, k, l, flag, offset, max_pos;
    double max_score;

    for (i = 0; i < sp->Para_num; i++) {
	if (sp->para_data[i].max_score >= 0) {
	    if (sp->para_data[i].type == PARA_KEY_P) {
		offset = 0;
	    }
	    else { /* in case of noun coordination, only permit modifiers to the words before the para key */
		offset = 1;
	    }

	    /* for each endpos */
	    for (l = sp->para_data[i].key_pos + 1; l < sp->Bnst_num; l++) {
		max_score = -INT_MAX;
		for (j = sp->para_data[i].iend_pos; j >= 0; j--) {
		    if (max_score < Para_matrix[i][j][l]) {
			max_score = Para_matrix[i][j][l];
			max_pos = j;
		    }
		}
		if (max_score >= 0) {
		    /* go up to search modifiers */
		    for (j = max_pos - 1; j >= 0; j--) {
			if (check_stop_extend(sp, j)) { /* extention stop */
			    break;
			}

			/* check dependency to pre-conjunct */
			flag = 0;
			for (k = j + 1; k <= sp->para_data[i].key_pos - offset; k++) {
			    if (Dpnd_matrix[j][k] && Quote_matrix[j][k]) {
				Para_matrix[i][j][l] = max_score;
				flag = 1;
				if (OptDisplay == OPT_DEBUG) {
				    printf("Para Extension (%s-%s-%s) -> %s\n", 
					   (sp->bnst_data + max_pos)->head_ptr->Goi, 
					   (sp->bnst_data + sp->para_data[i].key_pos)->head_ptr->Goi, 
					   (sp->bnst_data + l)->head_ptr->Goi, 
					   (sp->bnst_data + j)->head_ptr->Goi);
				}
				break;
			    }
			}
			if (flag == 0) {
			    break;
			}
		    }
		}
	    }
	}
    }
}

void set_cky(SENTENCE_DATA *sp, CKY *cky_ptr, CKY *left_ptr, CKY *right_ptr, int i, int j, int k, 
	     char dpnd_type, int direction) {
    int l;

    cky_ptr->i = i;
    cky_ptr->j = j;
    cky_ptr->next = NULL;
    cky_ptr->left = left_ptr;
    cky_ptr->right = right_ptr;
    cky_ptr->direction = direction;
    cky_ptr->dpnd_type = dpnd_type;
    cky_ptr->cp = 'a' + j;
    if (cky_ptr->direction == RtoL) {
	cky_ptr->b_ptr = cky_ptr->left->b_ptr;
    }
    else {
	cky_ptr->b_ptr = cky_ptr->right ? cky_ptr->right->b_ptr : sp->bnst_data + j;
    }
    cky_ptr->un_count = 0;
    for (l = 0; l < SCASE_CODE_SIZE; l++) cky_ptr->scase_check[l] = 0;
    cky_ptr->para_flag = 0;
    cky_ptr->para_score = -1;
    cky_ptr->score = 0;
}

CKY *new_cky_data(int *cky_table_num) {
    CKY *cky_ptr;

    cky_ptr = &(cky_table[*cky_table_num]);
    if (OptAnalysis == OPT_CASE && *cky_table_num > cpm_allocated_cky_num) {
	init_case_frame(&(cky_ptr->cpm.cf));
	cpm_allocated_cky_num = *cky_table_num;
    }

    (*cky_table_num)++;
    if (*cky_table_num >= CKY_TABLE_MAX) {
	fprintf(stderr, ";;; cky_table_num exceeded maximum\n");
	exit(1);
    }

    return cky_ptr;
}

int cky (SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr) {
    int i, j, k, l, sen_len, cky_table_num, dep_check[BNST_MAX];
    double best_score, para_score;
    char dpnd_type;
    CKY *cky_ptr, *left_ptr, *right_ptr, *best_ptr, *pre_ptr, *best_pre_ptr, *start_ptr;
    CKY **next_pp, **next_pp_for_ij;

    cky_table_num = 0;

    /* initialize */
    verb_num = 0;
    for (i = 0; i < sp->Bnst_num; i++) {
	dep_check[i] = -1;
	Best_mgr->dpnd.head[i] = -1;
	Best_mgr->dpnd.type[i] = 'D';
	if (Language == CHINESE && 
	    (check_feature((sp->bnst_data+i)->f, "VV") ||
	     check_feature((sp->bnst_data+i)->f, "VC") ||
	     check_feature((sp->bnst_data+i)->f, "VA") ||
	     check_feature((sp->bnst_data+i)->f, "VE"))) {
	    verb_num++;
	}
    }

    if (OptParaFix == 0) {
	discard_bad_coordination(sp);
	/* fix_predicate_coordination(sp); */
	/* extend_para_matrix(sp); */
	handle_incomplete_coordination(sp);
    }

    /* �롼�פϺ����鱦,�������
       i����j�ޤǤ����Ǥ�ޤȤ����� */
    for (j = 0; j < sp->Bnst_num; j++) { /* left to right (�����鱦) */
	for (i = j; i >= 0; i--) { /* bottom to top (�������) */
	    if (OptDisplay == OPT_DEBUG) {
		printf("(%d,%d)\n", i, j);
	    }
	    cky_matrix[i][j] = NULL;
	    if (i == j) {
		cky_ptr = new_cky_data(&cky_table_num);
		cky_matrix[i][j] = cky_ptr;

		set_cky(sp, cky_ptr, NULL, NULL, i, j, -1, 0, LtoR);
		cky_ptr->score = OptAnalysis == OPT_CASE ? 
		    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
	    }
	    else {
		next_pp_for_ij = NULL;	/* ���ΰ��֤˰�Ĥ�礬�Ǥ��Ƥʤ��� */

		/* merge (i .. i+k) and (i+k+1 .. j) */
		for (k = 0; k < j - i; k++) {
		    para_score = (sp->bnst_data + i + k)->para_num == -1 ? -1 : 
			Para_matrix[(sp->bnst_data + i + k)->para_num][i][j];
		    next_pp = NULL;
		    left_ptr = cky_matrix[i][i + k];
		    while (left_ptr) {
			right_ptr = cky_matrix[i + k + 1][j];
			while (right_ptr) {
			    /* make a phrase if condition is satisfied */
			    if ((dpnd_type = check_dpnd_possibility(sp, left_ptr->b_ptr->num, right_ptr->b_ptr->num, i, 
								    (j == sp->Bnst_num - 1) && dep_check[i + k] == -1 ? TRUE : FALSE)) && 
				(dpnd_type == 'P' || 
				 dep_check[i + k] <= 0 || /* no barrier */
				 dep_check[i + k] >= j || /* barrier */
				 relax_barrier_for_P(right_ptr, i + k, j, dep_check))) { /* barrier relaxation for P */
				cky_ptr = new_cky_data(&cky_table_num);
				if (next_pp == NULL) {
				    start_ptr = cky_ptr;
				}
				else {
				    *next_pp = cky_ptr;
				}

				set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, dpnd_type, 
					Dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num] == 'L' ? RtoL : LtoR);
				next_pp = &(cky_ptr->next);

				if (OptDisplay == OPT_DEBUG) {
				    printf("   (%d,%d), (%d,%d) b=%d [%s%s%s], %c(para=%.3f), score=", 
					   i, i + k, i + k + 1, j, dep_check[i + k], 
					   left_ptr->b_ptr->head_ptr->Goi, 
					   cky_ptr->direction == RtoL ? "<-" : "->", 
					   right_ptr->b_ptr->head_ptr->Goi, 
					   dpnd_type, para_score);
				}

				cky_ptr->para_score = para_score;
				cky_ptr->score = OptAnalysis == OPT_CASE ? 
				    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
			    }

			    /* if dpnd direction is B, check RtoL again */
			    if (Dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num] == 'B') {
				cky_ptr = new_cky_data(&cky_table_num);
				if (next_pp == NULL) {
				    start_ptr = cky_ptr;
				}
				else {
				    *next_pp = cky_ptr;
				}

				set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'B', RtoL);
				next_pp = &(cky_ptr->next);

				if (OptDisplay == OPT_DEBUG) {
				    printf("   (%d,%d), (%d,%d) [%s<-%s], score=", i, i + k, i + k + 1, j, 
					   left_ptr->b_ptr->head_ptr->Goi, 
					   right_ptr->b_ptr->head_ptr->Goi);
				}

				cky_ptr->para_score = para_score;
				cky_ptr->score = OptAnalysis == OPT_CASE ? 
				    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
			    }
			    right_ptr = right_ptr->next;
			}
			left_ptr = left_ptr->next;
		    }

		    if (next_pp) {
			/* choose the best one */
			cky_ptr = start_ptr;
			best_score = -INT_MAX;
			while (cky_ptr) {
			    if (cky_ptr->score > best_score) {
				best_score = cky_ptr->score;
				best_ptr = cky_ptr;
			    }
			    cky_ptr = cky_ptr->next;
			}
			start_ptr = best_ptr;
			start_ptr->next = NULL;

			if (next_pp_for_ij == NULL) {
			    cky_matrix[i][j] = start_ptr;
			}
			else {
			    *next_pp_for_ij = start_ptr;
			}
			next_pp_for_ij = &(start_ptr->next);

			/* barrier handling */
			if (j != sp->Bnst_num - 1) { /* don't check in case of relaxation */
			    if ((OptParaFix || Dpnd_matrix[i + k][j]) &&  /* don't set barrier in case of P */
				(sp->bnst_data + i + k)->dpnd_rule->barrier.fp[0] && 
				feature_pattern_match(&((sp->bnst_data + i + k)->dpnd_rule->barrier), 
						      (sp->bnst_data + j)->f, 
						      sp->bnst_data + i + k, sp->bnst_data + j) == TRUE) {
				dep_check[i + k] = j; /* set barrier */
			    }
			    else if (dep_check[i + k] == -1) {
				if (Language != CHINESE && 
				    (OptParaFix || Dpnd_matrix[i + k][j]) && /* don't set barrier in case of P */
				    (sp->bnst_data + i + k)->dpnd_rule->preference != -1 && 
				    (sp->bnst_data + i + k)->dpnd_rule->barrier.fp[0] == NULL) { /* no condition */
				    dep_check[i + k] = j; /* set barrier */
				}
				else {
				    dep_check[i + k] = 0; /* ��������������ʤ��Ȥ�1�Ĥ���Ω�������Ȥ򼨤� */
				}
			    }
			}
		    }
		}

		/* coordination that consists of more than 2 phrases */
		if (OptParaFix == 0) {
		    for (k = 1; k < j - i; k++) {
			next_pp = NULL;
			left_ptr = cky_matrix[i][i + k];
			while (left_ptr) {
			    if (left_ptr->dpnd_type == 'P') {
				right_ptr = cky_matrix[left_ptr->right->i][j];
				if (OptDisplay == OPT_DEBUG) {
				    printf("** (%d,%d), (%d,%d) b=%d [%s--%s], P(para=--), score=", 
					   i, i + k, left_ptr->right->i, j, dep_check[i + k], 
					   (sp->bnst_data + i)->head_ptr->Goi, 
					   (sp->bnst_data + j)->head_ptr->Goi);
				}
				while (right_ptr) {
				    if (right_ptr->dpnd_type == 'P') {
					cky_ptr = new_cky_data(&cky_table_num);
					if (next_pp == NULL) {
					    start_ptr = cky_ptr;
					}
					else {
					    *next_pp = cky_ptr;
					}

					set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'P', LtoR);
					next_pp = &(cky_ptr->next);

					cky_ptr->para_flag = 1;
					cky_ptr->para_score = cky_ptr->left->para_score + cky_ptr->right->para_score;
					cky_ptr->score = OptAnalysis == OPT_CASE ? 
					    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
				    }
				    right_ptr = right_ptr->next;
				}
				if (OptDisplay == OPT_DEBUG) {
				    printf("\n");
				}
			    }
			    left_ptr = left_ptr->next;
			}

			if (next_pp) {
			    /* choose the best one */
			    cky_ptr = start_ptr;
			    best_score = -INT_MAX;
			    while (cky_ptr) {
				if (cky_ptr->score > best_score) {
				    best_score = cky_ptr->score;
				    best_ptr = cky_ptr;
				}
				cky_ptr = cky_ptr->next;
			    }
			    start_ptr = best_ptr;
			    start_ptr->next = NULL;

			    if (next_pp_for_ij == NULL) {
				cky_matrix[i][j] = start_ptr;
			    }
			    else {
				*next_pp_for_ij = start_ptr;
			    }
			    next_pp_for_ij = &(start_ptr->next);
			}
		    }
		}
	    }
	}
    }

    if (OptDisplay == OPT_DEBUG) {
	printf(">>> n=%d\n", cky_table_num);
    }

    /* choose the best one */
    cky_ptr = cky_matrix[0][sp->Bnst_num - 1];
    if (!cky_ptr) {
	return FALSE;
    }

    if (cky_ptr->next) { /* if there are more than one possibility */
	best_score = -INT_MAX;
	pre_ptr = NULL;
	while (cky_ptr) {
	    if (cky_ptr->score > best_score) {
		best_score = cky_ptr->score;
		best_ptr = cky_ptr;
		best_pre_ptr = pre_ptr;
	    }
	    pre_ptr = cky_ptr;
	    cky_ptr = cky_ptr->next;
	}
	if (pre_ptr != best_ptr) {
	    if (best_pre_ptr) {
		best_pre_ptr->next = best_ptr->next;
	    }
	    else {
		cky_matrix[0][sp->Bnst_num - 1] = cky_matrix[0][sp->Bnst_num - 1]->next;
	    }
	    pre_ptr->next = best_ptr; /* move the best one to the end of the list */
	    best_ptr->next = NULL;
	}

	/* cky_ptr = cky_matrix[0][sp->Bnst_num - 1]; * when print all possible structures */
	cky_ptr = best_ptr;
    }

    /* count the number of predicates */
    Best_mgr->pred_num = 0;
    for (i = 0; i < sp->Tag_num; i++) {
	if ((sp->tag_data + i)->cf_num > 0 && 
	    ((sp->tag_data + i)->inum == 0 || /* the last basic phrase in a bunsetsu */
	     ((sp->tag_data + i)->inum == 1 && 
	      check_feature((sp->tag_data + i)->b_ptr->f, "����ñ�̼�:-1")))) { 
	    (sp->tag_data + i)->pred_num = Best_mgr->pred_num;
	    Best_mgr->cpm[Best_mgr->pred_num].pred_b_ptr = NULL;
	    Best_mgr->pred_num++;
	}
    }

    while (cky_ptr) {
	if (OptDisplay == OPT_DEBUG) {
	    printf("---------------------\n");
	    printf("score=%.3f\n", cky_ptr->score);
	}

	Best_mgr->dpnd.head[cky_ptr->b_ptr->num] = -1;
	Best_mgr->score = cky_ptr->score;
	convert_to_dpnd(Best_mgr, cky_ptr);
	cky_ptr = cky_ptr->next;
    }

    /* ̵�ʽ�°: ����ʸ��η�������˽������ */
    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if (Best_mgr->dpnd.head[i] < 0) {
	    /* ���ꤨ�ʤ�������� */
	    if (i >= Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]]) {
		continue;
	    }
	    Best_mgr->dpnd.head[i] = Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]];
	    /* Best_mgr->dpnd.check[i].pos[0] = Best_mgr->dpnd.head[i]; */
	}
    }

    return TRUE;
}
/*====================================================================

			     共参照解析

                                               R.SASANO 05. 9.24

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
	       void assign_anaphor_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 照応詞候補というfeatureを付与する */
    /* 照応詞候補とは複合名詞の末尾となっている語のこと */

    /* 強い複合名詞の基準 */
    /* 	固有表現 */
    /* 	末尾の語がサ変名詞であれば切る */
    /* 	末尾の語とその前の語が名詞格フレームにある組合せの場合は切る */
    /* 	末尾の語のが以下の語であれば切る */
    /* 		全体 一部 全部 */

    int i, j, k, tag_num;
    char word[64], buf[64], *cp;
    TAG_DATA *tag_ptr;
    
    for (i = 0; i < sp->Bnst_num; i++) {
	
	if (!check_feature((sp->bnst_data + i)->f, "体言")) continue;

	tag_num = (sp->bnst_data+i)->tag_num;
	tag_ptr = (sp->bnst_data + i)->tag_ptr;

 	for (j = tag_num - 1; j >= 0; j--) {

	    /* 固有表現内である場合は後回し */
	    if (check_feature((tag_ptr + j)->f, "NE") ||
		check_feature(((tag_ptr + j)->mrph_ptr + 
			       (tag_ptr + j)->mrph_num - 1)->f, "NE")) {
		break;
	    }

 	    if (/* 主辞である */
		j == tag_num - 1 ||
		
		/* 直後の名詞がサ変名詞である */
		((tag_ptr + j + 1)->mrph_ptr)->Hinshi == 6 &&
		((tag_ptr + j + 1)->mrph_ptr)->Bunrui == 2 ||

		/* 直後の名詞の格フレームの用例に存在する */
		(tag_ptr + j + 1)->cf_ptr &&
		check_examples(((tag_ptr + j)->mrph_ptr)->Goi2,
				((tag_ptr + j + 1)->cf_ptr)->ex_list[0],
				((tag_ptr + j + 1)->cf_ptr)->ex_num[0]) >= 0) {

		memset(word, 0, sizeof(char)*64);
		for (k = (tag_ptr + j)->head_ptr - (sp->bnst_data + i)->mrph_ptr; 
		     k >= 0; k--) {

		    /* 名詞接頭辞は含めない */
		    if (((tag_ptr + j)->head_ptr - k)->Hinshi == 13 &&
			((tag_ptr + j)->head_ptr - k)->Bunrui == 1) break;

		    if (!(tag_ptr + j)->anaphor_mrph_num) {
			(tag_ptr + j)->anaphor_mrph_num = k + 1;
		    }

		    strcat(word, ((tag_ptr + j)->head_ptr - k)->Goi2);
		}
		sprintf(buf, "照応詞候補:%s", word);
		assign_cfeature(&((tag_ptr + j)->f), buf);
	    } else {
		break;
	    }
	}
	
 	for (j = tag_num - 1; j >= 0; j--) {   
	    /* 固有表現の主辞には付与 */
	    if ((cp = check_feature((tag_ptr + j)->f, "NE"))) {
		while (strncmp(cp, ")", 1)) cp++;
		sprintf(buf, "照応詞候補:%s", cp + 1);
		assign_cfeature(&((tag_ptr + j)->f), buf);
		(tag_ptr + j)->anaphor_mrph_num = (tag_ptr + j)->proper_mrph_num;
		continue;
	    }
	}   
    }
}

/*==================================================================*/
               int search_antecedent(SENTENCE_DATA *sp, int i, 
				     char *anaphor, char *setubi)
/*==================================================================*/
{
    /* 用言とその省略格が与えられる */

    /* cf_ptr = cpm_ptr->cmm[0].cf_ptr である */
    /* 用言 cpm_ptr の cf_ptr->pp[n][0] 格が省略されている
       cf_ptr->ex[n] に似ている文節を探す */

    int j, k, l, anaphor_mrph_num;
    char word[64], buf[64];
    SENTENCE_DATA *sdp;
    TAG_DATA *tag_ptr;
 
    sdp = sentence_data + sp->Sen_num - 1;
    for (j = 0; j <= sdp - sentence_data; j++) { /* 照応先が何文前か */
	
	for (k = j ? (sdp - j)->Tag_num - 1 : i - 1;
	     k >= (sp->tag_data + i)->anaphor_mrph_num - 1; k--) { /* 照応先のタグ */
	    
	    tag_ptr = (sdp - j)->tag_data + k;	    		
	    if (!check_feature((tag_ptr)->f, "体言")) continue;

	    memset(word, 0, sizeof(char)*52);
	    anaphor_mrph_num = (sp->tag_data + i)->anaphor_mrph_num;

	    /* 後続の名詞性接尾じを付与 */
	    if (setubi && strcmp((tag_ptr->head_ptr + 1)->Goi2, setubi))
		continue;
		
	    for (l = anaphor_mrph_num - 1; l >= 0; l--) 
		strcat(word, (tag_ptr->head_ptr - l)->Goi2); /* 先行詞候補 */
	    
	    if (!strcmp(anaphor, word)) { /* 同表記の語を先行詞とみなす */
		if (j == 0) {
		    sprintf(buf, "C用;【%s%s】;=;0;%d;9.99:%s(同一文):%d文節",
			    word, setubi ? setubi : "", k, sp->KNPSID ? sp->KNPSID + 5 : "?", k);
		}
		else {
		    sprintf(buf, "C用;【%s%s】;=;%d;%d;9.99:%s(%d文前):%d文節",
			    word, setubi ? setubi : "", j, k, sp->KNPSID ? sp->KNPSID + 5 : "?", j, k);
		}
		assign_cfeature(&((sp->tag_data + i)->f), buf);

		/* 複数のタグに関係する場合の処理 */
		for (l = 0; l < anaphor_mrph_num; i--) 
		    l += (sp->tag_data + i)->mrph_num;
		return i;
	    }
	}	    
    }
    return -2;
}

/*==================================================================*/
	       void corefer_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, next_i;
    char *anaphor;
    MRPH_DATA *mrph_ptr;
    
    for (i = sp->Tag_num - 1; i >= 0; i--) { /* 解析文のタグ単位 */

	if (!(anaphor = check_feature((sp->tag_data + i)->f, "照応詞候補")) ||
	    !check_feature((sp->tag_data + i)->f, "NE") &&
	    ((sp->tag_data + i)->b_ptr)->child[0]) /* 修飾されている */
	    continue;

	if (check_feature((sp->tag_data + i)->f, "指示詞")) {
	    /* 指示詞の場合 */
	    continue;
	}
	
	mrph_ptr = (sp->tag_data + i)->head_ptr + 1; /* 主辞の次の形態素 */
	if (mrph_ptr->Hinshi == 14 && mrph_ptr->Bunrui < 5) {
	    /* 名詞性接尾辞が付いている場合 */
	    next_i = search_antecedent(sp, i, anaphor+11, mrph_ptr->Goi2);
	    if (next_i != -2) {
		i = next_i;
		continue;
	    }
	}
	next_i = search_antecedent(sp, i, anaphor+11, NULL);
	i = (next_i == -2) ? i : next_i;
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			     固有名詞処理

                                               S.Kurohashi 96. 7. 4

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
		   char *NEcheck(BNST_DATA *b_ptr)
/*==================================================================*/
{
    char *class;

    if (!(class = check_feature(b_ptr->f, "人名")) && 
	!(class = check_feature(b_ptr->f, "地名")) && 
	!(class = check_feature(b_ptr->f, "組織名")) && 
	!(class = check_feature(b_ptr->f, "固有名詞"))) {
	return NULL;
    }
    return class;
}

/*==================================================================*/
	 int NEparaCheck(BNST_DATA *b_ptr, BNST_DATA *h_ptr)
/*==================================================================*/
{
    char *class, str[11];

    /* b_ptr が固有表現である */
    if ((class = NEcheck(b_ptr)) == NULL) {
	return FALSE;
    }

    sprintf(str, "%s疑", class);

    /* h_ptr は固有表現ではない
       class と一致する「〜疑」がついている */
    if (NEcheck(h_ptr) || 
	!check_feature(h_ptr->f, str)) {
	return FALSE;
    }

    /* h_ptr を同じ固有表現クラスにする */
    assign_cfeature(&(h_ptr->f), class);
    assign_cfeature(&(h_ptr->f), "固並列OK");

    return TRUE;
}

/*==================================================================*/
	       void ne_para_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, value;
    char *cp;

    for (i = 0; i < sp->Bnst_num; i++) {
	/* 住所が入る並列はやめておく */
	if (check_feature(sp->bnst_data[i].f, "住所") || 
	    !check_feature(sp->bnst_data[i].f, "体言") || 
	    (sp->bnst_data[i].dpnd_head > 0 && 
	     !check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "体言"))) {
	    continue;
	}
	cp = check_feature(sp->bnst_data[i].f, "並結句数");
	if (cp) {
	    value = atoi(cp+strlen("並結句数:"));
	    if (value > 2) {
		/* 係り側を調べて固有名詞じゃなかったら、受け側を調べる */
		if (NEparaCheck(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
		    NEparaCheck(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
		continue;
	    }
	}

	cp = check_feature(sp->bnst_data[i].f, "並結文節数");
	if (cp) {
	    value = atoi(cp+strlen("並結文節数:"));
	    if (value > 1) {
		if (NEparaCheck(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
		    NEparaCheck(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
	    }
	}
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

		      unsupervised learning 関連

                                             S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include "knp.h"

void CheckCandidates(SENTENCE_DATA *sp)
{
    int i, j;
    TOTAL_MGR *tm = &Best_mgr;
    char buffer[DATA_LEN], buffer2[256], *cp;

    /* 各文節ごとにチェック用の feature を与える */
    for (i = 0; i < sp->Bnst_num; i++)
	if (tm->dpnd.check[i].num != -1) {
	    /* 係り側 -> 係り先 */
	    sprintf(buffer, "候補");
	    for (j = 0; j < tm->dpnd.check[i].num; j++) {

		/* テスト -check -optionalcase デ格 */
		if (OptOptionalCase) {
		    if ((cp = (char *)check_feature(sp->bnst_data[i].f, "係")) != NULL) {
			if (str_eq(cp+3, OptOptionalCase)) {
			    corpus_optional_case_comp(sp, sp->bnst_data+i, cp+3, sp->bnst_data[tm->dpnd.check[i].pos[j]], NULL);
			}
		    }
		}

		/* 候補ども */
		sprintf(buffer2, ":%d", tm->dpnd.check[i].pos[j]);
		if (strlen(buffer)+strlen(buffer2) >= DATA_LEN) {
		    fprintf(stderr, "Too long string <%s> (%d) in CheckCandidates.\n", buffer, tm->dpnd.check[i].num);
		    return;
		}
		strcat(buffer, buffer2);
	    }
	    assign_cfeature(&(sp->bnst_data[i].f), buffer);
	}
}

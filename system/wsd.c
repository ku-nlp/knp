/*====================================================================

                                 WSD

                                         Daisuke Kawahara 2014. 7. 25

    $Id$
====================================================================*/

#include "knp.h"

char	static_buffer[DATA_LEN];

/*==================================================================*/
            int print_features_for_wsd(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, second_print_flag = 0;
    MRPH_DATA *m_ptr;
    char *rep_str;

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        if ((rep_str = get_mrph_rep_from_f(m_ptr, FALSE))) {
            char *rep_id = rep2id(rep_str, strlen(rep_str), &(static_buffer[0]));
            if (rep_id[0]) {
                if (second_print_flag++)
                    fputc(' ', Outfp);
                fprintf(Outfp, "%s:1", rep_id);
            }
        }
    }
    fputc('\n', Outfp);
    return TRUE;
}

/*====================================================================
                               END
====================================================================*/

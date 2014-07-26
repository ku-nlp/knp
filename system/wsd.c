/*====================================================================

                                 WSD

                                         Daisuke Kawahara 2014. 7. 25

    $Id$
====================================================================*/

#include "knp.h"

char	static_buffer1[DATA_LEN];
char	static_buffer2[DATA_LEN];

/*==================================================================*/
                           void init_wsd()
/*==================================================================*/
{
    init_opal(KNP_DICT "/" WSD_MODEL_DIR_NAME);
}

/*==================================================================*/
                           void close_wsd()
/*==================================================================*/
{
    close_opal();
}

/*==================================================================*/
char *sprint_features_for_wsd(SENTENCE_DATA *sp, int print_class_label_flag)
/*==================================================================*/
{
    int i, second_print_flag = 0;
    MRPH_DATA *m_ptr;
    char *rep_str, *rep_id, *buf = static_buffer2;
    buf[0] = '\0';
    if (print_class_label_flag)
        strcat(buf, "0 ");

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        if ((rep_str = get_mrph_rep_from_f(m_ptr, FALSE))) {
            rep_id = rep2id(rep_str, strlen(rep_str), &(static_buffer1[0]));
            if (rep_id[0]) {
                if (second_print_flag++)
                    strcat(buf, " ");
                strcat(buf, rep_id);
                strcat(buf, ":1");
            }
        }
    }
    return buf;
}

/*==================================================================*/
            void print_features_for_wsd(SENTENCE_DATA *sp)
/*==================================================================*/
{
    fputs(sprint_features_for_wsd(sp, FALSE), Outfp);
    fputc('\n', Outfp);
}

/*==================================================================*/
                     void wsd(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, boundary_continue_flag, boundary_end_flag;
    MRPH_DATA *m_ptr;
    FEATURE *fp;
    char *cp, *start_cp, *orig_start_cp, *rep_id;
    char label[DATA_LEN], buf[DATA_LEN], pre_buf[DATA_LEN], feature_buf[DATA_LEN];

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
        pre_buf[0] = '\0';
        fp = m_ptr->f;
        while (fp) {
            if (!strncmp(fp->cp, "LD-type=Wikipedia", strlen("LD-type=Wikipedia")) && (cp = strstr(fp->cp, "RepForm="))) {
                buf[0] = '\0';
                cp += strlen("RepForm=");
                orig_start_cp = start_cp = cp;
                for (; ; cp++) {
                    boundary_continue_flag = boundary_end_flag = 0;
                    if (*cp == '+' || *cp == '?')
                        boundary_continue_flag = 1;
                    if (*cp == '_' || *cp == '>')
                        boundary_end_flag = 1;
                    if (boundary_continue_flag || boundary_end_flag) {
                        rep_id = rep2id(start_cp, cp - start_cp, &(static_buffer1[0]));
                        strcat(buf, rep_id);
                        if (boundary_continue_flag)
                            strncat(buf, cp, 1);
                        start_cp = cp + 1;
                    }
                    if (boundary_end_flag)
                        break;
                }

                if (pre_buf[0] && strcmp(pre_buf, buf) && 
                    opal_classify(pre_buf, sprint_features_for_wsd(sp, TRUE), label)) {
                    fprintf(Outfp, "TARGET %s -> LABEL %s\n", pre_buf, label);
                    sprintf(feature_buf, "語義曖昧性解消結果:%s", label);
                    assign_cfeature(&(m_ptr->f), feature_buf, FALSE);
                }
                strcpy(pre_buf, buf);
            }
            fp = fp->next;
        }
        if (pre_buf[0] && 
            opal_classify(pre_buf, sprint_features_for_wsd(sp, TRUE), label)) {
            fprintf(Outfp, "TARGET %s -> LABEL %s\n", pre_buf, label);
            sprintf(feature_buf, "語義曖昧性解消結果:%s", label);
            assign_cfeature(&(m_ptr->f), feature_buf, FALSE);
        }
    }
}

/*====================================================================
                               END
====================================================================*/

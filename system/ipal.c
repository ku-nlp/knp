/*
 *	ＩＰＡＬ データベース化
 *
 *	Sadao Kurohashi 1995/07/07
 */

#include <stdio.h>

/* from ipal.h */
#define IPAL_FIELD_NUM	72
#define IPAL_DATA_SIZE	128000
#define CASE_MAX_NUM	20

typedef struct {
    int point[IPAL_FIELD_NUM];
    unsigned char DATA[IPAL_DATA_SIZE];
} IPAL_TRANS_FRAME;

typedef struct {
    int id;			/* ID */
    int yomi;			/* 読み */
    int hyouki;			/* 表記 */
    int imi;			/* 意味 */
    int jyutugoso;		/* 述語素 */
    int kaku_keishiki[CASE_MAX_NUM];	/* 格形式 */
    int imisosei[CASE_MAX_NUM];		/* 意味素性 */
    int meishiku[CASE_MAX_NUM];		/* 名詞句 */
    int sase;			/* 態１ */
    int rare;			/* 態２ */
    int tyoku_noudou1;		/* 態３ */
    int tyoku_ukemi1;		/* 態４ */
    int tyoku_noudou2;		/* 態５ */
    int tyoku_ukemi2;		/* 態６ */
    int voice;			/* 態７ */
    unsigned char DATA[IPAL_DATA_SIZE];
} IPAL_FRAME;

char buffer[IPAL_DATA_SIZE];

IPAL_TRANS_FRAME ipal_frame;

void fprint_ipal_idx(FILE *fp, unsigned char *entry, 
		     unsigned char *hyouki, int address, int size, int flag)
{
    unsigned char output_buf[256];
    unsigned char *point;
    int length = 0;

    /* 読みをキーするとき */
    if (flag) {
	fprintf(fp, "%s %d:%d\n", entry, address, size);
    }
    else {
	fprintf(stderr, "%s was skipped.\n", entry);
    }

    for (point = hyouki; *point; point+=2) {

	if (*point == 0xa1 && *(point+1) == 0xa4) { /* "，" */
	    output_buf[length] = '\0';
	    if (!flag || strcmp(output_buf, entry)) {
		fprintf(fp, "%s %d:%d\n", output_buf, address, size);
		flag = 1;
	    }
	    length = 0;
	} else {
	    output_buf[length++] = *point;
	    output_buf[length++] = *(point+1);
	}
    }
    output_buf[length] = '\0';
    if (!flag || strcmp(output_buf, entry)) {
	fprintf(fp, "%s %d:%d\n", output_buf, address, size);
    }
}

main(int argc, char **argv)
{
    FILE *fp_idx, *fp_dat;
    char tag[256];
    int i, line, pos, address = 0, writesize, flag;

    if (argc < 3) {
	fprintf(stderr, "Usage: %s index-filename data-filename\n", argv[0]);
	exit(1);
    }
    if ( (fp_idx = fopen(argv[1], "w")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", argv[1]);
	exit(1);
    }
    if ( (fp_dat = fopen(argv[2], "w")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", argv[2]);
	exit(1);
    }

    line = 0;
    while (1) {

	/* データ読み込み */

	pos = 0;
	for (i = 0; i < IPAL_FIELD_NUM; i++, line++) {
	    
	    if (fgets(buffer, IPAL_DATA_SIZE, stdin) == NULL) {
		if (i != 0) {
		    fprintf(stderr, "Invalid data.\n");
		    exit(1);
		} else {
		    fclose(fp_idx);
		    fclose(fp_dat);
		    return 0;
		}
	    } else {
		sscanf(buffer, "%s %[^\n]\n", tag, &(ipal_frame.DATA[pos]));

		if (i == 0 && strcmp(tag, "ＩＤ")) {
		    fprintf(stderr, "Invalid data (around line %d).\n", line);
		    exit(1);
		}
	    }

	    ipal_frame.point[i] = pos;
	    if (!strcmp(&(ipal_frame.DATA[pos]), "nil")) {
		ipal_frame.DATA[pos] = '\0';
		pos += 1;
	    } else {
		pos += strlen(&(ipal_frame.DATA[pos])) + 1;
	    }
	    if (pos > IPAL_DATA_SIZE) {
		fprintf(stderr, "%d is small for IPAL record (%s).\n", 
			IPAL_DATA_SIZE, ipal_frame.DATA);
		exit(1);
	    }
	}

	writesize = sizeof(int)*IPAL_FIELD_NUM+pos;

	if (!strcmp(ipal_frame.DATA+ipal_frame.point[4], "和フレーム")) {
	    flag = 0;
	}
	else {
	    flag = 1;
	}

	/* アドレス書き出し */

	fprint_ipal_idx(fp_idx, 
			ipal_frame.DATA+ipal_frame.point[1], 
			ipal_frame.DATA+ipal_frame.point[2], 
			address, writesize, flag);

	/* データ書き出し */

	if (fwrite(&ipal_frame, writesize, 1, fp_dat) < 1) {
	    fprintf(stderr, "Error in fwrite.\n");
	    exit(1);
	}

	address += writesize;
    }
}


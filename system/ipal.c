/*
 *	ＩＰＡＬ データベース化
 *
 *	Sadao Kurohashi 1995/07/07
 */

#include <stdio.h>
#include <string.h>

/* from ipal.h */
#define IPAL_FIELD_NUM	64
#define IPAL_DATA_SIZE	256000
#define CASE_MAX_NUM	20

typedef struct {
    /* int point[IPAL_FIELD_NUM]; */
    unsigned char DATA[IPAL_DATA_SIZE];
} IPAL_TRANS_FRAME;

char buffer[IPAL_DATA_SIZE];

IPAL_TRANS_FRAME ipal_frame;
FILE *fp_idx, *fp_dat;


void fprint_ipal_idx(FILE *fp, unsigned char *entry, 
		     unsigned char *hyouki, unsigned char *pp, 
		     int address, int size, int flag)
{
    unsigned char output_buf[IPAL_DATA_SIZE];
    unsigned char *point;
    int length = 0;

    /* 読みをキーするとき */
    if (flag == 1) {
	fprintf(fp, "%s %d:%d\n", entry, address, size);
    }

    for (point = hyouki; *point; point++) {

	/* 用例の区切り */
	if (*point == ' ') {
	    output_buf[length] = '\0';
	    /* 読みと異なる場合に出力 */
	    if (flag != 1 || strcmp(output_buf, entry)) {
		if (pp) {
		    fprintf(fp, "%s-%s-%s %d:%d\n", output_buf, pp, entry, address, size);
		}
		else {
		    fprintf(fp, "%s %d:%d\n", output_buf, address, size);
		    flag = 1;
		}
	    }
	    length = 0;
	} else {
	    if (*point == ':') {
		output_buf[length++] = '\0';
	    }
	    else {
		output_buf[length++] = *point;
	    }

	    /* 日本語ならもう1byte進める */
	    if (*point & 0x80) {
		output_buf[length++] = *(point+1);
		point++;
	    }
	}
    }
    output_buf[length] = '\0';
    if (flag != 1 || strcmp(output_buf, entry)) {
	if (pp) {
	    fprintf(fp, "%s-%s-%s %d:%d\n", output_buf, pp, entry, address, size);
	}
	else {
	    fprintf(fp, "%s %d:%d\n", output_buf, address, size);
	}
    }
}

void write_data(IPAL_TRANS_FRAME *ipal_frame, int *point, int *closest, 
		 int writesize, int casenum, int *address, int flag) {
    int i;
    char *pp;

    fprint_ipal_idx(fp_idx, 
		    ipal_frame->DATA+point[1], 
		    ipal_frame->DATA+point[2], 
		    NULL, 
		    *address, writesize, flag);

    /* 「直前格要素-直前格-用言」で登録」 */
    if (flag) { /* ORの格フレームを除く */
	for (i = 0; i < casenum; i++) {
	    if (closest[i] > 0 && 
		*(ipal_frame->DATA+point[closest[i]]) != '\0') {
		pp = strdup(ipal_frame->DATA+point[i*3+4]);
		*(pp+strlen(pp)-1) = '\0'; /* *をけす */
		fprint_ipal_idx(fp_idx, 
				ipal_frame->DATA+point[2], /* 用言表記 */
				ipal_frame->DATA+point[closest[i]], /* 直前格要素群 */
				pp, 
				*address, writesize, 0);
		free(pp);
	    }
	}
    }

    /* データ書き出し */

    if (fwrite(ipal_frame, writesize, 1, fp_dat) < 1) {
	fprintf(stderr, "Error in fwrite.\n");
	exit(1);
    }

    *address += writesize;
}

main(int argc, char **argv)
{
    char tag[256], DATA[IPAL_DATA_SIZE], *pp, *token;
    int i, line = 0, pos = 0, address = 0, flag = 1, item, casenum;
    int closest[CASE_MAX_NUM], point[IPAL_FIELD_NUM];

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

    while (1) {
	line++;
	    
	if (fgets(buffer, IPAL_DATA_SIZE, stdin) == NULL) {
	    /* 最後のデータ */
	    write_data(&ipal_frame, point, closest, 
		       pos, casenum, &address, flag);
	    fclose(fp_idx);
	    fclose(fp_dat);
	    return 0;
	}

	sscanf(buffer, "%s %[^\n]\n", tag, DATA);

	if (!strcmp(tag, "ID")) {
	    /* アドレス書き出し */

	    if (line != 1) { /* 初回以外 */
		write_data(&ipal_frame, point, closest, 
			   pos, casenum, &address, flag);
	    }

	    /* 初期化 */
	    pos = 0;
	    item = 0;
	    casenum = 0;
	    memset(closest, 0, sizeof(int)*CASE_MAX_NUM);
	}
	else if (!strncmp(tag, "素性", 4)) {
	    flag = 1;
	    /* 要素をsplit */
	    token = strtok(DATA, " ");
	    while (token) {
		if (!strcmp(token, "和フレーム")) {
		    flag = 0;	/* 読みを登録しない */
		    break;
		}
		token = strtok(NULL, " ");
	    }
	}
	else if (!strncmp(tag, "格", 2)) {
	    casenum++;
	    if (casenum > CASE_MAX_NUM) {
		fprintf(stderr, "# of cases is more than MAX (%d).\n", CASE_MAX_NUM);
		exit(1);
	    }
	    /* 直前格 */
	    if (*(DATA+strlen(DATA)-1) == '*') {
		closest[(item-4)/3] = item+1; /* この格の用例の位置 */
	    }
	}

	point[item] = pos;
	strcpy(&(ipal_frame.DATA[pos]), DATA);
	if (!strcmp(DATA, "nil")) {
	    ipal_frame.DATA[pos] = '\0';
	    pos += 1;
	}
	else {
	    pos += strlen(DATA) + 1;
	}
	if (pos > IPAL_DATA_SIZE) {
	    fprintf(stderr, "%d is small for IPAL record (%s).\n", 
		    IPAL_DATA_SIZE, ipal_frame.DATA);
	    exit(1);
	}
	item++;
    }
}

int tolend(int i) {
    return (i >> 24) | ((i >> 16) & 0xff) << 8 | ((i >> 8) & 0xff) << 16 | (i & 0xff) << 24;
}

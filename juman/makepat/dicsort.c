/*
 *	dicsort.c - sort a dictionary
 *
 *	it does same as "sort -s +0 -1 | uniq | gawk -e '{printf("%s\0",$$0)}'"
 *
 *	by A.Kitauchi <akira-k@is.aist-nara.ac.jp>, Oct. 1996
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <juman.h>

#define _DEBUG

typedef struct _line_info {
    long ptr;
    char *midasi;
} line_info;

#define JUMAN_MALLOC_SIZE (1024 * 256)
static void *buffer_ptr[1024];
static int  buffer_ptr_num = 0;
static int  buffer_idx = JUMAN_MALLOC_SIZE;
extern char	*ProgName;

static void *jm_malloc_char(int size)
{
    if (buffer_idx + size >= JUMAN_MALLOC_SIZE) {
	if (buffer_ptr_num == 99)
	  error(1, "Can't allocate memory");
	buffer_ptr[buffer_ptr_num++] = my_alloc(JUMAN_MALLOC_SIZE);
	buffer_idx = 0;
    }

    buffer_idx += size;
    return (char *)(buffer_ptr[buffer_ptr_num - 1]) + buffer_idx - size;
}

static int midasi_compare(line_info *l1, line_info *l2)
{
    int rc;
    rc = strcmp(l1->midasi, l2->midasi);
    if (rc)
      return rc;
    return (int)(l1->ptr - l2->ptr);
}

int main(int argc, char *argv[])
{
    FILE *fp;
    line_info *line;
    int nline, size, l;
    char buf[1024], prebuf[1024];

    if (argc != 2) {
	fprintf(stderr, "usage: dicsort filename\n");
	exit(1);
    }

    ProgName = argv[0];

#ifdef DEBUG
    fprintf(stderr, "couting lines\n");
#endif
#ifdef _WIN32   
    fp = my_fopen(argv[1], "rb");
#else   
    fp = my_fopen(argv[1], "r");
#endif   

    for (nline = 0; fgets(buf, sizeof(buf), fp) != NULL; nline++);

#ifdef DEBUG
    fprintf(stderr, "%d lines\n", nline);
#endif
    line = (line_info *)my_alloc(sizeof(line_info) * nline);

#ifdef DEBUG
    fprintf(stderr, "reading file\n");
#endif
    rewind(fp);
    for (l = 0; l < nline; l++) {
	char *s;
	line[l].ptr = ftell(fp);
	fgets(buf, sizeof(buf), fp);
	s = strchr(buf, '\t');
	*s = '\0';
	size = strlen(buf) + 1;
	line[l].midasi = (char *)jm_malloc_char(size);
	strcpy(line[l].midasi, buf);
    }

#ifdef DEBUG
    fprintf(stderr, "sorting\n");
#endif
    qsort(line, nline, sizeof(line_info), (int (*)())midasi_compare);

#ifdef DEBUG
    fprintf(stderr, "outputting\n");
#endif
    prebuf[0] = '\0';
    for (l = 0; l < nline; l++) {
	fseek(fp, line[l].ptr, SEEK_SET);
	fgets(buf, sizeof(buf), fp);
	if (strcmp(prebuf, buf)) {
	    strcpy(prebuf, buf);
	    size = strlen(buf);
	    /* buf[size - 1] = '\0'; */
	    fputs(buf, stdout);
	    /* fputc('\0', stdout); */
	}
    }
    fclose(fp);

    return 0;
}

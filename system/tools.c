/*====================================================================

			       ツール群

                                               S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include "knp.h"

unsigned int seed[NSEED][256];

/*==================================================================*/
	 int knp_fprintf(FILE *output, const char *fmt, ...)
/*==================================================================*/
{
    va_list ap;
    char buffer[DATA_LEN];
    char *SJISbuffer;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

#ifndef _WIN32
    if (OptEncoding == ENCODING_SHIFTJIS) {
#endif
	SJISbuffer = (char *)toStringSJIS(buffer);
	fwrite(SJISbuffer, sizeof(char), strlen(SJISbuffer), output);
	free(SJISbuffer);
#ifndef _WIN32
    }
    else
	fwrite(buffer, sizeof(char), strlen(buffer), output);
#endif

    return TRUE;
}

/*==================================================================*/
	    void *malloc_data(size_t size, char *comment)
/*==================================================================*/
{
    void *p;
    if (!(p = malloc(size))) {
	fprintf(stderr, "Can't allocate memory for %s\n", comment);
	exit(-1);
    }
    return p;
}

/*==================================================================*/
      void *realloc_data(void *ptr, size_t size, char *comment)
/*==================================================================*/
{
    void *p;
    if (!(p = realloc(ptr, size))) {
	fprintf(stderr, "Can't allocate memory for %s\n", comment);
	exit(-1);
    }
    return p;
}

/*==================================================================*/
			   void init_hash()
/*==================================================================*/
{
    int i, j;

    srand(time(NULL));
    for (i = 0; i < NSEED; ++i) {
	for (j = 0; j < 256; ++j) {
	    seed[i][j] = rand();
	}
    }
}

/*==================================================================*/
	       int hash(unsigned char *key, int keylen)
/*==================================================================*/
{
    int i = 0, j = 0, hash = 0;

    while (i < keylen) {
	hash ^= seed[j++][key[i++]];
	j &= NSEED-1;
    }
    return (hash & (TBLSIZE-1));
}

/*==================================================================*/
	 unsigned char *katakana2hiragana(unsigned char *cp)
/*==================================================================*/
{
    int i;
    unsigned char *hira;

    hira = strdup(cp);

    for (i = 0; i < strlen(hira); i += BYTES4CHAR) { /* euc-jp */
	if (*(hira+i) == 0xa5) {
	    *(hira+i) = 0xa4;
	}
    }    
    return hira;	/* free してください */
}

/*==================================================================*/
	 unsigned char *hiragana2katakana(unsigned char *cp)
/*==================================================================*/
{
    int i;
    unsigned char *hira;

    hira = strdup(cp);

    for (i = 0; i < strlen(hira); i += BYTES4CHAR) { /* euc-jp */
	if (*(hira+i) == 0xa4) {
	    *(hira+i) = 0xa5;
	}
    }    
    return hira;	/* free してください */
}

/*==================================================================*/
	 char *strdup_with_check(const char *s)
/*==================================================================*/
{
    if (s) {
	return strdup(s);
    }
    else {
	return NULL;
    }
}

/*====================================================================
                               END
====================================================================*/

/*====================================================================

			       ツール群

                                               S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include "knp.h"

unsigned int seed[NSEED][256];

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

/*==================================================================*/
                   int utf8_length(unsigned char c)
/*==================================================================*/
{
    /* UTF-8文字の1バイト目から、その文字のバイト数を返す */

    if (c > 0xfb) {
	return 6;
    }
    else if (c > 0xf7) {
	return 5;
    }
    else if (c > 0xef) {
	return 4;
    }
    else if (c > 0xdf) {
	return 3;
    }
    else if (c > 0x7f) {
	return 2;
    }
    else {
	return 1;
    }
}

/*==================================================================*/
                     int string_length(char *cp)
/*==================================================================*/
{
    /* 文字列の文字数を返す */

    int length = 0;

    while (*cp) {
        length++;
        cp += utf8_length(*cp);
    }

    return length;
}

/*==================================================================*/
            int str_compare(const void *a, const void *b)
/*==================================================================*/
{
    /* sort function */
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcmp(str_a, str_b);
}

#ifdef _WIN32
/*==================================================================*/
                   char *SJIStoStringUTF8(char *str)
/*==================================================================*/
{
    wchar_t *str_unicode;
    char *str_utf8;
    int str_unicode_length, str_utf8_length;

    /* SJIS -> Unicode */
    str_unicode_length = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    str_unicode = (wchar_t *)malloc_data(sizeof(wchar_t) * str_unicode_length, "SJIStoStringUTF8");
    MultiByteToWideChar(CP_ACP, 0, str, -1, str_unicode, str_unicode_length);

    /* Uniocde -> UTF8 */
    str_utf8_length = WideCharToMultiByte(CP_UTF8, 0, str_unicode, str_unicode_length, NULL, 0, NULL, NULL);
    str_utf8 = (char *)malloc_data(str_utf8_length, "SJIStoStringUTF8");    
    WideCharToMultiByte(CP_UTF8, 0, str_unicode, str_unicode_length, str_utf8, str_utf8_length, NULL, NULL);
    free(str_unicode);

    return str_utf8;
}
#endif

/*====================================================================
                               END
====================================================================*/

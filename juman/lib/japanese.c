/* 
 *  MS Windows は入出力を SJIS <=> EUC に変換する必要があるため
 *  文字コード変換用の 汎用関数が必要
 *
 *  Added by Taku Kudoh (taku@pine.kuee.kyoto-u.ac.jp)
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NO_HANKAKU_SJIS

#define ESC           6
#define LF            10
#define CR            13

#define CHAROUT(ch) *str2 = (unsigned char)(ch); str2++;
#define HANKATA(a)  (a >= 161 && a <= 223)
#define ISMARU(a)   (a >= 202 && a <= 206)
#define ISNIGORI(a) ((a >= 182 && a <= 196) || (a >= 202 && a <= 206) || (a == 179))
#define SJIS1(A)    ((A >= 129 && A <= 159) || (A >= 224 && A <= 239))
#define SJIS2(A)    (A >= 64 && A <= 252)
#define ISEUC(A)    (A >= 161 && A <= 254)
#define ISLF(A)     (A == LF)
#define ISCR(A)     (A == CR)

static int stable[][2] = {
    {129,66},{129,117},{129,118},{129,65},{129,69},{131,146},{131,64},
    {131,66},{131,68},{131,70},{131,72},{131,131},{131,133},{131,135},
    {131,98},{129,91},{131,65},{131,67},{131,69},{131,71},{131,73},
    {131,74},{131,76},{131,78},{131,80},{131,82},{131,84},{131,86},
    {131,88},{131,90},{131,92},{131,94},{131,96},{131,99},{131,101},
    {131,103},{131,105},{131,106},{131,107},{131,108},{131,109},
    {131,110},{131,113},{131,116},{131,119},{131,122},{131,125},
    {131,126},{131,128},{131,129},{131,130},{131,132},{131,134},
    {131,136},{131,137},{131,138},{131,139},{131,140},{131,141},
    {131,143},{131,147},{129,74},{129,75}};

void _jis_shift(int *p1, int *p2) {
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int rowOffset = c1 < 95 ? 112 : 176;
    int cellOffset = c1 % 2 ? (c2 > 95 ? 32 : 31) : 126;

    *p1 = ((c1 + 1) >> 1) + rowOffset;
    *p2 += cellOffset;
}

void _sjis_shift(int *p1, int *p2) {
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int adjust = c2 < 159;
    int rowOffset = c1 < 160 ? 112 : 176;
    int cellOffset = adjust ? (c2 > 127 ? 32 : 31) : 126;

    *p1 = ((c1 - rowOffset) << 1) - adjust;
    *p2 -= cellOffset;
}

unsigned char *_sjis_han2zen(unsigned char *str, int *p1, int *p2) {
    register int c1, c2;

    c1 = (int)*str; str++;
    *p1 = stable[c1 - 161][0];
    *p2 = stable[c1 - 161][1];

    c2 = (int)*str;
    if (c2 == 222 && ISNIGORI(c1)) {
	if ((*p2 >= 74 && *p2 <= 103) || (*p2 >= 110 && *p2 <= 122))
	    (*p2)++;
	else if (*p1 == 131 && *p2 == 69)
	    *p2 = 148;
	str++;
    }

    if (c2 == 223 && ISMARU(c1) && (*p2 >= 110 && *p2 <= 122) ) {
	*p2 += 2;
	str++;
    }
    return str++;
}

void _shift2euc(unsigned char *str, unsigned char *str2) {
  int p1, p2;
  
  while ((p1 = (int)*str) != '\0') {
      if (SJIS1(p1)) {
	  if((p2 = (int)*(++str)) == '\0') break;
	  if (SJIS2(p2)) {
	      _sjis_shift(&p1, &p2);
	      p1 += 128;
	      p2 += 128;
	  }
	  CHAROUT(p1);
	  CHAROUT(p2);
	  str++;
	  continue;
      }

#ifdef NO_HANKAKU_SJIS
      if (HANKATA(p1)) {
	  str = _sjis_han2zen(str,&p1,&p2);
	  _sjis_shift(&p1,&p2);
	  p1 += 128;
	  p2 += 128;
	  CHAROUT(p1);
	  CHAROUT(p2);
	  continue;
      }
#endif

      CHAROUT(p1);
      str++;
  }
  *str2='\0';
}

void _euc2shift(unsigned char *str, unsigned char *str2) {
    int p1,p2;

    while ((p1 = (int)*str) != '\0') {
	if (ISEUC(p1)) {
	    if((p2 = (int)*(++str)) == '\0') break;
	    if (ISEUC(p2)) {
		p1 -= 128;
		p2 -= 128;
		_jis_shift(&p1,&p2);
	    }
	    CHAROUT(p1);
	    CHAROUT(p2);
	    str++;
	    continue;
	}

	CHAROUT(p1);
	str++;
    }
    *str2='\0';
}

unsigned char *_set_buffer(char *str) {
    static unsigned char *buf;
    if((buf = (unsigned char *)malloc((strlen(str) + 1) * 4)) == NULL) {
	fprintf(stderr, "Can't malloc buffer\n");
	exit(2);
    }
    return buf;
}

char *_replace_buffer(unsigned char *buf)  {
    char *str;

    if ((str = strdup(buf)) == NULL) {
	fprintf(stderr, "Can't malloc string buffer\n");
	exit(2);
    }
    free(buf);
    return str;
}

char *toStringEUC(char *str)  {
    unsigned char *buf;
    buf = _set_buffer(str);
    _shift2euc((unsigned char *)str, buf);
    return (char *)_replace_buffer(buf);
}

char *toStringSJIS(char *str)  {
    unsigned char *buf;
    buf = _set_buffer(str);
    _euc2shift((unsigned char *)str, buf);
    return _replace_buffer(buf);
}

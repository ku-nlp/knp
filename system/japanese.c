/* 
 *  MS Windows は入出力を SJIS <=> EUC に変換する必要があるため
 *  文字コード変換用の 汎用関数が必要
 *
 * Modified by (C) Kudoh Taku, Kyoto Univ. 1998
 * Original by (C) Kuramitsu Kimio, Tokyo Univ. 1996-97
 *
 *  japanese.c 
 */
 
#include "knp.h"
#include "japanese.h"

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

unsigned char *_to_jis(unsigned char *str) 
{
  *str = (unsigned char)ESC; str++;
  *str = (unsigned char)'$'; str++;
  *str = (unsigned char)'B'; str++;
  return str;
}

void _jis_shift(int *p1, int *p2)
{
  unsigned char c1 = *p1;
  unsigned char c2 = *p2;
  int rowOffset = c1 < 95 ? 112 : 176;
  int cellOffset = c1 % 2 ? (c2 > 95 ? 32 : 31) : 126;

  *p1 = ((c1 + 1) >> 1) + rowOffset;
  *p2 += cellOffset;
}

void _sjis_shift(int *p1, int *p2)
{
  unsigned char c1 = *p1;
  unsigned char c2 = *p2;
  int adjust = c2 < 159;
  int rowOffset = c1 < 160 ? 112 : 176;
  int cellOffset = adjust ? (c2 > 127 ? 32 : 31) : 126;

  *p1 = ((c1 - rowOffset) << 1) - adjust;
  *p2 -= cellOffset;
}

unsigned char *_sjis_han2zen(unsigned char *str, int *p1, int *p2)
{
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

void _shift2euc(unsigned char *str, unsigned char *str2)
{
  int p1,p2;
  
  while ((p1 = (int)*str) != '\0') {
    if (SJIS1(p1)) {
      if((p2 = (int)*(++str)) == '\0') break;
      if (SJIS2(p2)) {
        _sjis_shift(&p1,&p2);
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

#ifdef CONV_RETURN_CODE
    if (ISCR(p1)) {
      if((p2 = (int)*(++str)) == '\0') {
	CHAROUT(p1);
	break;
      }
      if (ISLF(p2)) {
	CHAROUT(LF);
      } else {
	CHAROUT(p1);
	CHAROUT(p2);
      }	
      str++;
      continue;
    } 
#endif

    CHAROUT(p1);
    str++;
  }
  *str2='\0';
}

void _euc2shift(unsigned char *str, unsigned char *str2)
{
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

#ifdef CONV_RETURN_CODE
    if (ISLF(p1)) {
      CHAROUT(CR);
      CHAROUT(LF);
      str++;
      continue;
    }
#endif

    CHAROUT(p1);
    str++;
  }
  *str2='\0';
}

unsigned char *_set_buffer(char *str) 
{
  static unsigned char *buf;
  if((buf = (unsigned char *)malloc((strlen(str) + 1) * 4)) == NULL) {
    fprintf(stderr, "Can't malloc buffer\n");
    exit(2);
  }
  return buf;
}

char *_replace_buffer(unsigned char *buf) 
{
  char *str;

  if ((str = strdup(buf)) == NULL) {
    fprintf(stderr, "Can't malloc string buffer\n");
    exit(2);
  }
  free(buf);
  return str;
}

char *toStringEUC(char *str) 
{
  unsigned char *buf;
  buf = _set_buffer(str);
  _shift2euc((unsigned char *)str, buf);
  return (char *)_replace_buffer(buf);
}

char *toStringSJIS(char *str) 
{
  unsigned char *buf;
  buf = _set_buffer(str);
  _euc2shift((unsigned char *)str, buf);
  return _replace_buffer(buf);
}

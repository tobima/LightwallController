
#include <stdarg.h>
#include "gfx.h"
#include "fcwall.h"     /* necessary, to know the window to print into */
#include "ff.h"

#define	CHECK_BUFFER_LENGTH		if (i > bufferlength)  { continue; }

#define MAX_FILLER                      11
#define MAXIMUM_INITIALIZATION          10

/* ----------- exported from Chibios: chprintf.c ------------ */
static char *long_to_string_with_divisor(char *p,
                                         long num,
                                         unsigned radix,
                                         long divisor)
{
  int i;
  char *q;
  long l, ll;

  l = num;
  if (divisor == 0) {
    ll = num;
  } else {
    ll = divisor;
  }

  q = p + MAX_FILLER;
  do {
    i = (int)(l % radix);
    i += '0';
    if (i > '9')
      i += 'A' - '0' - 10;
    *--q = i;
    l /= radix;
  } while ((ll /= radix) != 0);

  i = (int)(p + MAX_FILLER - q);
  do
    *p++ = *q++;
  while (--i);

  return p;
}

static char *ltoa(char *p, long num, unsigned radix) {

  return long_to_string_with_divisor(p, num, radix, 0);
}

#if CHPRINTF_USE_FLOAT
static char *ftoa(char *p, double num) {
  long l;
  unsigned long precision = FLOAT_PRECISION;

  l = num;
  p = long_to_string_with_divisor(p, l, 10, 0);
  *p++ = '.';
  l = (num - l) * precision;
  return long_to_string_with_divisor(p, l, 10, precision / 10);
}
#endif

static void util_vprintf(char *buffer, int bufferlength, const char *fmt, va_list ap)
{
  char *p, *s, c, filler;
  int i, precision, width;
  bool_t is_long, left_align;
  long l;
  int j;

  /* initializing */
  i = 0;

#if CHPRINTF_USE_FLOAT
  float f;
  char tmpbuf[2*MAX_FILLER + 1];
#else
  char tmpbuf[MAX_FILLER + 1];
#endif

  j = 0;
  while (TRUE) {
    c = *fmt++;
    if (c == 0) {
      return;
    }
    if (c != '%')
    {
      /* Check if the buffer is big enough */
      CHECK_BUFFER_LENGTH
      /* Update the buffer */
      buffer[j++] = (uint8_t) c;
      continue;
    }
    p = tmpbuf;
    s = tmpbuf;
    left_align = FALSE;
    if (*fmt == '-') {
      fmt++;
      left_align = TRUE;
    }
    filler = ' ';
    if ((*fmt == '.') || (*fmt == '0')) {
      fmt++;
      filler = '0';
    }
    width = 0;
    while (TRUE) {
      c = *fmt++;
      if (c >= '0' && c <= '9')
        c -= '0';
      else if (c == '*')
        c = va_arg(ap, int);
      else
        break;
      width = width * 10 + c;
    }
    precision = 0;
    if (c == '.') {
      while (TRUE) {
        c = *fmt++;
        if (c >= '0' && c <= '9')
          c -= '0';
        else if (c == '*')
          c = va_arg(ap, int);
        else
          break;
        precision *= 10;
        precision += c;
      }
    }
    /* Long modifier.*/
    if (c == 'l' || c == 'L') {
      is_long = TRUE;
      if (*fmt)
        c = *fmt++;
    }
    else
      is_long = (c >= 'A') && (c <= 'Z');

    /* Command decoding.*/
    switch (c) {
    case 'c':
      filler = ' ';
      *p++ = va_arg(ap, int);
      break;
    case 's':
      filler = ' ';
      if ((s = va_arg(ap, char *)) == 0)
        s = "(null)";
      if (precision == 0)
        precision = 32767;
      for (p = s; *p && (--precision >= 0); p++)
        ;
      break;
    case 'D':
    case 'd':
    case 'I':
    case 'i':
      if (is_long)
        l = va_arg(ap, long);
      else
        l = va_arg(ap, int);
      if (l < 0) {
        *p++ = '-';
        l = -l;
      }
      p = ltoa(p, l, 10);
      break;
#if CHPRINTF_USE_FLOAT
    case 'f':
      f = (float) va_arg(ap, double);
      if (f < 0) {
        *p++ = '-';
        f = -f;
      }
      p = ftoa(p, f);
      break;
#endif
    case 'X':
    case 'x':
      c = 16;
      goto unsigned_common;
    case 'U':
    case 'u':
      c = 10;
      goto unsigned_common;
    case 'O':
    case 'o':
      c = 8;
unsigned_common:
      if (is_long)
        l = va_arg(ap, unsigned long);
      else
        l = va_arg(ap, unsigned int);
      p = ltoa(p, l, c);
      break;
    default:
      *p++ = c;
      break;
    }
    i = (int)(p - s);
    if ((width -= i) < 0)
      width = 0;
    if (left_align == FALSE)
      width = -width;
    if (width < 0) {
      if (*s == '-' && filler == '0') {
	/* Check if the buffer is big enough */
        CHECK_BUFFER_LENGTH
        /* Update the buffer */
        buffer[j++] = (uint8_t)*s++;
        i--;
      }

      do
      {
	/* Check if the buffer is big enough */
	CHECK_BUFFER_LENGTH
	/* Update the buffer */
	buffer[j++] = (uint8_t) filler;
      }
      while (++width != 0);
    }

    while (--i >= 0)
    {
      /* Check if the buffer is big enough */
      CHECK_BUFFER_LENGTH
      /* Update the buffer */
      buffer[j++] = (uint8_t)*s++;
    }

    while (width)
    {
      /* Check if the buffer is big enough */
      CHECK_BUFFER_LENGTH
      /* Update the buffer */
      buffer[j++] = (uint8_t)filler;
      width--;
    }
  }
}

extern void gdispPrintf(int x, int y, font_t font, color_t color, int bufferlength, char* text, ...)
{
	va_list ap;
	int i = 0;
	int width = gdispGetFontMetric(font, fontMaxWidth) * bufferlength;
	int height = gdispGetFontMetric(font, fontHeight);
	char buffer[bufferlength];

	for(i=0; i < bufferlength; i++)
	{
		buffer[i]=0;
	}

	va_start(ap, text);
	util_vprintf(buffer, bufferlength, text, ap);
	va_end(ap);

	/*if (gGWdefault)
	{ FIXME also make the window-printf working
	    gwinSetColor(gGWdefault, color);
	    gwinSetBgColor(gGWdefault, Black);
	    gwinDrawStringBox(gGWdefault, x, y, width, height, buffer, justifyLeft);
	}
	else */
	{
	    gdispFillStringBox(MENU_BUTTON_WIDTH + x, y, width, height, buffer, font, color, Black, justifyLeft);
	}
}

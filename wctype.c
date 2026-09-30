// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <wctype.h>

#include <ctype.h>
#include <string.h>

/* The C.UTF-8 locale, without Unicode tables: ASCII is classified exactly by
 * the <ctype.h> functions, U+0080..U+009F are controls, U+00A0 is a blank and
 * every other code point from U+00A1 up counts as a letter (so as alnum,
 * graph and print). Case mapping covers ASCII and Latin-1. */
#define WC_ASCII(wc) ((wc) < 0x80u)

int iswalnum(wint_t wc) { return WC_ASCII(wc) ? isalnum((int)wc) : iswalpha(wc); }
int iswalpha(wint_t wc) {
  if (WC_ASCII(wc))
    return isalpha((int)wc);
  return wc > 0xA0u && wc != WEOF;
}
int iswblank(wint_t wc) { return wc == ' ' || wc == '\t' || wc == 0xA0u; }
int iswcntrl(wint_t wc) { return WC_ASCII(wc) ? iscntrl((int)wc) : wc < 0xA0u; }
int iswdigit(wint_t wc) { return wc - '0' < 10u; }
int iswgraph(wint_t wc) { return WC_ASCII(wc) ? isgraph((int)wc) : iswalpha(wc); }
int iswlower(wint_t wc) {
  if (WC_ASCII(wc))
    return islower((int)wc);
  return (wc >= 0xDFu && wc <= 0xFFu && wc != 0xF7u);
}
int iswprint(wint_t wc) { return WC_ASCII(wc) ? isprint((int)wc) : (wc >= 0xA0u && wc != WEOF); }
int iswpunct(wint_t wc) { return WC_ASCII(wc) ? ispunct((int)wc) : 0; }
int iswspace(wint_t wc) { return WC_ASCII(wc) ? isspace((int)wc) : 0; }
int iswupper(wint_t wc) {
  if (WC_ASCII(wc))
    return isupper((int)wc);
  return (wc >= 0xC0u && wc <= 0xDEu && wc != 0xD7u);
}
int iswxdigit(wint_t wc) { return WC_ASCII(wc) && isxdigit((int)wc); }

wint_t towlower(wint_t wc) {
  if (WC_ASCII(wc))
    return (wint_t)tolower((int)wc);
  return (wc >= 0xC0u && wc <= 0xDEu && wc != 0xD7u) ? wc + 0x20u : wc;
}
wint_t towupper(wint_t wc) {
  if (WC_ASCII(wc))
    return (wint_t)toupper((int)wc);
  return (wc >= 0xE0u && wc <= 0xFEu && wc != 0xF7u) ? wc - 0x20u : wc;
}
wint_t towlower_l(wint_t wc, locale_t locale) {
  (void)locale;
  return towlower(wc);
}
wint_t towupper_l(wint_t wc, locale_t locale) {
  (void)locale;
  return towupper(wc);
}

static const struct {
  const char *name;
  int (*fn)(wint_t);
} wctype_classes[] = {
    {"alnum", iswalnum}, {"alpha", iswalpha}, {"blank", iswblank}, {"cntrl", iswcntrl},
    {"digit", iswdigit}, {"graph", iswgraph}, {"lower", iswlower}, {"print", iswprint},
    {"punct", iswpunct}, {"space", iswspace}, {"upper", iswupper}, {"xdigit", iswxdigit},
};

/* A wctype_t is the class's index + 1; 0 names no class. */
wctype_t wctype(const char *name) {
  for (unsigned i = 0; i < sizeof(wctype_classes) / sizeof(wctype_classes[0]); i++)
    if (strcmp(name, wctype_classes[i].name) == 0)
      return i + 1;
  return 0;
}

int iswctype(wint_t wc, wctype_t desc) {
  if (desc == 0 || desc > sizeof(wctype_classes) / sizeof(wctype_classes[0]))
    return 0;
  return wctype_classes[desc - 1].fn(wc);
}

/* 1 = tolower, 2 = toupper, 0 = none. */
wctrans_t wctrans(const char *name) {
  if (strcmp(name, "tolower") == 0)
    return 1;
  if (strcmp(name, "toupper") == 0)
    return 2;
  return 0;
}

wint_t towctrans(wint_t wc, wctrans_t desc) {
  return desc == 1 ? towlower(wc) : desc == 2 ? towupper(wc) : wc;
}

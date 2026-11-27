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

#include "utest.h"

/* libc's own <ctype.h>, not the toolchain's: its inline versions are what
 * programs call, so they are checked against ctype.c's functions below. */
#include "../include/ctype.h"

int sut_isascii(int c);
int sut_isblank(int c);
int sut_isspace(int c);
int sut_isalpha(int c);
int sut_isdigit(int c);
int sut_isalnum(int c);
int sut_isupper(int c);
int sut_islower(int c);
int sut_isprint(int c);
int sut_ispunct(int c);
int sut_iscntrl(int c);
int sut_isgraph(int c);
int sut_isxdigit(int c);
int sut_tolower(int c);
int sut_toupper(int c);

#define CTYPE_EOF (-1)

/* The POSIX locale's classes (XBD 7.3.1 LC_CTYPE), spelled out rather than
 * computed the way ctype.c computes them. */
static const char *const UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char *const LOWER = "abcdefghijklmnopqrstuvwxyz";
static const char *const DIGIT = "0123456789";
static const char *const XDIGIT = "0123456789ABCDEFabcdef";
static const char *const SPACE = " \t\n\v\f\r";
static const char *const BLANK = " \t";
static const char *const PUNCT = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

/* c in s; NUL, EOF and bytes past 127 are in none of the sets */
static int in(char *s, int c) {
  if (c <= 0 || c > 127)
    return 0;
  return strchr(s, c) != nullptr;
}

static int ref_upper(int c) {
  return in(UPPER, c);
}
static int ref_lower(int c) {
  return in(LOWER, c);
}
static int ref_alpha(int c) {
  return in(UPPER, c) || in(LOWER, c);
}
static int ref_digit(int c) {
  return in(DIGIT, c);
}
static int ref_alnum(int c) {
  return ref_alpha(c) || ref_digit(c);
}
static int ref_xdigit(int c) {
  return in(XDIGIT, c);
}
static int ref_space(int c) {
  return in(SPACE, c);
}
static int ref_blank(int c) {
  return in(BLANK, c);
}
static int ref_punct(int c) {
  return in(PUNCT, c);
}
static int ref_graph(int c) {
  return ref_alnum(c) || ref_punct(c);
}
static int ref_print(int c) {
  return ref_graph(c) || c == ' ';
}
static int ref_cntrl(int c) {
  return (c >= 0 && c <= 0x1f) || c == 0x7f;
}
static int ref_ascii(int c) {
  return c >= 0 && c <= 0x7f;
}

typedef struct {
  const char *name;
  int (*sut)(int);
  int (*ref)(int);
  int count; /* members among 0..255 */
} ctype_class;

static const ctype_class CLASSES[] = {
    {"isupper", sut_isupper, ref_upper, 26},
    {"islower", sut_islower, ref_lower, 26},
    {"isalpha", sut_isalpha, ref_alpha, 52},
    {"isdigit", sut_isdigit, ref_digit, 10},
    {"isalnum", sut_isalnum, ref_alnum, 62},
    {"isxdigit", sut_isxdigit, ref_xdigit, 22},
    {"isspace", sut_isspace, ref_space, 6},
    {"isblank", sut_isblank, ref_blank, 2},
    {"ispunct", sut_ispunct, ref_punct, 32},
    {"isgraph", sut_isgraph, ref_graph, 94},
    {"isprint", sut_isprint, ref_print, 95},
    {"iscntrl", sut_iscntrl, ref_cntrl, 33},
    {"isascii", sut_isascii, ref_ascii, 128},
};

#define NCLASSES (sizeof(CLASSES) / sizeof(CLASSES[0]))

/* every classifier, for EOF and every unsigned char value; POSIX asks only
 * for nonzero on a member, so the answers are compared as truth values */
UTEST(ctype, classes_match_posix_locale) {
  for (size_t i = 0; i < NCLASSES; i++) {
    for (int c = CTYPE_EOF; c <= 255; c++) {
      int got = CLASSES[i].sut(c) != 0;
      int want = CLASSES[i].ref(c) != 0;
      if (got != want)
        printf("%s(%d): %d, want %d\n", CLASSES[i].name, c, got, want);
      ASSERT_EQ(want, got);
    }
  }
}

/* the sizes of the classes, which do not depend on the reference sets above */
UTEST(ctype, class_sizes) {
  for (size_t i = 0; i < NCLASSES; i++) {
    int n = 0;
    for (int c = 0; c <= 255; c++)
      n += CLASSES[i].sut(c) != 0;
    if (n != CLASSES[i].count)
      printf("%s: %d members, want %d\n", CLASSES[i].name, n, CLASSES[i].count);
    ASSERT_EQ(CLASSES[i].count, n);
  }
}

UTEST(ctype, eof_is_in_no_class) {
  for (size_t i = 0; i < NCLASSES; i++)
    ASSERT_FALSE(CLASSES[i].sut(CTYPE_EOF));
}

/* the relations C11 7.4.1 and POSIX state between the classes */
UTEST(ctype, class_relations) {
  for (int c = CTYPE_EOF; c <= 255; c++) {
    ASSERT_EQ(sut_isalpha(c) != 0, sut_isupper(c) != 0 || sut_islower(c) != 0);
    ASSERT_EQ(sut_isalnum(c) != 0, sut_isalpha(c) != 0 || sut_isdigit(c) != 0);
    ASSERT_EQ(sut_isgraph(c) != 0, sut_isalnum(c) != 0 || sut_ispunct(c) != 0);
    ASSERT_EQ(sut_isprint(c) != 0, sut_isgraph(c) != 0 || c == ' ');
    if (sut_isblank(c))
      ASSERT_TRUE(sut_isspace(c));
    if (sut_isdigit(c))
      ASSERT_TRUE(sut_isxdigit(c));
    if (sut_iscntrl(c))
      ASSERT_FALSE(sut_isprint(c));
  }
}

UTEST(ctype, tolower_toupper) {
  for (int c = CTYPE_EOF; c <= 255; c++) {
    int lower = ref_upper(c) ? c - 'A' + 'a' : c;
    int upper = ref_lower(c) ? c - 'a' + 'A' : c;
    if (sut_tolower(c) != lower)
      printf("tolower(%d): %d, want %d\n", c, sut_tolower(c), lower);
    ASSERT_EQ(lower, sut_tolower(c));
    if (sut_toupper(c) != upper)
      printf("toupper(%d): %d, want %d\n", c, sut_toupper(c), upper);
    ASSERT_EQ(upper, sut_toupper(c));
  }
  ASSERT_EQ(CTYPE_EOF, sut_tolower(CTYPE_EOF));
  ASSERT_EQ(CTYPE_EOF, sut_toupper(CTYPE_EOF));
  /* the neighbours of the letters stay as they are */
  ASSERT_EQ('@', sut_tolower('@'));
  ASSERT_EQ('[', sut_tolower('['));
  ASSERT_EQ('`', sut_toupper('`'));
  ASSERT_EQ('{', sut_toupper('{'));
}

/* <ctype.h>'s inline versions (the macros) answer as ctype.c's functions do */
UTEST(ctype, header_inlines_match_functions) {
  for (int c = CTYPE_EOF; c <= 255; c++) {
    ASSERT_EQ(sut_isupper(c) != 0, isupper(c) != 0);
    ASSERT_EQ(sut_islower(c) != 0, islower(c) != 0);
    ASSERT_EQ(sut_isalpha(c) != 0, isalpha(c) != 0);
    ASSERT_EQ(sut_isdigit(c) != 0, isdigit(c) != 0);
    ASSERT_EQ(sut_isalnum(c) != 0, isalnum(c) != 0);
    ASSERT_EQ(sut_isxdigit(c) != 0, isxdigit(c) != 0);
    ASSERT_EQ(sut_isspace(c) != 0, isspace(c) != 0);
    ASSERT_EQ(sut_isblank(c) != 0, isblank(c) != 0);
    ASSERT_EQ(sut_ispunct(c) != 0, ispunct(c) != 0);
    ASSERT_EQ(sut_isgraph(c) != 0, isgraph(c) != 0);
    ASSERT_EQ(sut_isprint(c) != 0, isprint(c) != 0);
    ASSERT_EQ(sut_iscntrl(c) != 0, iscntrl(c) != 0);
    ASSERT_EQ(sut_isascii(c) != 0, isascii(c) != 0);
    ASSERT_EQ(sut_tolower(c), tolower(c));
    ASSERT_EQ(sut_toupper(c), toupper(c));
  }
}

/* C11 7.1.4: a library macro evaluates its argument exactly once */
UTEST(ctype, header_macros_evaluate_argument_once) {
  static const char text[] = "aZ9 ";
  const char *p = text;

  ASSERT_TRUE(isalpha(*p++));
  ASSERT_TRUE(p == text + 1);
  ASSERT_EQ('Z', toupper(*p++));
  ASSERT_TRUE(p == text + 2);
  ASSERT_TRUE(isdigit(*p++));
  ASSERT_TRUE(p == text + 3);
  ASSERT_TRUE(isspace(*p++));
  ASSERT_TRUE(p == text + 4);

  int n = 0;
  ASSERT_FALSE(ispunct(n++ + 'a'));
  ASSERT_EQ(1, n);
}

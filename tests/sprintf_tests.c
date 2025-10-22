/*
 Copyright (c) 2025 Mateusz Stadnik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "utest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

#include "syscalls_stub.h"

int sut_sprintf(char *dst, const char *fmt, ...);

UTEST(sprintf, basic_formats) {
  char buf[128];

  sut_sprintf(buf, "int=%d hex=%x str=%s char=%c", 42, 0x2A, "hello", 'A');
  ASSERT_STREQ(buf, "int=42 hex=2a str=hello char=A");

  sut_sprintf(buf, "%u %o %X", 255u, 255, 255);
  ASSERT_STREQ(buf, "255 377 FF");
}

UTEST(sprintf, precision_star_string) {
  char buf[64];
  const char *s = "abcdefgh";

  sut_sprintf(buf, "p=%.*s", 3, s);
  ASSERT_STREQ(buf, "p=abc");

  sut_sprintf(buf, "p=%.*s", 0, s);
  ASSERT_STREQ(buf, "p=");

  sut_sprintf(buf, "p=%.*s", 10, s);
  ASSERT_STREQ(buf, "p=abcdefgh");

  /* embedded use */
  sut_sprintf(buf, "X%.*sY", 4, s);
  ASSERT_STREQ(buf, "XabcdY");
}

UTEST(sprintf, precision_star_integer) {
  char buf[64];

  /* precision pads with zeros on integers */
  sut_sprintf(buf, "%.*d", 5, 42);
  ASSERT_STREQ(buf, "00042");

  /* precision 0 and value 0 -> empty conversion for integer */
  sut_sprintf(buf, "v=%.*d", 0, 0);
  ASSERT_STREQ(buf, "v=");

  /* negative precision is ignored (treated as unspecified) */
  sut_sprintf(buf, "%.*d", -1, 42);
  ASSERT_STREQ(buf, "42");
}

UTEST(sprintf, width_and_precision_star) {
  char buf[64];

  /* width + precision from arguments */
  sut_sprintf(buf, "[%*.*s]", 6, 3, "abcdef");
  ASSERT_STREQ(buf, "[   abc]");

  /* width smaller than precision -> precision limits */
  sut_sprintf(buf, "[%*.*s]", 2, 4, "abcdef");
  ASSERT_STREQ(buf, "[abcd]");
}

UTEST(sprintf, mixed_cases_and_edge_conditions) {
  char buf[128];

  /* combination: integer precision with surrounding text */
  sut_sprintf(buf, "n=%.*d!", 3, 7);
  ASSERT_STREQ(buf, "n=007!");

  /* precision 0 for non-zero integer prints nothing for zero value only */
  sut_sprintf(buf, "z=%.*d", 0, 0);
  ASSERT_STREQ(buf, "z=");

  /* ensure behavior with larger precision than digits */
  sut_sprintf(buf, "%.*d", 6, -123);
  ASSERT_STREQ(buf, "-000123");
}

UTEST(sprintf, cppref_example) {
  char buf[128];
  const char *s = "Hello";
  sut_sprintf(buf, "Strings:\n"); // same as puts("Strings");
  ASSERT_STREQ(buf, "Strings:\n");
  sut_sprintf(buf, " padding:\n");
  ASSERT_STREQ(buf, " padding:\n");
  sut_sprintf(buf, "\t[%10s]\n", s);
  ASSERT_STREQ(buf, "\t[     Hello]\n");
  sut_sprintf(buf, "\t[%-10s]\n", s);
  ASSERT_STREQ(buf, "\t[Hello     ]\n");
  sut_sprintf(buf, "\t[%*s]\n", 10, s);
  ASSERT_STREQ(buf, "\t[     Hello]\n");
  sut_sprintf(buf, " truncating:\n");
  ASSERT_STREQ(buf, " truncating:\n");
  sut_sprintf(buf, "\t%.4s\n", s);
  ASSERT_STREQ(buf, "\tHell\n");
  sut_sprintf(buf, "\t%.*s\n", 3, s);
  ASSERT_STREQ(buf, "\tHel\n");
  sut_sprintf(buf, "Characters:\t%c %%\n", 'A');
  ASSERT_STREQ(buf, "Characters:\tA %\n");
  sut_sprintf(buf, "Integers:\n");
  ASSERT_STREQ(buf, "Integers:\n");
  sut_sprintf(buf, "\tDecimal:\t%i %d %.6i %i %.0i %+i %i\n", 1, 2, 3, 0, 0, 4,
              -4);
  ASSERT_STREQ(buf, "\tDecimal:\t1 2 000003 0  +4 -4\n");
  sut_sprintf(buf, "\tHexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
  ASSERT_STREQ(buf, "\tHexadecimal:\t5 a A 0x6\n");
  sut_sprintf(buf, "\tOctal:\t\t%o %#o %#o\n", 10, 10, 4);
  ASSERT_STREQ(buf, "\tOctal:\t\t12 012 04\n");
  sut_sprintf(buf, "Floating-point:\n");
  ASSERT_STREQ(buf, "Floating-point:\n");
  //   sut_sprintf(buf, "\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
  //   ASSERT_STREQ(buf, "\tPadding:\t01.50 1.50  1.50\n");
  //   sprintf(buf, "\tScientific:\t%E %e\n", 1.5, 1.5);
  //   ASSERT_STREQ(buf, "\tScientific:\t1.500000E+00 1.500000e+00\n");
  //   sprintf(buf, "\tHexadecimal:\t%a %A\n", 1.5, 1.5);
  //   ASSERT_STREQ(buf, "\tHexadecimal:\t0x1.8p+0 0X1.8P+0\n");
  //   sut_sprintf(buf, "\tSpecial values:\t0/0=%g 1/0=%g\n", 0.0 / 0.0, 1.0 /
  //   0.0);
  //   ASSERT_STREQ(buf, "\tSpecial values:\t0/0=-nan 1/0=inf\n");
  sut_sprintf(buf, "Fixed-width types:\n");
  ASSERT_STREQ(buf, "Fixed-width types:\n");
  sut_sprintf(buf, "\tLargest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n",
              UINT32_MAX, UINT32_MAX);
  ASSERT_STREQ(buf, "\tLargest 32-bit value is 4294967295 or 0xffffffff\n");
}

UTEST(sprintf, align_text_ps_like) {
  char buf[256];
  int len = 0;
  len += sut_sprintf(buf, "%*s", 7, "PID");
  ASSERT_STREQ("    PID", buf);
  len += sut_sprintf(buf+len, " %*s", -8, "TTY");
  ASSERT_STREQ("    PID TTY     ", buf);
  len += sut_sprintf(buf+len, " %*s", 8, "TIME");
  ASSERT_STREQ("    PID TTY          TIME", buf);
  len += sut_sprintf(buf+len, " %*s", -15, "CMD");
  ASSERT_STREQ("    PID TTY          TIME CMD            ", buf);

  buf[0] = '1';
  buf[1] = '\0';
  sut_sprintf(buf + 1, "%*s", 5, "");
  ASSERT_STREQ("1     ", buf);
  len = 0;
  len += sut_sprintf(buf, "%*.*s", 2, 2, "1");
  ASSERT_STREQ(" 1", buf);
  len += sut_sprintf(buf + len, "%*.*s", -8, 8, "?");
  ASSERT_STREQ(" 1?       ", buf);
  len += sut_sprintf(buf + len, "%*.*s", 8, 8, "00:00:00");
  ASSERT_STREQ(" 1?       00:00:00", buf);
  len += sut_sprintf(buf + len, "%*.*s", -1, 59, "stub");
  ASSERT_STREQ(" 1?       00:00:00stub", buf);



  // sut_sprintf(buf, "[%-10s]", "abc");
  // ASSERT_STREQ(buf, "[abc       ]");

  // sut_sprintf(buf, "[%5d]", 42);
  // ASSERT_STREQ(buf, "[   42]");

  // sut_sprintf(buf, "[%-5d]", 42);
  // ASSERT_STREQ(buf, "[42   ]");
}

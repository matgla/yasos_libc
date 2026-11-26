// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
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

/* The number parsers of atoi.c (yasc: `make qemu` runs this on QEMU, at the
 * device's 32-bit long): atoi, atol, strtol, strtoul, strtoll, strtoull.
 *
 * glibc is the reference, kept as data (strto_golden.inc, made by
 * strto_golden.sh): bare-metal newlib, the only libc on QEMU, reads "0x" as
 * no number at all where C (and glibc, and this libc) convert its 0.  The
 * table's long columns come from a 32-bit glibc, so the narrowing of strtol
 * and strtoul -- the device's alone -- is checked too.  What glibc cannot be
 * asked (the 0b prefix: C23 only), a null endptr and errno are asserted
 * below. */

#include "utest.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>

int sut_atoi(const char *s);
long sut_atol(const char *s);
long sut_strtol(const char *s, out char * view * maybe endptr, int base);
unsigned long sut_strtoul(const char *s, out char * view * maybe endptr, int base);
long long sut_strtoll(const char *s, out char * view * maybe endptr, int base);
unsigned long long sut_strtoull(const char *s, out char * view * maybe endptr, int base);

/* The library under test carries its own errno (rename_symbols.sh / the QEMU
 * suite rename every libc symbol): `errno` here would never see what it set. */
extern int sut_errno;

#include "strto_golden.inc"

#define N_OF(a) (sizeof(a) / sizeof((a)[0]))

UTEST(atoi, golden_table_is_complete)
{
  ASSERT_EQ(N_OF(INPUTS) * N_OF(BASES), N_OF(GOLDEN));
}

/* one function's answer against the table's: value, end, ERANGE */
static int same(const char *what, const struct strto_golden *g, int eq, long end, int range,
                int want_end, int want_range)
{
  if (eq && end == want_end && range == want_range)
    return 1;
  printf("%s(\"%s\", base %d): end %ld range %d, want end %d range %d%s\n", what, g->input, g->base, end, range,
         want_end, want_range, eq ? "" : " (value differs)");
  return 0;
}

UTEST(atoi, strtoll_matches_glibc)
{
  for (size_t i = 0; i < N_OF(GOLDEN); i++) {
    const struct strto_golden * view g = &GOLDEN[i];
    char * view end = nullptr;
    sut_errno = 0;
    long long v = sut_strtoll(g->input, &end, g->base);
    ASSERT_TRUE(same("strtoll", g, v == g->ll, end - g->input, sut_errno == ERANGE, g->ll_end, g->ll_range));
  }
}

UTEST(atoi, strtoull_matches_glibc)
{
  for (size_t i = 0; i < N_OF(GOLDEN); i++) {
    const struct strto_golden * view g = &GOLDEN[i];
    char * view end = nullptr;
    sut_errno = 0;
    unsigned long long v = sut_strtoull(g->input, &end, g->base);
    ASSERT_TRUE(same("strtoull", g, v == g->ull, end - g->input, sut_errno == ERANGE, g->ull_end, g->ull_range));
  }
}

/* 32-bit long: the narrowing of the long long answers */
UTEST(atoi, strtol_matches_glibc_32)
{
  ASSERT_EQ(4u, sizeof(long));
  for (size_t i = 0; i < N_OF(GOLDEN); i++) {
    const struct strto_golden * view g = &GOLDEN[i];
    char * view end = nullptr;
    sut_errno = 0;
    long v = sut_strtol(g->input, &end, g->base);
    ASSERT_TRUE(same("strtol", g, v == g->l, end - g->input, sut_errno == ERANGE, g->l_end, g->l_range));
  }
}

UTEST(atoi, strtoul_matches_glibc_32)
{
  for (size_t i = 0; i < N_OF(GOLDEN); i++) {
    const struct strto_golden * view g = &GOLDEN[i];
    char * view end = nullptr;
    sut_errno = 0;
    unsigned long v = sut_strtoul(g->input, &end, g->base);
    ASSERT_TRUE(same("strtoul", g, v == g->ul, end - g->input, sut_errno == ERANGE, g->ul_end, g->ul_range));
  }
}

/* atoi and atol are strtol(s, nullptr, 10) wherever that is defined: the
   value fits.  Past that C says nothing (atoi.c wraps). */
UTEST(atoi, atoi_atol_match_glibc_in_range)
{
  for (size_t i = 0; i < N_OF(GOLDEN); i++) {
    const struct strto_golden * view g = &GOLDEN[i];
    if (g->base != 10 || g->l_range || g->l < INT_MIN || g->l > INT_MAX)
      continue;
    if (sut_atoi(g->input) != g->l || sut_atol(g->input) != g->l)
      printf("atoi/atol(\"%s\"): %d / %ld, want %ld\n", g->input, sut_atoi(g->input), sut_atol(g->input), g->l);
    ASSERT_EQ((int)g->l, sut_atoi(g->input));
    ASSERT_EQ(g->l, sut_atol(g->input));
  }
}

/*
 * The three the assembler actually asks for (tccasm.c parses every operand
 * with strtoull(p, &p, 0)), spelled out so a failure names the instruction.
 */
UTEST(atoi, base_zero_is_read_off_the_prefix)
{
  char * view end = nullptr;

  /* `movs r3, #10` */
  ASSERT_EQ(10ULL, sut_strtoull("10", &end, 0));
  ASSERT_EQ('\0', (int)*end);

  /* `movw r3, #0xCCCD` */
  ASSERT_EQ(0xCCCDULL, sut_strtoull("0xCCCD", &end, 0));
  ASSERT_EQ('\0', (int)*end);

  /* `bne 1b` -- the value is 1 and the answer is that it stopped on the `b`,
   * which is how tccasm.c knows this is a backward label and not a number. */
  ASSERT_EQ(1ULL, sut_strtoull("1b", &end, 0));
  ASSERT_EQ('b', (int)*end);
}

/* The binary prefix, which C23 added and an assembler has always spelled that
   way; glibc answers it only in C23, so it is asserted here. */
UTEST(atoi, binary_prefix)
{
  char * view end = nullptr;

  ASSERT_EQ(5ULL, sut_strtoull("0b101", &end, 0));
  ASSERT_EQ('\0', (int)*end);
  ASSERT_EQ(5ULL, sut_strtoull("0B101", &end, 0));
  ASSERT_EQ('\0', (int)*end);
  ASSERT_EQ(5ULL, sut_strtoull("0b101", &end, 2));
  ASSERT_EQ('\0', (int)*end);
  ASSERT_EQ(-5LL, sut_strtoll("-0b101", &end, 0));
  ASSERT_EQ('\0', (int)*end);
  ASSERT_EQ(-5L, sut_strtol("  -0b101", &end, 0));
  ASSERT_EQ(5UL, sut_strtoul("+0b101", &end, 0));

  /* base 8 and 10 have no such prefix: the subject sequence is the `0` */
  ASSERT_EQ(0ULL, sut_strtoull("0b101", &end, 8));
  ASSERT_EQ('b', (int)*end);
  ASSERT_EQ(0LL, sut_strtoll("0b101", &end, 10));
  ASSERT_EQ('b', (int)*end);
  /* in base 16 `b` is a digit */
  ASSERT_EQ(0xb101ULL, sut_strtoull("0b101", &end, 16));
  ASSERT_EQ('\0', (int)*end);
  /* 32 binary digits: the whole unsigned long, and one past strtol's range */
  ASSERT_EQ(0xffffffffUL, sut_strtoul("0b11111111111111111111111111111111", &end, 0));
  sut_errno = 0;
  ASSERT_EQ(LONG_MAX, sut_strtol("0b10000000000000000000000000000000", &end, 0));
  ASSERT_EQ(ERANGE, sut_errno);
}

/* A prefix with nothing behind it is not a failed conversion: the longest
   initial subsequence of the expected form is the `0`. */
UTEST(atoi, bare_prefix_converts_the_zero)
{
  char * view end = nullptr;
  const char * view minus = "-0b";

  ASSERT_EQ(0ULL, sut_strtoull("0x", &end, 0));
  ASSERT_EQ('x', (int)*end);
  ASSERT_EQ(0ULL, sut_strtoull("0b", &end, 0));
  ASSERT_EQ('b', (int)*end);
  ASSERT_EQ(0LL, sut_strtoll(minus, &end, 0));
  ASSERT_TRUE(end == minus + 2);
  ASSERT_EQ(0ULL, sut_strtoull("0b2", &end, 2));
  ASSERT_EQ('b', (int)*end);

  /* Whereas nothing of the expected form at all hands the caller its own
   * pointer back -- blanks and sign included -- which is how it tells this
   * case from a real zero. */
  const char * view nope = "  -zzz";
  ASSERT_EQ(0ULL, sut_strtoull(nope, &end, 0));
  ASSERT_TRUE(end == nope);
  ASSERT_EQ(0L, sut_strtol(nope, &end, 10));
  ASSERT_TRUE(end == nope);
  ASSERT_EQ(0UL, sut_strtoul(nope, &end, 10));
  ASSERT_TRUE(end == nope);
}

/* ENDPTR may be null: nothing is written, the value is the same */
UTEST(atoi, null_endptr)
{
  ASSERT_EQ(-42L, sut_strtol(" -42x", nullptr, 10));
  ASSERT_EQ(255UL, sut_strtoul("0xff", nullptr, 0));
  ASSERT_EQ(LLONG_MIN, sut_strtoll("-9223372036854775808", nullptr, 10));
  ASSERT_EQ(ULLONG_MAX, sut_strtoull("-1", nullptr, 10));
  ASSERT_EQ(0ULL, sut_strtoull("junk", nullptr, 0));
  ASSERT_EQ(0LL, sut_strtoll("0x", nullptr, 0));
}

/* errno: ERANGE on overflow, and a caller's errno survives every call that
   does not overflow -- strtoul clears it around its strtoull and puts it back */
UTEST(atoi, errno_only_on_overflow)
{
  sut_errno = 1234;
  ASSERT_EQ(5L, sut_strtol("5", nullptr, 10));
  ASSERT_EQ(5UL, sut_strtoul("5", nullptr, 10));
  ASSERT_EQ(4294967295UL, sut_strtoul("-1", nullptr, 10));
  ASSERT_EQ(5LL, sut_strtoll("5", nullptr, 10));
  ASSERT_EQ(5ULL, sut_strtoull("5", nullptr, 10));
  ASSERT_EQ(0LL, sut_strtoll("junk", nullptr, 10));
  ASSERT_EQ(0ULL, sut_strtoull("", nullptr, 10));
  ASSERT_EQ(1234, sut_errno);

  sut_errno = 1234;
  ASSERT_EQ(ULONG_MAX, sut_strtoul("4294967296", nullptr, 10));
  ASSERT_EQ(ERANGE, sut_errno);
  sut_errno = 1234;
  ASSERT_EQ(ULONG_MAX, sut_strtoul("99999999999999999999999", nullptr, 10));
  ASSERT_EQ(ERANGE, sut_errno);
  sut_errno = 1234;
  ASSERT_EQ(LONG_MIN, sut_strtol("-2147483649", nullptr, 10));
  ASSERT_EQ(ERANGE, sut_errno);
  sut_errno = 1234;
  ASSERT_EQ(LLONG_MAX, sut_strtoll("9223372036854775808", nullptr, 10));
  ASSERT_EQ(ERANGE, sut_errno);
}

/* C's strtoul negates a "-" number in unsigned long: "-1" is ULONG_MAX, and
   the range is checked on the magnitude */
UTEST(atoi, strtoul_negates)
{
  char * view end = nullptr;
  sut_errno = 0;
  ASSERT_EQ(ULONG_MAX, sut_strtoul("-1", &end, 10));
  ASSERT_EQ(1UL, sut_strtoul("-4294967295", &end, 10));
  ASSERT_EQ('\0', (int)*end);
  ASSERT_EQ(0, sut_errno);
  ASSERT_EQ(ULONG_MAX, sut_strtoul("\t -4294967296", &end, 10));
  ASSERT_EQ(ERANGE, sut_errno);
  ASSERT_EQ('\0', (int)*end);
  sut_errno = 0;
  ASSERT_EQ(0x80000000UL, sut_strtoul("-0x80000000", &end, 0));
  ASSERT_EQ(0, sut_errno);
}

/*
 * atoi/atol at the limits.  INT_MIN's magnitude does not fit in an int, so an
 * old signed accumulator overflowed on "-2147483648" -- UB inside the libc for
 * an input C defines.  Neither may touch errno, nor read a base prefix.
 */
UTEST(atoi, atoi_atol_limits_leave_errno_alone)
{
  ASSERT_EQ(INT_MAX, sut_atoi("2147483647"));
  ASSERT_EQ(INT_MIN, sut_atoi("-2147483648"));
  ASSERT_EQ(LONG_MAX, sut_atol("2147483647"));
  ASSERT_EQ(LONG_MIN, sut_atol("-2147483648"));
  ASSERT_EQ(-42, sut_atoi(" \t-42abc"));
  ASSERT_EQ(12, sut_atoi("\n\v\f\r 12"));
  ASSERT_EQ(7, sut_atoi("+7"));
  ASSERT_EQ(8, sut_atoi("0008"));
  ASSERT_EQ(0, sut_atoi("0x10"));
  ASSERT_EQ(0, sut_atoi("-"));
  ASSERT_EQ(0, sut_atoi("- 5"));
  ASSERT_EQ(0, sut_atoi("+-5"));
  ASSERT_EQ(0, sut_atoi("x1"));
  ASSERT_EQ(0, sut_atoi(""));
  ASSERT_EQ(3, sut_atoi("3;4"));
  ASSERT_EQ(-9L, sut_atol("  -9z"));

  sut_errno = 1234;
  (void)sut_atoi("99999999999999999999");
  (void)sut_atol("-99999999999999999999999");
  (void)sut_atoi("junk");
  ASSERT_EQ(1234, sut_errno);
}

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

/*
 * `strtoll` / `strtoull`, against the host's.
 *
 * The pair used to be a loop with no base-0 handling, so `base == 0` -- the
 * "read the prefix and work it out" form -- multiplied every partial result by
 * zero and returned the value of the last digit alone.  It reached the screen
 * as a *compiler* bug: `tcc`'s inline assembler parses every asm operand with
 * `strtoull(p, &p, 0)`, so on the device `movs r3, #10` assembled to
 * `movs r3, #0`, `movw r3, #0xCCCD` to `mov.w r3, #13`, and -- because the old
 * loop also ran past the end of the number, swallowing the trailing letter that
 * `tccasm.c` tests for right afterwards -- `bne 1b` stopped being a backward
 * branch.  Three wrong instructions from one arithmetic mistake, and nothing
 * anywhere said so.
 *
 * So the assertion here is differential rather than hand-written: glibc's
 * answer for the same string and base, value AND `endptr` AND whether it set
 * ERANGE.  A table of expected numbers would only be as good as whoever typed
 * it, and this is the function that proved nobody checks.
 */

#include "utest.h"

#include <errno.h>
#include <stdlib.h>

long long sut_strtoll(const char *str, char **endptr, int base);
unsigned long long sut_strtoull(const char *str, char **endptr, int base);

/*
 * The library under test carries its own `errno` -- rename_symbols.sh renames
 * every symbol in the archive, this one included -- so `errno` in this file is
 * the host's and would never see what the SUT set.  Read the SUT's.
 */
extern int sut_errno;

/* The inputs that mattered, plus the ones that are hard: the prefixes, the
 * limits either side, the strings that are not numbers, and `1b`/`1f` -- what
 * a local label looks like to an assembler, and the case where stopping at the
 * right character is the whole answer. */
static const char *const CASES[] = {
    "10",       "100",      "0xCCCD",   "0xCCCC",   "1b",
    "1f",       "4",        "0",        "0777",     "0x0",
    "  42",     "-17",      "+9",       "0X1F",
    "12345678901234567890", "9223372036854775807",  "-9223372036854775808",
    "18446744073709551615", "18446744073709551616", "999999999999999999999999",
    "abc",      "",         "0x",       "7z",       "  -0x10zz",
    "  \t 0755q", "-0",     "1103515245",
};

static const int BASES[] = {0, 2, 8, 10, 16, 36};

/*
 * `0b` is deliberately NOT in that table, because the host cannot be asked
 * about it: glibc only accepts the binary prefix when the translation unit is
 * built as C23 (`strtoull` redirects to `__isoc23_strtoull`), and this suite
 * builds `-std=c11`, where the same glibc answers 0.  An oracle that changes
 * its mind with a compiler flag is not an oracle, so the prefix gets an
 * assertion of its own below.
 */

UTEST(strto, strtoll_matches_host) {
  for (size_t c = 0; c < sizeof(CASES) / sizeof(*CASES); ++c) {
    for (size_t b = 0; b < sizeof(BASES) / sizeof(*BASES); ++b) {
      const char *s = CASES[c];
      char *want_end;
      char *got_end;

      errno = 0;
      long long want = strtoll(s, &want_end, BASES[b]);
      int want_range = errno == ERANGE;
      sut_errno = 0;
      long long got = sut_strtoll(s, &got_end, BASES[b]);
      int got_range = sut_errno == ERANGE;

      ASSERT_EQ_MSG(want, got, s);
      ASSERT_EQ_MSG(want_end - s, got_end - s, s);
      ASSERT_EQ_MSG(want_range, got_range, s);
    }
  }
}

UTEST(strto, strtoull_matches_host) {
  for (size_t c = 0; c < sizeof(CASES) / sizeof(*CASES); ++c) {
    for (size_t b = 0; b < sizeof(BASES) / sizeof(*BASES); ++b) {
      const char *s = CASES[c];
      char *want_end;
      char *got_end;

      errno = 0;
      unsigned long long want = strtoull(s, &want_end, BASES[b]);
      int want_range = errno == ERANGE;
      sut_errno = 0;
      unsigned long long got = sut_strtoull(s, &got_end, BASES[b]);
      int got_range = sut_errno == ERANGE;

      ASSERT_EQ_MSG(want, got, s);
      ASSERT_EQ_MSG(want_end - s, got_end - s, s);
      ASSERT_EQ_MSG(want_range, got_range, s);
    }
  }
}

/*
 * The three the assembler actually asks for, spelled out rather than left to
 * the differential loop -- so a failure names the instruction it breaks rather
 * than a row of a table.
 */
UTEST(strto, base_zero_is_read_off_the_prefix) {
  char *end;

  /* `movs r3, #10` */
  ASSERT_EQ(10ULL, sut_strtoull("10", &end, 0));
  ASSERT_EQ('\0', *end);

  /* `movw r3, #0xCCCD` */
  ASSERT_EQ(0xCCCDULL, sut_strtoull("0xCCCD", &end, 0));
  ASSERT_EQ('\0', *end);

  /* `bne 1b` -- the value is 1 and the answer is that it stopped on the `b`,
   * which is how tccasm.c knows this is a backward label and not a number. */
  ASSERT_EQ(1ULL, sut_strtoull("1b", &end, 0));
  ASSERT_EQ('b', *end);
}

/*
 * The binary prefix, which C23 added and an assembler has always spelled that
 * way.  Asserted rather than compared against the host: see the note on BASES.
 */
UTEST(strto, binary_prefix) {
  char *end;

  ASSERT_EQ(5ULL, sut_strtoull("0b101", &end, 0));
  ASSERT_EQ('\0', *end);
  ASSERT_EQ(5ULL, sut_strtoull("0b101", &end, 2));
  ASSERT_EQ('\0', *end);
  ASSERT_EQ(5LL, sut_strtoll("-0b101", &end, 0) * -1);

  /* Base 8 has no such prefix, so the subject sequence is just the `0`. */
  ASSERT_EQ(0ULL, sut_strtoull("0b101", &end, 8));
  ASSERT_EQ('b', *end);
}

/*
 * A prefix with nothing behind it is not a failed conversion: the longest
 * initial subsequence of the expected form is the `0`.
 */
UTEST(strto, bare_prefix_converts_the_zero) {
  char *end;

  ASSERT_EQ(0ULL, sut_strtoull("0x", &end, 0));
  ASSERT_EQ('x', *end);
  ASSERT_EQ(0ULL, sut_strtoull("0b", &end, 0));
  ASSERT_EQ('b', *end);

  /* Whereas nothing of the expected form at all hands the caller its own
   * pointer back, which is how it tells this case from a real zero. */
  const char *nope = "zzz";
  ASSERT_EQ(0ULL, sut_strtoull(nope, &end, 0));
  ASSERT_EQ(nope, (const char *)end);
}

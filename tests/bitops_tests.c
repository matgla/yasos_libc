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

#include <limits.h>

#include "utest.h"

int sut_ffs(int value);
int sut_ffsl(long value);
int sut_ffsll(long long value);
/* libgcc's __clz/__ctz/__popcount/__parity{si,di}2 are the compiler
   runtime's now, not this libc's (tinycc lib/builtin.c, tested there by
   tests/ir_tests/911_libgcc_bit_helpers.c) */

UTEST(bitops_tests, first_set_bit_family) {
  ASSERT_EQ(0, sut_ffs(0));
  ASSERT_EQ(1, sut_ffs(1));
  ASSERT_EQ(4, sut_ffs(0x28));

  ASSERT_EQ(0, sut_ffsl(0));
  ASSERT_EQ(3, sut_ffsl(4));
  ASSERT_EQ((int)(sizeof(long) * CHAR_BIT - 1),
            sut_ffsl((long)(1UL << ((sizeof(long) * CHAR_BIT) - 2))));

  ASSERT_EQ(0, sut_ffsll(0));
  ASSERT_EQ(2, sut_ffsll(2));
  ASSERT_EQ(64, sut_ffsll(1ULL << 63));
}

/* POSIX: ffs* convert to unsigned, so the sign bit is the highest position. */
UTEST(bitops_tests, first_set_bit_negative_values) {
  const int int_bits = (int)(sizeof(int) * CHAR_BIT);
  const int long_bits = (int)(sizeof(long) * CHAR_BIT);

  ASSERT_EQ(1, sut_ffs(-1));
  ASSERT_EQ(2, sut_ffs(-2));
  ASSERT_EQ(int_bits, sut_ffs(INT_MIN));
  ASSERT_EQ(1, sut_ffs(INT_MAX));

  ASSERT_EQ(1, sut_ffsl(-1L));
  ASSERT_EQ(long_bits, sut_ffsl(LONG_MIN));
  ASSERT_EQ(1, sut_ffsl(LONG_MAX));

  ASSERT_EQ(1, sut_ffsll(-1LL));
  ASSERT_EQ(64, sut_ffsll(LLONG_MIN));
  ASSERT_EQ(1, sut_ffsll(LLONG_MAX));
  /* the high word alone: a 32-bit split must not lose it */
  ASSERT_EQ(33, sut_ffsll(1LL << 32));
  ASSERT_EQ(41, sut_ffsll((1LL << 40) | (1LL << 62)));
}

/* Every bit position, alone and with every higher bit set. */
UTEST(bitops_tests, every_bit_position) {
  const int w = (int)(sizeof(unsigned int) * CHAR_BIT);
  int i;

  for (i = 0; i < w; i++) {
    unsigned int bit = 1U << i;
    unsigned int from = ~0U << i;

    ASSERT_EQ(i + 1, sut_ffs((int)bit));
    ASSERT_EQ(i + 1, sut_ffs((int)from));
    ASSERT_EQ(i + 1, sut_ffsl((long)(1UL << i)));
  }

  for (i = 0; i < 64; i++) {
    unsigned long long bit = 1ULL << i;
    unsigned long long from = ~0ULL << i;

    ASSERT_EQ(i + 1, sut_ffsll((long long)bit));
    ASSERT_EQ(i + 1, sut_ffsll((long long)from));
  }
}

/* Straightforward references, checked on pseudo-random values. */
static int ref_ffs64(unsigned long long v) {
  int i;
  for (i = 0; i < 64; i++)
    if (v & (1ULL << i))
      return i + 1;
  return 0;
}

UTEST(bitops_tests, matches_reference_on_random_values) {
  unsigned long long x = 0x9E3779B97F4A7C15ULL;
  int n;

  for (n = 0; n < 2000; n++) {
    unsigned long long v;
    unsigned int v32;
    unsigned long vl;

    /* xorshift64, then thin out some values so long zero runs occur */
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    v = x;
    if (n & 1)
      v &= x << (n % 61);
    if (n & 2)
      v >>= n % 63;
    v32 = (unsigned int)v;
    vl = (unsigned long)v;

    ASSERT_EQ(ref_ffs64(v32), sut_ffs((int)v32));
    ASSERT_EQ(ref_ffs64(vl), sut_ffsl((long)vl));
    ASSERT_EQ(ref_ffs64(v), sut_ffsll((long long)v));
  }
}

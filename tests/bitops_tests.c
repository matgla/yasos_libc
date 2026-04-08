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
int sut___clzsi2(unsigned int value);
int sut___ctzsi2(unsigned int value);
int sut___paritysi2(unsigned int value);
int sut___popcountsi2(unsigned int value);
int sut___clzdi2(unsigned long long value);
int sut___ctzdi2(unsigned long long value);
int sut___popcountdi2(unsigned long long value);
int sut___paritydi2(unsigned long long value);

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

UTEST(bitops_tests, clz_and_ctz_helpers) {
  ASSERT_EQ((int)(sizeof(unsigned int) * CHAR_BIT), sut___clzsi2(0));
  ASSERT_EQ((int)(sizeof(unsigned int) * CHAR_BIT - 1), sut___clzsi2(1));
  ASSERT_EQ(0, sut___clzsi2(1U << ((sizeof(unsigned int) * CHAR_BIT) - 1)));
  ASSERT_EQ((int)(sizeof(unsigned int) * CHAR_BIT), sut___ctzsi2(0));
  ASSERT_EQ(0, sut___ctzsi2(1));
  ASSERT_EQ(7, sut___ctzsi2(1U << 7));

  ASSERT_EQ((int)(sizeof(unsigned long long) * CHAR_BIT), sut___clzdi2(0));
  ASSERT_EQ((int)(sizeof(unsigned long long) * CHAR_BIT - 1), sut___clzdi2(1));
  ASSERT_EQ(0, sut___clzdi2(1ULL << 63));
  ASSERT_EQ((int)(sizeof(unsigned long long) * CHAR_BIT), sut___ctzdi2(0));
  ASSERT_EQ(0, sut___ctzdi2(1));
  ASSERT_EQ(17, sut___ctzdi2(1ULL << 17));
}

UTEST(bitops_tests, popcount_and_parity_helpers) {
  ASSERT_EQ(0, sut___popcountsi2(0));
  ASSERT_EQ(4, sut___popcountsi2(0xF0));
  ASSERT_EQ(0, sut___paritysi2(0));
  ASSERT_EQ(1, sut___paritysi2(0x07));
  ASSERT_EQ(0, sut___paritysi2(0x0F));

  ASSERT_EQ(0, sut___popcountdi2(0));
  ASSERT_EQ(32, sut___popcountdi2(0xF0F0F0F0F0F0F0F0ULL));
  ASSERT_EQ(0, sut___paritydi2(0));
  ASSERT_EQ(1, sut___paritydi2(0x1111111111111111ULL));
  ASSERT_EQ(0, sut___paritydi2(0x3333333333333333ULL));
}
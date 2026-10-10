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

void *sut_memchr(const void *s, int c, long n);
void *sut_mempcpy(void *dst, const void *src, size_t n);
void *sut_memcpy(void *dst, const void *src, size_t n);
void *sut_memmove(void *dst, const void *src, size_t n);
char *sut_strchr(char *s, int c);
size_t sut_strspn(const char *s, const char *accept);
size_t sut_strcspn(const char *s, const char *reject);
char *sut_strtok(char *s, const char *delimiters);

UTEST(string_tests, memchr) {
  const unsigned char bytes[] = {'A', 'B', 0, 'C', 'B'};

  ASSERT_EQ(bytes + 1, sut_memchr(bytes, 'B', sizeof(bytes)));
  ASSERT_EQ(bytes + 2, sut_memchr(bytes, 0, sizeof(bytes)));
  ASSERT_EQ(NULL, sut_memchr(bytes, 'B', 1));
  ASSERT_EQ(NULL, sut_memchr(bytes, 'Z', sizeof(bytes)));
  ASSERT_EQ(NULL, sut_memchr(bytes, 'A', 0));
}

UTEST(string_tests, mempcpy) {
  unsigned char dst[8] = {0};
  const unsigned char src[] = {'a', 'b', 'c', 'd'};

  ASSERT_EQ(dst + sizeof(src), sut_mempcpy(dst, src, sizeof(src)));
  ASSERT_EQ(0, memcmp(dst, src, sizeof(src)));
  ASSERT_EQ(dst, sut_mempcpy(dst, src, 0));
}

/* memcpy takes a word path once the destination is word-aligned, and the
   unaligned case now copies words with unaligned loads: every length and
   every source/destination byte offset has its own head, tail and loop.
   Compare a full window, so an overwritten guard byte is a failure too. */
UTEST(string_tests, memcpy_alignment_sweep) {
  static unsigned char src[64], dst[80], want[80];

  for (size_t n = 0; n <= 40; ++n)
    for (int so = 0; so < 4; ++so)
      for (int off = 0; off < 4; ++off) {
        for (size_t i = 0; i < sizeof src; ++i)
          src[i] = (unsigned char)(0x81 + i);
        for (size_t i = 0; i < sizeof dst; ++i)
          dst[i] = want[i] = (unsigned char)(0x3f - (i & 7));
        for (size_t i = 0; i < n; ++i)
          want[off + i] = src[so + i];
        ASSERT_EQ(dst + off, sut_memcpy(dst + off, src + so, n));
        ASSERT_EQ(0, memcmp(dst, want, sizeof dst));
      }
}

/* memmove's descending half copies backwards from the end, and its
   ascending half delegates to memcpy: both overlap directions need the
   same sweep now that copies are wider than one byte. */
UTEST(string_tests, memmove_overlap_sweep) {
  static unsigned char buf[96], want[96];

  for (size_t n = 1; n <= 40; ++n)
    for (int gap = 1; gap < 8; ++gap)
      for (int off = 0; off < 4; ++off) {
        for (size_t i = 0; i < sizeof buf; ++i)
          buf[i] = want[i] = (unsigned char)(0x21 + i);
        memmove(want + off, want + off + gap, n); /* host reference */
        sut_memmove(buf + off, buf + off + gap, n);
        ASSERT_EQ(0, memcmp(buf, want, sizeof buf));

        for (size_t i = 0; i < sizeof buf; ++i)
          buf[i] = want[i] = (unsigned char)(0x51 + i);
        memmove(want + off + gap, want + off, n);
        sut_memmove(buf + off + gap, buf + off, n);
        ASSERT_EQ(0, memcmp(buf, want, sizeof buf));
      }
}

UTEST(string_tests, strchr) {
  char *str = "Wait is this a Hello, World?";
  char *result = sut_strchr(str, 'W');
  ASSERT_EQ(result, str);
  result = sut_strchr(str + 1, 'W');
  ASSERT_EQ(result, str + 22);
  result = sut_strchr(str + 23, 'W');
  ASSERT_EQ(result, NULL);
}

UTEST(string_tests, strspn) {
  char *str1 = "Wait is this a Hello, World?";
  char *str2 = "Wita ";
  size_t result = sut_strspn(str1, str2);
  ASSERT_EQ(result, 6);
  result = sut_strspn(str1 + 6, str2);
  ASSERT_EQ(result, 0);
  result = sut_strspn(str1 + 22, str2);
  ASSERT_EQ(result, 1);
}

UTEST(string_tests, strcspn) {
  char *str1 = "Wait is this a Hello, World?";
  char *str2 = "sh?";
  size_t result = sut_strcspn(str1, str2);
  ASSERT_EQ(result, 6);
  result = sut_strcspn(str1 + 6, str2);
  ASSERT_EQ(result, 0);
  result = sut_strcspn(str1 + 22, str2);
  ASSERT_EQ(result, 5);
}

UTEST(string_tests, strtok) {
  char str[] = "Wait is-this a Hello, World?";
  char *delimiters = " -";
  char *result = sut_strtok(str, delimiters);
  ASSERT_STREQ(result, "Wait");
  result = sut_strtok(NULL, delimiters);
  ASSERT_STREQ(result, "is");
  result = sut_strtok(NULL, delimiters);
  ASSERT_STREQ(result, "this");
  result = sut_strtok(NULL, delimiters);
  ASSERT_STREQ(result, "a");
  result = sut_strtok(NULL, delimiters);
  ASSERT_STREQ(result, "Hello,");
  result = sut_strtok(NULL, delimiters);
  ASSERT_STREQ(result, "World?");
  result = sut_strtok(NULL, delimiters);
  ASSERT_EQ(result, NULL);
}
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
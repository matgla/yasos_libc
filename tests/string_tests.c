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

char *sut_strchr(char *s, int c);
size_t sut_strspn(const char *s, const char *accept);
size_t sut_strcspn(const char *s, const char *reject);
char *sut_strtok(char *s, const char *delimiters);

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
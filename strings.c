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

#include <strings.h>

int strncasecmp(const char *s1, const char *s2, size_t n) {
  while (n-- && *s1 && *s2) {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 >= 'A' && c1 <= 'Z')
      c1 += 'a' - 'A';
    if (c2 >= 'A' && c2 <= 'Z')
      c2 += 'a' - 'A';
    if (c1 != c2)
      return c1 - c2;
  }
  return n ? *s1 - *s2 : 0;
}

int strncasecmp_l(const char *s1, const char *s2, size_t n, locale_t locale) {
  // For simplicity, we ignore locale in this implementation.
  // In a real implementation, you would use locale-specific rules for case
  // conversion.
  return strncasecmp(s1, s2, n);
}

int strcasecmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 >= 'A' && c1 <= 'Z')
      c1 += 'a' - 'A';
    if (c2 >= 'A' && c2 <= 'Z')
      c2 += 'a' - 'A';
    if (c1 != c2)
      return c1 - c2;
  }
  return *s1 - *s2;
}
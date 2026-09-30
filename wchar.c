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

#include <wchar.h>
#include <stdlib.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int mbtowc(wchar_t *pwc, const char *s, size_t n) {
  if (s == NULL)
    return 0; /* no shift states */
  if (n == 0)
    return -1;
  if (pwc)
    *pwc = (unsigned char)*s;
  return *s != '\0';
}

size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps) {
  printf("TODO: Implement wcrtomb\n");
  return 0;
}

int wcwidth(wchar_t wc) {
  return isprint(wc) ? 1 : 0;
}
wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, size_t n) {
  return (wchar_t *)memcpy(dst, src, n * sizeof(wchar_t));
}

wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, size_t n) {
  return (wchar_t *)memmove(dst, src, n * sizeof(wchar_t));
}

wchar_t *wmemset(wchar_t *dst, wchar_t c, size_t n) {
  for (size_t i = 0; i < n; i++)
    dst[i] = c;
  return dst;
}

int wmemcmp(const wchar_t *a, const wchar_t *b, size_t n) {
  for (size_t i = 0; i < n; i++)
    if (a[i] != b[i])
      return a[i] < b[i] ? -1 : 1;
  return 0;
}

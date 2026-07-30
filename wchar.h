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

#pragma once

#include <stddef.h>

// wint_t normally arrives from <stddef.h>, which supplies the toolchain's
// __WINT_TYPE__ (unsigned int here) under the canonical _WINT_T guard. Define
// it only as a fallback, and with the same underlying type -- spelling it
// `char` would be an incompatible redefinition, and a 1-byte wint_t cannot
// hold every wchar_t value plus WEOF as the standard requires.
#ifndef _WINT_T
#define _WINT_T
typedef __WINT_TYPE__ wint_t;
#endif

typedef struct mbstate_t {
  int __count;
  unsigned char __value[4];
} mbstate_t;

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

#define WINT_WIDTH 32
#define WINT_MAX 0xffffffffU
#define WINT_MIN 0U

size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
int wcwidth(wchar_t wc);
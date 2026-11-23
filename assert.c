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

/* What a failed assert() calls (assert.h). It writes with write(2) rather
   than stdio, so it still reports when the failure is a broken stream or a
   corrupted heap, and abort()s: the shell reports the signal, a debugger
   stops there. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void put(const char *s)
{
  if (s)
    write(2, s, strlen(s));
}

void __assert_fail(const char *expr, const char *file, unsigned int line, const char *func)
{
  char num[12];
  char *p = num + sizeof(num);
  *--p = '\0';
  do {
    *--p = (char)('0' + line % 10);
    line /= 10;
  } while (line);
  put(file);
  put(":");
  put(p);
  put(": ");
  if (func && *func) {
    put(func);
    put(": ");
  }
  put("Assertion `");
  put(expr);
  put("' failed.\n");
  abort();
}

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

/* The bounded and the explicit string functions: strlcpy and strlcat (POSIX
 * 2024), memset_explicit (C23) and explicit_bzero. */

#include <string.h>

/* SRC into DST, cut to SIZE - 1 bytes and always terminated when SIZE > 0.
   Returns strlen(SRC): a result >= SIZE means it was cut. */
size_t strlcpy(char *dst, const char *src, size_t size)
{
  size_t len = strlen(src);
  if (size) {
    size_t n = len < size - 1 ? len : size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
  }
  return len;
}

/* SRC after the string in DST, the whole within SIZE bytes. Returns the
   length it tried to make; when DST holds no terminator within SIZE, it is
   left alone and the result is SIZE + strlen(SRC). */
size_t strlcat(char *dst, const char *src, size_t size)
{
  size_t dlen = strnlen(dst, size);
  if (dlen == size)
    return size + strlen(src);
  return dlen + strlcpy(dst + dlen, src, size - dlen);
}

/* memset that is never optimised away, for secrets about to go out of scope.
   tcc knows memset by name (its dead store passes drop one into a buffer
   that is not read again) but not this function, so a call is opaque to it;
   the empty asm keeps a compiler that sees through calls (LTO) from dropping
   the stores here. */
void *memset_explicit(void *s, int c, size_t n)
{
  memset(s, c, n);
  __asm__ __volatile__("" : : "r"(s) : "memory");
  return s;
}

void explicit_bzero(void *s, size_t n)
{
  memset_explicit(s, 0, n);
}

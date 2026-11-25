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

/* ffs, ffsl, ffsll (<strings.h>).  The compiler's bit helpers (__clzsi2,
   __popcountdi2, ...) are not the libc's: tinycc's libtcc1 has them
   (lib/builtin.c), as libgcc and compiler-rt do for gcc and clang. */

static int first_set_bit(unsigned long long value) {
  int position = 1;

  while (value != 0) {
    if ((value & 1ULL) != 0)
      return position;
    value >>= 1;
    position++;
  }

  return 0;
}

int ffs(int value) {
  return first_set_bit((unsigned int)value);
}

int ffsl(long value) {
  return first_set_bit((unsigned long)value);
}

int ffsll(long long value) {
  return first_set_bit((unsigned long long)value);
}

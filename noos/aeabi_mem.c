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

/* The ARM EABI memory helpers (RTABI 4.3.4), for the no-OS build on ARM.
 *
 * Compilers emit these rather than memcpy/memset for struct copies and
 * clears, so in a kernel they carry most of the traffic. Userspace gets them
 * from tcc's runtime library; with no such library linked they come from here,
 * and all of them land on the word-wise loops in string.c. The 4/8 variants
 * only promise alignment the generic code re-derives, so they share a body.
 *
 * Note the argument order: __aeabi_memset is (dest, n, c), not memset's. */

#include <stddef.h>
#include <string.h>

void __aeabi_memcpy(void *dest, const void *src, size_t n) {
  memcpy(dest, src, n);
}

void __aeabi_memcpy4(void *dest, const void *src, size_t n) {
  memcpy(dest, src, n);
}

void __aeabi_memcpy8(void *dest, const void *src, size_t n) {
  memcpy(dest, src, n);
}

void __aeabi_memmove(void *dest, const void *src, size_t n) {
  memmove(dest, src, n);
}

void __aeabi_memmove4(void *dest, const void *src, size_t n) {
  memmove(dest, src, n);
}

void __aeabi_memmove8(void *dest, const void *src, size_t n) {
  memmove(dest, src, n);
}

void __aeabi_memset(void *dest, size_t n, int c) {
  memset(dest, c, n);
}

void __aeabi_memset4(void *dest, size_t n, int c) {
  memset(dest, c, n);
}

void __aeabi_memset8(void *dest, size_t n, int c) {
  memset(dest, c, n);
}

void __aeabi_memclr(void *dest, size_t n) {
  memset(dest, 0, n);
}

void __aeabi_memclr4(void *dest, size_t n) {
  memset(dest, 0, n);
}

void __aeabi_memclr8(void *dest, size_t n) {
  memset(dest, 0, n);
}

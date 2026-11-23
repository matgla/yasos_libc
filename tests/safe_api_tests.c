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

/* strlcpy, strlcat, memset_explicit, explicit_bzero, reallocarray and
 * <stdckdint.h>. */

#include "utest.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "syscalls_stub.h"
#include "../stdckdint.h"

size_t sut_strlcpy(char *dst, const char *src, size_t size);
size_t sut_strlcat(char *dst, const char *src, size_t size);
void *sut_memset_explicit(void *s, int c, size_t n);
void sut_explicit_bzero(void *s, size_t n);
void *sut_reallocarray(void *p, size_t n, size_t m);
void sut_free(void *p);
extern int sut_errno;

UTEST(safe_api, strlcpy) {
  char b[8];
  memset(b, 'x', sizeof(b));
  ASSERT_EQ((size_t)3, sut_strlcpy(b, "abc", sizeof(b)));
  ASSERT_STREQ("abc", b);
  ASSERT_EQ((size_t)11, sut_strlcpy(b, "hello world", sizeof(b))); /* cut: result >= size */
  ASSERT_STREQ("hello w", b);
  ASSERT_EQ((size_t)0, sut_strlcpy(b, "", sizeof(b)));
  ASSERT_STREQ("", b);
  memset(b, 'x', sizeof(b));
  ASSERT_EQ((size_t)3, sut_strlcpy(b, "abc", 1));
  ASSERT_EQ('\0', b[0]);
  ASSERT_EQ('x', b[1]);
  ASSERT_EQ((size_t)3, sut_strlcpy(NULL, "abc", 0)); /* size 0: nothing written */
}

UTEST(safe_api, strlcat) {
  char b[8] = "ab";
  ASSERT_EQ((size_t)4, sut_strlcat(b, "cd", sizeof(b)));
  ASSERT_STREQ("abcd", b);
  ASSERT_EQ((size_t)9, sut_strlcat(b, "efghi", sizeof(b)));
  ASSERT_STREQ("abcdefg", b);
  ASSERT_EQ((size_t)7, sut_strlcat(b, "", sizeof(b)));
  /* no terminator within size: dst left alone, size + strlen(src) */
  char full[4] = {'a', 'b', 'c', 'd'};
  ASSERT_EQ((size_t)6, sut_strlcat(full, "xy", sizeof(full)));
  ASSERT_EQ(0, memcmp(full, "abcd", 4));
  ASSERT_EQ((size_t)2, sut_strlcat(NULL, "xy", 0));
}

UTEST(safe_api, memset_explicit_and_explicit_bzero) {
  unsigned char b[16];
  memset(b, 0x5a, sizeof(b));
  ASSERT_TRUE(sut_memset_explicit(b + 2, 0x1ff, 10) == b + 2); /* the fill is (unsigned char)c */
  for (size_t i = 0; i < sizeof(b); i++)
    ASSERT_EQ(i >= 2 && i < 12 ? 0xff : 0x5a, b[i]);
  sut_explicit_bzero(b, 4);
  for (size_t i = 0; i < 4; i++)
    ASSERT_EQ(0, b[i]);
  ASSERT_EQ(0xff, b[4]);
  ASSERT_TRUE(sut_memset_explicit(b, 0, 0) == b);
}

/* malloc's pools come from mmap: the host's */
static void pool_syscalls(int number, const void *args, syscall_result *result) {
  result->result = 0;
  result->err = -1;
  if (number == sys_mmap) {
    const mmap_context *ctx = (const mmap_context *)args;
    void *m = mmap(NULL, (size_t)ctx->length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *ctx->result = m == MAP_FAILED ? NULL : m;
  } else if (number == sys_munmap) {
    const munmap_context *ctx = (const munmap_context *)args;
    munmap(ctx->addr, (size_t)ctx->length);
  } else if (number == sys_mremap) {
    *((const mremap_context *)args)->result = NULL;
  }
}

UTEST(safe_api, reallocarray) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = pool_syscalls;
  int *p = sut_reallocarray(NULL, 4, sizeof(int));
  ASSERT_TRUE(p != NULL);
  for (int i = 0; i < 4; i++)
    p[i] = i * 7;
  int *q = sut_reallocarray(p, 64, sizeof(int));
  ASSERT_TRUE(q != NULL);
  for (int i = 0; i < 4; i++)
    ASSERT_EQ(i * 7, q[i]);
  /* n * m does not fit: null, ENOMEM, and the block is still the caller's */
  sut_errno = 0;
  ASSERT_TRUE(sut_reallocarray(q, SIZE_MAX / 2 + 2, 2) == NULL);
  ASSERT_EQ(ENOMEM, sut_errno);
  ASSERT_TRUE(sut_reallocarray(q, 2, SIZE_MAX) == NULL);
  for (int i = 0; i < 4; i++)
    ASSERT_EQ(i * 7, q[i]);
  sut_free(q);
}

UTEST(safe_api, stdckdint) {
  int r;
  ASSERT_FALSE(ckd_add(&r, 2, 3));
  ASSERT_EQ(5, r);
  ASSERT_TRUE(ckd_add(&r, INT_MAX, 1));
  ASSERT_EQ(INT_MIN, r); /* wrapped */
  ASSERT_TRUE(ckd_sub(&r, INT_MIN, 1));
  ASSERT_TRUE(ckd_mul(&r, 65536, 65536));
  ASSERT_FALSE(ckd_mul(&r, -46340, 46340));
  unsigned u;
  ASSERT_TRUE(ckd_sub(&u, 1, 2)); /* a negative value in an unsigned result */
  ASSERT_EQ(UINT_MAX, u);
  ASSERT_FALSE(ckd_add(&u, -1, 2)); /* operands of other types: the exact value counts */
  ASSERT_EQ(1u, u);
  long long ll;
  ASSERT_FALSE(ckd_mul(&ll, 4000000000u, 2));
  ASSERT_EQ(8000000000LL, ll);
  unsigned char c;
  ASSERT_TRUE(ckd_add(&c, 200, 100));
  ASSERT_EQ(44, c);
  ASSERT_EQ(202311L, __STDC_VERSION_STDCKDINT_H__);
}

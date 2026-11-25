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

/* The _FORTIFY_SOURCE runtime (../fortify.c): a call that fits does what the
 * plain function does, one that does not writes the message and aborts
 * before touching the bytes past the destination.  The syscalls are faked as
 * in assert_tests.c: writes are captured, the kill of abort() jumps back. */

#include "utest.h"

#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

#include "syscalls_stub.h"

void *sut___memcpy_chk(void *dst, const void *src, size_t n, size_t dstlen);
void *sut___memmove_chk(void *dst, const void *src, size_t n, size_t dstlen);
void *sut___mempcpy_chk(void *dst, const void *src, size_t n, size_t dstlen);
void *sut___memset_chk(void *s, int v, size_t n, size_t dstlen);
char *sut___strcpy_chk(char *dst, const char *src, size_t dstlen);
char *sut___stpcpy_chk(char *dst, const char *src, size_t dstlen);
char *sut___strncpy_chk(char *dst, const char *src, size_t n, size_t dstlen);
char *sut___stpncpy_chk(char *dst, const char *src, size_t n, size_t dstlen);
char *sut___strcat_chk(char *dst, const char *src, size_t dstlen);
char *sut___strncat_chk(char *dst, const char *src, size_t n, size_t dstlen);
int sut___sprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, ...);
int sut___snprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, ...);

static const char msg[] = "*** buffer overflow detected ***: terminated\n";
static char out[256];
static int out_len;
static int raised;
static jmp_buf back;

static void fake_syscall(int number, const void *args, syscall_result *result) {
  result->result = 0;
  result->err = -1;
  if (number == sys_write) {
    const write_context *ctx = (const write_context *)args;
    int n = (int)ctx->count;
    if (out_len + n > (int)sizeof(out) - 1)
      n = (int)sizeof(out) - 1 - out_len;
    memcpy(out + out_len, ctx->buf, n);
    out_len += n;
    *ctx->result = n;
  } else if (number == sys_kill) {
    raised = ((const kill_context *)args)->sig;
    longjmp(back, 1);
  }
}

/* an 8-byte destination with guard bytes after it */
static char buf[16];

static void reset(void) {
  memset(out, 0, sizeof(out));
  out_len = 0;
  raised = 0;
  memset(buf, '#', sizeof(buf));
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = fake_syscall;
}

static int guard_intact(void) {
  for (int i = 8; i < (int)sizeof(buf); i++)
    if (buf[i] != '#')
      return 0;
  return 1;
}

#define EXPECT_OVERFLOW(call)                                                                                          \
  do {                                                                                                                 \
    reset();                                                                                                           \
    if (!setjmp(back))                                                                                                 \
      (void)(call);                                                                                                    \
    ASSERT_EQ(SIGABRT, raised);                                                                                        \
    ASSERT_STREQ(msg, out);                                                                                            \
    ASSERT_TRUE(guard_intact());                                                                                       \
  } while (0)

UTEST(fortify, mem_functions_that_fit) {
  reset();
  ASSERT_TRUE(sut___memcpy_chk(buf, "abcdefgh", 8, 8) == buf);
  ASSERT_EQ(0, memcmp(buf, "abcdefgh", 8));
  ASSERT_TRUE(sut___memmove_chk(buf + 1, buf, 7, 7) == buf + 1);
  ASSERT_EQ(0, memcmp(buf, "aabcdefg", 8));
  ASSERT_TRUE(sut___mempcpy_chk(buf, "xy", 2, 8) == buf + 2);
  ASSERT_TRUE(sut___memset_chk(buf, 'z', 8, 8) == buf);
  ASSERT_EQ(0, memcmp(buf, "zzzzzzzz", 8));
  ASSERT_TRUE(sut___memcpy_chk(buf, "abc", 3, (size_t)-1) == buf);
  ASSERT_EQ(0, raised);
  ASSERT_EQ(0, out_len);
  ASSERT_TRUE(guard_intact());
}

UTEST(fortify, mem_functions_that_overflow) {
  EXPECT_OVERFLOW(sut___memcpy_chk(buf, "abcdefghi", 9, 8));
  EXPECT_OVERFLOW(sut___memmove_chk(buf, "abcdefghi", 9, 8));
  EXPECT_OVERFLOW(sut___mempcpy_chk(buf, "abcdefghi", 9, 8));
  EXPECT_OVERFLOW(sut___memset_chk(buf, 0, 9, 8));
}

UTEST(fortify, string_functions_that_fit) {
  reset();
  ASSERT_TRUE(sut___strcpy_chk(buf, "1234567", 8) == buf);
  ASSERT_STREQ("1234567", buf);
  ASSERT_TRUE(sut___stpcpy_chk(buf, "abc", 8) == buf + 3);
  ASSERT_TRUE(sut___strcat_chk(buf, "defg", 8) == buf);
  ASSERT_STREQ("abcdefg", buf);
  ASSERT_TRUE(sut___strncpy_chk(buf, "xy", 8, 8) == buf);
  ASSERT_EQ(0, memcmp(buf, "xy\0\0\0\0\0\0", 8));
  ASSERT_TRUE(sut___stpncpy_chk(buf, "uvw", 3, 8) == buf + 3);
  buf[3] = 0;
  /* only the bytes strncat takes from the source count */
  ASSERT_TRUE(sut___strncat_chk(buf, "1234567890", 4, 8) == buf);
  ASSERT_STREQ("uvw1234", buf);
  ASSERT_EQ(0, raised);
  ASSERT_TRUE(guard_intact());
}

UTEST(fortify, string_functions_that_overflow) {
  EXPECT_OVERFLOW(sut___strcpy_chk(buf, "12345678", 8));
  EXPECT_OVERFLOW(sut___stpcpy_chk(buf, "12345678", 8));
  EXPECT_OVERFLOW(sut___strncpy_chk(buf, "1", 9, 8));
  EXPECT_OVERFLOW(sut___stpncpy_chk(buf, "1", 9, 8));
  reset();
  memcpy(buf, "abcd", 5);
  if (!setjmp(back))
    sut___strcat_chk(buf, "efgh", 8);
  ASSERT_EQ(SIGABRT, raised);
  ASSERT_STREQ(msg, out);
  ASSERT_TRUE(guard_intact());
  reset();
  memcpy(buf, "abcd", 5);
  if (!setjmp(back))
    sut___strncat_chk(buf, "efghijk", 4, 8);
  ASSERT_EQ(SIGABRT, raised);
  ASSERT_TRUE(guard_intact());
  /* a destination with no nul in its size */
  EXPECT_OVERFLOW(sut___strcat_chk(buf, "", 8));
}

UTEST(fortify, printf_functions) {
  reset();
  ASSERT_EQ(7, sut___sprintf_chk(buf, 1, 8, "%d-%s", 12, "abcd"));
  ASSERT_STREQ("12-abcd", buf);
  ASSERT_EQ(9, sut___snprintf_chk(buf, 8, 1, 8, "%d", 123456789));
  ASSERT_STREQ("1234567", buf);
  ASSERT_EQ(3, sut___sprintf_chk(buf, 1, (size_t)-1, "%s", "abc"));
  ASSERT_EQ(0, raised);
  ASSERT_TRUE(guard_intact());
  EXPECT_OVERFLOW(sut___sprintf_chk(buf, 1, 8, "%d", 12345678));
  EXPECT_OVERFLOW(sut___sprintf_chk(buf, 1, 0, "%s", ""));
  EXPECT_OVERFLOW(sut___snprintf_chk(buf, 9, 1, 8, "%d", 1));
}

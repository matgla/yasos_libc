/*
 Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*/

#include "utest.h"

#include <string.h>
#include <sys/syscall.h>

#include "syscalls_stub.h"

int sut_puts(const char *s);
int sut_fputs(const char *s, void *fp);
int sut_fflush(void *fp);
extern void *sut_stdout;

/* Buffer to capture write() output */
static char capture_buf[256];
static int capture_len;

static void capture_write(int number, const void *args,
                          syscall_result *result) {
  (void)number;
  const write_context *ctx = (const write_context *)args;
  int to_copy = (int)ctx->count;
  if (capture_len + to_copy > (int)sizeof(capture_buf))
    to_copy = (int)sizeof(capture_buf) - capture_len;
  memcpy(capture_buf + capture_len, ctx->buf, to_copy);
  capture_len += to_copy;
  *ctx->result = to_copy;
  result->result = 0;
  result->err = -1;
}

static void reset_capture(void) {
  capture_len = 0;
  memset(capture_buf, 0, sizeof(capture_buf));
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = capture_write;
}

UTEST(stdio_tests, puts_returns_char_count) {
  reset_capture();
  int ret = sut_puts("hello");
  /* "hello" + '\n' = 6 */
  ASSERT_EQ(ret, 6);
  ASSERT_EQ(capture_len, 6);
  ASSERT_EQ(0, memcmp(capture_buf, "hello\n", 6));

  reset_capture();
  ret = sut_puts("");
  /* "" + '\n' = 1 */
  ASSERT_EQ(ret, 1);
  ASSERT_EQ(capture_len, 1);
  ASSERT_EQ(capture_buf[0], '\n');

  reset_capture();
  ret = sut_puts("abc");
  ASSERT_EQ(ret, 4);
  ASSERT_EQ(capture_len, 4);
  ASSERT_EQ(0, memcmp(capture_buf, "abc\n", 4));
}

UTEST(stdio_tests, fputs_returns_char_count) {
  reset_capture();
  int ret = sut_fputs("hello", sut_stdout);
  ASSERT_EQ(ret, 5);
  sut_fflush(sut_stdout);
  ASSERT_EQ(capture_len, 5);
  ASSERT_EQ(0, memcmp(capture_buf, "hello", 5));
}

UTEST(stdio_tests, fputs_returns_eof_on_null_file) {
  int ret = sut_fputs("hello", NULL);
  ASSERT_EQ(ret, -1);
}

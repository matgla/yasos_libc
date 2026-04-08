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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include "syscalls_stub.h"

int sut_puts(const char *s);
int sut_fputs(const char *s, void *fp);
int sut_fflush(void *fp);
char *sut_tmpnam(char *buffer);
void *sut_freopen(const char *path, const char *mode, void *fp);
int sut_fileno(void *fp);
extern void *sut_stdout;

/* Buffer to capture write() output */
static char capture_buf[256];
static int capture_len;
static int syscall_sequence[8];
static int syscall_sequence_len;
static int freopen_closed_fd;
static int freopen_open_fd;
static int freopen_open_flags;
static char freopen_open_path[64];

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
  syscall_sequence_len = 0;
  freopen_closed_fd = -1;
  freopen_open_fd = -1;
  freopen_open_flags = 0;
  memset(freopen_open_path, 0, sizeof(freopen_open_path));
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = capture_write;
}

static void record_syscall(int number) {
  if (syscall_sequence_len < (int)(sizeof(syscall_sequence) /
                                   sizeof(syscall_sequence[0]))) {
    syscall_sequence[syscall_sequence_len++] = number;
  }
}

static void freopen_syscalls(int number, const void *args,
                             syscall_result *result) {
  record_syscall(number);

  switch (number) {
  case sys_write: {
    const write_context *ctx = (const write_context *)args;
    int to_copy = (int)ctx->count;
    if (capture_len + to_copy > (int)sizeof(capture_buf))
      to_copy = (int)sizeof(capture_buf) - capture_len;
    memcpy(capture_buf + capture_len, ctx->buf, to_copy);
    capture_len += to_copy;
    *ctx->result = to_copy;
    result->result = 0;
    result->err = -1;
    return;
  }
  case sys_close:
    freopen_closed_fd = *(const int *)args;
    result->result = 0;
    result->err = -1;
    return;
  case sys_open: {
    const open_context *ctx = (const open_context *)args;
    freopen_open_fd = 11;
    freopen_open_flags = ctx->flags;
    strncpy(freopen_open_path, ctx->path, sizeof(freopen_open_path) - 1);
    result->result = freopen_open_fd;
    result->err = -1;
    return;
  }
  default:
    result->result = 0;
    result->err = -1;
    return;
  }
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

UTEST(stdio_tests, tmpnam_generates_unique_paths) {
  char first[L_tmpnam];
  char second[L_tmpnam];
  char *internal;

  ASSERT_EQ(first, sut_tmpnam(first));
  ASSERT_EQ(second, sut_tmpnam(second));
  ASSERT_NE(NULL, sut_tmpnam(NULL));

  internal = sut_tmpnam(NULL);
  ASSERT_NE(NULL, internal);
  ASSERT_STREQ(first, first);
  ASSERT_STREQ(second, second);
  ASSERT_NE(0, strcmp(first, second));
  ASSERT_EQ(0, memcmp(first, P_tmpdir "/tmp_", 9));
  ASSERT_EQ(0, memcmp(second, P_tmpdir "/tmp_", 9));
  ASSERT_EQ(0, memcmp(internal, P_tmpdir "/tmp_", 9));
}

UTEST(stdio_tests, freopen_flushes_closes_and_reopens_stream) {
  reset_capture();
  trigger_supervisor_call_fake.custom_fake = freopen_syscalls;

  ASSERT_EQ(3, sut_fputs("abc", sut_stdout));
  ASSERT_EQ(sut_stdout, sut_freopen("/tmp/reopened.log", "w", sut_stdout));

  ASSERT_EQ(3, capture_len);
  ASSERT_EQ(0, memcmp(capture_buf, "abc", 3));
  ASSERT_EQ(3, syscall_sequence_len);
  ASSERT_EQ(sys_write, syscall_sequence[0]);
  ASSERT_EQ(sys_close, syscall_sequence[1]);
  ASSERT_EQ(sys_open, syscall_sequence[2]);
  ASSERT_EQ(STDOUT_FILENO, freopen_closed_fd);
  ASSERT_STREQ("/tmp/reopened.log", freopen_open_path);
  ASSERT_EQ(O_WRONLY | O_CREAT | O_TRUNC, freopen_open_flags);
  ASSERT_EQ(freopen_open_fd, sut_fileno(sut_stdout));
}

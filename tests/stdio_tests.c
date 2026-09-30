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
#include <sys/mman.h>
#include <sys/syscall.h>

#include "syscalls_stub.h"

int sut_puts(const char *s);
int sut_fputs(const char *s, void *fp);
int sut_fflush(void *fp);
char *sut_tmpnam(char *buffer);
void *sut_freopen(const char *path, const char *mode, void *fp);
int sut_fileno(void *fp);
void *sut_fdopen(int fd, const char *mode);
long sut_fread(void *v, long sz, long n, void *fp);
int sut_feof(void *fp);
int sut_ferror(void *fp);
void sut_clearerr(void *fp);
int sut_ungetc(int c, void *fp);
int sut_fgetc(void *fp);
int sut_fclose(void *fp);
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
  /* freopen() reopens a stream and (re)allocates its buffers, so the libc
     allocator's internal mmap/munmap may fire here. Service them with the real
     host mmap so malloc() returns usable, page-aligned memory — but do NOT
     record them: the test asserts only the observable I/O sequence
     (write, close, open). */
  if (number == sys_mmap) {
    const mmap_context *ctx = (const mmap_context *)args;
    *ctx->result = mmap(NULL, (size_t)ctx->length, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    result->result = 0;
    result->err = -1;
    return;
  }
  if (number == sys_munmap) {
    const munmap_context *ctx = (const munmap_context *)args;
    munmap(ctx->addr, (size_t)ctx->length);
    result->result = 0;
    result->err = -1;
    return;
  }

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

/* tmpnam probes each candidate with access(F_OK) and keeps the first one that
   does NOT already exist. Report "not found" (rc != 0) for every probe so the
   first candidate is accepted; without this the stub leaves access() returning
   0 ("exists") and tmpnam exhausts TMP_MAX and returns NULL. */
static void tmpnam_access_not_found(int number, const void *args,
                                    syscall_result *result) {
  (void)number;
  (void)args;
  result->result = -1;
  result->err = -1;
}

UTEST(stdio_tests, tmpnam_generates_unique_paths) {
  char first[L_tmpnam];
  char second[L_tmpnam];
  char *internal;

  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = tmpnam_access_not_found;

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

/* End-of-file and error state of an input stream: feof() used to peek a byte
 * into a `char`, which is unsigned on ARM, so it never reported the end and
 * pushed back a 0xFF for the next fread() -- GNU make's
 * "while (!feof(stdin)) fread()" copy of `make -f -` input never ended. */
static const char *feof_input;
static int feof_input_len;
static int feof_input_pos;
static int feof_read_fails;

static void feof_syscalls(int number, const void *args, syscall_result *result) {
  result->err = 0;
  result->result = 0;
  if (number == sys_mmap) {
    const mmap_context *ctx = args;
    *ctx->result = mmap(NULL, ctx->length, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  } else if (number == sys_munmap) {
    const munmap_context *ctx = args;
    munmap(ctx->addr, ctx->length);
  } else if (number == sys_read) {
    const read_context *ctx = args;
    int n = feof_input_len - feof_input_pos;
    if (feof_read_fails) {
      result->err = 5; /* EIO */
      result->result = -1;
      return;
    }
    if (n > (int)ctx->count)
      n = (int)ctx->count;
    memcpy(ctx->buf, feof_input + feof_input_pos, n);
    feof_input_pos += n;
    *ctx->result = n;
  }
}

static void *feof_open(const char *input, int fails) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = feof_syscalls;
  feof_input = input;
  feof_input_len = (int)strlen(input);
  feof_input_pos = 0;
  feof_read_fails = fails;
  return sut_fdopen(3, "r");
}

UTEST(stdio_tests, fread_loop_until_feof_ends) {
  void *fp = feof_open("abc", 0);
  char buf[16], all[64];
  long n, total = 0;
  int rounds = 0;

  ASSERT_TRUE(fp != NULL);
  ASSERT_EQ(0, sut_feof(fp));
  while (!sut_feof(fp) && !sut_ferror(fp) && rounds++ < 10) {
    n = sut_fread(buf, 1, sizeof(buf), fp);
    memcpy(all + total, buf, n);
    total += n;
  }
  ASSERT_LT(rounds, 10);
  ASSERT_EQ(3, total);
  ASSERT_EQ(0, memcmp(all, "abc", 3));
  ASSERT_NE(0, sut_feof(fp));
  ASSERT_EQ(0, sut_ferror(fp));
  sut_fclose(fp);
}

UTEST(stdio_tests, feof_does_not_consume_input) {
  void *fp = feof_open("xy", 0);

  ASSERT_EQ(0, sut_feof(fp));
  ASSERT_EQ('x', sut_fgetc(fp));
  ASSERT_EQ(0, sut_feof(fp));
  ASSERT_EQ('y', sut_fgetc(fp));
  ASSERT_EQ(0, sut_feof(fp)); /* the end is only known once a read finds it */
  ASSERT_EQ(EOF, sut_fgetc(fp));
  ASSERT_NE(0, sut_feof(fp));
  sut_fclose(fp);
}

UTEST(stdio_tests, ungetc_and_clearerr_reset_eof) {
  void *fp = feof_open("", 0);

  ASSERT_EQ(EOF, sut_fgetc(fp));
  ASSERT_NE(0, sut_feof(fp));
  ASSERT_EQ('q', sut_ungetc('q', fp));
  ASSERT_EQ(0, sut_feof(fp));
  ASSERT_EQ('q', sut_fgetc(fp));
  ASSERT_EQ(EOF, sut_ungetc(EOF, fp));
  ASSERT_EQ(EOF, sut_fgetc(fp));
  ASSERT_NE(0, sut_feof(fp));
  sut_clearerr(fp);
  ASSERT_EQ(0, sut_feof(fp));
  sut_fclose(fp);
}

UTEST(stdio_tests, read_error_sets_ferror_not_feof) {
  void *fp = feof_open("zzz", 1);
  char buf[8];

  ASSERT_EQ(0, sut_fread(buf, 1, sizeof(buf), fp));
  ASSERT_NE(0, sut_ferror(fp));
  ASSERT_EQ(0, sut_feof(fp));
  sut_clearerr(fp);
  ASSERT_EQ(0, sut_ferror(fp));
  sut_fclose(fp);
}

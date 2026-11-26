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

/* assert(): the message __assert_fail writes, the abort after it, and NDEBUG
 * read again at each inclusion. The syscalls are faked: writes are captured,
 * the kill of abort() jumps back here. The macro is libc's (../assert.h), its
 * call renamed to the renamed libc function. */

#include "utest.h"

#include <setjmp.h>
#include <signal.h>
#include <string.h>

#include "syscalls_stub.h"

#define __assert_fail sut___assert_fail
#include "../assert.h"

static char buf[256];
static size_t out_len;
static int raised;
static jmp_buf back;

static void fake_syscall(int number, const void *args, inout syscall_result *result) {
  result->result = 0;
  result->err = -1;
  if (number == sys_write) {
    const write_context *view ctx = cast<ptr(const write_context *view)>(args);
    size_t n = ctx->count;
    if (out_len + n > cast<int(int)>(sizeof(buf)) - 1)
      n = sizeof(buf) - 1 - out_len;
    memcpy(buf + out_len, ctx->buf, n);
    out_len += n;
    *ctx->result = cast<int(ssize_t)>(n);
  } else if (number == sys_kill) {
    raised = cast<ptr(const kill_context *view)>(args)->sig;
    longjmp(back, 1);
  }
}

static void reset(void) {
  memset(buf, 0, sizeof(buf));
  out_len = 0;
  raised = 0;
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = fake_syscall;
}

UTEST(assert, failure_prints_and_aborts) {
  reset();
  volatile int line = 0; /* set after setjmp, read after longjmp (C 7.13.2.1) */
  if (!setjmp(back)) {
    line = __LINE__ + 1;
    assert(1 + 1 == 3);
  }
  ASSERT_EQ(SIGABRT, raised);
  char want[256] = undefined;
  snprintf(want, sizeof(want), "%s:%d: %s: Assertion `1 + 1 == 3' failed.\n", __FILE__, line, __func__);
  ASSERT_STREQ(want, buf);
}

UTEST(assert, true_and_commas_do_nothing) {
  reset();
  if (!setjmp(back)) {
    assert(1);
    assert((int[]){1, 2}[1] == 2); /* a comma: one argument (C23) */
  }
  ASSERT_EQ(0, raised);
  ASSERT_EQ(0U, out_len);
}

UTEST(assert, no_function_name) {
  reset();
  if (!setjmp(back))
    sut___assert_fail("x", "f.c", 7, "");
  ASSERT_EQ(SIGABRT, raised);
  ASSERT_STREQ("f.c:7: Assertion `x' failed.\n", buf);
}

#define NDEBUG
#include "../assert.h"
static int ndebug_assert(void) {
  assert(0);
  return 1;
}
#undef NDEBUG
#include "../assert.h"

UTEST(assert, ndebug_at_each_inclusion) {
  reset();
  if (!setjmp(back))
    ASSERT_EQ(1, ndebug_assert());
  ASSERT_EQ(0, raised);
  if (!setjmp(back))
    assert(0);
  ASSERT_EQ(SIGABRT, raised);
}

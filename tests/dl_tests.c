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

/* dl.c (yasc: `make qemu` runs this on QEMU): dlopen, dlsym and dlclose over
 * faked system calls, and dlerror -- the last failure's message, naming the
 * path or symbol and the kernel's errno, read once; a call that works leaves
 * it and errno as they were. apps/dltest checks the same on the device. */

#include "utest.h"

#include <errno.h>
#include <string.h>

#include "syscalls_stub.h"

/* libc's (dlfcn.h): the tests build against newlib's headers, which have none */
#define RTLD_NOW 0x00002

int sut_dlclose(drop void *owned handle);
void *?owned(sut_dlclose) sut_dlopen(char *?path, int mode);
void *?borrows(static) sut_dlsym(inout void *?handle, char *name);
char *?borrows(static) sut_dlerror(void);
char *borrows(static) sut_strerror(int errnum);

/* The library under test carries its own errno (the QEMU suite renames every
 * libc symbol). */
extern int sut_errno;

static int fail_with; /* the errno the next dl call fails with, 0: it works */
static int calls;
static int handle_object; /* what a dlopen that works gives */
static int symbol_object; /* what a dlsym that works finds */

static void fake_syscall(int number, void *args, inout syscall_result *result) {
  ++calls;
  result->err = fail_with;
  result->result = fail_with ? -1 : 0;
  if (fail_with)
    return;
  if (number == sys_dlopen)
    *cast<ptr(const dlopen_context *view)>(args)->result = &handle_object;
  else if (number == sys_dlsym)
    *cast<ptr(const dlsym_context *view)>(args)->result = &symbol_object;
}

static void reset(void) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = fake_syscall;
  fail_with = 0;
  calls = 0;
  sut_errno = 0;
  (void)sut_dlerror(); /* what an earlier test left */
}

/* "WHAT: NAME: strerror(ERR)" in WANT, NAME left out when null */
static void message_for(out char want[192], char *what, char *?name, int err) {
  if (name)
    snprintf(want, 192, "%s: %s: %s", what, name, sut_strerror(err));
  else
    snprintf(want, 192, "%s: %s", what, sut_strerror(err));
}

/* dlerror() says that once, then nothing */
#define EXPECT_MESSAGE(what, name, err)                                                                                \
  do {                                                                                                                 \
    char want[192] = undefined;                                                                                        \
    message_for(want, what, name, err);                                                                                \
    const char *?msg = sut_dlerror();                                                                             \
    ASSERT_TRUE(msg != nullptr);                                                                                       \
    ASSERT_STREQ(want, msg);                                                                                           \
    ASSERT_TRUE(sut_dlerror() == nullptr);                                                                             \
  } while (0)

UTEST(dl, dlerror_is_null_without_a_failure) {
  reset();
  ASSERT_TRUE(sut_dlerror() == nullptr);
  ASSERT_EQ(0, calls);
}

UTEST(dl, dlopen_failure_names_the_path) {
  reset();
  fail_with = ENOENT;
  ASSERT_TRUE(sut_dlopen("/nonexistent/libnope.so", RTLD_NOW) == nullptr);
  ASSERT_EQ(ENOENT, sut_errno);
  EXPECT_MESSAGE("dlopen", "/nonexistent/libnope.so", ENOENT);
}

UTEST(dl, dlopen_of_null_failing_says_no_name) {
  reset();
  fail_with = EINVAL;
  ASSERT_TRUE(sut_dlopen(nullptr, 0) == nullptr);
  EXPECT_MESSAGE("dlopen", nullptr, EINVAL);
}

UTEST(dl, calls_that_work_leave_errno_and_say_nothing) {
  reset();
  sut_errno = EAGAIN;
  /* every call before the first ASSERT, which may return with h open */
  void *?owned(sut_dlclose) h = sut_dlopen("libdltest.so", RTLD_NOW);
  const bool opened = h == &handle_object;
  const bool found = sut_dlsym(h, "dltest_answer") == &symbol_object;
  int closed = -1;
  if (h)
    closed = sut_dlclose(h);
  ASSERT_TRUE(opened);
  ASSERT_TRUE(found);
  ASSERT_EQ(0, closed);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_TRUE(sut_dlerror() == nullptr);
  ASSERT_EQ(3, calls);
}

UTEST(dl, dlsym_failure_names_the_symbol) {
  reset();
  fail_with = ENOENT;
  ASSERT_TRUE(sut_dlsym(nullptr, "dltest_missing") == nullptr);
  EXPECT_MESSAGE("dlsym", "dltest_missing", ENOENT);
}

UTEST(dl, dlclose_failure_is_reported) {
  reset();
  fail_with = EINVAL;
  int local = 0;
  ASSERT_NE(0, sut_dlclose(&local));
  EXPECT_MESSAGE("dlclose", nullptr, EINVAL);
}

UTEST(dl, the_last_failure_is_reported) {
  reset();
  fail_with = ENOENT;
  (void)sut_dlopen("/nonexistent/first.so", RTLD_NOW);
  (void)sut_dlsym(nullptr, "dltest_second_missing");
  EXPECT_MESSAGE("dlsym", "dltest_second_missing", ENOENT);
}

UTEST(dl, a_call_that_works_keeps_the_message) {
  reset();
  fail_with = ENOENT;
  (void)sut_dlopen("/nonexistent/libnope.so", RTLD_NOW);
  fail_with = 0;
  ASSERT_TRUE(sut_dlsym(nullptr, "dltest_answer") == &symbol_object);
  EXPECT_MESSAGE("dlopen", "/nonexistent/libnope.so", ENOENT);
}

UTEST(dl, a_long_path_is_cut_not_overrun) {
  reset();
  char path[400] = undefined;
  memset(path, 'a', sizeof path - 1);
  path[sizeof path - 1] = '\0';
  fail_with = ENOENT;
  ASSERT_TRUE(sut_dlopen(path, RTLD_NOW) == nullptr);
  const char *?msg = sut_dlerror();
  ASSERT_TRUE(msg != nullptr);
  const bool fits = strlen(msg) < 192;
  ASSERT_TRUE(fits);
  ASSERT_EQ(0, strncmp(msg, "dlopen: aaaa", 12));
}

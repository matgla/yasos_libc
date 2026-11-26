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

/* openat() (fcntl.c): the sys_open context it builds -- path, flags and dirfd
 * as given, the mode read from the variadic argument only with O_CREAT and
 * passed through the creation mask (umask, sys/stat.c) -- and the kernel's
 * answer handed back, errno set from it on a failure.  The syscall is faked;
 * the fake copies the context, which lives in openat's frame. */

#include "utest.h"

#include <errno.h>
#include <string.h>

#include "syscalls_stub.h"

/* libc's own <fcntl.h>, not the toolchain's: newlib numbers O_CREAT and the
 * rest apart, and the kernel reads libc's numbers. */
#include "../include/fcntl.h"

/* Declared without fcntl.h's owned(close): the fds here are the fake's
 * numbers, nothing to close. */
int sut_openat(int dirfd, const char *pathname, int flags, ...);
int sut_umask(int mask);

/* The library under test carries its own errno (the QEMU suite renames every
 * libc symbol): `errno` here would never see what it set. */
extern int sut_errno;

static struct {
  int number;
  char path[64];
  int path_null;
  int flags;
  int mode;
  int fd;
} seen;

/* what the fake answers */
static int answer_result;
static int answer_err;

static void fake_syscall(int number, const void *args, inout syscall_result *result) {
  seen.number = number;
  if (number == sys_open) {
    const open_context *view ctx = cast<ptr(const open_context *view)>(args);
    seen.path_null = ctx->path == nullptr;
    if (ctx->path != nullptr) {
      strncpy(seen.path, ctx->path, sizeof(seen.path) - 1);
      seen.path[sizeof(seen.path) - 1] = '\0';
    }
    seen.flags = ctx->flags;
    seen.mode = ctx->mode;
    seen.fd = ctx->fd;
  }
  result->result = answer_result;
  result->err = answer_err;
}

/* The creation mask openat sees, and the one put back after each test:
 * libc's default (sys/stat.c), so the other suites find it as they would. */
#define DEFAULT_UMASK 022

static void reset(int result, int err) {
  memset(&seen, 0, sizeof(seen));
  seen.mode = -1;
  answer_result = result;
  answer_err = err;
  sut_errno = 0;
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = fake_syscall;
}

UTEST(fcntl, openat_passes_path_flags_and_dirfd) {
  reset(5, 0);
  ASSERT_EQ(5, sut_openat(3, "dir/file.txt", O_RDWR | O_APPEND));
  ASSERT_EQ(1u, trigger_supervisor_call_fake.call_count);
  ASSERT_TRUE(seen.number == sys_open);
  ASSERT_FALSE(seen.path_null);
  ASSERT_STREQ("dir/file.txt", seen.path);
  ASSERT_EQ(O_RDWR | O_APPEND, seen.flags);
  ASSERT_EQ(3, seen.fd);
  ASSERT_EQ(0, sut_errno);
}

UTEST(fcntl, openat_at_fdcwd_reaches_the_kernel) {
  reset(4, 0);
  ASSERT_EQ(4, sut_openat(AT_FDCWD, "relative", O_RDONLY));
  ASSERT_EQ(AT_FDCWD, seen.fd);
  ASSERT_STREQ("relative", seen.path);
  ASSERT_EQ(O_RDONLY, seen.flags);
}

/* Without O_CREAT the variadic mode is not read: a mode given anyway (or
 * none at all) goes to the kernel as 0. */
UTEST(fcntl, openat_without_creat_sends_mode_zero) {
  reset(6, 0);
  ASSERT_EQ(6, sut_openat(AT_FDCWD, "a", O_WRONLY | O_TRUNC));
  ASSERT_EQ(0, seen.mode);

  reset(6, 0);
  ASSERT_EQ(6, sut_openat(AT_FDCWD, "a", O_WRONLY | O_TRUNC, 0777));
  ASSERT_EQ(0, seen.mode);
  ASSERT_EQ(O_WRONLY | O_TRUNC, seen.flags);
}

UTEST(fcntl, openat_creat_applies_the_default_umask) {
  reset(7, 0);
  ASSERT_EQ(DEFAULT_UMASK, sut_umask(DEFAULT_UMASK));
  ASSERT_EQ(7, sut_openat(AT_FDCWD, "new", O_WRONLY | O_CREAT, 0666));
  ASSERT_EQ(0644, seen.mode);
  ASSERT_EQ(O_WRONLY | O_CREAT, seen.flags);
}

UTEST(fcntl, openat_creat_follows_umask) {
  reset(8, 0);
  sut_umask(0);
  ASSERT_EQ(8, sut_openat(AT_FDCWD, "open", O_RDWR | O_CREAT, 0777));
  ASSERT_EQ(0777, seen.mode);

  reset(8, 0);
  sut_umask(077);
  ASSERT_EQ(8, sut_openat(AT_FDCWD, "private", O_RDWR | O_CREAT, 0666));
  ASSERT_EQ(0600, seen.mode);

  /* the mask takes permission bits only; it never adds any */
  reset(8, 0);
  sut_umask(0);
  ASSERT_EQ(8, sut_openat(AT_FDCWD, "ro", O_RDWR | O_CREAT, 0444));
  ASSERT_EQ(0444, seen.mode);

  sut_umask(DEFAULT_UMASK);
}

/* the bits above the permissions (setuid, setgid, sticky) are not masked */
UTEST(fcntl, openat_creat_keeps_special_bits) {
  reset(9, 0);
  sut_umask(DEFAULT_UMASK);
  ASSERT_EQ(9, sut_openat(AT_FDCWD, "s", O_WRONLY | O_CREAT, 04777));
  ASSERT_EQ(04755, seen.mode);
}

UTEST(fcntl, openat_creat_excl_flags_unchanged) {
  reset(10, 0);
  sut_umask(DEFAULT_UMASK);
  int flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC;
  ASSERT_EQ(10, sut_openat(4, "lock", flags, 0600));
  ASSERT_EQ(flags, seen.flags);
  ASSERT_EQ(4, seen.fd);
  ASSERT_EQ(0600, seen.mode);
}

UTEST(fcntl, openat_failure_sets_errno) {
  reset(-1, ENOENT);
  ASSERT_EQ(-1, sut_openat(AT_FDCWD, "missing", O_RDONLY));
  ASSERT_EQ(ENOENT, sut_errno);

  reset(-1, EEXIST);
  sut_umask(DEFAULT_UMASK);
  ASSERT_EQ(-1, sut_openat(AT_FDCWD, "there", O_WRONLY | O_CREAT | O_EXCL, 0644));
  ASSERT_EQ(EEXIST, sut_errno);
}

/* a success leaves errno as it was (POSIX: errno is set only on failure) */
UTEST(fcntl, openat_success_keeps_errno) {
  reset(3, 0);
  sut_errno = EINTR;
  ASSERT_EQ(3, sut_openat(AT_FDCWD, "x", O_RDONLY));
  ASSERT_EQ(EINTR, sut_errno);
}

/* the path pointer is passed as it is: the kernel checks it, not libc */
UTEST(fcntl, openat_empty_path_passed_through) {
  reset(-1, ENOENT);
  ASSERT_EQ(-1, sut_openat(AT_FDCWD, "", O_RDONLY));
  ASSERT_FALSE(seen.path_null);
  ASSERT_STREQ("", seen.path);
  ASSERT_EQ(ENOENT, sut_errno);
}

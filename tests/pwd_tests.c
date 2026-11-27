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

/* pwd.c and setuid/seteuid over a faked kernel (/etc/passwd from a string) */

#include "utest.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "serve_memory.h" /* and syscalls_stub.h */

/* libc's, which the tests' newlib headers do not have or number apart */
#define LIBC_O_CLOEXEC 02000000
#define LIBC_ENOSYS 38 /* newlib's is 88; the rest these tests use agree */

struct passwd {
  const char *pw_name;
  const char *pw_passwd;
  uid_t pw_uid;
  gid_t pw_gid;
  const char *pw_gecos;
  const char *pw_dir;
  const char *pw_shell;
};

struct passwd *?borrows(static) sut_getpwnam(char *name);
struct passwd *?borrows(static) sut_getpwuid(uid_t uid);
int sut_getpwnam_r(char *name, out struct passwd *pwd, inout char *buf, size_t size,
                   out struct passwd *?borrows(pwd) *result);
int sut_getpwuid_r(uid_t uid, out struct passwd *pwd, inout char *buf, size_t size,
                   out struct passwd *?borrows(pwd) *result);
struct passwd *?borrows(static) sut_getpwent(void);
void sut_setpwent(void);
void sut_endpwent(void);
int sut_setuid(uid_t uid);
int sut_seteuid(uid_t uid);
uid_t sut_getuid(void);

/* The library under test carries its own errno (the QEMU suite renames every
 * libc symbol). */
extern int sut_errno;

#define ALICE "alice:x:1000:100:Alice Liddell,Room 1:/home/alice:/bin/sh"

static const char DB[] = "root:x:0:0:root:/root:/bin/sh\n"
"# comment:x:1:1::/:/bin/sh\n"
"daemon:*:1:1:daemon:/usr/sbin:/sbin/nologin\n"
"\n"
"six:x:2:2:a:/b\n"
"eight:x:3:3:a:/b:/c:d\n"
"baduid:x:x1:4::/:/bin/sh\n"
"badgid:x:5:-1::/:/bin/sh\n"
"nouid:x::5::/:/bin/sh\n"
"toobig:x:65536:5::/:/bin/sh\n"
"+nis:x:6:6::/:/bin/sh\n"
"-nis:x:7:7::/:/bin/sh\n" ALICE "\n"
"nologin:x:1001:100:::\n"
"twin:x:1000:100:Twin:/home/twin:/bin/sh\n"
"last:x:1002:100:Last:/home/last:/bin/sh";

#define FAKE_FD 9

static struct {
  const char *?content; /* nullptr: open fails with open_err */
  int open_err;
  int read_err; /* the read after read_after_bytes fails with it */
  size_t read_after_bytes;
  size_t chunk; /* at most this much a read, 0: as much as asked */
  size_t pos;
  int opens, closes, reads;
  int open_flags;
  int id_err; /* setuid/seteuid: ENOSYS, or 0 to answer 0 */
  int id_calls;
  int last_number;
  int last_uid;
} k;

static void fake_syscall(int number, void *args, inout syscall_result *result) {
  if (serve_memory(number, args, result))
    return;
  result->err = 0;
  result->result = 0;
  k.last_number = number;
  if (number == sys_open) {
    const auto ctx = cast<ptr(const open_context *)>(args);
    ++k.opens;
    k.open_flags = ctx->flags;
    if (!ctx->path || strcmp(ctx->path, "/etc/passwd") != 0 || !k.content) {
      result->err = k.content ? ENOENT : k.open_err;
      result->result = -1;
      return;
    }
    k.pos = 0;
    result->result = FAKE_FD;
  } else if (number == sys_read) {
    const auto ctx = cast<ptr(const read_context *)>(args);
    ++k.reads;
    const char *?content = k.content;
    if (ctx->fd != FAKE_FD || !content) {
      result->err = EBADF;
      result->result = -1;
      return;
    }
    if (k.read_err && k.pos >= k.read_after_bytes) {
      result->err = k.read_err;
      result->result = -1;
      return;
    }
    size_t n = strlen(content) - k.pos;
    if (n > ctx->count)
      n = ctx->count;
    if (k.chunk && n > k.chunk)
      n = k.chunk;
    void *?buf = ctx->buf;
    if (buf)
      memcpy(buf, content + k.pos, n);
    k.pos += n;
    *ctx->result = cast<int(ssize_t)>(n);
  } else if (number == sys_close) {
    ++k.closes;
  } else if (number == sys_setuid || number == sys_seteuid) {
    const auto ctx = cast<ptr(const setuid_context *)>(args);
    ++k.id_calls;
    k.last_uid = ctx->uid;
    result->err = k.id_err;
    result->result = k.id_err ? -1 : 0;
  } else if (number == sys_getuid) {
    result->result = 0;
  } else {
    result->err = LIBC_ENOSYS;
    result->result = -1;
  }
}

/* an id as an int, for the assertions */
static int I(uid_t id) {
  return id;
}

static void reset(void) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = fake_syscall;
  memset(&k, 0, sizeof k);
  k.open_err = ENOENT;
  k.id_err = LIBC_ENOSYS;
  sut_endpwent(); /* what an earlier test left open */
  k.closes = 0;
  sut_errno = 0;
}

#define EXPECT_USER(p, name, passwd, uid, gid, gecos, dir, shell)                                                      \
  do {                                                                                                                 \
    ASSERT_STREQ(name, (p)->pw_name);                                                                                  \
    ASSERT_STREQ(passwd, (p)->pw_passwd);                                                                              \
    ASSERT_EQ(uid, I((p)->pw_uid));                                                                                    \
    ASSERT_EQ(gid, I((p)->pw_gid));                                                                                    \
    ASSERT_STREQ(gecos, (p)->pw_gecos);                                                                                \
    ASSERT_STREQ(dir, (p)->pw_dir);                                                                                    \
    ASSERT_STREQ(shell, (p)->pw_shell);                                                                                \
  } while (0)

/* ---- getpwnam, getpwuid ------------------------------------------------- */

UTEST(pwd, getpwnam_finds_an_entry) {
  reset();
  k.content = DB;
  const struct passwd *?p = sut_getpwnam("alice");
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "alice", "x", 1000, 100, "Alice Liddell,Room 1", "/home/alice", "/bin/sh");
  ASSERT_EQ(0, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
  ASSERT_TRUE((k.open_flags & LIBC_O_CLOEXEC) != 0);
}

UTEST(pwd, getpwuid_finds_the_first_entry_with_the_uid) {
  reset();
  k.content = DB;
  const struct passwd *?p = sut_getpwuid(1000);
  ASSERT_TRUE(p != nullptr);
  if (p)
    ASSERT_STREQ("alice", p->pw_name);
  p = sut_getpwuid(0);
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "root", "x", 0, 0, "root", "/root", "/bin/sh");
  p = sut_getpwuid(1);
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "daemon", "*", 1, 1, "daemon", "/usr/sbin", "/sbin/nologin");
}

UTEST(pwd, empty_fields_are_empty_strings) {
  reset();
  k.content = DB;
  const struct passwd *?p = sut_getpwnam("nologin");
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "nologin", "x", 1001, 100, "", "", "");
}

UTEST(pwd, the_last_line_needs_no_newline) {
  reset();
  k.content = DB;
  const struct passwd *?p = sut_getpwuid(1002);
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "last", "x", 1002, 100, "Last", "/home/last", "/bin/sh");
}

UTEST(pwd, lines_that_are_not_entries_are_skipped) {
  reset();
  k.content = DB;
  const char *bad[] = {"# comment", "six", "eight", "baduid", "badgid", "nouid", "toobig", "+nis", "-nis", ""};
  for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i)
    ASSERT_TRUE(sut_getpwnam(bad[i]) == nullptr);
  for (uid_t uid = 2; uid <= 7; ++uid)
    ASSERT_TRUE(sut_getpwuid(uid) == nullptr);
  ASSERT_EQ(0, sut_errno);
}

UTEST(pwd, not_found_leaves_errno_as_it_was) {
  reset();
  k.content = DB;
  ASSERT_TRUE(sut_getpwnam("nobody") == nullptr);
  ASSERT_EQ(0, sut_errno);
  sut_errno = EAGAIN;
  ASSERT_TRUE(sut_getpwuid(4242) == nullptr);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(pwd, a_name_is_matched_whole) {
  reset();
  k.content = DB;
  ASSERT_TRUE(sut_getpwnam("alic") == nullptr);
  ASSERT_TRUE(sut_getpwnam("alicex") == nullptr);
  ASSERT_TRUE(sut_getpwnam("twin") != nullptr);
}

UTEST(pwd, reads_of_a_few_bytes_give_the_same_entries) {
  reset();
  k.content = DB;
  k.chunk = 2;
  const struct passwd *?p = sut_getpwnam("last");
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "last", "x", 1002, 100, "Last", "/home/last", "/bin/sh");
  ASSERT_TRUE(k.reads > 100);
}

UTEST(pwd, a_line_longer_than_every_buffer_is_read_whole) {
  static char big[800] = {};
  strcpy(big, "first:x:5:5::/:/bin/sh\nbig:x:500:500:");
  for (int i = 0; i < 60; ++i)
    strcat(big, "0123456789");
  strcat(big, ":/home/big:/bin/sh\nafter:x:501:501:A:/:/bin/sh\n");
  reset();
  k.content = big;
  k.chunk = 50;
  struct passwd *?p = sut_getpwuid(500);
  ASSERT_TRUE(p != nullptr);
  if (p) {
    ASSERT_EQ(600u, strlen(p->pw_gecos));
    ASSERT_STREQ("/home/big", p->pw_dir);
  }
  p = sut_getpwnam("after");
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "after", "x", 501, 501, "A", "/", "/bin/sh");
}

/* ---- the _r forms ------------------------------------------------------- */

UTEST(pwd, getpwnam_r_lays_the_entry_out_in_the_buffer) {
  reset();
  k.content = DB;
  struct passwd pwd = {};
  char buf[128] = {};
  struct passwd *?result = nullptr;
  ASSERT_EQ(0, sut_getpwnam_r("alice", &pwd, buf, sizeof buf, &result));
  ASSERT_TRUE(result == &pwd);
  EXPECT_USER(&pwd, "alice", "x", 1000, 100, "Alice Liddell,Room 1", "/home/alice", "/bin/sh");
  ASSERT_TRUE(pwd.pw_name == buf);
  ASSERT_TRUE(pwd.pw_shell > buf && pwd.pw_shell < buf + sizeof buf);
  ASSERT_EQ(0, sut_errno);
}

UTEST(pwd, r_forms_not_found_return_zero_and_null) {
  reset();
  k.content = DB;
  struct passwd pwd = {};
  char buf[64] = {};
  struct passwd *?result = &pwd;
  sut_errno = EAGAIN;
  ASSERT_EQ(0, sut_getpwnam_r("nobody", &pwd, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  result = &pwd;
  ASSERT_EQ(0, sut_getpwuid_r(4242, &pwd, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(pwd, a_buffer_one_byte_short_is_erange) {
  reset();
  k.content = DB;
  struct passwd pwd = {};
  char buf[128] = {};
  struct passwd *?result = &pwd;
  const size_t need = strlen(ALICE) + 1; /* the line, its colons NULs */
  ASSERT_EQ(ERANGE, sut_getpwuid_r(1000, &pwd, buf, need - 1, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_EQ(0, sut_getpwuid_r(1000, &pwd, buf, need, &result));
  ASSERT_TRUE(result == &pwd);
  EXPECT_USER(&pwd, "alice", "x", 1000, 100, "Alice Liddell,Room 1", "/home/alice", "/bin/sh");
  ASSERT_EQ(0, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

/* ---- the database's own failures ---------------------------------------- */

UTEST(pwd, without_etc_passwd_root_is_the_only_user) {
  reset();
  const struct passwd *?p = sut_getpwuid(0);
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "root", "x", 0, 0, "root", "/root", "/bin/sh");
  ASSERT_EQ(0, sut_errno); /* not open's ENOENT */
  ASSERT_TRUE(sut_getpwnam("root") != nullptr);
  ASSERT_TRUE(sut_getpwnam("alice") == nullptr);
  ASSERT_TRUE(sut_getpwuid(1000) == nullptr);
  ASSERT_EQ(0, sut_errno);
  struct passwd pwd = {};
  char buf[64] = {};
  struct passwd *?result = nullptr;
  ASSERT_EQ(0, sut_getpwnam_r("root", &pwd, buf, sizeof buf, &result));
  ASSERT_TRUE(result == &pwd);
  EXPECT_USER(&pwd, "root", "x", 0, 0, "root", "/root", "/bin/sh");
}

UTEST(pwd, a_database_that_cannot_be_opened_is_an_error) {
  reset();
  k.open_err = EACCES;
  ASSERT_TRUE(sut_getpwnam("root") == nullptr);
  ASSERT_EQ(EACCES, sut_errno);
  sut_errno = 0;
  struct passwd pwd = {};
  char buf[64] = {};
  struct passwd *?result = &pwd;
  ASSERT_EQ(EACCES, sut_getpwuid_r(0, &pwd, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_EQ(0, sut_errno);
}

UTEST(pwd, a_read_error_is_an_error) {
  reset();
  k.content = DB;
  k.read_err = EIO;
  k.read_after_bytes = 40;
  k.chunk = 10;
  struct passwd pwd = {};
  char buf[64] = {};
  struct passwd *?result = &pwd;
  ASSERT_EQ(EIO, sut_getpwnam_r("last", &pwd, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_TRUE(sut_getpwuid(1002) == nullptr);
  ASSERT_EQ(EIO, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

/* ---- getpwent ----------------------------------------------------------- */

UTEST(pwd, getpwent_gives_every_entry_in_order) {
  reset();
  k.content = DB;
  const char *want[] = {"root", "daemon", "alice", "nologin", "twin", "last"};
  for (size_t i = 0; i < sizeof want / sizeof want[0]; ++i) {
    const struct passwd *?p = sut_getpwent();
    ASSERT_TRUE(p != nullptr);
    if (p)
      ASSERT_STREQ(want[i], p->pw_name);
  }
  sut_errno = EAGAIN;
  ASSERT_TRUE(sut_getpwent() == nullptr);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_TRUE(sut_getpwent() == nullptr);
  ASSERT_EQ(1, k.opens);
  sut_endpwent();
  ASSERT_EQ(1, k.closes);
}

UTEST(pwd, setpwent_starts_again) {
  reset();
  k.content = DB;
  (void)sut_getpwent();
  (void)sut_getpwent();
  sut_setpwent();
  const struct passwd *?p = sut_getpwent();
  ASSERT_TRUE(p != nullptr);
  if (p)
    ASSERT_STREQ("root", p->pw_name);
  sut_endpwent();
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(pwd, lookups_do_not_move_getpwent) {
  reset();
  k.content = DB;
  (void)sut_getpwent(); /* root */
  ASSERT_TRUE(sut_getpwnam("last") != nullptr);
  const struct passwd *?p = sut_getpwent();
  ASSERT_TRUE(p != nullptr);
  if (p)
    ASSERT_STREQ("daemon", p->pw_name);
  sut_endpwent();
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(pwd, getpwent_without_etc_passwd) {
  reset();
  const struct passwd *?p = sut_getpwent();
  ASSERT_TRUE(p != nullptr);
  if (p)
    EXPECT_USER(p, "root", "x", 0, 0, "root", "/root", "/bin/sh");
  ASSERT_TRUE(sut_getpwent() == nullptr);
  ASSERT_EQ(0, sut_errno);
  sut_endpwent();
}

UTEST(pwd, endpwent_without_getpwent_does_nothing) {
  reset();
  k.content = DB;
  sut_errno = EAGAIN;
  sut_endpwent();
  sut_setpwent();
  ASSERT_EQ(0, k.closes);
  ASSERT_EQ(EAGAIN, sut_errno);
}

/* ---- unistd.c: the process's user ids ----------------------------------- */

UTEST(pwd, setuid_and_seteuid_are_the_kernels) {
  reset();
  ASSERT_EQ(-1, sut_setuid(1000));
  ASSERT_EQ(LIBC_ENOSYS, sut_errno);
  ASSERT_TRUE(k.last_number == sys_setuid);
  ASSERT_EQ(1000, k.last_uid);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_seteuid(1001));
  ASSERT_EQ(LIBC_ENOSYS, sut_errno);
  ASSERT_TRUE(k.last_number == sys_seteuid);
  ASSERT_EQ(1001, k.last_uid);
  k.id_err = 0;
  sut_errno = 0;
  ASSERT_EQ(0, sut_setuid(0));
  ASSERT_EQ(0, sut_errno);
}

UTEST(pwd, setuid_of_minus_one_is_einval) {
  reset();
  const uid_t minus_one = 0xffff; /* (uid_t)-1: uid_t is 16 bits */
  ASSERT_EQ(-1, sut_setuid(minus_one));
  ASSERT_EQ(EINVAL, sut_errno);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_seteuid(minus_one));
  ASSERT_EQ(EINVAL, sut_errno);
  ASSERT_EQ(0, k.id_calls);
}

UTEST(pwd, getuid_is_root) {
  reset();
  ASSERT_EQ(0, I(sut_getuid()));
}

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

/* grp.c and unistd.c's group ids over a faked kernel (/etc/group from a
 * string, group-id calls ENOSYS or a value) */

#include "utest.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "serve_memory.h" /* and syscalls_stub.h */

/* libc's, which the tests' newlib headers do not have or number apart */
#define LIBC_O_CLOEXEC 02000000
#define LIBC_NGROUPS_MAX 32
#define LIBC_SC_NGROUPS_MAX 5
#define LIBC_ENOSYS 38 /* newlib's is 88; the rest these tests use agree */

struct group {
  const char *gr_name;
  const char *gr_passwd;
  gid_t gr_gid;
  const char *?@(view, const) *?gr_mem;
};

struct group *?borrows(static) sut_getgrnam(char *name);
struct group *?borrows(static) sut_getgrgid(gid_t gid);
int sut_getgrnam_r(char *name, out struct group *grp, inout char *buf, size_t size,
                   out struct group *?borrows(grp) *result);
int sut_getgrgid_r(gid_t gid, out struct group *grp, inout char *buf, size_t size,
                   out struct group *?borrows(grp) *result);
struct group *?borrows(static) sut_getgrent(void);
void sut_setgrent(void);
void sut_endgrent(void);
int sut_getgrouplist(char *user, gid_t group, inout gid_t *?groups, inout int *ngroups);
int sut_setgroups(size_t size, gid_t list[?size]);
int sut_initgroups(char *user, gid_t group);
gid_t sut_getgid(void);
gid_t sut_getegid(void);
int sut_setgid(gid_t gid);
int sut_setegid(gid_t gid);
int sut_getgroups(int size, inout gid_t *?list);
long sut_sysconf(int name);

/* The library under test carries its own errno (the QEMU suite renames every
 * libc symbol). */
extern int sut_errno;

static const char DB[] = "root:x:0:\n"
"# a comment:x:1:\n"
"wheel:x:10:root,alice\n"
"\n"
"short:x:11\n"
"badgid:x:1a:\n"
"nogid:x::\n"
"toobig:x:65536:\n"
"extra:x:12:a:b\n"
"+nis:x:13:\n"
"-nis:x:14:\n"
"users:*:100:alice,,bob,\n"
"empty::200:\n"
"wheel2:x:10:alice\n"
"alicegrp:x:1000:alice\n"
"last:x:300:carol";

#define FAKE_FD 7

static struct {
  const char *?content; /* nullptr: open fails with open_err */
  int open_err;
  int read_err; /* the read after read_after_bytes fails with it */
  size_t read_after_bytes;
  size_t chunk; /* at most this much a read, 0: as much as asked */
  size_t pos;
  int opens, closes, reads;
  int open_flags;
  int group_err; /* the group-id calls: ENOSYS, or 0 to answer group_result */
  int group_result;
  int group_calls;
  int last_number;
  size_t set_size;
  gid_t set_list[64];
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
    if (!ctx->path || strcmp(ctx->path, "/etc/group") != 0 || !k.content) {
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
  } else if (number == sys_getgid || number == sys_getegid || number == sys_setgid || number == sys_setegid ||
             number == sys_getgroups || number == sys_setgroups) {
    ++k.group_calls;
    if (number == sys_setgroups) {
      const auto ctx = cast<ptr(const setgroups_context *)>(args);
      k.set_size = ctx->size;
      const gid_t *?list = ctx->list;
      for (size_t i = 0; i < ctx->size && i < 64 && list; ++i)
        k.set_list[i] = list[i];
    }
    result->err = k.group_err;
    result->result = k.group_err ? -1 : k.group_result;
  } else {
    result->err = LIBC_ENOSYS;
    result->result = -1;
  }
}

/* P is in BASE's N bytes, aligned for a pointer */
static bool aligned_in(char *?view *p, char *base, size_t n) {
  const uintptr_t at = cast<addr(uintptr_t)>(p);
  const uintptr_t from = cast<addr(uintptr_t)>(base);
  return at >= from && at < from + n && at % sizeof(char *view) == 0;
}

/* a gid as an int, for the assertions */
static int I(gid_t gid) {
  return gid;
}

static void reset(void) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = fake_syscall;
  memset(&k, 0, sizeof k);
  k.open_err = ENOENT;
  k.group_err = LIBC_ENOSYS;
  sut_endgrent(); /* what an earlier test left open */
  k.closes = 0;
  sut_errno = 0;
}

/* NAME's members, comma-separated, in OUT */
static void members(struct group *g, out char dst[128]) {
  dst[0] = '\0';
  const char *?@(view, const) *?mem = g->gr_mem;
  for (size_t i = 0; mem && mem[i]; ++i) {
    const char *?m = mem[i];
    if (!m)
      break;
    if (i)
      strcat(dst, ",");
    strcat(dst, m);
  }
}

#define EXPECT_GROUP(g, name, passwd, gid, mems)                                                                       \
  do {                                                                                                                 \
    char got_[128] = undefined;                                                                                        \
    ASSERT_STREQ(name, (g)->gr_name);                                                                                  \
    ASSERT_STREQ(passwd, (g)->gr_passwd);                                                                              \
    ASSERT_EQ(gid, I((g)->gr_gid));                                                                                  \
    members(g, got_);                                                                                                  \
    ASSERT_STREQ(mems, got_);                                                                                          \
  } while (0)

/* ---- getgrnam, getgrgid ------------------------------------------------- */

UTEST(grp, getgrnam_finds_an_entry_with_its_members) {
  reset();
  k.content = DB;
  const struct group *?g = sut_getgrnam("wheel");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "wheel", "x", 10, "root,alice");
  ASSERT_EQ(0, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
  ASSERT_TRUE((k.open_flags & LIBC_O_CLOEXEC) != 0);
}

UTEST(grp, getgrgid_finds_the_first_entry_with_the_gid) {
  reset();
  k.content = DB;
  const struct group *?g = sut_getgrgid(10);
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "wheel", "x", 10, "root,alice");
  g = sut_getgrgid(0);
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "root", "x", 0, "");
}

UTEST(grp, empty_members_are_left_out) {
  reset();
  k.content = DB;
  const struct group *?g = sut_getgrnam("users");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "users", "*", 100, "alice,bob");
  g = sut_getgrnam("empty");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "empty", "", 200, "");
}

UTEST(grp, the_last_line_needs_no_newline) {
  reset();
  k.content = DB;
  const struct group *?g = sut_getgrgid(300);
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "last", "x", 300, "carol");
}

UTEST(grp, lines_that_are_not_entries_are_skipped) {
  reset();
  k.content = DB;
  const char *bad[] = {"# a comment", "short", "badgid", "nogid", "toobig", "extra", "+nis", "-nis", ""};
  for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i)
    ASSERT_TRUE(sut_getgrnam(bad[i]) == nullptr);
  for (gid_t gid = 11; gid <= 14; ++gid)
    ASSERT_TRUE(sut_getgrgid(gid) == nullptr);
  ASSERT_TRUE(sut_getgrgid(1) == nullptr);
  ASSERT_EQ(0, sut_errno);
}

UTEST(grp, not_found_leaves_errno_as_it_was) {
  reset();
  k.content = DB;
  ASSERT_TRUE(sut_getgrnam("nobody") == nullptr);
  ASSERT_EQ(0, sut_errno);
  sut_errno = EAGAIN;
  ASSERT_TRUE(sut_getgrgid(4242) == nullptr);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(grp, a_name_is_matched_whole) {
  reset();
  k.content = DB;
  ASSERT_TRUE(sut_getgrnam("whee") == nullptr);
  ASSERT_TRUE(sut_getgrnam("wheel22") == nullptr);
  const struct group *?g = sut_getgrnam("wheel2");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "wheel2", "x", 10, "alice");
}

UTEST(grp, reads_of_a_few_bytes_give_the_same_entries) {
  reset();
  k.content = DB;
  k.chunk = 3;
  const struct group *?g = sut_getgrnam("alicegrp");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "alicegrp", "x", 1000, "alice");
  ASSERT_TRUE(k.reads > 50);
}

UTEST(grp, a_line_longer_than_every_buffer_is_read_whole) {
  static char big[700] = {};
  strcpy(big, "first:x:1:\nbig:x:500:");
  for (int i = 0; i < 100; ++i) {
    char m[8] = undefined;
    snprintf(m, sizeof m, "%sm%02d", i ? "," : "", i);
    strcat(big, m);
  }
  strcat(big, "\nafter:x:501:z\n");
  reset();
  k.content = big;
  k.chunk = 50;
  struct group *?g = sut_getgrgid(500);
  ASSERT_TRUE(g != nullptr);
  size_t n = 0;
  const char *?@(view, const) *?mem = g ? g->gr_mem : nullptr;
  while (mem && mem[n])
    ++n;
  ASSERT_EQ(100u, n);
  if (mem && n == 100) {
    ASSERT_STREQ("m00", mem[0]);
    ASSERT_STREQ("m99", mem[99]);
  }
  g = sut_getgrnam("after");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "after", "x", 501, "z");
}

/* ---- the _r forms ------------------------------------------------------- */

UTEST(grp, getgrnam_r_lays_the_entry_out_in_the_buffer) {
  reset();
  k.content = DB;
  struct group grp = {};
  char buf[256] = {};
  struct group *?result = nullptr;
  ASSERT_EQ(0, sut_getgrnam_r("users", &grp, buf, sizeof buf, &result));
  ASSERT_TRUE(result == &grp);
  EXPECT_GROUP(&grp, "users", "*", 100, "alice,bob");
  /* every string and the member array are in buf */
  ASSERT_TRUE(grp.gr_name >= buf && grp.gr_name < buf + sizeof buf);
  const char *?@(view, const) *?mem = grp.gr_mem;
  ASSERT_TRUE(mem != nullptr);
  ASSERT_TRUE(mem && aligned_in(mem, buf, sizeof buf));
  ASSERT_EQ(0, sut_errno);
}

UTEST(grp, getgrgid_r_with_a_misaligned_buffer) {
  reset();
  k.content = DB;
  struct group grp = {};
  char buf[257] = {};
  struct group *?result = nullptr;
  ASSERT_EQ(0, sut_getgrgid_r(10, &grp, buf + 1, sizeof buf - 1, &result));
  ASSERT_TRUE(result == &grp);
  EXPECT_GROUP(&grp, "wheel", "x", 10, "root,alice");
}

UTEST(grp, r_forms_not_found_return_zero_and_null) {
  reset();
  k.content = DB;
  struct group grp = {};
  char buf[64] = {};
  struct group *?result = &grp;
  sut_errno = EAGAIN;
  ASSERT_EQ(0, sut_getgrnam_r("nobody", &grp, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  result = &grp;
  ASSERT_EQ(0, sut_getgrgid_r(4242, &grp, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(grp, a_buffer_too_small_is_erange_and_a_bigger_one_works) {
  reset();
  k.content = DB;
  struct group grp = {};
  char buf[128] = {};
  struct group *?result = &grp;
  /* "wheel\0x\0" "10\0root,alice\0": 22 bytes, plus three pointers */
  size_t size = 0;
  int rc = ERANGE;
  for (; size <= sizeof buf && rc == ERANGE; ++size) {
    result = &grp;
    rc = sut_getgrnam_r("wheel", &grp, buf, size, &result);
    if (rc == ERANGE)
      ASSERT_TRUE(result == nullptr);
  }
  ASSERT_EQ(0, rc);
  ASSERT_TRUE(result == &grp);
  const size_t least = 22 + 3 * sizeof(char *view);
  ASSERT_TRUE(size - 1 >= least);
  EXPECT_GROUP(&grp, "wheel", "x", 10, "root,alice");
  ASSERT_EQ(0, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

/* ---- the database's own failures ---------------------------------------- */

UTEST(grp, without_etc_group_root_is_the_only_group) {
  reset();
  sut_errno = 0;
  const struct group *?g = sut_getgrnam("root");
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "root", "x", 0, "");
  ASSERT_EQ(0, sut_errno); /* not open's ENOENT */
  ASSERT_TRUE(sut_getgrgid(0) != nullptr);
  ASSERT_TRUE(sut_getgrnam("wheel") == nullptr);
  ASSERT_TRUE(sut_getgrgid(10) == nullptr);
  ASSERT_EQ(0, sut_errno);
  struct group grp = {};
  char buf[64] = {};
  struct group *?result = nullptr;
  ASSERT_EQ(0, sut_getgrgid_r(0, &grp, buf, sizeof buf, &result));
  ASSERT_TRUE(result == &grp);
  EXPECT_GROUP(&grp, "root", "x", 0, "");
}

UTEST(grp, a_database_that_cannot_be_opened_is_an_error) {
  reset();
  k.open_err = EACCES;
  ASSERT_TRUE(sut_getgrnam("root") == nullptr);
  ASSERT_EQ(EACCES, sut_errno);
  sut_errno = 0;
  struct group grp = {};
  char buf[64] = {};
  struct group *?result = &grp;
  ASSERT_EQ(EACCES, sut_getgrgid_r(0, &grp, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_EQ(0, sut_errno);
}

UTEST(grp, a_read_error_is_an_error) {
  reset();
  k.content = DB;
  k.read_err = EIO;
  k.read_after_bytes = 20;
  k.chunk = 10;
  struct group grp = {};
  char buf[64] = {};
  struct group *?result = &grp;
  ASSERT_EQ(EIO, sut_getgrnam_r("last", &grp, buf, sizeof buf, &result));
  ASSERT_TRUE(result == nullptr);
  ASSERT_TRUE(sut_getgrgid(300) == nullptr);
  ASSERT_EQ(EIO, sut_errno);
  ASSERT_EQ(k.opens, k.closes);
}

/* ---- getgrent ----------------------------------------------------------- */

UTEST(grp, getgrent_gives_every_entry_in_order) {
  reset();
  k.content = DB;
  const char *want[] = {"root", "wheel", "users", "empty", "wheel2", "alicegrp", "last"};
  for (size_t i = 0; i < sizeof want / sizeof want[0]; ++i) {
    const struct group *?g = sut_getgrent();
    ASSERT_TRUE(g != nullptr);
    if (g)
      ASSERT_STREQ(want[i], g->gr_name);
  }
  sut_errno = EAGAIN;
  ASSERT_TRUE(sut_getgrent() == nullptr);
  ASSERT_EQ(EAGAIN, sut_errno);
  ASSERT_TRUE(sut_getgrent() == nullptr);
  ASSERT_EQ(1, k.opens);
  sut_endgrent();
  ASSERT_EQ(1, k.closes);
}

UTEST(grp, setgrent_starts_again) {
  reset();
  k.content = DB;
  (void)sut_getgrent();
  (void)sut_getgrent();
  sut_setgrent();
  const struct group *?g = sut_getgrent();
  ASSERT_TRUE(g != nullptr);
  if (g)
    ASSERT_STREQ("root", g->gr_name);
  sut_endgrent();
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(grp, lookups_do_not_move_getgrent) {
  reset();
  k.content = DB;
  (void)sut_getgrent(); /* root */
  ASSERT_TRUE(sut_getgrnam("last") != nullptr);
  const struct group *?g = sut_getgrent();
  ASSERT_TRUE(g != nullptr);
  if (g)
    ASSERT_STREQ("wheel", g->gr_name);
  sut_endgrent();
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(grp, getgrent_without_etc_group) {
  reset();
  const struct group *?g = sut_getgrent();
  ASSERT_TRUE(g != nullptr);
  if (g)
    EXPECT_GROUP(g, "root", "x", 0, "");
  ASSERT_TRUE(sut_getgrent() == nullptr);
  ASSERT_EQ(0, sut_errno);
  sut_endgrent();
}

UTEST(grp, endgrent_without_getgrent_does_nothing) {
  reset();
  k.content = DB;
  sut_errno = EAGAIN;
  sut_endgrent();
  sut_setgrent();
  ASSERT_EQ(0, k.closes);
  ASSERT_EQ(EAGAIN, sut_errno);
}

/* ---- getgrouplist, setgroups, initgroups -------------------------------- */

UTEST(grp, getgrouplist_puts_the_group_first_and_each_gid_once) {
  reset();
  k.content = DB;
  gid_t groups[8] = {};
  int n = 8;
  /* wheel (10), users (100), wheel2 (10 again), alicegrp (1000: the group) */
  ASSERT_EQ(3, sut_getgrouplist("alice", 1000, groups, &n));
  ASSERT_EQ(3, n);
  ASSERT_EQ(1000, I(groups[0]));
  ASSERT_EQ(10, I(groups[1]));
  ASSERT_EQ(100, I(groups[2]));
  n = 8;
  ASSERT_EQ(1, sut_getgrouplist("nobody", 77, groups, &n));
  ASSERT_EQ(77, I(groups[0]));
  ASSERT_EQ(k.opens, k.closes);
}

UTEST(grp, getgrouplist_too_few_says_how_many) {
  reset();
  k.content = DB;
  gid_t groups[2] = {};
  int n = 2;
  ASSERT_EQ(-1, sut_getgrouplist("alice", 1000, groups, &n));
  ASSERT_EQ(3, n);
  ASSERT_EQ(1000, I(groups[0]));
  ASSERT_EQ(10, I(groups[1]));
  n = 0;
  ASSERT_EQ(-1, sut_getgrouplist("root", 0, nullptr, &n));
  ASSERT_EQ(2, n); /* 0, wheel */
}

UTEST(grp, setgroups_is_the_kernels) {
  reset();
  k.content = DB;
  const gid_t list[] = {4, 5};
  ASSERT_EQ(-1, sut_setgroups(2, list));
  ASSERT_EQ(LIBC_ENOSYS, sut_errno);
  ASSERT_TRUE(k.last_number == sys_setgroups);
  ASSERT_EQ(2u, k.set_size);
  ASSERT_EQ(5, I(k.set_list[1]));
  k.group_err = 0;
  sut_errno = 0;
  ASSERT_EQ(0, sut_setgroups(0, nullptr));
  ASSERT_EQ(0, sut_errno);
}

UTEST(grp, setgroups_past_ngroups_max_is_einval) {
  reset();
  k.content = DB;
  gid_t list[LIBC_NGROUPS_MAX + 1] = {};
  ASSERT_EQ(-1, sut_setgroups(LIBC_NGROUPS_MAX + 1, list));
  ASSERT_EQ(EINVAL, sut_errno);
  ASSERT_EQ(0, k.group_calls);
}

UTEST(grp, initgroups_sets_getgrouplists_list) {
  reset();
  k.content = DB;
  ASSERT_EQ(-1, sut_initgroups("alice", 1000));
  ASSERT_EQ(LIBC_ENOSYS, sut_errno);
  ASSERT_EQ(3u, k.set_size);
  ASSERT_EQ(1000, I(k.set_list[0]));
  ASSERT_EQ(10, I(k.set_list[1]));
  ASSERT_EQ(100, I(k.set_list[2]));
  k.group_err = 0;
  ASSERT_EQ(0, sut_initgroups("root", 0));
  ASSERT_EQ(2u, k.set_size);
  ASSERT_EQ(k.opens, k.closes);
}

/* ---- unistd.c: the process's group ids ---------------------------------- */

UTEST(grp, getgid_and_getegid_say_0_while_the_kernel_has_none) {
  reset();
  k.content = DB;
  sut_errno = EAGAIN;
  ASSERT_EQ(0, I(sut_getgid()));
  ASSERT_EQ(0, I(sut_getegid()));
  ASSERT_EQ(EAGAIN, sut_errno); /* they cannot fail */
  ASSERT_EQ(2, k.group_calls);
}

UTEST(grp, getgid_and_getegid_say_what_the_kernel_does) {
  reset();
  k.content = DB;
  k.group_err = 0;
  k.group_result = 1000;
  ASSERT_EQ(1000, I(sut_getgid()));
  ASSERT_TRUE(k.last_number == sys_getgid);
  ASSERT_EQ(1000, I(sut_getegid()));
  ASSERT_TRUE(k.last_number == sys_getegid);
  k.group_result = 65535;
  ASSERT_EQ(65535, I(sut_getgid()));
  ASSERT_EQ(0, sut_errno);
}

UTEST(grp, setgid_and_setegid_are_the_kernels) {
  reset();
  k.content = DB;
  ASSERT_EQ(-1, sut_setgid(10));
  ASSERT_EQ(LIBC_ENOSYS, sut_errno);
  ASSERT_TRUE(k.last_number == sys_setgid);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_setegid(10));
  ASSERT_EQ(LIBC_ENOSYS, sut_errno);
  ASSERT_TRUE(k.last_number == sys_setegid);
  k.group_err = 0;
  ASSERT_EQ(0, sut_setgid(10));
}

UTEST(grp, setgid_of_minus_one_is_einval) {
  reset();
  k.content = DB;
  const gid_t minus_one = 0xffff; /* (gid_t)-1: gid_t is 16 bits */
  ASSERT_EQ(-1, sut_setgid(minus_one));
  ASSERT_EQ(EINVAL, sut_errno);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_setegid(minus_one));
  ASSERT_EQ(EINVAL, sut_errno);
  ASSERT_EQ(0, k.group_calls);
}

UTEST(grp, getgroups_has_none_while_the_kernel_has_none) {
  reset();
  k.content = DB;
  gid_t list[4] = {};
  sut_errno = EAGAIN;
  ASSERT_EQ(0, sut_getgroups(4, list));
  ASSERT_EQ(0, sut_getgroups(0, nullptr));
  ASSERT_EQ(EAGAIN, sut_errno);
}

UTEST(grp, getgroups_says_what_the_kernel_does) {
  reset();
  k.content = DB;
  k.group_err = 0;
  k.group_result = 3;
  gid_t list[4] = {};
  ASSERT_EQ(3, sut_getgroups(4, list));
  ASSERT_TRUE(k.last_number == sys_getgroups);
  k.group_err = EINVAL;
  ASSERT_EQ(-1, sut_getgroups(1, list));
  ASSERT_EQ(EINVAL, sut_errno);
}

UTEST(grp, getgroups_of_a_negative_size_is_einval) {
  reset();
  k.content = DB;
  gid_t list[1] = {};
  ASSERT_EQ(-1, sut_getgroups(-1, list));
  ASSERT_EQ(EINVAL, sut_errno);
  ASSERT_EQ(0, k.group_calls);
}

UTEST(grp, sysconf_ngroups_max) {
  reset();
  k.content = DB;
  ASSERT_EQ(LIBC_NGROUPS_MAX, sut_sysconf(LIBC_SC_NGROUPS_MAX));
  ASSERT_EQ(0, k.group_calls);
}

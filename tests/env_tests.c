/* setenv, unsetenv, putenv and getenv on the SUT's own environ. */

#define _GNU_SOURCE /* host mremap */
#include "utest.h"

#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include "syscalls_stub.h"

extern char **sut_environ;
extern int sut_errno;
char *sut_getenv(const char *name);
int sut_setenv(const char *name, const char *value, int overwrite);
int sut_unsetenv(const char *name);
int sut_putenv(char *string);

static size_t env_entries(void) {
  size_t n = 0;
  while (sut_environ[n])
    n++;
  return n;
}

/* Every test starts from an inherited (not malloc'd) environment, as exec
 * leaves it. */
static char inherited_home[] = "HOME=/root";
static char inherited_path[] = "PATH=/bin";
static char *inherited[3];

/* setenv allocates with the SUT's malloc, which maps its memory: serve the
 * memory syscalls from the host (as regex_tests.c does). */
static void host_memory_syscalls(int number, const void *args, syscall_result *result) {
  result->err = 0;
  result->result = 0;
  if (number == sys_mmap) {
    const mmap_context *ctx = args;
    *ctx->result = mmap(NULL, ctx->length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  } else if (number == sys_munmap) {
    const munmap_context *ctx = args;
    munmap(ctx->addr, ctx->length);
  } else if (number == sys_mremap) {
    const mremap_context *ctx = args;
    *ctx->result = mremap(ctx->addr, ctx->old_length, ctx->new_length, 0);
  }
}

static void reset_env(void) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = host_memory_syscalls;
  inherited[0] = inherited_home;
  inherited[1] = inherited_path;
  inherited[2] = NULL;
  sut_environ = inherited;
}

UTEST(env, getenv_reads_inherited) {
  reset_env();
  ASSERT_STREQ("/root", sut_getenv("HOME"));
  ASSERT_STREQ("/bin", sut_getenv("PATH"));
  ASSERT_TRUE(sut_getenv("HOM") == NULL);
  ASSERT_TRUE(sut_getenv("HOMEX") == NULL);
  sut_environ = NULL;
  ASSERT_TRUE(sut_getenv("HOME") == NULL);
}

UTEST(env, setenv_adds_and_overwrites) {
  reset_env();
  ASSERT_EQ(0, sut_setenv("TERM", "vt100", 0));
  ASSERT_STREQ("vt100", sut_getenv("TERM"));
  ASSERT_EQ(3u, env_entries());
  /* The inherited array is copied, not written past its end. */
  ASSERT_TRUE(sut_environ != inherited);
  ASSERT_TRUE(inherited[2] == NULL);

  ASSERT_EQ(0, sut_setenv("TERM", "xterm", 0));
  ASSERT_STREQ("vt100", sut_getenv("TERM"));
  ASSERT_EQ(0, sut_setenv("TERM", "xterm", 1));
  ASSERT_STREQ("xterm", sut_getenv("TERM"));
  ASSERT_EQ(0, sut_setenv("HOME", "/home/x", 1));
  ASSERT_STREQ("/home/x", sut_getenv("HOME"));
  /* The inherited string is replaced, never freed or edited. */
  ASSERT_STREQ("HOME=/root", inherited_home);
  ASSERT_EQ(3u, env_entries());

  ASSERT_EQ(0, sut_setenv("EMPTY", "", 1));
  ASSERT_STREQ("", sut_getenv("EMPTY"));
}

UTEST(env, setenv_grows_past_initial_capacity) {
  reset_env();
  char name[16], value[16];
  for (int i = 0; i < 100; i++) {
    snprintf(name, sizeof name, "V%d", i);
    snprintf(value, sizeof value, "%d", i * 3);
    ASSERT_EQ(0, sut_setenv(name, value, 1));
  }
  ASSERT_EQ(102u, env_entries());
  for (int i = 0; i < 100; i++) {
    snprintf(name, sizeof name, "V%d", i);
    snprintf(value, sizeof value, "%d", i * 3);
    ASSERT_STREQ(value, sut_getenv(name));
  }
  ASSERT_STREQ("/root", sut_getenv("HOME"));
  for (int i = 0; i < 100; i++) {
    snprintf(name, sizeof name, "V%d", i);
    ASSERT_EQ(0, sut_unsetenv(name));
  }
  ASSERT_EQ(2u, env_entries());
}

UTEST(env, setenv_rejects_bad_names) {
  reset_env();
  sut_errno = 0;
  ASSERT_EQ(-1, sut_setenv("", "x", 1));
  ASSERT_EQ(EINVAL, sut_errno);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_setenv("A=B", "x", 1));
  ASSERT_EQ(EINVAL, sut_errno);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_setenv(NULL, "x", 1));
  ASSERT_EQ(EINVAL, sut_errno);
  sut_errno = 0;
  ASSERT_EQ(-1, sut_unsetenv("A=B"));
  ASSERT_EQ(EINVAL, sut_errno);
  ASSERT_EQ(2u, env_entries());
}

UTEST(env, unsetenv_removes_every_copy) {
  static char dup1[] = "X=1", dup2[] = "X=2", other[] = "Y=3";
  static char *env[] = {dup1, other, dup2, NULL};
  sut_environ = env;
  ASSERT_EQ(0, sut_unsetenv("X"));
  ASSERT_TRUE(sut_getenv("X") == NULL);
  ASSERT_STREQ("3", sut_getenv("Y"));
  ASSERT_EQ(1u, env_entries());
  ASSERT_EQ(0, sut_unsetenv("MISSING"));
  ASSERT_EQ(1u, env_entries());
}

UTEST(env, putenv_uses_the_callers_string) {
  reset_env();
  static char entry[] = "LANG=C";
  ASSERT_EQ(0, sut_putenv(entry));
  ASSERT_STREQ("C", sut_getenv("LANG"));
  entry[5] = 'D';
  ASSERT_STREQ("D", sut_getenv("LANG"));

  ASSERT_EQ(0, sut_setenv("LANG", "pl", 1));
  ASSERT_STREQ("pl", sut_getenv("LANG"));
  static char again[] = "LANG=en";
  ASSERT_EQ(0, sut_putenv(again));
  ASSERT_STREQ("en", sut_getenv("LANG"));

  static char remove[] = "LANG";
  ASSERT_EQ(0, sut_putenv(remove));
  ASSERT_TRUE(sut_getenv("LANG") == NULL);
  ASSERT_EQ(2u, env_entries());
}

UTEST(env, program_replaced_environ) {
  reset_env();
  ASSERT_EQ(0, sut_setenv("A", "1", 1));
  /* A program that builds its own array, as toybox's xsetenv does. */
  static char b[] = "B=2";
  static char *own[] = {b, NULL};
  sut_environ = own;
  ASSERT_EQ(0, sut_setenv("C", "3", 1));
  ASSERT_STREQ("2", sut_getenv("B"));
  ASSERT_STREQ("3", sut_getenv("C"));
  ASSERT_TRUE(sut_getenv("A") == NULL);
  ASSERT_TRUE(own[1] == NULL);
}

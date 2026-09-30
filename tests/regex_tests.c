/*
 * regcomp / regexec, against the host's.
 *
 * The engine used to be a small ERE-only matcher: in a basic expression it
 * took `\(` for a literal and `(` for a group -- backwards -- and it never
 * tried a match at the end of the subject, so `^$` could not match an empty
 * line.  sed, grep and expr all sit on it, and an autoconf `configure` is
 * made of basic-syntax sed scripts, so it could not run on the device.
 *
 * Differential, like strto_tests.c: for every pattern x subject x flags, the
 * SUT must agree with glibc on whether it compiles, whether it matches, and
 * where every subexpression starts and ends.
 */

#define _GNU_SOURCE /* host mremap */
#include "utest.h"

#include <regex.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "syscalls_stub.h"

/* The engine allocates, and the SUT's malloc gets its memory from mmap, which
 * the shared syscall fake does not implement: serve the memory syscalls from
 * the host for the duration of each test (mremap in place only, as on the
 * device). */
static void host_memory_syscalls(int number, const void *args, syscall_result *result)
{
  result->err = 0;
  result->result = 0;
  if (number == sys_mmap) {
    const mmap_context *ctx = args;
    void *p = mmap(NULL, ctx->length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *ctx->result = p;
  } else if (number == sys_munmap) {
    const munmap_context *ctx = args;
    munmap(ctx->addr, ctx->length);
  } else if (number == sys_mremap) {
    const mremap_context *ctx = args;
    *ctx->result = mremap(ctx->addr, ctx->old_length, ctx->new_length, 0);
  }
}

static void use_host_memory(void)
{
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = host_memory_syscalls;
}

/* The SUT's regex_t / regmatch_t (musl layout, regoff_t is long). */
typedef struct {
  size_t re_nsub;
  void *opaque, *padding[4];
  size_t nsub2;
  char padding2;
} sut_regex_t;
typedef struct {
  long rm_so, rm_eo;
} sut_regmatch_t;

int sut_regcomp(sut_regex_t *preg, const char *regex, int cflags);
int sut_regexec(const sut_regex_t *preg, const char *string, size_t nmatch, sut_regmatch_t *pmatch,
                int eflags);
void sut_regfree(sut_regex_t *preg);
size_t sut_regerror(int errcode, const sut_regex_t *preg, char *errbuf, size_t size);

/* The flag values the SUT uses; glibc's are the same numbers. */
#define SUT_EXTENDED 1
#define SUT_ICASE 2
#define SUT_NEWLINE 4
#define SUT_NOSUB 8
#define SUT_NOTBOL 1

struct rcase {
  const char *pattern;
  int ere;
};

/* Basic and extended expressions of the kinds sed / expr / grep / configure
 * use, plus the ones that separate the two syntaxes. */
static const struct rcase PATTERNS[] = {
    {"^$", 0},
    {"^$", 1},
    {"a", 0},
    {"^X\\(.*\\)", 0},              /* expr "X$x" : 'X\(.*\)' */
    {"\\(a\\)\\(b*\\)c", 0},
    {"(a)(b*)c", 1},
    {"(a)", 0},                     /* BRE: parentheses are literal */
    {"a\\{2,3\\}", 0},
    {"a{2,3}", 1},
    {"a+", 0},                      /* BRE: '+' is literal */
    {"a+", 1},
    {"a|b", 1},
    {"[[:alpha:]][[:alnum:]_]*", 0},
    {"[^/]*$", 0},
    {"^[0-9][0-9]*\\.[0-9]*", 0},
    {"\\(.\\)\\1", 0},              /* back-reference */
    {"[]a]", 0},
    {"[a-c]x", 1},
    {"s/\\(x\\)/y/", 0},
    {"^ *#", 0},
    {"\\$", 0},
    {"x*", 0},
    {"(ab|a)(bc|c)", 1},
};

static const char *const SUBJECTS[] = {
    "", "a", "aa", "aaa", "abbc", "(a)", "a+", "X/usr/lib", "foo_bar1 baz", "path/to/file",
    "12.5e3", "xx", "]", "bx", "s/x/y/", "   # comment", "cost $5", "abc",
};

UTEST(regex, agrees_with_host)
{
  use_host_memory();
  size_t p, s;
  for (p = 0; p < sizeof(PATTERNS) / sizeof(PATTERNS[0]); p++) {
    for (int icase = 0; icase < 2; icase++) {
      int hflags = (PATTERNS[p].ere ? REG_EXTENDED : 0) | (icase ? REG_ICASE : 0);
      int sflags = (PATTERNS[p].ere ? SUT_EXTENDED : 0) | (icase ? SUT_ICASE : 0);
      regex_t host;
      sut_regex_t sut;
      int hrc = regcomp(&host, PATTERNS[p].pattern, hflags);
      int src = sut_regcomp(&sut, PATTERNS[p].pattern, sflags);
      if ((hrc == 0) != (src == 0)) {
        printf("regcomp disagrees on '%s' (ere=%d): host %d sut %d\n", PATTERNS[p].pattern, PATTERNS[p].ere,
               hrc, src);
        ASSERT_TRUE(0);
      }
      if (hrc != 0)
        continue;
      ASSERT_EQ((size_t)host.re_nsub, sut.re_nsub);
      for (s = 0; s < sizeof(SUBJECTS) / sizeof(SUBJECTS[0]); s++) {
        regmatch_t hm[4];
        sut_regmatch_t sm[4];
        int hr = regexec(&host, SUBJECTS[s], 4, hm, 0);
        int sr = sut_regexec(&sut, SUBJECTS[s], 4, sm, 0);
        if ((hr == 0) != (sr == 0)) {
          printf("regexec disagrees: '%s' (ere=%d icase=%d) on '%s': host %d sut %d\n", PATTERNS[p].pattern,
                 PATTERNS[p].ere, icase, SUBJECTS[s], hr, sr);
          ASSERT_TRUE(0);
        }
        if (hr != 0)
          continue;
        for (size_t k = 0; k <= host.re_nsub && k < 4; k++) {
          if (hm[k].rm_so != sm[k].rm_so || hm[k].rm_eo != sm[k].rm_eo) {
            printf("offsets disagree: '%s' on '%s' group %zu: host [%d,%d) sut [%ld,%ld)\n",
                   PATTERNS[p].pattern, SUBJECTS[s], k, (int)hm[k].rm_so, (int)hm[k].rm_eo, sm[k].rm_so,
                   sm[k].rm_eo);
            ASSERT_TRUE(0);
          }
        }
      }
      regfree(&host);
      sut_regfree(&sut);
    }
  }
}

UTEST(regex, notbol_and_newline)
{
  use_host_memory();
  sut_regex_t sut;
  sut_regmatch_t m[1];
  ASSERT_EQ(0, sut_regcomp(&sut, "^b", 0));
  ASSERT_EQ(0, sut_regexec(&sut, "b", 1, m, 0));
  ASSERT_NE(0, sut_regexec(&sut, "b", 1, m, SUT_NOTBOL));
  sut_regfree(&sut);

  ASSERT_EQ(0, sut_regcomp(&sut, "^b", SUT_NEWLINE));
  ASSERT_EQ(0, sut_regexec(&sut, "a\nb", 1, m, 0));
  ASSERT_EQ(2, m[0].rm_so);
  sut_regfree(&sut);

  ASSERT_EQ(0, sut_regcomp(&sut, "x", SUT_NOSUB));
  ASSERT_EQ(0, sut_regexec(&sut, "axb", 0, NULL, 0));
  sut_regfree(&sut);
}

UTEST(regex, error_messages)
{
  use_host_memory();
  sut_regex_t sut;
  char buf[64];
  int rc = sut_regcomp(&sut, "a\\(", 0);
  ASSERT_NE(0, rc);
  ASSERT_GT(sut_regerror(rc, &sut, buf, sizeof(buf)), (size_t)1);
  ASSERT_GT(strlen(buf), (size_t)0);
}

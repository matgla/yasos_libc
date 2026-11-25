/* dirname and basename (libgen.c, yasc: `make qemu` runs this on QEMU) against
 * POSIX.1-2024 -- XSH basename(), dirname(): DESCRIPTION, APPLICATION USAGE
 * and the EXAMPLES table -- and glibc's answers where POSIX gives one.  They
 * modify their argument, so each call gets its own copy. */

#include "utest.h"

#include <string.h>

char *borrows(static, path) sut_dirname(char path[maybe :0]);
char *borrows(static, path) sut_basename(char path[maybe :0]);

static int either(const char *view got, const char *view want, const char *view or_want)
{
  if (!strcmp(got, want) || !strcmp(got, or_want))
    return 1;
  printf("got \"%s\", want \"%s\" or \"%s\"\n", got, want, or_want);
  return 0;
}

/* path, then dirname's and basename's answers (glibc's) */
static const char *view const GLIBC[][3] = {
    {"usr/lib", "usr", "lib"}, {"/usr/lib", "/usr", "lib"},  {"/usr/", "/", "usr"},
    {"usr", ".", "usr"},       {"/", "/", "/"},              {"", ".", "."},
    {".", ".", "."},           {"..", ".", ".."},            {"a/b/", "a", "b"},
    {"a//b", "a", "b"},        {"/a", "/", "a"},             {"a/", ".", "a"},
    {"./configure", ".", "configure"}, {"../x/y//", "../x", "y"}, {"///", "/", "/"},
    {"a///", ".", "a"},
};

UTEST(libgen, dirname_cases)
{
  for (size_t i = 0; i < sizeof(GLIBC) / sizeof(GLIBC[0]); i++) {
    char s[64] = undefined;
    strcpy(s, GLIBC[i][0]);
    const char *view r = sut_dirname(s);
    if (strcmp(r, GLIBC[i][1]) != 0)
      printf("dirname(\"%s\"): \"%s\", want \"%s\"\n", GLIBC[i][0], r, GLIBC[i][1]);
    ASSERT_STREQ(GLIBC[i][1], r);
  }
  ASSERT_STREQ(".", sut_dirname(nullptr));
  /* POSIX lets "//" give "/" or "//"; glibc keeps "//", we (like musl) give "/". */
  char root[] = "//";
  ASSERT_STREQ("/", sut_dirname(root));
}

UTEST(libgen, basename_cases)
{
  for (size_t i = 0; i < sizeof(GLIBC) / sizeof(GLIBC[0]); i++) {
    char s[64] = undefined;
    strcpy(s, GLIBC[i][0]);
    const char *view r = sut_basename(s);
    if (strcmp(r, GLIBC[i][2]) != 0)
      printf("basename(\"%s\"): \"%s\", want \"%s\"\n", GLIBC[i][0], r, GLIBC[i][2]);
    ASSERT_STREQ(GLIBC[i][2], r);
  }
  ASSERT_STREQ(".", sut_basename(nullptr));
}

/* POSIX.1-2024's EXAMPLES table; where it allows two answers, either */
UTEST(libgen, posix_examples)
{
  static const char *view const CASES[][5] = {
      /* path, dirname, or, basename, or */
      {"usr", ".", ".", "usr", "usr"},
      {"usr/", ".", ".", "usr", "usr"},
      {"", ".", ".", ".", "."},
      {"/", "/", "/", "/", "/"},
      {"//", "/", "//", "/", "//"},
      {"///", "/", "///", "/", "/"},
      {"/usr/", "/", "/", "usr", "usr"},
      {"/usr/lib", "/usr", "/usr", "lib", "lib"},
      {"//usr//lib//", "//usr", "/usr", "lib", "lib"},
      {"/home//dwc//test", "/home//dwc", "/home/dwc", "test", "test"},
      {"/home/.././test", "/home/../.", "/home/..", "test", "test"}, /* ".." stays */
      {"/home/dwc/.", "/home/dwc", "/home/dwc", ".", "."},
      {"//foo", "//", "/", "foo", "foo"}, /* APPLICATION USAGE: "//" or "/" */
  };
  for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); i++) {
    char d[64] = undefined, b[64] = undefined;
    strcpy(d, CASES[i][0]);
    strcpy(b, CASES[i][0]);
    ASSERT_TRUE(either(sut_dirname(d), CASES[i][1], CASES[i][2]));
    ASSERT_TRUE(either(sut_basename(b), CASES[i][3], CASES[i][4]));
  }
}

/* POSIX sets no length limit (no pathname resolution, "always successful", no
   errors defined): past PATH_MAX (128) the answer is the same */
UTEST(libgen, longer_than_path_max)
{
  static char p[300], want[300];
  memset(p, 'b', sizeof p - 1);
  memcpy(p, "/x/", 3);
  memcpy(p + sizeof p - 5, "/c//", 4); /* 299 characters */
  memcpy(want, p, sizeof p - 5);       /* "/x/bbb...b" */
  ASSERT_STREQ(want, sut_dirname(p));
  memset(p, 'b', sizeof p - 1);
  memcpy(p, "/x/", 3);
  memcpy(p + sizeof p - 5, "/c//", 4);
  ASSERT_STREQ("c", sut_basename(p));
  memset(p, 'b', sizeof p - 1); /* one component */
  ASSERT_STREQ(".", sut_dirname(p));
  memset(p, 'b', sizeof p - 1);
  ASSERT_EQ(sizeof p - 1, strlen(sut_basename(p)));
}

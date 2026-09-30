/* dirname, against the host's POSIX dirname (it modifies its argument, so
 * each side gets its own copy). */

#include "utest.h"

#include <libgen.h>
#include <string.h>

char *sut_dirname(char *path);

UTEST(libgen, dirname_agrees_with_host)
{
  static const char *const CASES[] = {"usr/lib", "/usr/lib", "/usr/", "usr", "/", "", ".", "..",
                                      "a/b/", "a//b", "/a", "a/", "./configure", "../x/y//"};
  for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); i++) {
    char h[64], s[64];
    strcpy(h, CASES[i]);
    strcpy(s, CASES[i]);
    const char *hr = dirname(h);
    const char *sr = sut_dirname(s);
    if (strcmp(hr, sr) != 0)
      printf("dirname(\"%s\"): host \"%s\" sut \"%s\"\n", CASES[i], hr, sr);
    ASSERT_STREQ(hr, sr);
  }
  ASSERT_STREQ(".", sut_dirname(NULL));
  /* POSIX lets "//" give "/" or "//"; glibc keeps "//", we (like musl) give "/". */
  char root[] = "//";
  ASSERT_STREQ("/", sut_dirname(root));
}

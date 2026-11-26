#pragma once

/* Flag values are glibc's, deliberately: toybox's lib/portability.h defines
 * FNM_LEADING_DIR and FNM_CASEFOLD itself when a libc does not, and it uses
 * 8 and 16 -- so anything else here would disagree with a header that has
 * already made its mind up. */
#define FNM_PATHNAME (1 << 0) /* a slash is matched only by a slash */
#define FNM_NOESCAPE (1 << 1) /* backslash is an ordinary character */
#define FNM_PERIOD (1 << 2)   /* a leading dot must be matched literally */
#define FNM_LEADING_DIR (1 << 3) /* the pattern may end at a slash */
#define FNM_CASEFOLD (1 << 4)    /* compare without regard to case */

#define FNM_FILE_NAME FNM_PATHNAME

#define FNM_NOMATCH 1

int fnmatch(const char *pattern, const char *string, int flags);

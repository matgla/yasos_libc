/*
 * fnmatch(3) -- shell wildcards against a string.
 *
 * The header declared it and nothing defined it, which is a shape this libc
 * has had before ([[strtoull]]): the program that wants it does not fail to
 * build against the header, it fails to *link*, and only once somebody enables
 * the applet that needs it. Here that was `grep`, which uses fnmatch for the
 * globs behind --include and --exclude, and which therefore could not be built
 * into the rootfs at all -- `.tcc: error: undefined symbol 'fnmatch'`.
 *
 * ## Iterative, not recursive
 *
 * The textbook implementation recurses on every `*`, which on a pattern like
 * `*a*a*a*a*b` against a long string is exponential -- and on a device whose
 * whole stack is measured in kilobytes, the depth is the more immediate
 * problem. This backtracks instead, remembering the one place a `*` could give
 * a character back, which is all a single-star language needs: constant stack,
 * and the pathological pattern degrades to O(n*m) rather than 2^n.
 *
 * ## The three flags that change what a wildcard is
 *
 * `FNM_PATHNAME` is the one with teeth: a slash then has to be matched by a
 * slash in the pattern, so `*` no longer crosses a directory boundary and a
 * bracket cannot contain one. That is checked where the star gives ground --
 * if the character it would have to swallow is a slash, the match fails there
 * rather than continuing into the next segment.
 *
 * `FNM_PERIOD` makes a leading dot special, at the start of the string and
 * (under FNM_PATHNAME) after every slash: it has to be matched by a literal
 * dot, never by `*`, `?` or a bracket. This is what stops `*` matching the
 * dotfiles in a directory.
 *
 * `FNM_NOESCAPE` takes the meaning off backslash, so `\*` stops being a
 * literal asterisk and becomes a backslash followed by any string.
 */

#include <ctype.h>
#include <fnmatch.h>
#include <stddef.h>

static int same(char a, char b, int flags) {
  if (flags & FNM_CASEFOLD) {
    return tolower((unsigned char)a) == tolower((unsigned char)b);
  }
  return a == b;
}

/*
 * Match one bracket expression against one character.
 *
 * On success `*after` is left on the character following the `]`, which is
 * what lets the caller carry on without knowing how long the expression was.
 * An unterminated `[` is not an error: POSIX says it is then an ordinary
 * character, which is why that case reports no-match here and the caller
 * retries the `[` as a literal.
 */
static int bracket(const char *pattern, char c, int flags, const char **after) {
  const char *p = pattern + 1; /* past the '[' */
  int negated = 0;
  int matched = 0;

  if (*p == '!' || *p == '^') {
    negated = 1;
    p++;
  }
  /* A ']' first is a literal ']' rather than the end of an empty set. */
  if (*p == ']') {
    if (same(*p, c, flags)) matched = 1;
    p++;
  }

  for (; *p && *p != ']'; p++) {
    char lo = *p;

    if (lo == '\\' && !(flags & FNM_NOESCAPE) && p[1]) {
      lo = *++p;
    }
    if (p[1] == '-' && p[2] && p[2] != ']') {
      char hi = p[2];
      p += 2;
      if (hi == '\\' && !(flags & FNM_NOESCAPE) && p[1]) {
        hi = *++p;
      }
      if (flags & FNM_CASEFOLD) {
        int want = tolower((unsigned char)c);
        if ((want >= tolower((unsigned char)lo) &&
             want <= tolower((unsigned char)hi)) ||
            (c >= lo && c <= hi)) {
          matched = 1;
        }
      } else if (c >= lo && c <= hi) {
        matched = 1;
      }
    } else if (same(lo, c, flags)) {
      matched = 1;
    }
  }

  if (*p != ']') {
    return 0; /* unterminated: the caller treats '[' as an ordinary character */
  }
  *after = p + 1;

  /* A slash is never inside a bracket under FNM_PATHNAME, however the set is
   * written -- including a negated one, which would otherwise match it. */
  if ((flags & FNM_PATHNAME) && c == '/') {
    return 0;
  }
  return negated ? !matched : matched;
}

int fnmatch(const char *pattern, const char *string, int flags) {
  const char *p = pattern;
  const char *s = string;
  const char *star_p = NULL; /* the pattern just after the last '*' */
  const char *star_s = NULL; /* what that '*' was standing at */
  int leading = 1;           /* the next character begins a path segment */

  while (*s) {
    /* FNM_LEADING_DIR: the pattern is allowed to describe just the first few
     * segments. It is satisfied the moment the pattern runs out standing on a
     * slash -- "the string continues into a subdirectory" -- and NOT by a
     * pattern that still has `/...` left to match, which is the reading that
     * would make `*` match `a` and is what glibc refuses. */
    if (!*p && (flags & FNM_LEADING_DIR) && *s == '/') {
      return 0;
    }

    /* A dot at the start of a segment is only ever matched by a real dot. */
    if ((flags & FNM_PERIOD) && leading && *s == '.' && *p != '.') {
      return FNM_NOMATCH;
    }

    if (*p == '*') {
      while (*p == '*') p++;   /* several in a row are one */
      if (!*p) {
        /* Trailing '*' takes the rest -- but not across a slash, if slashes
         * are structural. */
        if (flags & FNM_PATHNAME) {
          while (*s && *s != '/') s++;
          if (!*s) return 0;
          return (flags & FNM_LEADING_DIR) ? 0 : FNM_NOMATCH;
        }
        return 0;
      }
      star_p = p;
      star_s = s;
      leading = 0;
      continue;
    }

    if (*p) {
      const char *after = NULL;
      int hit = 0;

      if (*p == '[') {
        hit = bracket(p, *s, flags, &after);
        if (hit) {
          p = after;
        } else if (after) {
          hit = 0; /* a well-formed set that simply did not match */
        } else {
          hit = same('[', *s, flags); /* unterminated: an ordinary '[' */
          if (hit) p++;
        }
      } else if (*p == '?') {
        hit = !((flags & FNM_PATHNAME) && *s == '/');
        if (hit) p++;
      } else {
        char want = *p;
        if (want == '\\' && !(flags & FNM_NOESCAPE) && p[1]) {
          want = p[1];
          if (same(want, *s, flags)) {
            p += 2;
            hit = 1;
          }
        } else if (same(want, *s, flags)) {
          p++;
          hit = 1;
        }
      }

      if (hit) {
        leading = (flags & FNM_PATHNAME) && *s == '/';
        s++;
        continue;
      }
    }

    /* No match here. Give the last '*' one more character, unless doing so
     * would carry it over a slash it is not allowed to cross. */
    if (!star_p) {
      return FNM_NOMATCH;
    }
    if ((flags & FNM_PATHNAME) && *star_s == '/') {
      return FNM_NOMATCH;
    }
    if ((flags & FNM_PERIOD) && leading && *star_s == '.') {
      return FNM_NOMATCH;
    }
    star_s++;
    s = star_s;
    p = star_p;
  }

  while (*p == '*') p++;
  return *p ? FNM_NOMATCH : 0;
}

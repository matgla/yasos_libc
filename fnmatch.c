/*
 * fnmatch(3) -- shell wildcards against a string.
 *
 * Iterative: a mismatch backtracks to the last `*` instead of recursing, so
 * stack use is constant and the worst case is O(n*m).
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
 * Match one bracket expression against `c`: `*hit` says whether it matched.
 * Returns the pattern just past the `]`, or null when the `[` is
 * unterminated (it is then an ordinary character).
 */
static const char *?borrows(pattern) bracket(char *pattern, char c, int flags, out int *hit) {
  const char *p = pattern + 1; /* past the '[' */
  int negated = 0;
  int matched = 0;

  if (*p == '!' || *p == '^') {
    negated = 1;
    p++;
  }
  /* A leading ']' is a literal. */
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
    *hit = 0;
    return nullptr;
  }
  /* Under FNM_PATHNAME no bracket, negated or not, matches '/'. */
  *hit = (flags & FNM_PATHNAME) && c == '/' ? 0 : negated ? !matched : matched;
  return p + 1;
}

int fnmatch(char *pattern, char *string, int flags) {
  const char *p = pattern;
  const char *s = string;
  const char *?star_p = nullptr; /* the pattern just after the last '*' */
  const char *?star_s = nullptr; /* what that '*' was standing at */
  int leading = 1;           /* the next character begins a path segment */

  while (*s) {
    /* FNM_LEADING_DIR: the pattern ran out at a '/' in the string. */
    if (!*p && (flags & FNM_LEADING_DIR) && *s == '/') {
      return 0;
    }

    /* FNM_PERIOD: a leading dot only matches a literal dot. */
    if ((flags & FNM_PERIOD) && leading && *s == '.' && *p != '.') {
      return FNM_NOMATCH;
    }

    if (*p == '*') {
      while (*p == '*') p++;
      if (!*p) {
        /* Trailing '*' takes the rest, but not across '/' under FNM_PATHNAME. */
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
      int hit = 0;

      if (*p == '[') {
        const char *?end = bracket(p, *s, flags, &hit);
        if (end) {
          if (hit) p = end;
        } else {
          hit = same('[', *s, flags); /* unterminated: literal '[' */
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

    /* Mismatch: let the last '*' take one more character. */
    if (!star_p || !star_s) {
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

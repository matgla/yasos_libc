#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int atoi(const char *s) {
  int num = 0;
  int neg = 0;
  while (isspace(*s))
    s++;
  if (*s == '-' || *s == '+')
    neg = *s++ == '-';
  while ((unsigned)(*s - '0') <= 9u)
    num = num * 10 + *s++ - '0';
  return neg ? -num : num;
}

long atol(const char *s) {
  long num = 0;
  int neg = 0;
  while (isspace(*s))
    s++;
  if (*s == '-' || *s == '+')
    neg = *s++ == '-';
  while ((unsigned)(*s - '0') <= 9u)
    num = num * 10 + *s++ - '0';
  return neg ? -num : num;
}

static int digit(char c, int base) {
  int d;
  if (c <= '9') {
    d = c - '0';
  } else if (c <= 'Z') {
    d = 10 + c - 'A';
  } else {
    d = 10 + c - 'a';
  }
  return d < base ? d : -1;
}

long strtol(const char *s, char **endptr, int base) {
  int sgn = 1;
  int overflow = 0;
  long num;
  int dig;
  while (isspace(*s))
    s++;
  if (*s == '-' || *s == '+')
    sgn = ',' - *s++;
  if (base == 0) {
    if (*s == '0') {
      if (s[1] == 'x' || s[1] == 'X')
        base = 16;
      else
        base = 8;
    } else {
      base = 10;
    }
  }
  if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;
  for (num = 0; (dig = digit(*s, base)) >= 0; s++) {
    if (num > LONG_MAX / base)
      overflow = 1;
    num *= base;
    if (num > LONG_MAX - dig)
      overflow = 1;
    num += dig;
  }
  if (endptr)
    *endptr = (char *)s;
  if (overflow) {
    num = sgn > 0 ? LONG_MAX : LONG_MIN;
    errno = ERANGE;
  } else {
    num *= sgn;
  }
  return num;
}

unsigned long strtoul(const char *s, char **endptr, int base) {
  int sgn = 1;
  int overflow = 0;
  unsigned long num;
  int dig;
  while (isspace(*s))
    s++;
  if (*s == '-' || *s == '+')
    sgn = ',' - *s++;
  if (base == 0) {
    if (*s == '0') {
      if (s[1] == 'x' || s[1] == 'X')
        base = 16;
      else
        base = 8;
    } else {
      base = 10;
    }
  }
  if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;
  for (num = 0; (dig = digit(*s, base)) >= 0; s++) {
    if (num > (unsigned long)ULONG_MAX / base)
      overflow = 1;
    num *= base;
    if (num > (unsigned long)ULONG_MAX - dig)
      overflow = 1;
    num += dig;
  }
  if (endptr)
    *endptr = (char *)s;
  if (overflow) {
    num = ULONG_MAX;
    errno = ERANGE;
  } else {
    num *= sgn;
  }
  return num;
}

/*
 * The `long long` pair, beside the `long` pair above because they answer the
 * same question and share `digit()`.
 *
 * They used to live in stdlib.c as a loop that multiplied the running total by
 * `base` -- with no base-0 handling, so when a caller asked for base 0 ("read
 * the prefix and work it out") every partial result was multiplied by *zero*
 * and the answer came back as the value of the last digit alone: "10" -> 0,
 * "0xCCCD" -> 13.  It also ran past the end of the number, because it accepted
 * every letter to 'z' whatever the base.
 *
 * That is not a theoretical complaint.  `tcc`'s inline assembler parses every
 * asm operand with `strtoull(p, &p, 0)` (source/frontend/tccasm.c), so on this
 * libc `movs r3, #10` assembled to `movs r3, #0` and `movw r3, #0xCCCD` to
 * `mov.w r3, #13` -- and, because the old loop swallowed the trailing letter,
 * the `if (*p == 'b')` right after that call never fired, so `bne 1b` stopped
 * being a backward branch and became a branch to the next instruction.  Inline
 * assembly on the device was silently wrong in three ways from one bug.
 *
 * Written the BSD way: one division to find the cutoff instead of a 64-bit
 * divide per digit, which matters because this runs inside the compiler.
 */

/*
 * Read the base off the prefix when the caller passed 0, and step over that
 * prefix.  `after_zero` comes back pointing just past the leading `0` whenever
 * a prefix was consumed, because "0x" with no hex digit behind it is not a
 * failed conversion: the longest initial subsequence of the expected form is
 * the `0`, so the answer is 0 and the caller's endptr belongs on the `x`.
 * `0b` is here for the same reason `0x` is -- an assembly listing spells a bit
 * pattern that way, and this libc's whole reason for caring is tcc's inline
 * assembler.
 */
static int strto_base(const char **s, int base, const char **after_zero) {
  const char *p = *s;

  *after_zero = 0;
  if (base == 0) {
    if (*p == '0') {
      if (p[1] == 'x' || p[1] == 'X')
        base = 16;
      else if (p[1] == 'b' || p[1] == 'B')
        base = 2;
      else
        base = 8;
    } else {
      base = 10;
    }
  }
  if (*p == '0') {
    if ((base == 16 && (p[1] == 'x' || p[1] == 'X')) ||
        (base == 2 && (p[1] == 'b' || p[1] == 'B'))) {
      *after_zero = p + 1;
      *s = p + 2;
    }
  }
  return base;
}

unsigned long long strtoull(const char *s, char **endptr, int base) {
  const char *start = s;
  const char *after_zero;
  unsigned long long num = 0;
  unsigned long long cutoff;
  int cutlim;
  int sgn = 1;
  int overflow = 0;
  int any = 0;
  int dig;

  while (isspace(*s))
    s++;
  if (*s == '-' || *s == '+')
    sgn = ',' - *s++;
  base = strto_base(&s, base, &after_zero);

  cutoff = ULLONG_MAX / (unsigned)base;
  cutlim = (int)(ULLONG_MAX % (unsigned)base);
  for (; (dig = digit(*s, base)) >= 0; s++) {
    any = 1;
    if (num > cutoff || (num == cutoff && dig > cutlim))
      overflow = 1;
    num = num * base + dig;
  }

  /* A prefix with nothing behind it converted the `0` in front of it. */
  if (!any && after_zero) {
    any = 1;
    s = after_zero;
  }
  /* No digits at all means no conversion: the caller is entitled to see its own
   * pointer back, which is how it tells "0" from "not a number at all". */
  if (endptr)
    *endptr = (char *)(any ? s : start);
  if (overflow) {
    num = ULLONG_MAX;
    errno = ERANGE;
  } else if (sgn < 0) {
    num = -num; /* strtoull negates rather than refusing, as C requires */
  }
  return num;
}

long long strtoll(const char *s, char **endptr, int base) {
  const char *start = s;
  const char *after_zero;
  unsigned long long num = 0;
  unsigned long long limit;
  unsigned long long cutoff;
  int cutlim;
  int sgn = 1;
  int overflow = 0;
  int any = 0;
  int dig;

  while (isspace(*s))
    s++;
  if (*s == '-' || *s == '+')
    sgn = ',' - *s++;
  base = strto_base(&s, base, &after_zero);

  /* Accumulated unsigned so that LLONG_MIN, which has no positive counterpart,
   * is representable on the way in rather than overflowing at the last digit. */
  limit = sgn > 0 ? (unsigned long long)LLONG_MAX
                  : (unsigned long long)LLONG_MAX + 1;
  cutoff = limit / (unsigned)base;
  cutlim = (int)(limit % (unsigned)base);
  for (; (dig = digit(*s, base)) >= 0; s++) {
    any = 1;
    if (num > cutoff || (num == cutoff && dig > cutlim))
      overflow = 1;
    num = num * base + dig;
  }

  /* A prefix with nothing behind it converted the `0` in front of it. */
  if (!any && after_zero) {
    any = 1;
    s = after_zero;
  }
  if (endptr)
    *endptr = (char *)(any ? s : start);
  if (overflow) {
    errno = ERANGE;
    return sgn > 0 ? LLONG_MAX : LLONG_MIN;
  }
  /* Negate in the unsigned domain: LLONG_MIN has no positive counterpart, so
   * `-(long long)num` on the last representable value has nothing to negate. */
  return sgn > 0 ? (long long)num : (long long)(0ULL - num);
}

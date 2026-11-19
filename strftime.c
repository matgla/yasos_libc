#include "rorel.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

static char *ROREL const wday_ab[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char *ROREL const wday[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                       "Thursday", "Friday", "Saturday"};
static char *ROREL const mon_ab[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *ROREL const mon[] = {"January",   "February", "March",    "April",
                      "May",       "June",     "July",     "August",
                      "September", "October",  "November", "December"};

static char *putstr(char *d, char *s) {
  while (*s)
    *d++ = *s++;
  return d;
}

static char *puti(char *s, unsigned long n, int wid, int zpad) {
  int i;
  for (i = wid - 1; i >= 0; i--) {
    if (!n) {
      if (!zpad && i < wid - 1)
        s[i] = ' ';
      else
        s[i] = '0';
    } else {
      s[i] = '0' + n % 10;
      n /= 10;
    }
  }
  return s + wid;
}

/* Variable-width unsigned decimal, unlike `puti` which pads or truncates to a
   fixed width. %s needs it: an epoch second is ten digits today and eleven in
   2286, and truncating it to a fixed width would silently produce a plausible
   wrong number. */
static char *putul(char *s, unsigned long n) {
  char digits[24];
  int i = 0;
  do {
    digits[i++] = '0' + (char)(n % 10);
    n /= 10;
  } while (n);
  while (i > 0)
    *s++ = digits[--i];
  return s;
}

/* The zone's offset from tm_gmtoff, which localtime() and mktime() fill in,
   as +hhmm / -hhmm. */
static char *puttz(char *s, const struct tm *tm) {
  long d = tm->tm_gmtoff / 60;
  if (d < 0) {
    *s++ = '-';
    d = -d;
  } else {
    *s++ = '+';
  }
  s = puti(s, d / 60, 2, 1);
  s = puti(s, d % 60, 2, 1);
  return s;
}

long strftime(char *s, size_t n, const char *f, struct tm *tm) {
  int val;
  char *beg = s;
  char *e = s + n;
  while (s + 1 < e && *f) {
    int c = *f++;
    if (c != '%') {
      *s++ = c;
      continue;
    }
    c = *f++;
    switch (c) {
    case '%':
      *s++ = '%';
      break;
    case 'a':
      s = putstr(s, wday_ab[tm->tm_wday]);
      break;
    case 'A':
      s = putstr(s, wday[tm->tm_wday]);
      break;
    case 'b':
    case 'h':
      s = putstr(s, mon_ab[tm->tm_mon]);
      break;
    case 'B':
      s = putstr(s, mon[tm->tm_mon]);
      break;
    // case 'c':
    //   s += strftime(s, e - s, "%b %a %d %k:%M:%S %Z %Y", tm);
    //   break;
    case 'C':
      s = puti(s, (1900 + tm->tm_year) / 100, 2, 1);
      break;
    case 'd':
      s = puti(s, tm->tm_mday, 2, 1);
      break;
    case 'D':
      s += strftime(s, e - s, "%m/%d/%y", tm);
      break;
    case 'e':
      s = puti(s, tm->tm_mday, 2, 0);
      break;
    case 'F':
      /* ISO 8601, which is what %F means -- dashes, not slashes. `ls -l` uses
         it for every timestamp it prints, so the slashes were visible on every
         long listing the system has ever produced. */
      s += strftime(s, e - s, "%Y-%m-%d", tm);
      break;
    case 'H':
      s = puti(s, tm->tm_hour, 2, 1);
      break;
    case 'I':
      val = tm->tm_hour > 12 ? tm->tm_hour - 12 : tm->tm_hour;
      s = puti(s, val ? val : 12, 2, 1);
      break;
    case 'j':
      s = puti(s, tm->tm_yday + 1, 3, 1);
      break;
    case 'k':
      s = puti(s, tm->tm_hour, 2, 1);
      break;
    case 'm':
      s = puti(s, tm->tm_mon + 1, 2, 1);
      break;
    case 'M':
      s = puti(s, tm->tm_min, 2, 1);
      break;
    case 'n':
      *s++ = '\n';
      break;
    case 'p':
      s = putstr(s, tm->tm_hour >= 12 ? "PM" : "AM");
      break;
    case 'P':
      s = putstr(s, tm->tm_hour >= 12 ? "pm" : "am");
      break;
    case 'r':
      s += strftime(s, e - s, "%I:%M:%S %p", tm);
      break;
    case 'R':
      s += strftime(s, e - s, "%H:%M", tm);
      break;
    case 's':
      /* Seconds since the epoch. mktime() is the inverse of the localtime()
         that produced `tm`, timezone included. toybox's `date` asks for this
         with a bare "%s" and treats a zero-length result as a bad format, so
         its absence made `date +%s` an error rather than a missing field. */
      s = putul(s, (unsigned long)mktime(tm));
      break;
    case 'S':
      s = puti(s, tm->tm_sec, 2, 1);
      break;
    case 't':
      *s++ = '\t';
      break;
    case 'T':
      s += strftime(s, e - s, "%H:%M:%S", tm);
      break;
    case 'u':
      s = puti(s, tm->tm_wday ? tm->tm_wday : 7, 1, 0);
      break;
    case 'w':
      s = puti(s, tm->tm_wday, 1, 0);
      break;
    case 'x':
      s += strftime(s, e - s, "%b %a %d", tm);
      break;
    // case 'X':
    //   s += strftime(s, e - s, "%k:%M:%S", tm);
    //   break;
    case 'y':
      s = puti(s, tm->tm_year % 100, 2, 1);
      break;
    case 'Y':
      s = puti(s, 1900 + tm->tm_year, 4, 1);
      break;
    case 'z':
      s = puttz(s, tm);
      break;
    case 'Z':
      /* tm_zone, which localtime() points at the zone's abbreviation; a
         struct tm put together by hand has none, and gets the current zone's
         name for its tm_isdst. */
      if (tm->tm_zone) {
        s = putstr(s, tm->tm_zone);
      } else {
        tzset();
        s = putstr(s, tzname[tm->tm_isdst > 0]);
      }
      break;
    default:
      break;
    }
  }
  *s = '\0';
  return s - beg;
}

/* --- strptime -------------------------------------------------------------
 * POSIX's conversions in the C locale, plus glibc's %z, %s, %F, %k, %l, %G,
 * %g, %V and the E/O modifiers (accepted, ignored). As glibc: white space in
 * the format matches any amount of it in the input, numbers may be preceded
 * by blanks and take at most their field's width of digits, %y alone is
 * 1969..2068, %I takes its half of the day from %p, and once a date has been
 * read tm_wday and tm_yday are worked out from it. toybox's date -d and
 * touch -d, and sort -M, are the callers here. */

struct strptime_state {
  int have_I, is_pm;
  int have_century, century;
  int have_y, year_in_century;
  int have_wday, have_yday, have_mon, have_mday;
  int want_xday;
};

static int sp_space(int c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}

static int sp_lower(int c) {
  return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

/* At most `width` digits after optional blanks, in [lo, hi]. */
static const char *sp_num(const char *s, int width, int lo, int hi, int *out) {
  int v = 0, n = 0;
  while (*s == ' ')
    s++;
  while (n < width && *s >= '0' && *s <= '9') {
    v = v * 10 + (*s++ - '0');
    n++;
  }
  if (!n || v < lo || v > hi)
    return NULL;
  *out = v;
  return s;
}

/* One of `n` names, full or abbreviated (`abbr`), any case; the longest
   match wins, so "Mar" does not stop short of "March". */
static const char *sp_name(const char *s, char *ROREL const *full, char *ROREL const *abbr, int n,
                           int *out) {
  int best = -1;
  size_t best_len = 0;
  for (int i = 0; i < n; i++) {
    const char *names[2] = {full[i], abbr[i]};
    for (int k = 0; k < 2; k++) {
      size_t len = 0;
      while (names[k][len] && sp_lower(s[len]) == sp_lower(names[k][len]))
        len++;
      if (!names[k][len] && len > best_len) {
        best = i;
        best_len = len;
      }
    }
  }
  if (best < 0)
    return NULL;
  *out = best;
  return s + best_len;
}

static int sp_leap(long y) {
  return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

/* Days from 1970-01-01 (proleptic Gregorian). */
static long sp_days(long y, int m, int d) {
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  long yoe = y - era * 400;
  long doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

static const char *sp_parse(const char *s, const char *f, struct tm *tm, struct strptime_state *st) {
  int v;
  while (*f) {
    if (sp_space(*f)) {
      while (sp_space(*s))
        s++;
      f++;
      continue;
    }
    if (*f != '%') {
      if (*s++ != *f++)
        return NULL;
      continue;
    }
    f++;
    if (*f == 'E' || *f == 'O')
      f++;
    switch (*f++) {
    case '%':
      if (*s++ != '%')
        return NULL;
      break;
    case 'n':
    case 't':
      while (sp_space(*s))
        s++;
      break;
    case 'a':
    case 'A':
      if (!(s = sp_name(s, wday, wday_ab, 7, &tm->tm_wday)))
        return NULL;
      st->have_wday = 1;
      break;
    case 'b':
    case 'B':
    case 'h':
      if (!(s = sp_name(s, mon, mon_ab, 12, &tm->tm_mon)))
        return NULL;
      st->have_mon = 1;
      st->want_xday = 1;
      break;
    case 'c':
      if (!(s = sp_parse(s, "%a %b %e %H:%M:%S %Y", tm, st)))
        return NULL;
      break;
    case 'C':
      if (!(s = sp_num(s, 2, 0, 99, &st->century)))
        return NULL;
      st->have_century = 1;
      st->want_xday = 1;
      break;
    case 'd':
    case 'e':
      if (!(s = sp_num(s, 2, 1, 31, &tm->tm_mday)))
        return NULL;
      st->have_mday = 1;
      st->want_xday = 1;
      break;
    case 'D':
    case 'x':
      if (!(s = sp_parse(s, "%m/%d/%y", tm, st)))
        return NULL;
      break;
    case 'F':
      if (!(s = sp_parse(s, "%Y-%m-%d", tm, st)))
        return NULL;
      break;
    case 'H':
    case 'k':
      if (!(s = sp_num(s, 2, 0, 23, &tm->tm_hour)))
        return NULL;
      st->have_I = 0;
      break;
    case 'I':
    case 'l':
      if (!(s = sp_num(s, 2, 1, 12, &tm->tm_hour)))
        return NULL;
      st->have_I = 1;
      break;
    case 'j':
      if (!(s = sp_num(s, 3, 1, 366, &v)))
        return NULL;
      tm->tm_yday = v - 1;
      st->have_yday = 1;
      st->want_xday = 1;
      break;
    case 'm':
      if (!(s = sp_num(s, 2, 1, 12, &v)))
        return NULL;
      tm->tm_mon = v - 1;
      st->have_mon = 1;
      st->want_xday = 1;
      break;
    case 'M':
      if (!(s = sp_num(s, 2, 0, 59, &tm->tm_min)))
        return NULL;
      break;
    case 'p':
      if (sp_lower(s[0]) == 'a' && sp_lower(s[1]) == 'm')
        st->is_pm = 0;
      else if (sp_lower(s[0]) == 'p' && sp_lower(s[1]) == 'm')
        st->is_pm = 1;
      else
        return NULL;
      s += 2;
      break;
    case 'r':
      if (!(s = sp_parse(s, "%I:%M:%S %p", tm, st)))
        return NULL;
      break;
    case 'R':
      if (!(s = sp_parse(s, "%H:%M", tm, st)))
        return NULL;
      break;
    case 's': {
      /* Seconds since the epoch, as local time (glibc). */
      long long secs = 0;
      int neg = *s == '-', n = 0;
      if (neg)
        s++;
      while (*s >= '0' && *s <= '9' && n < 20) {
        secs = secs * 10 + (*s++ - '0');
        n++;
      }
      if (!n)
        return NULL;
      time_t t = (time_t)(neg ? -secs : secs);
      if (!localtime_r(&t, tm))
        return NULL;
      break;
    }
    case 'S':
      if (!(s = sp_num(s, 2, 0, 61, &tm->tm_sec)))
        return NULL;
      break;
    case 'T':
    case 'X':
      if (!(s = sp_parse(s, "%H:%M:%S", tm, st)))
        return NULL;
      break;
    case 'u':
      if (!(s = sp_num(s, 1, 1, 7, &v)))
        return NULL;
      tm->tm_wday = v % 7;
      st->have_wday = 1;
      break;
    case 'w':
      if (!(s = sp_num(s, 1, 0, 6, &tm->tm_wday)))
        return NULL;
      st->have_wday = 1;
      break;
    case 'U':
    case 'W':
    case 'V':
      /* Week numbers: read, not used to place the date. */
      if (!(s = sp_num(s, 2, 0, 53, &v)))
        return NULL;
      break;
    case 'g':
      if (!(s = sp_num(s, 2, 0, 99, &v)))
        return NULL;
      break;
    case 'G':
      if (!(s = sp_num(s, 4, 0, 9999, &v)))
        return NULL;
      break;
    case 'y':
      if (!(s = sp_num(s, 2, 0, 99, &st->year_in_century)))
        return NULL;
      st->have_y = 1;
      st->want_xday = 1;
      break;
    case 'Y':
      if (!(s = sp_num(s, 4, 0, 9999, &v)))
        return NULL;
      tm->tm_year = v - 1900;
      st->have_y = st->have_century = 0;
      st->want_xday = 1;
      break;
    case 'z': {
      /* Z, or +hh[[:]mm]: into tm_gmtoff. */
      int sign, h, m = 0;
      while (*s == ' ')
        s++;
      if (*s == 'Z') {
        s++;
        tm->tm_gmtoff = 0;
        break;
      }
      if (*s != '+' && *s != '-')
        return NULL;
      sign = *s++ == '-' ? -1 : 1;
      if (!(s[0] >= '0' && s[0] <= '9' && s[1] >= '0' && s[1] <= '9'))
        return NULL;
      h = (s[0] - '0') * 10 + (s[1] - '0');
      s += 2;
      if (*s == ':')
        s++;
      if (s[0] >= '0' && s[0] <= '9' && s[1] >= '0' && s[1] <= '9') {
        m = (s[0] - '0') * 10 + (s[1] - '0');
        s += 2;
      }
      if (h > 24 || m > 59)
        return NULL;
      tm->tm_gmtoff = sign * (h * 3600L + m * 60L);
      break;
    }
    case 'Z':
      /* A zone name: skipped, as glibc does (no table to look it up in). */
      while (*s && !sp_space(*s))
        s++;
      break;
    default:
      return NULL;
    }
  }
  return s;
}

char *strptime(const char *s, const char *fmt, struct tm *tm) {
  static const unsigned short ydays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  struct strptime_state st = {0};
  const char *end = sp_parse(s, fmt, tm, &st);
  if (!end)
    return NULL;
  /* %p only moves a 12-hour %I; with %H it is read and means nothing. */
  if (st.have_I)
    tm->tm_hour = tm->tm_hour % 12 + (st.is_pm ? 12 : 0);
  if (st.have_century) {
    tm->tm_year = st.have_y ? st.century * 100 + st.year_in_century - 1900 : (st.century - 19) * 100;
  } else if (st.have_y) {
    tm->tm_year = st.year_in_century < 69 ? st.year_in_century + 100 : st.year_in_century;
  }
  if (st.want_xday) {
    long year = 1900L + tm->tm_year;
    if (st.have_yday && !(st.have_mon && st.have_mday)) {
      /* A day of the year without a month and day: place it. */
      int m = 11;
      while (m > 0 && tm->tm_yday < ydays[m] + (m > 1 && sp_leap(year)))
        m--;
      if (!st.have_mon)
        tm->tm_mon = m;
      if (!st.have_mday)
        tm->tm_mday = tm->tm_yday - ydays[tm->tm_mon] - (tm->tm_mon > 1 && sp_leap(year)) + 1;
      st.have_mon = st.have_mday = 1;
    }
    if (tm->tm_mon >= 0 && tm->tm_mon <= 11) {
      long days = sp_days(year, tm->tm_mon + 1, tm->tm_mday);
      if (!st.have_wday)
        tm->tm_wday = (int)(((days + 4) % 7 + 7) % 7);
      if (!st.have_yday)
        tm->tm_yday = ydays[tm->tm_mon] + (tm->tm_mon > 1 && sp_leap(year)) + tm->tm_mday - 1;
    }
  }
  return (char *)end;
}

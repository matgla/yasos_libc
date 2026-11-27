/* Time zones and broken-down time.
 *
 * The kernel keeps UTC and knows nothing about zones (gettimeofday reports
 * tz_minuteswest 0, as Linux does); local time is this file's business, as it
 * is glibc's and musl's. The zone comes from:
 *
 *   TZ unset           /etc/localtime, a TZif file (in the image a symlink to
 *                      /var/etc/localtime, so the card holds the choice);
 *                      missing or unreadable means UTC
 *   TZ=""              UTC
 *   TZ=Europe/Warsaw   a TZif file (RFC 8536) under $TZDIR, else
 *                      /usr/share/zoneinfo; an absolute path is used as is
 *   TZ=CET-1CEST,M3.5.0,M10.5.0/3
 *                      a POSIX TZ string: the rules themselves, no file
 *
 * A leading ':' is accepted and skipped. Anything that is neither a file nor a
 * valid POSIX string falls back to UTC, as glibc does.
 *
 * A TZif file lists the zone's transitions up to the year it was compiled and
 * ends with a POSIX string for everything after the last one, so the POSIX
 * rules serve both cases. The file is read whole (a slim zone is about 1 KB)
 * and kept; the zone is looked up again only when $TZ changes.
 *
 * The no-OS build (the kernel's static libc) has no files: there only TZ
 * strings and UTC exist. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef long long i64;

#define SPD 86400L

long timezone;
int daylight;
static const char utc_name[] = "UTC";
const char *tzname[2] = {utc_name, utc_name};

/* --- calendar ------------------------------------------------------------
 * Proleptic Gregorian, days counted from 1970-01-01 (H. Hinnant's
 * days_from_civil / civil_from_days): exact for negative times and any year,
 * where the old loop over years stopped at 1970. */

static i64 floor_div(i64 a, i64 b) {
  i64 q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0)))
    q--;
  return q;
}

static int is_leap(i64 y) {
  return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int month_days(i64 y, int m) {
  static const unsigned char days[] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
  return days[m - 1] + (m == 2 && is_leap(y));
}

/* Days from 1970-01-01 to y-m-d (m 1..12, d 1..31). */
static i64 days_from_civil(i64 y, int m, int d) {
  y -= m <= 2;
  i64 era = floor_div(y, 400);
  i64 yoe = y - era * 400;
  i64 doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  i64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

/* UTC seconds `t` into `tm`, fields only (no zone). 0 when the year does not
   fit tm_year. */
static int secs_to_tm(i64 t, out struct tm *tm) {
  i64 days = floor_div(t, SPD);
  long rem = cast<int(long)>(t - days * SPD);
  i64 z = days + 719468;
  i64 era = floor_div(z, 146097);
  long doe = cast<int(long)>(z - era * 146097);
  long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  long mp = (5 * doy + 2) / 153;
  int mday = cast<as(int)>(doy - (153 * mp + 2) / 5 + 1);
  int mon = cast<as(int)>(mp < 10 ? mp + 3 : mp - 9);
  i64 year = yoe + era * 400 + (mon <= 2);
  if (year - 1900 > 0x7fffffffLL || year - 1900 < -0x7fffffffLL - 1) {
    *tm = (struct tm){};
    return 0;
  }
  tm->tm_year = cast<int(int)>(year - 1900);
  tm->tm_mon = mon - 1;
  tm->tm_mday = mday;
  tm->tm_yday = cast<int(int)>(days - days_from_civil(year, 1, 1));
  tm->tm_wday = cast<int(int)>(((days + 4) % 7 + 7) % 7);
  tm->tm_hour = cast<as(int)>(rem / 3600);
  tm->tm_min = cast<as(int)>(rem / 60 % 60);
  tm->tm_sec = cast<as(int)>(rem % 60);
  return 1;
}

/* `tm`'s fields as seconds, as if they were UTC; any field may be out of its
   range (mktime's normalisation is this arithmetic). */
static i64 tm_to_secs(struct tm *tm) {
  i64 mon = tm->tm_mon;
  i64 year = 1900LL + tm->tm_year + floor_div(mon, 12);
  mon -= floor_div(mon, 12) * 12;
  i64 days =
      days_from_civil(year, cast<int(int)>(mon) + 1, 1) + tm->tm_mday - 1;
  return days * SPD + tm->tm_hour * 3600LL + tm->tm_min * 60LL + tm->tm_sec;
}

/* --- POSIX TZ strings ------------------------------------------------------
 *   std offset [dst [offset] [,start[/time],end[/time]]]
 * Offsets are positive west of Greenwich (EST5); everything below keeps
 * seconds EAST (utoff), as TZif does. */

struct tzrule {
  char kind; /* 'J' Jn (no Feb 29), 'D' n (0..365), 'M' Mm.w.d */
  int m, w, d;
  long secs; /* local time of day of the switch, may be negative or > 24h */
};

struct posix_tz {
  char std_name[16];
  char dst_name[16];
  long std_off;
  long dst_off;
  int has_dst;
  struct tzrule start;
  struct tzrule end;
};

static int is_alpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_digit(int c) {
  return c >= '0' && c <= '9';
}

static const char *?view parse_name(char *s, out char *out, size_t cap) {
  size_t n = 0;
  *out = '\0';
  if (*s == '<') {
    s++;
    while (*s && *s != '>') {
      if (!is_alpha(*s) && !is_digit(*s) && *s != '+' && *s != '-')
        return nullptr;
      if (n + 1 >= cap)
        return nullptr;
      out[n++] = *s++;
    }
    if (*s != '>')
      return nullptr;
    s++;
  } else {
    while (is_alpha(*s)) {
      if (n + 1 >= cap)
        return nullptr;
      out[n++] = *s++;
    }
  }
  if (n < 3)
    return nullptr;
  out[n] = '\0';
  return s;
}

static const char *?view parse_num(char *s, out long *out, long max) {
  long v = 0;
  *out = 0;
  if (!is_digit(*s))
    return nullptr;
  while (is_digit(*s)) {
    v = v * 10 + (*s++ - '0');
    if (v > max)
      return nullptr;
  }
  *out = v;
  return s;
}

/* [+-]hh[:mm[:ss]] in seconds. */
static const char *?view parse_time(char *?s, out long *out,
                                    long max_hours) {
  long sign = 1, h, m = 0, sec = 0;
  *out = 0;
  if (!s) {
    return nullptr;
  }
  if (*s == '+' || *s == '-')
    sign = *s++ == '-' ? -1 : 1;
  if (!(s = parse_num(s, &h, max_hours)))
    return nullptr;
  if (*s == ':') {
    if (!(s = parse_num(s + 1, &m, 59)))
      return nullptr;
    if (*s == ':' && !(s = parse_num(s + 1, &sec, 59)))
      return nullptr;
  }
  *out = sign * (h * 3600 + m * 60 + sec);
  return s;
}

static const char *?view parse_rule(char *?s, out struct tzrule *r) {
  long a, b, c;
  *r = (struct tzrule){};
  if (!s) {
    return nullptr;
  }
  if (*s == 'J') {
    if (!(s = parse_num(s + 1, &a, 365)) || a < 1)
      return nullptr;
    r->kind = 'J';
    r->d = cast<as(int)>(a);
  } else if (*s == 'M') {
    if (!(s = parse_num(s + 1, &a, 12)) || a < 1 || *s != '.')
      return nullptr;
    if (!(s = parse_num(s + 1, &b, 5)) || b < 1 || *s != '.')
      return nullptr;
    if (!(s = parse_num(s + 1, &c, 6)))
      return nullptr;
    r->kind = 'M';
    r->m = cast<as(int)>(a);
    r->w = cast<as(int)>(b);
    r->d = cast<as(int)>(c);
  } else {
    if (!(s = parse_num(s, &a, 365)))
      return nullptr;
    r->kind = 'D';
    r->d = cast<as(int)>(a);
  }
  r->secs = 7200;
  /* RFC 8536 3.3.1 allows -167..167 hours here. */
  if (*s == '/' && !(s = parse_time(s + 1, &r->secs, 167)))
    return nullptr;
  return s;
}

static int parse_posix(char *?s, out struct posix_tz *p) {
  long off;
  memset(p, 0, sizeof(*p));
  if (!s) {
    return 0;
  }
  if (!(s = parse_name(s, p->std_name, sizeof(p->std_name))))
    return 0;
  if (!(s = parse_time(s, &off, 24)))
    return 0;
  p->std_off = -off;
  if (!*s)
    return 1;
  if (!(s = parse_name(s, p->dst_name, sizeof(p->dst_name))))
    return 0;
  p->has_dst = 1;
  p->dst_off = p->std_off + 3600;
  if (*s && *s != ',') {
    if (!(s = parse_time(s, &off, 24)))
      return 0;
    p->dst_off = -off;
  }
  if (*s == ',') {
    if (!(s = parse_rule(s + 1, &p->start)) || *s != ',')
      return 0;
    if (!(s = parse_rule(s + 1, &p->end)))
      return 0;
  } else {
    /* No rule: POSIX leaves it to the implementation; this is glibc's and
       musl's choice, the current US one. */
    struct tzrule us_start = {'M', 3, 2, 0, 7200};
    struct tzrule us_end = {'M', 11, 1, 0, 7200};
    p->start = us_start;
    p->end = us_end;
  }
  return *s == '\0';
}

/* The rule's day in `year` at its time of day, local time, in seconds as if
   that local time were UTC. */
static i64 rule_local(i64 year, struct tzrule *r) {
  i64 jan1 = days_from_civil(year, 1, 1);
  i64 day;
  if (r->kind == 'J') {
    day = jan1 + r->d - 1 + (is_leap(year) && r->d >= 60);
  } else if (r->kind == 'D') {
    day = jan1 + r->d;
  } else {
    i64 first = days_from_civil(year, r->m, 1);
    int wday1 = cast<int(int)>(((first + 4) % 7 + 7) % 7);
    int mday = (r->d - wday1 + 7) % 7 + (r->w - 1) * 7;
    int mdays = month_days(year, r->m);
    while (mday >= mdays)
      mday -= 7;
    day = first + mday;
  }
  return day * SPD + r->secs;
}

struct zone_info {
  long off;
  int isdst;
  const char *abbr;
};

static const char *borrows(static) intern(char *s);

/* Interned copies of the POSIX names, so tm_zone outlives a change of zone. */
static const char *posix_std_abbr;
static const char *posix_dst_abbr;

static void posix_at(struct posix_tz *p, i64 t, out struct zone_info *z) {
  z->off = p->std_off;
  z->isdst = 0;
  z->abbr = posix_std_abbr;
  if (!p->has_dst)
    return;
  i64 year;
  {
    struct tm tm = {};
    if (!secs_to_tm(t + p->std_off, &tm))
      return;
    year = 1900LL + tm.tm_year;
  }
  i64 start = rule_local(year, &p->start) - p->std_off;
  i64 end = rule_local(year, &p->end) - p->dst_off;
  int dst = start < end ? (t >= start && t < end) : !(t >= end && t < start);
  if (dst) {
    z->off = p->dst_off;
    z->isdst = 1;
    z->abbr = posix_dst_abbr;
  }
}

/* --- zone state ------------------------------------------------------------
 */

enum { ZONE_UTC, ZONE_POSIX, ZONE_FILE };

static int zone_mode = ZONE_UTC;
static int zone_loaded;
static int zone_tz_unset;          /* the loaded zone came from an unset TZ */
static char *?owned(free) zone_tz; /* ... or from this value of it */
static struct posix_tz zone_posix;
static int zone_has_rules; /* ZONE_FILE: the footer gave zone_posix */

/* ZONE_FILE: the file, and views into its version 2+ (or version 1) body.
   borrows(zfile): the compiler keeps the views pointing into the file zfile
   owns now, or at zempty -- where zfile is freed or replaced they are set
   again before anything could read them (zfile_drop). */
static unsigned char *?owned(free) zfile;
static const unsigned char zempty[1];
static const unsigned char *borrows(zfile) ztrans = zempty;
static const unsigned char *borrows(zfile) zidx = zempty;
static const unsigned char *borrows(zfile) ztypes = zempty;
static const char **?owned(free) zabbr; /* interned abbreviation per type */
static unsigned long ztimecnt, ztypecnt;
static int ztsize; /* 8, or 4 for a version 1 file */

/* The file goes, and its views with it -- to zempty, with no call between
   that could read them. */
static void zfile_drop(void) {
  free(move(zfile));
  zfile = nullptr;
  ztrans = zempty;
  zidx = zempty;
  ztypes = zempty;
}

/* Abbreviations live here for good: tm_zone and tzname point at them, and a
   struct tm may outlive the zone it was made in (toybox's date switches TZ
   to parse its input). A zone change interns its new names; past the pool
   they go to the heap, which only a long run of distinct zones reaches.
   The table owns every name -- the pool, and the heap ones through
   `spilled_names` -- so a caller only borrows: nothing it gets is freed. */
struct spilled_name {
  struct spilled_name *?owned next;
  char s[];
};
static struct spilled_name *?owned spilled_names;

static const char *borrows(static) intern(char *s) {
  static char pool[256];
  static size_t used;
  size_t n = strlen(s) + 1;
  for (char *p = pool; p < pool + used; p += strlen(p) + 1)
    if (!strcmp(p, s))
      return p;
  for (struct spilled_name *?q = spilled_names; q; q = q->next)
    if (!strcmp(q->s, s))
      return q->s;
  if (used + n <= sizeof(pool)) {
    memcpy(pool + used, s, n);
    used += n;
    return pool + used - n;
  }
  struct spilled_name *?owned h = malloc(sizeof(*h) + n);
  if (!h)
    return utc_name;
  memcpy(h->s, s, n);
  h->next = move(spilled_names);
  spilled_names = move(h);
  return spilled_names->s;
}

static unsigned long be32(unsigned char *p) {
  return (cast<as(unsigned long)>(p[0]) << 24) |
      (cast<as(unsigned long)>(p[1]) << 16) |
      (cast<as(unsigned long)>(p[2]) << 8) | p[3];
}

static i64 ztime(unsigned long i) {
  const unsigned char *p = ztrans + i * ztsize;
  if (ztsize == 4)
    return cast<bits(int)>(be32(p)); /* 32 bits, signed */
  return cast<bits(i64)>((cast<as(unsigned long long)>(be32(p)) << 32) |
                         be32(p + 4));
}

static void ztype(unsigned long type, out struct zone_info *z) {
  const unsigned char *p = ztypes + type * 6;
  z->off = cast<bits(int)>(be32(p));
  z->isdst = p[4] != 0;
  z->abbr = zabbr[type];
}

/* The last transition at or before `t`, or -1 before the first (or none). */
static long zfind(i64 t) {
  if (ztimecnt == 0 || t < ztime(0))
    return -1;
  unsigned long lo = 0, hi = ztimecnt - 1;
  while (lo < hi) {
    unsigned long mid = lo + (hi - lo + 1) / 2;
    if (ztime(mid) <= t)
      lo = mid;
    else
      hi = mid - 1;
  }
  return cast<int(long)>(lo);
}

/* The zone's rules govern `t`: a POSIX zone, or a file past its last
   transition with a footer. */
static int zone_rules_at(i64 t) {
  if (zone_mode == ZONE_POSIX)
    return 1;
  return zone_mode == ZONE_FILE && zone_has_rules &&
      (ztimecnt == 0 || t >= ztime(ztimecnt - 1));
}

static void zone_at(i64 t, out struct zone_info *z) {
  if (zone_rules_at(t)) {
    posix_at(&zone_posix, t, z);
    return;
  }
  if (zone_mode == ZONE_UTC) {
    z->off = 0;
    z->isdst = 0;
    z->abbr = utc_name;
    return;
  }
  /* RFC 8536 3.2: type 0 before the first transition, the last
     transition's type after it when there is no footer. */
  long i = zfind(t);
  ztype(i < 0 ? 0 : zidx[i], z);
}

/* Offsets with isdst `want` in force around `t`, nearest first: the rules'
   where they govern `t`, and those of the transitions on either side. For
   mktime, which tries each and falls back on the first. */
static int zone_offsets_near(i64 t, int want, out long *out, int max) {
  int n = 0;
  *out = 0;
  if (zone_mode == ZONE_UTC)
    return 0;
  if (zone_rules_at(t)) {
    if (!want)
      out[n++] = zone_posix.std_off;
    else if (zone_posix.has_dst)
      out[n++] = zone_posix.dst_off;
  }
  if (zone_mode == ZONE_FILE) {
    long i = zfind(t);
    struct zone_info o = {};
    for (long k = 0; n < max && k < cast<int(long)>(ztimecnt); k++) {
      long cand[2] = {i - k, i + k + 1};
      for (int c = 0; c < 2 && n < max; c++) {
        if (cand[c] < -1 || cand[c] >= cast<int(long)>(ztimecnt))
          continue;
        ztype(cand[c] < 0 ? 0 : zidx[cand[c]], &o);
        if (o.isdst == want)
          out[n++] = o.off;
      }
      /* Near enough: two of each side is past any single overlap. */
      if (k >= 2 && n)
        break;
    }
  }
  return n;
}

#ifndef YASLIBC_NOOS
/* The TZif file zfile holds (len bytes): check it and point the z* views
   into it. */
static int zparse(size_t len) {
  if (!zfile)
    return 0;
  const unsigned char *buf = zfile;
  if (len < 44 || memcmp(buf, "TZif", 4))
    return 0;
  int version = buf[4];
  const unsigned char *h = buf;
  int tsize = 4;
  for (;;) {
    unsigned long isutcnt = be32(h + 20), isstdcnt = be32(h + 24);
    unsigned long leapcnt = be32(h + 28), timecnt = be32(h + 32);
    unsigned long typecnt = be32(h + 36), charcnt = be32(h + 40);
    if (typecnt == 0 || typecnt > 256 || charcnt == 0 || timecnt > 65536 ||
        charcnt > 65536 || leapcnt > 65536 || isutcnt > typecnt ||
        isstdcnt > typecnt)
      return 0;
    size_t body = timecnt * tsize + timecnt + typecnt * 6 + charcnt +
        leapcnt * (tsize + 4) + isstdcnt + isutcnt;
    const unsigned char *data = h + 44;
    if (cast<int(size_t)>(data - buf) + body > len)
      return 0;
    if (tsize == 4 && version >= '2') {
      /* Skip the 32-bit body to the 64-bit header. */
      h = data + body;
      if (cast<int(size_t)>(h - buf) + 44 > len || memcmp(h, "TZif", 4))
        return 0;
      tsize = 8;
      continue;
    }
    ztrans = data;
    zidx = ztrans + timecnt * tsize;
    ztypes = zidx + timecnt;
    const char *abbrs = cast<ptr(const char *)>(ztypes + typecnt * 6);
    if (abbrs[charcnt - 1] != '\0')
      return 0;
    for (unsigned long i = 0; i < timecnt; i++)
      if (zidx[i] >= typecnt)
        return 0;
    zabbr = calloc(typecnt, sizeof(*zabbr));
    if (!zabbr)
      return 0;
    for (unsigned long i = 0; i < typecnt; i++) {
      unsigned long ai = ztypes[i * 6 + 5];
      if (ai >= charcnt) {
        free(move(zabbr));
        zabbr = nullptr;
        return 0;
      }
      zabbr[i] = intern(abbrs + ai);
    }
    ztimecnt = timecnt;
    ztypecnt = typecnt;
    ztsize = tsize;
    zone_has_rules = 0;
    /* The footer: "\n<POSIX TZ>\n", possibly empty. */
    const unsigned char *f = data + body;
    if (tsize == 8 && f < buf + len && *f == '\n') {
      const unsigned char *?e = memchr(f + 1, '\n', buf + len - (f + 1));
      if (e && e - f - 1 < 64) {
        char rules[64] = undefined;
        memcpy(rules, f + 1, cast<int(size_t)>(e - f - 1));
        rules[e - f - 1] = '\0';
        zone_has_rules = rules[0] && parse_posix(rules, &zone_posix);
      }
    }
    return 1;
  }
}

static unsigned char *?owned(free) read_file(char *path, out size_t *len) {
  *len = 0;
  int ?owned(close) fd = open(path, O_RDONLY);
  if (fd < 0)
    return nullptr;
  size_t cap = 2048, n = 0;
  unsigned char *?owned(free) buf = malloc(cap);
  while (buf) {
    if (n == cap) {
      if (cap >= 65536) {
        free(buf);
        buf = nullptr;
        break;
      }
      /* null: buf is still ours */
      unsigned char *?owned(free) grown = reallocarray(move(buf), cap, 2);
      if (!grown) {
        free(buf);
        buf = nullptr;
        break;
      }
      buf = move(grown);
      cap *= 2;
    }
    ssize_t r = read(fd, buf + n, cap - n);
    if (r < 0) {
      free(buf);
      buf = nullptr;
    } else if (r == 0) {
      break;
    } else {
      n += cast<int(size_t)>(r);
    }
  }
  close(fd);
  *len = n;
  return buf;
}

static int load_path(char *path) {
  size_t len = 0;
  unsigned char *?owned(free) buf = read_file(path, &len);
  if (!buf)
    return 0;
  zfile_drop();
  zfile = move(buf);
  /* a new zfile: its views wait at zempty until zparse points them into it */
  ztrans = zempty;
  zidx = zempty;
  ztypes = zempty;
  if (!zparse(len)) {
    zfile_drop();
    return 0;
  }
  return 1;
}

/* `name` as an IANA zone: an absolute path, or relative to $TZDIR, else the
   system's zoneinfo. No ".." anywhere: a zone name is not a way out. */
static int load_zone_file(char *name) {
  char path[256] = {};
  if (!*name || strstr(name, ".."))
    return 0;
  if (name[0] == '/')
    return load_path(name);
  const char *?dirs[2] = {getenv("TZDIR"), "/usr/share/zoneinfo"};
  for (int i = 0; i < 2; i++) {
    if (!dirs[i] || !*dirs[i])
      continue;
    if (snprintf(path, sizeof(path), "%s/%s", dirs[i], name) >=
        cast<int(int)>(sizeof(path)))
      continue;
    if (load_path(path))
      return 1;
  }
  return 0;
}
#endif

static void zone_reset(void) {
  zfile_drop();
  free(move(zabbr));
  zabbr = nullptr;
  ztimecnt = ztypecnt = 0;
  zone_has_rules = 0;
  zone_mode = ZONE_UTC;
}

/* The globals POSIX promises after tzset: standard and daylight names and the
   standard offset (seconds WEST, unlike everything above). As glibc sets
   them: the rules for the present where there are some; daylight is set and
   tzname[1] named for a zone that ever had daylight time (Asia/Kolkata in
   1942) even when its rules have none now. */
static void zone_globals(void) {
  tzname[0] = tzname[1] = utc_name;
  timezone = 0;
  daylight = 0;
  if (zone_mode == ZONE_POSIX || (zone_mode == ZONE_FILE && zone_has_rules)) {
    posix_std_abbr = intern(zone_posix.std_name);
    posix_dst_abbr =
        zone_posix.has_dst ? intern(zone_posix.dst_name) : posix_std_abbr;
  }
  if (zone_mode == ZONE_FILE) {
    struct zone_info z = {};
    int have_std = 0;
    ztype(0, &z);
    tzname[0] = tzname[1] = z.abbr;
    timezone = -z.off;
    for (unsigned long i = ztimecnt; i-- > 0;) {
      ztype(zidx[i], &z);
      if (z.isdst && !daylight) {
        tzname[1] = z.abbr;
        daylight = 1;
      } else if (!z.isdst && !have_std) {
        tzname[0] = z.abbr;
        timezone = -z.off;
        have_std = 1;
      }
    }
    if (!daylight)
      tzname[1] = tzname[0];
  }
  if (zone_mode == ZONE_POSIX || (zone_mode == ZONE_FILE && zone_has_rules)) {
    tzname[0] = posix_std_abbr;
    timezone = -zone_posix.std_off;
    if (zone_posix.has_dst) {
      tzname[1] = posix_dst_abbr;
      daylight = 1;
    } else if (!daylight) {
      tzname[1] = posix_std_abbr;
    }
  }
}

void tzset(void) {
  const char *?tz = getenv("TZ");
  if (zone_loaded && (tz ? (!zone_tz_unset && zone_tz && !strcmp(tz, borrow(zone_tz)))
                         : zone_tz_unset))
    return;
  zone_reset();
  free(zone_tz);
  zone_tz = tz ? strdup(tz) : nullptr;
  zone_tz_unset = !tz;
  zone_loaded = 1;
  if (!tz) {
#ifndef YASLIBC_NOOS
    if (load_path("/etc/localtime"))
      zone_mode = ZONE_FILE;
#endif
  } else if (*tz) {
    const char *name = *tz == ':' ? tz + 1 : tz;
#ifndef YASLIBC_NOOS
    /* A file first, as glibc does: EST5EDT is both a valid POSIX string and a
       zone with real history. A POSIX string with rules has a comma, which no
       zone name does. */
    if (!strchr(name, ',') && load_zone_file(name))
      zone_mode = ZONE_FILE;
    else
#endif
        if (parse_posix(name, &zone_posix))
      zone_mode = ZONE_POSIX;
  }
  zone_globals();
}

/* --- public conversions ----------------------------------------------------
 */

struct tm *?borrows(tm) gmtime_r(time_t *t, out struct tm *tm) {
  if (!secs_to_tm(*t, tm)) {
    errno = EOVERFLOW;
    return nullptr;
  }
  tm->tm_isdst = 0;
  tm->tm_gmtoff = 0;
  tm->tm_zone = utc_name;
  return tm;
}

struct tm *?borrows(static) gmtime(time_t *t) {
  static struct tm tm;
  return gmtime_r(t, &tm);
}

struct tm *?borrows(tm) localtime_r(time_t *t, out struct tm *tm) {
  struct zone_info z = {};
  tzset();
  zone_at(*t, &z);
  if (!secs_to_tm(cast<int(i64)>(*t) + z.off, tm)) {
    errno = EOVERFLOW;
    return nullptr;
  }
  tm->tm_isdst = z.isdst;
  tm->tm_gmtoff = z.off;
  tm->tm_zone = z.abbr;
  return tm;
}

struct tm *?borrows(static) localtime(time_t *t) {
  static struct tm tm;
  return localtime_r(t, &tm);
}

static int fits_time_t(i64 t) {
  if (cast<trunc(time_t)>(t) == t)
    return 1;
  errno = EOVERFLOW;
  return 0;
}

time_t timegm(inout struct tm *tm) {
  i64 t = tm_to_secs(tm);
  if (!fits_time_t(t))
    return cast<int(time_t)>(-1);
  time_t r = cast<int(time_t)>(t);
  gmtime_r(&r, tm);
  return r;
}

time_t mktime(inout struct tm *tm) {
  struct zone_info used = {}, in_force = {};
  tzset();
  i64 local = tm_to_secs(tm);
  /* Guess the offset at `local` read as UTC, then settle it at the instant
     that guess gives. Where the two disagree the guess straddled a
     transition, and the second offset is the one in force before it. In a
     gap (a spring-forward hour that never happens) that reads the time with
     the offset from before the jump, so 02:30 becomes 03:30 daylight time,
     as glibc and musl have it. */
  zone_at(local, &used);
  i64 t = local - used.off;
  zone_at(t, &in_force);
  if (in_force.off != used.off) {
    used = in_force;
    t = local - used.off;
    zone_at(t, &in_force);
  }
  /* A caller that says which of standard or daylight time it means gets the
     time read with that offset, as glibc does: 01:30 on the fall-back night
     is two instants and tm_isdst picks one; 02:30 "daylight" on the
     spring-forward night is 01:30 standard time; noon "daylight" in January
     is 11:00. */
  if (tm->tm_isdst >= 0) {
    int want = tm->tm_isdst > 0;
    if (used.isdst != want) {
      if (in_force.isdst == want) {
        t = local - in_force.off;
      } else {
        long offs[8] = {};
        int n = zone_offsets_near(t, want, offs, 8), found = 0;
        for (int i = 0; i < n && !found; i++) {
          struct zone_info z = {};
          zone_at(local - offs[i], &z);
          if (z.off == offs[i] && z.isdst == want) {
            t = local - offs[i];
            found = 1;
          }
        }
        if (!found && n)
          t = local - offs[0];
      }
    }
  }
  if (!fits_time_t(t))
    return cast<int(time_t)>(-1);
  time_t r = cast<int(time_t)>(t);
  if (!localtime_r(&r, tm))
    return cast<int(time_t)>(-1);
  return r;
}

char *borrows(buf) asctime_r(struct tm *tm, out char *buf) {
  static const char wdays[] = "SunMonTueWedThuFriSat";
  static const char mons[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int wd = tm->tm_wday >= 0 && tm->tm_wday < 7 ? tm->tm_wday : 0;
  int mo = tm->tm_mon >= 0 && tm->tm_mon < 12 ? tm->tm_mon : 0;
  /* POSIX sizes the buffer at 26 bytes, enough for a four-digit year; a wider
     one is cut there rather than written past it. */
  char line[80] = {};
  snprintf(line, sizeof(line), "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
           wdays + wd * 3, mons + mo * 3, tm->tm_mday, tm->tm_hour, tm->tm_min,
           tm->tm_sec, 1900 + tm->tm_year);
  memcpy(buf, line, 25);
  buf[25] = '\0';
  return buf;
}

char *borrows(static) asctime(struct tm *tm) {
  static char buf[26];
  return asctime_r(tm, buf);
}

char *?borrows(buf) ctime_r(time_t *t, out char *buf) {
  struct tm tm = {};
  *buf = '\0';
  if (!localtime_r(t, &tm))
    return nullptr;
  return asctime_r(&tm, buf);
}

char *?borrows(static) ctime(time_t *t) {
  static char buf[26];
  return ctime_r(t, buf);
}

/* Time zones: localtime_r, mktime, timegm, tzset and strftime's %z/%Z,
 * checked against the host's glibc on the same TZ values. The IANA cases read
 * the host's /usr/share/zoneinfo through the SUT's open/read (served from the
 * host below) and are skipped where the host lacks a zone. */

#define _GNU_SOURCE /* host mremap, timegm, tm_gmtoff */
#include "utest.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "syscalls_stub.h"

extern char **sut_environ;
extern long sut_timezone;
extern int sut_daylight;
extern char *sut_tzname[2];
struct tm *sut_localtime_r(const time_t *t, struct tm *tm);
struct tm *sut_gmtime_r(const time_t *t, struct tm *tm);
time_t sut_mktime(struct tm *tm);
time_t sut_timegm(struct tm *tm);
void sut_tzset(void);
long sut_strftime(char *s, size_t len, const char *fmt, struct tm *tm);
char *sut_asctime_r(const struct tm *tm, char *buf);

/* What the SUT opens as /etc/localtime: NULL makes it missing. */
static const char *etc_localtime;
static const char *sut_tzdir;
/* Files the SUT opened under sut_tzdir. */
static int tzdir_opens;

static void host_syscalls(int number, const void *args, syscall_result *result) {
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
  } else if (number == sys_open) {
    const open_context *ctx = args;
    const char *path = ctx->path;
    if (!strcmp(path, "/etc/localtime"))
      path = etc_localtime ? etc_localtime : "/nonexistent/localtime";
    int fd = open(path, O_RDONLY);
    if (fd >= 0 && sut_tzdir && !strncmp(path, sut_tzdir, strlen(sut_tzdir)))
      tzdir_opens++;
    result->result = fd;
    if (fd < 0)
      result->err = errno;
  } else if (number == sys_read) {
    const read_context *ctx = args;
    ssize_t r = read(ctx->fd, ctx->buf, ctx->count);
    if (r < 0) {
      result->result = -1;
      result->err = errno;
    } else {
      *ctx->result = r;
    }
  } else if (number == sys_close) {
    close(*(const int *)args);
  }
}

static char tz_entry[128];
static char tzdir_entry[160];
static char *sut_env[3]; /* TZ, TZDIR (sut_tzdir: NULL for the default) */

/* TZ for both libcs; NULL unsets it. */
static void use_tz(const char *tz) {
  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = host_syscalls;
  if (tz) {
    snprintf(tz_entry, sizeof(tz_entry), "TZ=%s", tz);
    sut_env[0] = tz_entry;
    sut_env[1] = NULL;
    setenv("TZ", tz, 1);
  } else {
    sut_env[0] = NULL;
    unsetenv("TZ");
  }
  if (sut_tzdir) {
    snprintf(tzdir_entry, sizeof(tzdir_entry), "TZDIR=%s", sut_tzdir);
    sut_env[tz ? 1 : 0] = tzdir_entry;
    sut_env[tz ? 2 : 1] = NULL;
  }
  sut_environ = sut_env;
  tzset();
  sut_tzset();
}

static int have_zone(const char *name) {
  char path[256];
  snprintf(path, sizeof(path), "/usr/share/zoneinfo/%s", name);
  return access(path, R_OK) == 0;
}

static int same_tm(const struct tm *a, const struct tm *b) {
  return a->tm_sec == b->tm_sec && a->tm_min == b->tm_min && a->tm_hour == b->tm_hour &&
         a->tm_mday == b->tm_mday && a->tm_mon == b->tm_mon && a->tm_year == b->tm_year &&
         a->tm_wday == b->tm_wday && a->tm_yday == b->tm_yday && a->tm_isdst == b->tm_isdst &&
         a->tm_gmtoff == b->tm_gmtoff && a->tm_zone && b->tm_zone &&
         !strcmp(a->tm_zone, b->tm_zone);
}

static int same_wall(const struct tm *a, const struct tm *b) {
  return a->tm_sec == b->tm_sec && a->tm_min == b->tm_min && a->tm_hour == b->tm_hour &&
         a->tm_mday == b->tm_mday && a->tm_mon == b->tm_mon && a->tm_year == b->tm_year;
}

static void print_tm(const char *who, const struct tm *t) {
  fprintf(stderr, "  %s: %04d-%02d-%02d %02d:%02d:%02d wday %d yday %d isdst %d gmtoff %ld %s\n",
          who, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
          t->tm_wday, t->tm_yday, t->tm_isdst, t->tm_gmtoff, t->tm_zone ? t->tm_zone : "(null)");
}

/* localtime_r and mktime from `from` to 2100 against glibc. Returns the number
   of mismatches (reporting the first few). */
static int compare_zone(const char *tz, time_t from) {
  int bad = 0;
  use_tz(tz);
  if (sut_timezone != timezone || sut_daylight != daylight || strcmp(sut_tzname[0], tzname[0]) ||
      strcmp(sut_tzname[1], tzname[1])) {
    fprintf(stderr, "%s: tzset: sut timezone %ld daylight %d %s/%s, glibc %ld %d %s/%s\n", tz,
            sut_timezone, sut_daylight, sut_tzname[0], sut_tzname[1], timezone, daylight,
            tzname[0], tzname[1]);
    bad++;
  }
  /* An odd step so samples drift through every hour and minute. */
  for (time_t t = from; t < 4102444800L && bad < 5; t += 3 * 3600 + 17 * 60 + 13) {
    struct tm want, got;
    localtime_r(&t, &want);
    if (!sut_localtime_r(&t, &got) || !same_tm(&want, &got)) {
      fprintf(stderr, "%s: localtime_r(%ld)\n", tz, (long)t);
      print_tm("glibc", &want);
      print_tm("sut  ", &got);
      bad++;
      continue;
    }
    /* Round trip with tm_isdst known, and glibc's answer when it is not. */
    struct tm a = want, b = want;
    time_t back = sut_mktime(&a);
    /* Two offsets of the same flavour can overlap too (Morocco leaving +01
       for +00 standard time): no tm_isdst tells those apart. */
    if (back != t && !(same_wall(&a, &want) && a.tm_isdst == want.tm_isdst)) {
      fprintf(stderr, "%s: mktime(localtime(%ld)) = %ld\n", tz, (long)t, (long)back);
      bad++;
    }
    a = want;
    a.tm_isdst = -1;
    b.tm_isdst = -1;
    time_t wantm = mktime(&b), gotm = sut_mktime(&a);
    /* The fall-back hour happens twice, and with tm_isdst -1 POSIX lets
       mktime pick either; glibc's pick depends on its previous call. Either
       instant that reads back as the same wall clock time is right. */
    if (wantm != gotm && same_wall(&a, &want) && same_wall(&b, &want))
      continue;
    if (wantm != gotm || !same_tm(&a, &b)) {
      fprintf(stderr, "%s: mktime(isdst -1) of localtime(%ld): sut %ld glibc %ld\n", tz,
              (long)t, (long)gotm, (long)wantm);
      print_tm("glibc", &b);
      print_tm("sut  ", &a);
      bad++;
    }
  }
  return bad;
}

static const char *const posix_zones[] = {
    "CET-1CEST,M3.5.0,M10.5.0/3",   /* Europe */
    "EST5EDT4,M3.2.0/2,M11.1.0/2", /* US, explicit */
    "AEST-10AEDT,M10.1.0,M4.1.0/3", /* southern hemisphere: wraps the year */
    "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0", /* Lord Howe: half-hour DST */
    "IST-5:30",                     /* no DST, odd offset */
    "<-03>3",                       /* quoted numeric name */
    "XXX3YYY,J60/1:30,300/-1",      /* Jn and n rules, odd and negative times */
    "WGT3WGST,M3.5.0/-2,M10.5.0/-1", /* negative rule times (America/Nuuk) */
};

/* From 1970: before it glibc applies no POSIX rules at all (compute_change
   clamps the year to 1970), where this libc applies them proleptically, as
   musl does. */
UTEST(tz, posix_strings_match_glibc) {
  for (size_t i = 0; i < sizeof(posix_zones) / sizeof(posix_zones[0]); i++)
    EXPECT_EQ(0, compare_zone(posix_zones[i], 0));
}

static const char *const iana_zones[] = {
    "UTC",
    "Europe/Warsaw",
    "America/New_York",
    "America/Sao_Paulo",
    "Australia/Lord_Howe",
    "Asia/Kolkata",
    "Pacific/Apia",
    "Pacific/Chatham",
    "Africa/Casablanca",
    "Europe/Dublin",
    "America/St_Johns",
    "Antarctica/Troll",
};

UTEST(tz, iana_files_match_glibc) {
  int ran = 0;
  for (size_t i = 0; i < sizeof(iana_zones) / sizeof(iana_zones[0]); i++) {
    if (!have_zone(iana_zones[i]))
      continue;
    ran++;
    EXPECT_EQ(0, compare_zone(iana_zones[i], -2147483647L));
  }
  if (!ran)
    fprintf(stderr, "tz: no /usr/share/zoneinfo on this host; IANA cases skipped\n");
}

/* What the image ships: slim files (zic -b slim, as build_rootfs.sh makes
   them) have no 32-bit data and leave more to the footer's rules. Built from
   the host's own tzdata.zi, so the same rules glibc reads in the fat files. */
UTEST(tz, slim_files_match_glibc) {
  if (access("/usr/share/zoneinfo/tzdata.zi", R_OK) ||
      system("zic -b slim -d build/zoneinfo-slim /usr/share/zoneinfo/tzdata.zi 2>/dev/null")) {
    fprintf(stderr, "tz: no zic or tzdata.zi on this host; slim cases skipped\n");
    return;
  }
  sut_tzdir = "build/zoneinfo-slim";
  tzdir_opens = 0;
  for (size_t i = 0; i < sizeof(iana_zones) / sizeof(iana_zones[0]); i++)
    if (have_zone(iana_zones[i]))
      EXPECT_EQ(0, compare_zone(iana_zones[i], -2147483647L));
  sut_tzdir = NULL;
  /* Not the host's fat files by way of the fallback directory. */
  EXPECT_GT(tzdir_opens, 0);
}

UTEST(tz, absolute_path_and_colon) {
  if (!have_zone("Europe/Warsaw"))
    return;
  time_t t = 1751371200; /* 2025-07-01 12:00 UTC */
  struct tm got;
  use_tz(":/usr/share/zoneinfo/Europe/Warsaw");
  ASSERT_TRUE(sut_localtime_r(&t, &got) != NULL);
  EXPECT_EQ(14, got.tm_hour);
  EXPECT_STREQ("CEST", got.tm_zone);
  use_tz(":Europe/Warsaw");
  ASSERT_TRUE(sut_localtime_r(&t, &got) != NULL);
  EXPECT_EQ(14, got.tm_hour);
}

UTEST(tz, unset_reads_etc_localtime_else_utc) {
  time_t t = 1751371200;
  struct tm got;
  etc_localtime = NULL;
  use_tz(NULL);
  ASSERT_TRUE(sut_localtime_r(&t, &got) != NULL);
  EXPECT_EQ(12, got.tm_hour);
  EXPECT_STREQ("UTC", got.tm_zone);
  EXPECT_EQ(0, sut_timezone);
  if (have_zone("America/New_York")) {
    etc_localtime = "/usr/share/zoneinfo/America/New_York";
    /* The unset TZ is cached; a set one in between makes it load again. */
    use_tz("UTC0");
    use_tz(NULL);
    ASSERT_TRUE(sut_localtime_r(&t, &got) != NULL);
    EXPECT_EQ(8, got.tm_hour);
    EXPECT_STREQ("EDT", got.tm_zone);
    EXPECT_EQ(5 * 3600, sut_timezone);
    etc_localtime = NULL;
  }
}

UTEST(tz, bad_values_are_utc) {
  static const char *const bad[] = {"", "Nowhere/Atlantis", "../../etc/passwd", "AB3",
                                    "CET-1CEST,M13.1.0,M10.5.0", "CET-1CEST,M3.5.0", "Abc/Def"};
  time_t t = 1751371200;
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    struct tm got;
    use_tz(bad[i]);
    ASSERT_TRUE(sut_localtime_r(&t, &got) != NULL);
    EXPECT_EQ(12, got.tm_hour);
    EXPECT_EQ(0, got.tm_gmtoff);
    EXPECT_EQ(0, sut_daylight);
  }
}

UTEST(tz, strftime_offset_and_name) {
  time_t t = 1751371200;
  struct tm tm;
  char buf[64];
  use_tz("<+0530>-5:30");
  sut_localtime_r(&t, &tm);
  sut_strftime(buf, sizeof(buf), "%H:%M %z %Z", &tm);
  EXPECT_STREQ("17:30 +0530 +0530", buf);
  use_tz("NST3:30NDT,M3.2.0,M11.1.0");
  sut_localtime_r(&t, &tm);
  sut_strftime(buf, sizeof(buf), "%H:%M %z %Z", &tm);
  EXPECT_STREQ("09:30 -0230 NDT", buf);
  /* A hand-made struct tm has no tm_zone: the zone's name for its isdst. */
  memset(&tm, 0, sizeof(tm));
  tm.tm_isdst = 0;
  sut_strftime(buf, sizeof(buf), "%Z", &tm);
  EXPECT_STREQ("NST", buf);
}

UTEST(tz, timegm_normalises_like_glibc) {
  static const int cases[][6] = {
      /* year-1900, mon, mday, hour, min, sec */
      {125, 0, 1, 0, 0, 0},     {124, 1, 29, 23, 59, 60}, {125, 13, 35, 25, 61, -1},
      {125, -1, 0, 0, 0, 0},    {69, 11, 31, 23, 59, 59}, {1, 0, 1, 0, 0, 0},
      {200, 1, 29, 12, 0, 0},   {125, 2, -40, -5, 0, 0},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    struct tm a = {0}, b;
    a.tm_year = cases[i][0];
    a.tm_mon = cases[i][1];
    a.tm_mday = cases[i][2];
    a.tm_hour = cases[i][3];
    a.tm_min = cases[i][4];
    a.tm_sec = cases[i][5];
    b = a;
    time_t want = timegm(&b), got = sut_timegm(&a);
    EXPECT_EQ(want, got);
    /* glibc calls UTC "GMT" here. */
    b.tm_zone = a.tm_zone;
    EXPECT_TRUE(same_tm(&a, &b));
  }
}

UTEST(tz, asctime_format) {
  time_t t = 1751371200;
  struct tm tm;
  char buf[26];
  use_tz("UTC0");
  sut_gmtime_r(&t, &tm);
  EXPECT_STREQ("Tue Jul  1 12:00:00 2025\n", sut_asctime_r(&tm, buf));
}

/* Times that never happen and times that happen twice. */
static int check_mktime(const char *tz, int hour, int min, int isdst, int mday, int mon,
                         const char *want) {
  struct tm tm = {0};
  char buf[64];
  use_tz(tz);
  tm.tm_year = 125;
  tm.tm_mon = mon;
  tm.tm_mday = mday;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_isdst = isdst;
  sut_mktime(&tm);
  sut_strftime(buf, sizeof(buf), "%F %T %z %Z", &tm);
  if (!strcmp(buf, want))
    return 0;
  fprintf(stderr, "%s %02d:%02d isdst %d: got %s, want %s\n", tz, hour, min, isdst, buf, want);
  return 1;
}

UTEST(tz, mktime_gap_and_overlap) {
  const char *const zones[] = {"CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Warsaw"};
  for (int i = 0; i < 2; i++) {
    if (i && !have_zone("Europe/Warsaw"))
      break;
    /* 2025-03-30 02:30 does not exist: read with the offset before the jump. */
    EXPECT_EQ(0, check_mktime(zones[i], 2, 30, -1, 30, 2, "2025-03-30 03:30:00 +0200 CEST"));
    /* ... and as daylight time it is half an hour before the jump. */
    EXPECT_EQ(0, check_mktime(zones[i], 2, 30, 1, 30, 2, "2025-03-30 01:30:00 +0100 CET"));
    /* 2025-10-26 02:30 happens twice: tm_isdst says which. */
    EXPECT_EQ(0, check_mktime(zones[i], 2, 30, 1, 26, 9, "2025-10-26 02:30:00 +0200 CEST"));
    EXPECT_EQ(0, check_mktime(zones[i], 2, 30, 0, 26, 9, "2025-10-26 02:30:00 +0100 CET"));
    /* tm_isdst 1 in winter: an hour earlier on the clock, as glibc does. */
    EXPECT_EQ(0, check_mktime(zones[i], 12, 0, 1, 15, 0, "2025-01-15 11:00:00 +0100 CET"));
  }
}

/* Written by `tcc -yasc-c-header` from yasc/time.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "yasc/time.h"
#else
#pragma once

#include <sys/time.h>
#include <sys/types.h>
#include <sys/_attrs.h>

int nanosleep(struct timespec *req, struct timespec *rem) __yl_nonnull(1);

/* Processor time. There is no per-process CPU clock, so clock() measures
 * wall time since its first call; only differences are meaningful anyway. */
typedef long clock_t;
#define CLOCKS_PER_SEC 1000000L
clock_t clock(void);

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  long tm_gmtoff;
  char *tm_zone;
};


time_t time(time_t *timep);
long strftime(char *s, size_t len, const char *fmt, struct tm *tm) __yl_nonnull(3, 4);
struct tm *localtime(const time_t *timep) __yl_nonnull(1);
struct tm *localtime_r(const time_t *timer, struct tm *result) __yl_nonnull(1, 2);
struct tm *gmtime(const time_t *timep) __yl_nonnull(1);
struct tm *gmtime_r(const time_t *timer, struct tm *result) __yl_nonnull(1, 2);

/* Time zones: $TZ (an IANA name or a POSIX TZ string), else /etc/localtime,
 * else UTC. tzset() sets these; localtime() and mktime() call it. */
extern long timezone; /* standard time, seconds WEST of UTC */
extern int daylight;  /* the zone has daylight saving time */
extern char *tzname[2];
void tzset(void);

time_t mktime(struct tm *tm) __yl_nonnull(1);
time_t timegm(struct tm *tm) __yl_nonnull(1);
char *asctime(const struct tm *tm) __yl_nonnull(1);
char *asctime_r(const struct tm *tm, char *buf) __yl_nonnull(1, 2);
char *ctime(const time_t *timep) __yl_nonnull(1);
char *ctime_r(const time_t *timep, char *buf) __yl_nonnull(1, 2);
/* past the parsed text in S, or null when it does not match FMT */
char *strptime(const char *s, const char *fmt, struct tm *tm) __yl_nonnull(1, 2, 3);
#endif /* __YASC__ */

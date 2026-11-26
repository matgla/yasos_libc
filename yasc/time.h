#pragma yasc
#pragma tcc c_begin
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
#pragma tcc c_end
struct tm * borrows(static) maybe localtime(const time_t * view timep) __yl_nonnull(1);
struct tm * borrows(result) maybe localtime_r(const time_t * view timer, out struct tm * view result) __yl_nonnull(1, 2);
struct tm * borrows(static) maybe gmtime(const time_t * view timep) __yl_nonnull(1);
struct tm * borrows(result) maybe gmtime_r(const time_t * view timer, out struct tm * view result) __yl_nonnull(1, 2);
#pragma tcc c_begin

/* Time zones: $TZ (an IANA name or a POSIX TZ string), else /etc/localtime,
 * else UTC. tzset() sets these; localtime() and mktime() call it. */
extern long timezone; /* standard time, seconds WEST of UTC */
extern int daylight;  /* the zone has daylight saving time */
extern char *tzname[2];
void tzset(void);

time_t mktime(struct tm *tm) __yl_nonnull(1);
time_t timegm(struct tm *tm) __yl_nonnull(1);
#pragma tcc c_end
char * borrows(static) asctime(const struct tm * view tm) __yl_nonnull(1);
char * borrows(buf) asctime_r(const struct tm * view tm, out char * view buf) __yl_nonnull(1, 2);
char * borrows(static) maybe ctime(const time_t * view timep) __yl_nonnull(1);
char * borrows(buf) maybe ctime_r(const time_t * view timep, out char * view buf) __yl_nonnull(1, 2);
/* past the parsed text in S, or null when it does not match FMT */
char * borrows(s) maybe strptime(const char * view s, const char * view fmt, inout struct tm * view tm) __yl_nonnull(1, 2, 3);

#pragma yasc
#pragma once

#include <sys/time.h>
#include <sys/types.h>
#include <sys/_attrs.h>

int nanosleep(struct timespec *req, out struct timespec *?rem);

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
  const char *tm_zone; /* the library's: copy it to keep it */
};

time_t time(out time_t *?timep);
/* the length written, 0 when it (with its NUL) does not fit in LEN */
long strftime(out char s[len], size_t len, char *fmt, struct tm *tm);
struct tm *?borrows(static) localtime(time_t *timep);
struct tm *?borrows(result) localtime_r(time_t *timer, out struct tm *result);
struct tm *?borrows(static) gmtime(time_t *timep);
struct tm *?borrows(result) gmtime_r(time_t *timer, out struct tm *result);

/* Time zones: $TZ (an IANA name or a POSIX TZ string), else /etc/localtime,
 * else UTC. tzset() sets these; localtime() and mktime() call it. */
extern long timezone; /* standard time, seconds WEST of UTC */
extern int daylight;  /* the zone has daylight saving time */
extern const char *c_api(char *) tzname[2];
void tzset(void);

time_t mktime(inout struct tm *tm);
time_t timegm(inout struct tm *tm);
char *borrows(static) asctime(struct tm *tm);
char *borrows(buf) asctime_r(struct tm *tm, out char *buf);
char *?borrows(static) ctime(time_t *timep);
char *?borrows(buf) ctime_r(time_t *timep, out char *buf);
/* past the parsed text in S, or null when it does not match FMT */
char *?borrows(s) strptime(char *s, char *fmt, inout struct tm *tm);

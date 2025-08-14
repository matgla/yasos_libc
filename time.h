#pragma once

#include <sys/time.h>
#include <sys/types.h>

int nanosleep(struct timespec *req, struct timespec *rem);

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
long strftime(char *s, size_t len, const char *fmt, struct tm *tm);
struct tm *localtime(time_t *timep);
struct tm *localtime_r(const time_t *timer, struct tm *result);

struct tm *gmtime(time_t *timep);

extern long timezone;
void tzset(void);

time_t mktime(struct tm *tm);
char *strptime(const char *s, const char *fmt, struct tm *tm);
#pragma once

#include <sys/types.h>

/* tv_sec is spelled time_t and tv_usec suseconds_t, as POSIX has them, so the
   two can never drift apart from time_t again (sys/types.h says why that
   mattered). Both are `long` today. */
typedef struct timeval {
  time_t tv_sec;
  suseconds_t tv_usec;
} timeval;

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

struct timespec {
  time_t tv_sec;
  long tv_nsec;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv, const struct timezone *tz);
int utimes(const char *path, const struct timeval times[2]);

typedef int clockid_t;
int clock_gettime(clockid_t clockid, struct timespec *res);

#define CLOCK_REALTIME 1
#define CLOCK_MONOTONIC 2

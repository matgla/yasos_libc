#pragma once

typedef struct timeval {
  long tv_sec;
  long tv_usec;
} timeval;

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

typedef struct timespec {
  long tv_sec;
  long tv_nsec;
} timespec;

int gettimeofday(struct timeval *tv, struct timezone *tz);

typedef int clockid_t;
int clock_gettime(clockid_t clockid, struct timespec *res);

#define CLOCK_REALTIME 1
#define CLOCK_MONOTONIC 2

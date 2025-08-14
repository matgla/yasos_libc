#pragma once

#include <sys/time.h>

typedef struct rusage {
  struct timeval ru_utime; /* user time used */
  struct timeval ru_stime; /* system time used */
  long ru_maxrss;          /* maximum resident set size */
  long ru_ixrss;           /* integral shared memory size */
} rusage;

int getrusage(int who, struct rusage *usage);

typedef unsigned long rlim_t;
typedef struct rlimit {
  rlim_t rlim_cur; /* current (soft) limit */
  rlim_t rlim_max; /* hard limit */
} rlimit;

#define RUSAGE_SELF 1
#define RUSAGE_CHILDREN 2

#define RLIMIT_AS 1
#define RLIMIT_CORE 2
#define RLIMIT_CPU 3
#define RLIMIT_DATA 4
#define RLIMIT_FSIZE 5
#define RLIMIT_LOCKS 6
#define RLIMIT_MEMLOCK 7
#define RLIMIT_MSGQUEUE 8
#define RLIMIT_NICE 9
#define RLIMIT_NOFILE 10
#define RLIMIT_NPROC 11
#define RLIMIT_RSS 12
#define RLIMIT_RTPRIO 13
#define RLIMIT_SIGPENDING 14
#define RLIMIT_STACK 15

#define RLIM_INFINITY ((rlim_t) - 1)

#pragma once

#include <sys/time.h>
#include <sys/types.h>

typedef struct rusage {
  struct timeval ru_utime; /* user time used */
  struct timeval ru_stime; /* system time used */
  long ru_maxrss;          /* maximum resident set size */
  long ru_ixrss;           /* integral shared memory size */
} rusage;

int getrusage(int who, struct rusage *usage);

/* 64-bit on every architecture, as musl has it -- NOT `unsigned long`.
 *
 * This is ABI, not taste: a program built against musl for 32-bit ARM passes a
 * 16-byte `struct rlimit`, so a libc with 8-byte limits reads a caller's
 * rlim_cur as (cur | max << 32) and writes back half a struct. The Zig compiler
 * raises its own descriptor limit on startup that way, and with the narrow
 * definition it ended up setting RLIMIT_NOFILE to 0 -- after which every open()
 * failed with EMFILE and the error pointed at whatever file was unlucky enough
 * to be next. */
typedef unsigned long long rlim_t;
typedef struct rlimit {
  rlim_t rlim_cur; /* current (soft) limit */
  rlim_t rlim_max; /* hard limit */
} rlimit;

int getrlimit(int resource, struct rlimit *old_limit);
int setrlimit(int resource, const struct rlimit *new_limit);
int prlimit(pid_t pid, int resource, const struct rlimit *new_limit,
            struct rlimit *old_limit);

#define RUSAGE_SELF 1
#define RUSAGE_CHILDREN 2

#define RLIMIT_CPU 0
#define RLIMIT_FSIZE 1
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#define RLIMIT_CORE 4
#define RLIMIT_RSS 5
#define RLIMIT_NPROC 6
#define RLIMIT_NOFILE 7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS 9
#define RLIMIT_LOCKS 10
#define RLIMIT_SIGPENDING 11
#define RLIMIT_MSGQUEUE 12
#define RLIMIT_NICE 13
#define RLIMIT_RTPRIO 14
#define RLIMIT_RTTIME 15

#define RLIM_NLIMITS 16

#define RLIM_INFINITY ((rlim_t) - 1)

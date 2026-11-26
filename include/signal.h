// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <sys/ucontext.h>

#define NSIG 32

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT 6
#define SIGFPE 8
#define SIGKILL 9
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGUNUSED 31
#define SIGBUS 7
#define SIGUSR1 10
#define SIGUSR2 12
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPWR 30
#define SIGSYS 31

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define FPE_INTDIV 1
#define FPE_FLTDIV 2

#define SIGCLD SIGCHLD
#define SIGPOLL SIGIO

typedef void (*sighandler_t)(int);

#define SIG_ERR ((void (*)(int)) - 1)
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_HOLD ((void (*)(int))2)

sighandler_t signal(int signum, sighandler_t action);
int kill(int pid, int sig);
int raise(int sig);

typedef struct siginfo {
  int si_code;
} siginfo_t;

typedef struct sigaction {
  void (*sa_handler)(int);
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer)(void);
  int sa_sigaction;
} sigaction_t;

int sigsuspend(const sigset_t *mask);
int sigaddset(sigset_t *set, int signum);
int sigemptyset(sigset_t *set);
int sigaction(int sig, const struct sigaction *sa, struct sigaction *old_sa);
int sigfillset(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigpending(sigset_t *set);

typedef char sig_atomic_t;
extern const char *const sys_siglist[];
extern const char *const sys_signame[];

#define SA_NOCLDSTOP 0x0001
#define SA_NOCLDWAIT 0x0002
#define SA_NODEFER 0x0004
#define SA_ONSTACK 0x0008
#define SA_RESETHAND 0x0010
#define SA_RESTART 0x0020
#define SA_RESTORER 0x0040
#define SA_SIGINFO 0x0080
#define SA_UNSUPPORTED 0x0100
#define SA_EXPOSE_TAGBITS 0x0200
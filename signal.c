#include <errno.h>
#include <signal.h>

#include <stddef.h>

#include <stdio.h>

int sigreturn(unsigned long n);

const char *const sys_siglist[] = {
    "Undefined signal 0",
};

const char *const sys_signame[] = {
    "SIGNULL",
};

struct ksa {
  void *handler;
  unsigned long flags;
  int (*restorer)(unsigned long n);
  sigset_t mask;
};

/* YasOS delivers no signals, so the honest answer to "install this handler" is
 * that it is recorded and will never fire -- not a failure. Refusing instead
 * breaks every runtime that arms a handler on startup as a matter of course
 * (Zig's does, and so does anything with a crash reporter), and a stub that
 * returned -1 without setting errno made it worse: the caller then read
 * whatever the previous failing call had left there and reported that. */
int __sigaction(int sig, const struct sigaction *sa, struct sigaction *old) {
  (void)sig;
  (void)sa;
  if (old) {
    struct sigaction none = {0};
    *old = none;
  }
  return 0;
}

int sigaction(int sig, const struct sigaction *sa, struct sigaction *old_sa) {
  return __sigaction(sig, sa, old_sa);
}

sighandler_t signal(int sig, sighandler_t func) {
  struct sigaction sa = {.sa_handler = func, .sa_flags = SA_RESTART};
  if (__sigaction(sig, &sa, &sa) < 0)
    return SIG_ERR;
  return sa.sa_handler;
}

int sigemptyset(sigset_t *set) {
  for (int i = 0; i < sizeof(set->__bits) / sizeof(long); i++)
    set->__bits[i] = 0;
  return 0;
}

int sigaddset(sigset_t *set, int signum) {
  if (signum < 1 || signum > 31)
    return -1;
  set->__bits[signum / (sizeof(long) * 8)] |=
      (1UL << (signum % (sizeof(long) * 8)));
  return 0;
}

/* Nothing is ever delivered, so nothing is ever pending and the mask has no
 * effect: report the empty set and succeed. Callers block signals around a
 * critical section and check the result; failing there aborts work that had
 * nothing to do with signals. */
int sigprocmask(int howm, const sigset_t *set, sigset_t *oldset) {
  (void)howm;
  (void)set;
  if (oldset)
    sigemptyset(oldset);
  return 0;
}

int sigsuspend(const sigset_t *mask) {
  errno = ENOSYS;
  return -1;
}

int sigfillset(sigset_t *set) {
  if (!set) {
    errno = EINVAL;
    return -1;
  }
  for (int i = 0; i < sizeof(set->__bits) / sizeof(long); i++)
    set->__bits[i] = ~0UL;
  return 0;
}
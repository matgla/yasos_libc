#include <errno.h>
#include <signal.h>

#include <stddef.h>

#include <stdio.h>

#include "sys/syscall.h"

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

/* The kernel delivers SIGINT (Ctrl-C on the console) and the signals kill()
 * sends, with their default action -- end the process -- unless the process
 * ignores them.  It runs no handlers: a handler installed here is recorded,
 * never called, and keeps the signal from ending the process, which is what a
 * program that installs one is counting on.  SIG_IGN stays across exec, a
 * handler does not (POSIX).  Refusing instead would break every runtime that
 * arms a handler on startup as a matter of course (Zig's does).
 *
 * The handler itself only libc remembers, for `old`; vfork children share this
 * table with their parent, so what they set shows in the parent's answers. */
static void (*installed[32])(int);

int __sigaction(int sig, const struct sigaction *sa, struct sigaction *old) {
  int previous = 0;
  sigaction_context context = {.sig = sig, .disposition = -1, .old = &previous};
  if (sig < 1 || sig > 31) {
    errno = EINVAL;
    return -1;
  }
  if (sa)
    context.disposition = sa->sa_handler == SIG_DFL ? 0 : sa->sa_handler == SIG_IGN ? 1 : 2;
  if (trigger_syscall(sys_sigaction, &context) < 0)
    return -1;
  if (old) {
    struct sigaction none = {0};
    *old = none;
    old->sa_handler = previous == 1 ? SIG_IGN : previous == 2 && installed[sig] ? installed[sig] : SIG_DFL;
  }
  if (sa)
    installed[sig] = sa->sa_handler;
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
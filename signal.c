#include <signal.h>

#include <stddef.h>

#include <stdio.h>

int sigreturn(unsigned long n);

struct ksa {
  void *handler;
  unsigned long flags;
  int (*restorer)(unsigned long n);
  sigset_t mask;
};

int __sigaction(int sig, const struct sigaction *sa, struct sigaction *old) {
  printf("TODO: Implement __sigaction\n");
  return -1;
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

int sigprocmask(int howm, const sigset_t *set, sigset_t *oldset) {
  printf("TODO: Implement sigprocmask\n");
  return -1;
}

int sigsuspend(const sigset_t *mask) {
  printf("TODO: Implement sigsuspend\n");
  return -1;
}

int sigfillset(sigset_t *set) {
  printf("TODO: Implement sigfillset\n");
  return -1;
}
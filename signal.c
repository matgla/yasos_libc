#include <signal.h>

#include <stddef.h>

#define SA_NOCLDSTOP 1
#define SA_NOCLDWAIT 2
#define SA_SIGINFO 4
#define SA_ONSTACK 0x08000000
#define SA_RESTART 0x10000000
#define SA_NODEFER 0x40000000
#define SA_RESETHAND 0x80000000
#define SA_RESTORER 0x04000000

int sigreturn(unsigned long n);

struct ksa {
  void *handler;
  unsigned long flags;
  int (*restorer)(unsigned long n);
  sigset_t mask;
};

int __sigaction(int sig, const struct sigaction *sa, struct sigaction *old) {
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
  return -1;
}
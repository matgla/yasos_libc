#include <sys/resource.h>

#include <sys/syscall.h>

int prlimit(pid_t pid, int resource, const struct rlimit *new_limit,
            struct rlimit *old_limit) {
  const prlimit_context context = {
      .pid = pid,
      .resource = resource,
      .new_limit = new_limit,
      .old_limit = old_limit,
  };

  return trigger_syscall(sys_prlimit, &context);
}

int getrlimit(int resource, struct rlimit *old_limit) {
  return prlimit(0, resource, NULL, old_limit);
}

int setrlimit(int resource, const struct rlimit *new_limit) {
  return prlimit(0, resource, new_limit, NULL);
}
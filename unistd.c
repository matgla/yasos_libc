// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#include <unistd.h>

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <errno.h>

#include <sys/syscall.h>
#include <sys/wait.h>

#include <stdio.h>
int sleep(int n) {
  struct timespec req = {n, 0};
  struct timespec rem;
  if (nanosleep(&req, &rem))
    return rem.tv_sec;
  return 0;
}

int usleep(int n) {
  struct timespec req = {0, n * 1000};
  struct timespec rem;
  if (nanosleep(&req, &rem))
    return rem.tv_sec;
  return 0;
}

#define EXECARGS (1 << 7)

int execle(char *path, ...) {
  va_list ap;
  char *argv[EXECARGS];
  char **envp;
  int argc = 0;
  va_start(ap, path);
  while (argc + 1 < EXECARGS && (argv[argc] = va_arg(ap, char *)))
    argc++;
  envp = va_arg(ap, char **);
  va_end(ap);
  argv[argc] = NULL;
  execve(path, argv, envp);
  return -1;
}

int execvp(char *cmd, char *argv[]) {
  char path[512];
  char *p = getenv("PATH");
  if (strchr(cmd, '/'))
    return execve(cmd, argv, environ);
  if (!p)
    p = "/bin";
  while (*p) {
    char *s = path;
    while (*p && *p != ':')
      *s++ = *p++;
    if (s != path)
      *s++ = '/';
    strcpy(s, cmd);
    execve(path, argv, environ);
    if (*p == ':')
      p++;
  }
  return -1;
}

int execv(char *path, char *argv[]) {
  return execve(path, argv, environ);
}

int wait(int *status) {
  return waitpid(-1, status, 0);
}

int raise(int sig) {
  return kill(getpid(), sig);
}

pid_t getpid() {
  bool parent = false;
  return trigger_syscall(sys_getpid, &parent);
}

int getppid(void) {
  bool parent = true;
  return trigger_syscall(sys_getpid, &parent);
}

void abort(void) {
  raise(SIGABRT);
  while (1)
    ;
}

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  const char *path = "/usr/bin/sh";
  memcpy(buf, path, strlen(path));
  return strlen(path);
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  printf("TODO: Implement readlink\n");
  return -1; // Not implemented
}

uid_t geteuid(void) {
  return trigger_syscall(sys_geteuid, NULL);
}

uid_t getuid(void) {
  return trigger_syscall(sys_getuid, NULL);
}

int setuid(uid_t uid) {
  printf("TODO: Implement setuid\n");
  return -1; // Not implementedj
}

pid_t setsid(void) {
  printf("TODO: Implement setsid\n");
  return -1; // Not implemented
}

int setgid(gid_t gid) {
  printf("TODO: Implement setgid\n");
  return -1; // Not implemented
}

int fchown(int fd, uid_t owner, gid_t group) {
  printf("TODO: Implement fchown\n");
  return -1; // Not implemented
}

int link(const char *oldpath, const char *newpath) {
  printf("TODO: Implement link\n");
  return -1; // Not implemented
}

int chroot(const char *path) {
  printf("TODO: Implement chroot\n");
  return -1; // Not implemented
}

int faccessat(int dirfd, const char *pathname, int mode, int flags) {
  printf("TODO: Implement faccessat\n");
  return -1; // Not implemented
}

int gethostname(char *name, size_t size) {
  const char *default_hostname = "localhost";
  const size_t hostname_size = strlen(default_hostname);
  if (size > hostname_size) {
    memcpy(name, default_hostname, hostname_size);
    return 0; // Success
  }
  return -1;
}

int sethostname(const char *name, size_t size) {
  printf("TODO: Implement sethostname\n");
  return -1; // Not implemented
}

int access(char *name, int type) {
  return 0;
}

// int unlink(const char *path) {
//   printf("TODO: Implement unlink\n");
//   return -1; // Not implemented
// }

int pipe(int fds[2]) {
  printf("TODO: Implement pipe\n");
  return -1; // Not implemented
}

int dup(int fd) {
  dup_context ctx = {.fd = fd, .newfd = -1};
  return trigger_syscall(sys_dup, &ctx);
}

int dup2(int fd, int fd2) {
  dup_context ctx = {.fd = fd, .newfd = fd2};
  return trigger_syscall(sys_dup, &ctx);
}

void _exit(int status) {
  exit(status);
}

long sysconf(int name) {
  errno = EINVAL;
  return -1;
}

pid_t getsid(pid_t pid) {
  printf("TODO: Implement getsid\n");
  return -1; // Not implemented
}
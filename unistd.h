// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

/* access() flags */
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#define TRUE 1
#define FALSE 0

int access(char *name, int type);
int unlink(const char *path);

extern char **environ;

int isatty(int fd);
int close(int fd);

ssize_t write(int fd, const void *buf, size_t n);
ssize_t read(int fd, void *buf, size_t n);
int fsync(int fd);

/* lseek() whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

long lseek(int fd, long offset, int whence);

int pipe(int fds[2]);
int dup(int fd);
int dup2(int fd, int fd2);

int vfork(void);
int fork(void);
int getpid(void);
int getppid(void);

int execve(const char *path, char *const argv[], char *const envp[]);
int execle(char *path, ...);
int execvp(char *file, char *argv[]);
int execv(char *path, char *argv[]);

void *sbrk(intptr_t increment);

void _exit(int status);

int sleep(int n);

char *getcwd(char *buf, size_t size);
int chdir(const char *path);

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);

uid_t getuid(void);
uid_t geteuid(void);
int setuid(uid_t uid);
pid_t setsid(void);
int setgid(gid_t gid);

int fchown(int fd, uid_t owner, gid_t group);
int link(const char *oldpath, const char *newpath);
int chroot(const char *path);
int faccessat(int dirfd, const char *pathname, int mode, int flags);
int gethostname(char *name, size_t size);
int sethostname(const char *name, size_t size);

long sysconf(int name);
pid_t getsid(pid_t pid);

#define _CS_PATH "/bin"

#define _SC_ARG_MAX 1
#define _SC_PAGESIZE 2
#define _SC_CLK_TCK 3
#define _SC_NPROCESSORS_CONF 4
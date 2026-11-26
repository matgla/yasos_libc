#pragma yasc
#pragma tcc c_begin
/*
 *   Copyright (c) 2025 Mateusz Stadnik

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/_attrs.h>

/* access() flags */
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#define TRUE 1
#define FALSE 0

int access(char *name, int type) __yl_nonnull(1);
int unlink(const char *path) __yl_nonnull(1);

extern char **environ;

#pragma tcc c_end
int isatty(int view fd);

/* an invalid handle is fine (EBADF), as free(NULL) is */
int close(int drop owned(close) maybe fd);
ssize_t write(int view fd, const void * maybe buf, size_t n);
/* buf may be null with n == 0 */
ssize_t read(int view fd, out void * maybe buf, size_t n);
int fsync(int view fd);
#pragma tcc c_begin
void sync(void);

/* lseek() whence */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#pragma tcc c_end
long lseek(int view fd, long offset, int whence);
off64_t lseek64(int view fd, off64_t offset, int whence);

/* both written on success (0) only */
int pipe(out int owned(close) maybe fds[2]) __yl_nonnull(1);
int pipe2(out int owned(close) maybe fds[2], int flags) __yl_nonnull(1);
int owned(close) maybe dup(int view fd);
/* The result is FD2 again (or -1): a number the caller picked and already
   accounts for -- usually 0, 1 or 2, which no variable owns -- so it is not an
   owned handle, or every dup2(p[1], 1) would leak. FD is only borrowed. */
int dup2(int view fd, int fd2);
#pragma tcc c_begin

int vfork(void);
int fork(void);
int getpid(void);
int getppid(void);

int execve(const char *path, char *const argv[], char *const envp[]) __yl_nonnull(1, 2);
int execle(const char *path, ...) __yl_nonnull(1);
int execvp(const char *file, char *argv[]) __yl_nonnull(1, 2);
int execv(const char *path, char *argv[]) __yl_nonnull(1, 2);

void *sbrk(intptr_t increment);

__attribute__((noreturn)) void _exit(int status);

int sleep(int n);

char *getcwd(char *buf, size_t size);
int chdir(const char *path) __yl_nonnull(1);
int rmdir(const char *path) __yl_nonnull(1);
#pragma tcc c_end
int fchdir(int view fd);

/* DIRFD may be AT_FDCWD; the link's text is not NUL-terminated */
ssize_t readlinkat(int view maybe dirfd, const char *pathname, out char *buf, size_t bufsiz) __yl_nonnull(2);
#pragma tcc c_begin
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) __yl_nonnull(1);

uid_t getuid(void);
uid_t geteuid(void);
int setuid(uid_t uid);
pid_t setsid(void);
int setgid(gid_t gid);

int chown(const char *path, uid_t owner, gid_t group) __yl_nonnull(1);
#pragma tcc c_end
int fchown(int view fd, uid_t owner, gid_t group);
int fchownat(int view maybe dirfd, const char *pathname, uid_t owner, gid_t group,
             int flags);
#pragma tcc c_begin
int lchown(const char *path, uid_t owner, gid_t group) __yl_nonnull(1);
int link(const char *oldpath, const char *newpath) __yl_nonnull(1, 2);
int chroot(const char *path) __yl_nonnull(1);
#pragma tcc c_end
int faccessat(int view maybe dirfd, const char *pathname, int mode, int flags) __yl_nonnull(2);
#pragma tcc c_begin
int gethostname(char *name, size_t size);
int sethostname(const char *name, size_t size);

long sysconf(int name);
pid_t getsid(pid_t pid);

#pragma tcc c_end
int unlinkat(int view maybe dirfd, const char *pathname, int flags) __yl_nonnull(2);
int linkat(int view maybe olddirfd, const char *oldpath, int view maybe newdirfd, const char *newpath,
           int flags) __yl_nonnull(2, 4);
int symlinkat(const char *target, int view maybe newdirfd, const char *linkpath) __yl_nonnull(1, 3);
#pragma tcc c_begin
int symlink(const char *target, const char *linkpath) __yl_nonnull(1, 2);

#pragma tcc c_end
int ftruncate(int view fd, off_t length);

/* null when there is no login name (here always "root", but code written for
   any libc tests it) */
char * borrows(static) maybe getlogin(void);
#pragma tcc c_begin
unsigned alarm(unsigned seconds);

#define _CS_PATH "/bin"

#define _SC_ARG_MAX 1
#define _SC_PAGESIZE 2
#define _SC_CLK_TCK 3
#define _SC_NPROCESSORS_CONF 4

#define _POSIX_VERSION 200112L
#pragma tcc c_end

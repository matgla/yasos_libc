// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/* The system-call layer of the no-OS build (YASLIBC_NOOS), which replaces
 * syscalls.c there: this libc linked into something with no kernel beneath it
 * -- the kernel itself, first of all.
 *
 * It follows newlib's porting model. The POSIX calls below forward to
 * underscore-prefixed hooks (_read, _write, _sbrk, _exit, ...) that the
 * embedding program defines; every hook has a weak default that fails with
 * ENOSYS, so anything that is never wired up still links and reports itself
 * at run time instead of at link time.
 *
 * The rest of libc reaches the kernel through trigger_syscall(). Here that is a
 * weak ENOSYS stub as well: the files that use it (stdio's rename, fcntl, ...)
 * build unchanged, and a program that has a real syscall path can supply its
 * own. */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>

#define WEAK __attribute__((weak))

static int nosys(void) {
  errno = ENOSYS;
  return -1;
}

WEAK int trigger_syscall(int number, const void *args) {
  (void)number;
  (void)args;
  return nosys();
}

/* ── Porting hooks ────────────────────────────────────────────────────── */

WEAK ssize_t _read(int fd, void *buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;
  return nosys();
}

WEAK ssize_t _write(int fd, const void *buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;
  return nosys();
}

WEAK int _open(const char *path, int flags, int mode) {
  (void)path;
  (void)flags;
  (void)mode;
  return nosys();
}

WEAK int _close(int fd) {
  (void)fd;
  return nosys();
}

WEAK off_t _lseek(int fd, off_t offset, int whence) {
  (void)fd;
  (void)offset;
  (void)whence;
  return nosys();
}

WEAK int _fstat(int fd, struct stat *st) {
  (void)fd;
  (void)st;
  return nosys();
}

WEAK int _isatty(int fd) {
  (void)fd;
  errno = ENOSYS;
  return 0;
}

WEAK int _unlink(const char *path) {
  (void)path;
  return nosys();
}

WEAK int _kill(pid_t pid, int sig) {
  (void)pid;
  (void)sig;
  return nosys();
}

WEAK pid_t _getpid(void) {
  return 1;
}

WEAK int _gettimeofday(struct timeval *tv, struct timezone *tz) {
  (void)tv;
  (void)tz;
  return nosys();
}

/* No heap until the program provides one: malloc returns NULL. */
WEAK void *_sbrk(intptr_t increment) {
  (void)increment;
  errno = ENOMEM;
  return (void *)-1;
}

WEAK __attribute__((noreturn)) void _exit(int status) {
  (void)status;
  for (;;) {
  }
}

/* ── POSIX front ends ─────────────────────────────────────────────────── */

ssize_t read(int fd, void *buf, size_t count) {
  return _read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return _write(fd, buf, count);
}

int open(const char *path, int flags, ...) {
  return _open(path, flags, 0);
}

int close(int fd) {
  return _close(fd);
}

off_t lseek(int fd, off_t offset, int whence) {
  return _lseek(fd, offset, whence);
}

int fstat(int fd, struct stat *st) {
  return _fstat(fd, st);
}

int isatty(int fd) {
  return _isatty(fd);
}

int unlink(const char *path) {
  return _unlink(path);
}

int kill(pid_t pid, int sig) {
  return _kill(pid, sig);
}

int getpid(void) {
  return _getpid();
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
  return _gettimeofday(tv, tz);
}

time_t time(time_t *timep) {
  struct timeval tv;
  if (_gettimeofday(&tv, NULL) < 0)
    return (time_t)-1;
  if (timep)
    *timep = tv.tv_sec;
  return tv.tv_sec;
}

void *sbrk(intptr_t increment) {
  return _sbrk(increment);
}

extern __attribute__((noreturn)) void __libc_finalize_and_exit(int status);
__attribute__((noreturn)) void exit(int status) {
  __libc_finalize_and_exit(status);
}

/* ── Static constructors ──────────────────────────────────────────────── */

/* Run by the startup code once .data and .bss are in place. The bounds come
 * from the linker script; they are weak so an image without constructors
 * links too. Unlike newlib's this does not call _init(): nothing here links
 * crti.o, so there is no .init section for it to run. */
extern void (*__preinit_array_start[])(void) __attribute__((weak));
extern void (*__preinit_array_end[])(void) __attribute__((weak));
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));

void __libc_init_array(void) {
  for (void (**f)(void) = __preinit_array_start; f < __preinit_array_end; ++f)
    (*f)();
  for (void (**f)(void) = __init_array_start; f < __init_array_end; ++f)
    (*f)();
}

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

#include <sys/types.h>

#include <errno.h>
#include <regex.h>
#include <stdarg.h>

#include <stdint.h>

#include <signal.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/times.h>

#include "syscalls.h"

#ifdef YASLIBC_ARM_SVC_TRIGGER
void __attribute__((noinline)) __attribute__((naked))
trigger_supervisor_call(int number, const void *args, void *result,
                        optional_errno *err) {
  asm("svc 0");
  asm("bx lr");
}

#endif

void trigger_supervisor_call(int number, const void *args, void *result,
                             optional_errno *err);

void trigger_syscall(int number, const void *args, void *result) {
  optional_errno err = {
      .isset = 0,
  };
  trigger_supervisor_call(number, args, result, &err);
  // if (err.isset) {
  //   errno = err.err;
  // }
}

pid_t getpid() {
  pid_t result;
  trigger_syscall(sys_getpid, NULL, &result);
  return result;
}

int kill(pid_t pid, int sig) {
  int result;
  const kill_context context = {
      .pid = pid,
      .sig = sig,
  };
  trigger_syscall(sys_kill, &context, &result);
  return result;
}

// int sigprocmask(int how, const sigset_t *set,
//                 sigset_t * oldset) {
//   // TODO: implement
//   return 0;
// }

int close(int fd) {
  int result;
  trigger_syscall(sys_close, &fd, &result);
  return result;
}

int fcntl(int filedes, int cmd, ...) {
  // TODO: implement
  return 0;
}

void exit(int status) {
  trigger_syscall(sys_exit, &status, NULL);
  while (1) {
  }
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t result;
  const read_context context = {.fd = fd, .buf = buf, .count = count};
  trigger_syscall(sys_read, &context, &result);
  return result;
}

// int _gettimeofday(struct timeval *restrict tv,
//                   struct timezone *_Nullable restrict tz) {
//   // TODO implement
//   return 0;
// }

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t result;
  const write_context context = {
      .fd = fd,
      .buf = buf,
      .count = count,
  };
  trigger_syscall(sys_write, &context, &result);
  return result;
}

void __aeabi_memset(void *dest, size_t n, int c) {
  /*Note that relative to ANSI memset, __aeabi_memset hase the order
    of its second and third arguments reversed.  */
  // uint8_t *ptr = (uint8_t *)dest;
  // while (--n) *ptr++ = (uint8_t)c;
}

void __aeabi_uldivmod() {
}

void _putchar(char c) {
  ssize_t result;
  const write_context context = {
      .fd = 1,
      .buf = &c,
      .count = 1,
  };
  trigger_syscall(sys_write, &context, &result);
}

pid_t vfork() {
  pid_t result;
  trigger_syscall(sys_vfork, NULL, &result);
  return result;
}

int unlink(const char *pathname) {
  int result;
  trigger_syscall(sys_unlink, pathname, &result);
  return result;
}

int execve(const char *pathname, char *const argv[], char *const envp[]) {
  execve_result result;
  const execve_context context = {
      .filename = pathname,
      .argv = argv,
      .envp = envp,
  };
  trigger_syscall(sys_execve, &context, &result);

  return 0;
}

char *getcwd(char *buf, size_t size) {
  char *result;
  const getcwd_context context = {
      .buf = buf,
      .size = size,
  };
  trigger_syscall(sys_getcwd, &context, &result);
  return result;
}

// void __libc_fini(void *array) {
//   typedef void (*Destructor)();
//   Destructor *fini_array = (Destructor *)(array);
//   // first must be -1 last must be 0
//   const Destructor minus1 = (Destructor)(-1);
//   if (array == NULL || fini_array[0] != minus1) {
//     return;
//   }

//   int count = 0;
//   while (fini_array[count] != NULL) {
//     ++count;
//   }

//   while (count > 0) {
//     if (fini_array[count] != minus1) {
//       fini_array[count--]();
//     }
//   }
// }

// void _fini() {}

// int _link(const char *oldpath, const char *newpath) {
//   int result;
//   const link_context context = {
//       .oldpath = oldpath,
//       .newpath = newpath,
//   };
//   trigger_syscall(sys_link, &context, &result);
//   return result;
// }

// int _mkdir(const char *path, mode_t mode) {
//   int result;
//   const mkdir_context context = {
//       .path = path,
//       .mode = mode,
//   };
//   trigger_syscall(sys_mkdir, &context, &result);
//   return result;
// }

// off_t _lseek(int fd, off_t offset, int whence) {
//   // off_t result;
//   // const lseek_context context = {
//   //     fd = fd,
//   //     offset = offset,
//   //     whence = whence,
//   // };
//   // trigger_syscall(sys_lseek, &context, &result);
//   // return result;
//   return 0;
// }

int isatty(int fd) {
  int result;
  trigger_syscall(sys_isatty, &fd, &result);
  return result;
}

// pid_t _wait(int *_Nullable wstatus) {
//   pid_t result;
//   trigger_syscall(sys_wait, wstatus, &result);
//   return result;
// }

pid_t waitpid(pid_t pid, int *status, int options) {
  pid_t result;
  const waitpid_context context = {
      .pid = pid,
      .status = status,
      .options = options,
  };
  trigger_syscall(sys_waitpid, &context, &result);
  return result;
}

// int _getentropy(void *buffer, size_t length) {
//   int result;
//   const getentropy_context context = {
//       .buffer = buffer,
//       .length = length,
//   };
//   trigger_syscall(sys_getentropy, &context, &result);
//   return result;
// }

// int _stat(const char *restrict pathname, struct stat *restrict statbuf) {
//   int result;
//   const stat_context context = {
//       .pathname = pathname,
//       .statbuf = statbuf,
//   };
//   trigger_syscall(sys_stat, &context, &result);
//   return result;
// }

// clock_t _times(struct tms *buf) {
//   clock_t result;
//   trigger_syscall(sys_stat, buf, &result);
//   return result;
// }

// void _init() {}

void *sbrk(intptr_t increment) {
  sbrk_result result;
  trigger_syscall(sys_sbrk, &increment, &result);
  return result.result;
}

int open(const char *filename, int flags, ...) {
  va_list args;
  va_start(args, flags);
  int mode = va_arg(args, int);
  va_end(args);
  const open_context context = {
      .path = filename,
      .flags = flags,
      .mode = mode,
  };
  int result;
  trigger_syscall(sys_open, &context, &result);
  return result;
}

ssize_t getdents(int fd, struct dirent *de, size_t len) {
  ssize_t result;
  const getdents_context context = {
      .fd = fd,
      .dirp = de,
      .count = len,
  };
  trigger_syscall(sys_getdents, &context, &result);
  return result;
}

int ioctl(int fd, int op, ...) {
  va_list args;
  va_start(args, op);
  void *arg = va_arg(args, void *);
  va_end(args);

  int result;
  const ioctl_context context = {
      .fd = fd,
      .op = op,
      .arg = arg,
  };
  trigger_syscall(sys_ioctl, &context, &result);
  return result;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
  int result;
  const gettimeofday_context context = {
      .tv = tv,
      .tz = tz,
  };
  trigger_syscall(sys_gettimeofday, &context, &result);
  return 0;
}

// int _fstat(int fd, struct stat *buf) {
//   int result;
//   const fstat_context context = {
//       .fd = fd,
//       .buf = buf,
//   };
//   trigger_syscall(sys_fstat, &context, &result);
//   return result;
// }

int nanosleep(struct timespec *req, struct timespec *rem) {
  int result;
  const nanosleep_context context = {
      .req = req,
      .rem = rem,
  };
  trigger_syscall(sys_nanosleep, &context, &result);
  return result;
}

void *mmap(void *addr, int len, int prot, int flags, int fd, int offset) {
  mmap_result result;
  const mmap_context context = {
      .addr = addr,
      .length = len,
      .prot = prot,
      .flags = flags,
      .fd = fd,
      .offset = offset,
  };
  trigger_syscall(sys_mmap, &context, &result);
  return result.memory;
}

int munmap(void *addr, int len) {
  int result;
  const munmap_context context = {
      .addr = addr,
      .length = len,
  };
  trigger_syscall(sys_munmap, &context, &result);
  return result;
}
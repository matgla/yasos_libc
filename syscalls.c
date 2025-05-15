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
inline void __attribute__((naked))
trigger_supervisor_call(int number, const void *args, syscall_result *result) {
  asm("svc 0");
  asm("bx lr");
}
#endif

void trigger_supervisor_call(int number, const void *args,
                             syscall_result *result);

int trigger_syscall(int number, const void *args) {
  syscall_result sresult = {
      .result = 0,
      .err = 0,
  };

  trigger_supervisor_call(number, args, &sresult);

  if (sresult.result > 10) {
    while (true) {
    }
  }

  if (sresult.err >= 1) {
    errno = sresult.err;
  }
  return sresult.result;
}

pid_t getpid() {
  return trigger_syscall(sys_getpid, NULL);
}

int kill(pid_t pid, int sig) {
  const kill_context context = {
      .pid = pid,
      .sig = sig,
  };
  return trigger_syscall(sys_kill, &context);
}

// int sigprocmask(int how, const sigset_t *set,
//                 sigset_t * oldset) {
//   // TODO: implement
//   return 0;
// }

int close(int fd) {
  int result;
  return trigger_syscall(sys_close, &fd);
}

void exit(int status) {
  trigger_syscall(sys_exit, &status);
  while (1) {
  }
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t result;
  if (buf == NULL) {
    return -1;
  }

  if (count == 0) {
    return 0;
  }

  const read_context context = {
      .fd = fd,
      .buf = buf,
      .count = count,
      .result = &result,
  };
  trigger_syscall(sys_read, &context);
  return result;
}

// int _gettimeofday(struct timeval *restrict tv,
//                   struct timezone *_Nullable restrict tz) {
//   // TODO implement
//   return 0;
// }

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t result;
  if (buf == NULL) {
    return -1;
  }

  if (count == 0) {
    return 0;
  }
  const write_context context = {
      .fd = fd,
      .buf = buf,
      .count = count,
      .result = &result,
  };
  trigger_syscall(sys_write, &context);
  return result;
}

void _putchar(char c) {
  const write_context context = {
      .fd = 1,
      .buf = &c,
      .count = 1,
  };
  trigger_syscall(sys_write, &context);
}

pid_t vfork() {
  return trigger_syscall(sys_vfork, NULL);
}

int unlink(const char *pathname) {
  return trigger_syscall(sys_unlink, pathname);
}

int execve(const char *pathname, char *const argv[], char *const envp[]) {
  const execve_context context = {
      .filename = pathname,
      .argv = (char **const)argv,
      .envp = (char **const)envp,
  };
  return trigger_syscall(sys_execve, &context);
}

char *getcwd(char *buf, size_t size) {
  char *result;
  const getcwd_context context = {
      .buf = buf,
      .size = size,
      .result = &result,
  };
  trigger_syscall(sys_getcwd, &context);
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

off_t lseek(int fd, off_t offset, int whence) {
  off_t result;
  const lseek_context context = {
      .fd = fd,
      .offset = offset,
      .whence = whence,
      .result = &result,
  };
  trigger_syscall(sys_lseek, &context);
  return result;
}

int isatty(int fd) {
  return trigger_syscall(sys_isatty, &fd);
}

int remove(const char *pathname) {
  const remove_context context = {
      .pathname = pathname,
  };
  return trigger_syscall(sys_unlink, &context);
}

// pid_t _wait(int *_Nullable wstatus) {
//   pid_t result;
//   trigger_syscall(sys_wait, wstatus, &result);
//   return result;
// }

pid_t waitpid(pid_t pid, int *status, int options) {
  const waitpid_context context = {
      .pid = pid,
      .status = status,
      .options = options,
  };
  return trigger_syscall(sys_waitpid, &context);
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
  return trigger_syscall(sys_open, &context);
}

ssize_t getdents(int fd, struct dirent *de, size_t len) {
  ssize_t result;
  const getdents_context context = {
      .fd = fd,
      .dirp = de,
      .count = len,
      .result = &result,
  };
  trigger_syscall(sys_getdents, &context);
  return result;
}

int ioctl(int fd, int op, ...) {
  va_list args;
  va_start(args, op);
  void *arg = va_arg(args, void *);
  va_end(args);

  const ioctl_context context = {
      .fd = fd,
      .op = op,
      .arg = arg,
  };
  return trigger_syscall(sys_ioctl, &context);
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
  const gettimeofday_context context = {
      .tv = tv,
      .tz = tz,
  };
  return trigger_syscall(sys_gettimeofday, &context);
}

time_t time(time_t *timep) {
  time_t result;
  time_context context = {
      .timep = timep,
      .result = &result,
  };
  trigger_syscall(sys_time, &context);
  return result;
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
  const nanosleep_context context = {
      .req = req,
      .rem = rem,
  };
  return trigger_syscall(sys_nanosleep, &context);
}

void *mmap(void *addr, int len, int prot, int flags, int fd, int offset) {
  void *result = NULL;
  const mmap_context context = {
      .addr = addr,
      .length = len,
      .prot = prot,
      .flags = flags,
      .fd = fd,
      .offset = offset,
      .result = &result,
  };
  trigger_syscall(sys_mmap, &context);
  return result;
}

int munmap(void *addr, int len) {
  const munmap_context context = {
      .addr = addr,
      .length = len,
  };
  return trigger_syscall(sys_munmap, &context);
}

int mprotect(void *addr, size_t len, int prot) {
  int result;
  const mprotect_context context = {
      .addr = addr,
      .length = len,
      .prot = prot,
  };
  trigger_syscall(sys_mprotect, &context);
  return result;
}

int chdir(const char *path) {
  const chdir_context context = {
      .path = path,
  };
  return trigger_syscall(sys_chdir, &context);
}

int fcntl(int fd, int op, ...) {
  va_list args;
  va_start(args, op);
  void *arg = va_arg(args, void *);
  va_end(args);

  const fcntl_context context = {
      .fd = fd,
      .op = op,
      .arg = arg,
  };
  return trigger_syscall(sys_fcntl, &context);
}

char *realpath(const char *path, char *resolved_path) {
  const realpath_context context = {
      .path = path,
      .resolved_path = resolved_path,
  };
  trigger_syscall(sys_realpath, &context);
  return resolved_path;
}
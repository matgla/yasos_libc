// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
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

#include <sys/stat.h>
#include <sys/types.h>

#include <sys/syscall.h>

#include <fcntl.h>

#include <stdio.h>

#include <sys/time.h>
#include <utime.h>

int stat(const char *file, struct stat *buf) {
  if (file == NULL || buf == NULL) {
    return -1;
  }

  stat_context context = {
      .pathname = file,
      .statbuf = buf,
      .fd = AT_FDCWD,
      .follow_links = true,
  };

  int rc = trigger_syscall(sys_stat, &context);
  return rc;
}

int fstat(int fd, struct stat *buf) {
  if (buf == NULL) {
    return -1;
  }

  stat_context context = {
      .pathname = NULL,
      .statbuf = buf,
      .fd = fd,
      .follow_links = true,
  };

  return trigger_syscall(sys_stat, &context);
}

int lstat(const char *file, struct stat *buf) {
  stat_context context = {
      .pathname = file,
      .statbuf = buf,
      .fd = AT_FDCWD,
      .follow_links = false,
  };
  return trigger_syscall(sys_stat, &context);
}

int chmod(char *file, int mode) {
  printf("TODO: implement sys/stat chmod\n");
  return 0;
}

int fchmod(int fd, int mode) {
  printf("TODO: implement sys/stat fchmod\n");
  return 0;
}

int fchmodat(int dirfd, const char *pathname, int mode, int flags) {
  printf("TODO: implement sys/stat fchmodat\n");
  return 0;
}

int umask(int mask) {
  return 0xffffffff & 0777;
}

int mkdir(char *path, int mode) {
  mkdir_context context = {
      .path = (const char *)path,
      .mode = mode,
      .fd = AT_FDCWD,
  };

  return trigger_syscall(sys_mkdir, &context);
}

int mknod(char *path, int mode, int dev) {
  printf("TODO: implement sys/stat mknod\n");
  return 0;
}

int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
  printf("TODO: implement sys/stat mknodat\n");
  return 0;
}

int mkfifo(char *path, int mode) {
  printf("TODO: implement sys/stat mkfifo\n");
  return 0;
}

int fstatat(int fd, const char *path, struct stat *buf, int flag) {
  if (buf == NULL) {
    return -1;
  }

  // struct stat statinfo = {
  // };

  stat_context context = {
      .pathname = path,
      .statbuf = buf,
      .fd = fd,
      .follow_links = !(flag & AT_SYMLINK_NOFOLLOW),
  };

  int rc = trigger_syscall(sys_stat, &context);
  return rc;
}

int mkdirat(int dirfd, const char *pathname, mode_t mode) {
  mkdir_context context = {
      .path = (const char *)pathname,
      .mode = mode,
      .fd = dirfd,
  };

  return trigger_syscall(sys_mkdir, &context);
}

int utimensat(int dirfd, const char *pathname, const struct timespec times[2],
              int flags) {
  if (pathname == NULL) {
    return -1;
  }

  utimensat_context context = {
      .pathname = pathname,
      .times = times,
      .fd = dirfd,
      .flags = flags,
  };

  return trigger_syscall(sys_utimensat, &context);
}

int futimens(int fd, const struct timespec times[2]) {
  /* No path: the kernel takes the open file straight off the descriptor, the
     same shape fstat() uses. */
  utimensat_context context = {
      .pathname = NULL,
      .times = times,
      .fd = fd,
      .flags = 0,
  };

  return trigger_syscall(sys_utimensat, &context);
}

/* The two pre-utimensat spellings, both of which reduce to it.  Declared in
   <utime.h> and <sys/time.h> since before this libc had an implementation to
   go with them, so anything that reached for one got a link error; a program
   compiled on the device is as likely to use these as the modern call. */
int utime(char *path, struct utimbuf *times) {
  struct timespec spec[2];

  if (times == NULL) {
    return utimensat(AT_FDCWD, path, NULL, 0);
  }

  spec[0].tv_sec = times->actime;
  spec[0].tv_nsec = 0;
  spec[1].tv_sec = times->modtime;
  spec[1].tv_nsec = 0;
  return utimensat(AT_FDCWD, path, spec, 0);
}

int utimes(const char *path, const struct timeval times[2]) {
  struct timespec spec[2];

  if (times == NULL) {
    return utimensat(AT_FDCWD, path, NULL, 0);
  }

  spec[0].tv_sec = times[0].tv_sec;
  spec[0].tv_nsec = times[0].tv_usec * 1000;
  spec[1].tv_sec = times[1].tv_sec;
  spec[1].tv_nsec = times[1].tv_usec * 1000;
  return utimensat(AT_FDCWD, path, spec, 0);
}

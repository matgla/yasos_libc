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
  printf("TODO: implement sys/stat utimensat\n");
  return 0;
}

int futimens(int fd, const struct timespec times[2]) {
  printf("TODO: implement sys/stat futimens\n");
  return 0;
}
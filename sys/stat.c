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

#include <stdio.h>

int stat(char *file, struct stat *buf) {
  if (file == NULL || buf == NULL) {
    return -1;
  }

  struct stat statinfo = {
  };

  stat_context context = {
    .pathname = file,
    .statbuf = buf,
    .fd = 0,
  };

  int rc = trigger_syscall(sys_stat, &context);
  return rc;
}

int fstat(int fd, struct stat *buf) {
  printf("TODO: implement sys/stat fstat\n");
  return 0;
}

int lstat(char *file, struct stat *buf) {
  printf("TODO: implement sys/stat lstat\n");
  return 0;
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
  printf("TODO: implement sys/stat mkdir\n");
  return 0;
}

int mknod(char *path, int mode, int dev) {
  printf("TODO: implement sys/stat mknod\n");
  return 0;
}

int mkfifo(char *path, int mode) {
  printf("TODO: implement sys/stat mkfifo\n");
  return 0;
}

int fstatat(int fd, const char *path, struct stat *buf, int flag) {
  printf("fstatat called with fd=%d, path=%s, buf=%p, flag=%d\n", fd, path, buf, flag);
  if (buf == NULL) {
    return -1;
  }

  struct stat statinfo = {
  };

  stat_context context = {
    .pathname = path,
    .statbuf = buf,
    .fd = fd,
  };

  int rc = trigger_syscall(sys_stat, &context);
  return rc;
}

int mkdirat(int dirfd, const char *pathname, mode_t mode) {
  printf("TODO: implement sys/stat mkdirat\n");
  return 0;
}
// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <sys/time.h>
#include <sys/types.h>
#include <sys/_attrs.h>

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define S_IFMT 0170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* utimensat()/futimens() tv_nsec sentinels: "use the current time" and "leave
 * this timestamp alone". Same values Linux uses, because toybox and make both
 * hard-code the behaviour these imply. */
#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)

struct stat {
  uint32_t st_dev;
  unsigned long st_ino;
  uint16_t st_mode;
  uint16_t st_nlink;
  uint16_t st_uid;
  uint16_t st_gid;
  uint32_t st_rdev;
  unsigned long st_size;
  unsigned long st_blksize;
  unsigned long st_blocks;
  struct timespec st_atim;
  struct timespec st_mtim;
  struct timespec st_ctim;
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
};

int stat(const char *file, struct stat *buf) __yl_nonnull(1, 2);
int fstat(int fd, struct stat *buf) __yl_nonnull(2);
int lstat(const char *file, struct stat *buf) __yl_nonnull(1, 2);

int chmod(char *file, int mode) __yl_nonnull(1);
int fchmod(int fd, int mode);
int fchmodat(int dirfd, const char *pathname, int mode, int flags);
int umask(int mask);
int mkdir(char *path, int mode) __yl_nonnull(1);
int mknod(char *path, int mode, int dev) __yl_nonnull(1);
int mkfifo(char *path, int mode) __yl_nonnull(1);

int fstatat(int fd, const char *path, struct stat *buf, int flag) __yl_nonnull(2, 3);
int mkdirat(int dirfd, const char *pathname, mode_t mode) __yl_nonnull(2);

int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) __yl_nonnull(2);

int utimensat(int dirfd, const char *pathname, const struct timespec times[2],
              int flags);

int futimens(int fd, const struct timespec times[2]);
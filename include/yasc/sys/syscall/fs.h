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

#pragma yasc
#pragma once

/* Paths and directories: open, stat, directory entries, links, renames,
 * permissions, mounts and the working directory. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

struct stat;
struct dirent;
struct timespec;
struct statvfs;

typedef struct open_context {
  const char * view maybe path;
  int flags;
  int mode;
  int fd;
} open_context;

typedef struct stat_context {
  const char * view maybe pathname;
  struct stat * view maybe statbuf;
  int fd;
  uint8_t follow_links;
} stat_context;

typedef struct fstat_context {
  int fd;
  struct stat * view maybe buf;
} fstat_context;

typedef struct statvfs_context {
  /* Path to describe, or NULL to describe the file `fd` is open on. */
  const char * view maybe path;
  int fd;
  struct statvfs * view maybe buf;
} statvfs_context;

typedef struct getdents_context {
  int fd;
  struct dirent * view maybe dirp;
  size_t count;
  ssize_t * view result;
} getdents_context;

typedef struct mkdir_context {
  const char * view maybe path;
  mode_t mode;
  int fd;
} mkdir_context;

typedef struct link_context {
  int olddirfd;
  const char * view maybe oldpath;
  int newdirfd;
  const char * view maybe newpath;
  int flags;
} link_context;

typedef struct symlink_context {
  const char * view maybe target;
  int dirfd;
  const char * view maybe linkpath;
} symlink_context;

/* readlink(2)/readlinkat(2). `buf` is not NUL-terminated on return, and a
 * target longer than `bufsiz` is silently truncated -- both as POSIX has it. */
typedef struct readlink_context {
  const char * view maybe pathname;
  char * view maybe buf;
  size_t bufsiz;
  int fd;
} readlink_context;

/* renameat2(2) flags. Same values as Linux; stdio.h repeats the definitions for
   programs that only include it. Only NOREPLACE changes what the kernel does. */
#define RENAME_NOREPLACE (1 << 0)
#define RENAME_EXCHANGE (1 << 1)
#define RENAME_WHITEOUT (1 << 2)

/* renameat2(2) arguments. */
typedef struct rename_context {
  int olddirfd;
  const char * view maybe oldpath;
  int newdirfd;
  const char * view maybe newpath;
  unsigned int flags;
} rename_context;

typedef struct unlink_context {
  int dirfd;
  const char * view maybe pathname;
  int flags;
} unlink_context;

typedef struct remove_context {
  const char * view maybe pathname;
} remove_context;

typedef struct realpath_context {
  const char * view maybe path;
  char * view maybe resolved_path;
} realpath_context;

typedef struct access_context {
  const char * view maybe pathname;
  int mode;
  int dirfd;
  int flags;
} access_context;

/* `times` is NULL for "now", otherwise two timespecs: [0] access, [1]
 * modification, either of which may carry UTIME_NOW or UTIME_OMIT in tv_nsec.
 *
 * Shaped like stat_context: `pathname` NULL means "the file `fd` is open on"
 * (futimens), and otherwise `fd` is the directory a relative path resolves
 * against. */
typedef struct utimensat_context {
  const char * view maybe pathname;
  const struct timespec * view maybe times;
  int fd;
  int flags;
} utimensat_context;

/* fchmodat(2); fchmod(2) is path NULL with fd the open file. */
typedef struct chmod_context {
  const char * view maybe path;
  int fd;
  mode_t mode;
  int flags; /* AT_SYMLINK_NOFOLLOW */
} chmod_context;

/* fchownat(2); fchown(2) is path NULL with fd the open file. -1 leaves an id
 * unchanged. */
typedef struct chown_context {
  const char * view maybe path;
  int fd;
  uid_t uid;
  gid_t gid;
  int flags; /* AT_SYMLINK_NOFOLLOW */
} chown_context;

typedef struct mount_context {
  const char * view maybe source;
  const char * view maybe target;
  const char * view maybe filesystemtype;
  unsigned long mountflags;
  /* The option string ("nofail,spill=/var/tmp"), or NULL. */
  const void * view maybe data;
} mount_context;

typedef struct umount_context {
  const char * view maybe target;
  int flags;
} umount_context;

typedef struct getcwd_context {
  char * view maybe buf;
  size_t size;
  char * view maybe * view result;
} getcwd_context;

typedef struct chdir_context {
  const char * view maybe path;
} chdir_context;

typedef struct fchdir_context {
  int fd;
} fchdir_context;

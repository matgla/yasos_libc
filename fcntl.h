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

#define O_RDONLY 00000
#define O_WRONLY 00001
#define O_RDWR 00002
#define O_ACCMODE 00003
#define O_CREAT 00100
#define O_EXCL 00200
#define O_NOCTTY 00400
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_SYNC 0010000
#define FASYNC 0020000
#ifdef __arm__
#define O_DIRECTORY 0040000
#define O_NOFOLLOW 0100000
#define O_DIRECT 0200000
#define O_LARGEFILE 0400000
#else
#define O_DIRECT 0040000
#define O_LARGEFILE 0100000
#define O_DIRECTORY 0200000
#define O_NOFOLLOW 0400000
#endif
#define O_NOATIME 001000000

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7
#define F_SETOWN 8
#define F_GETOWN 9
#define F_SETSIG 10
#define F_GETSIG 11

#define FD_CLOEXEC 1

#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2

#define AT_FDCWD -100

#define AT_SYMLINK_NOFOLLOW 0x1
#define AT_NO_AUTOMOUNT 0x2

int open(const char *path, int flags, ...);
int creat(const char *path, int mode);
int fcntl(int fd, int cmd, ...);

int openat(int dirfd, const char *pathname, int flags, ...);

typedef int32_t pid_t;
typedef signed long off_t;

struct flock {
  short l_type;   /* Type of lock: F_RDLCK, F_WRLCK, or F_UNLCK */
  short l_whence; /* How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END */
  off_t l_start;  /* Starting offset for lock */
  off_t l_len;    /* Length of lock (0 means until end of file) */
  pid_t l_pid;    /* Process ID of the process holding the lock */
};

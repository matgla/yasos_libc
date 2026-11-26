/* Written by `tcc -yasc-c-header` from yasc/mntent.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "yasc/mntent.h"
#else
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

#pragma once

#include <stdio.h>
#include <sys/_attrs.h>

#define MNTTAB "/etc/fstab"
#define MOUNTED "/proc/mounts"
#define _PATH_MNTTAB "/etc/fstab"
#define _PATH_MOUNTED "/proc/mounts"

#define MNTTYPE_IGNORE "ignore"
#define MNTOPT_DEFAULTS "defaults"
#define MNTOPT_RO "ro"
#define MNTOPT_RW "rw"
#define MNTOPT_NOAUTO "noauto"

struct mntent {
  char *mnt_fsname; /* Filesystem name */
  char *mnt_dir;    /* Directory on which filesystem is mounted */
  char *mnt_type;   /* Type of filesystem */
  char *mnt_opts;   /* Mount options */
  int mnt_freq;     /* Dump frequency */
  int mnt_passno;   /* Pass number for fsck */
};


/* endmntent first: setmntent names it as its deallocator (sys/_attrs.h) */
int endmntent(FILE *streamp);
FILE *setmntent(const char *filename, const char *type) __yl_dealloc(endmntent, 1) __yl_nonnull(1, 2);
/* null at the end of the table */
struct mntent *getmntent(FILE *stream) __yl_nonnull(1);
int addmntent(FILE *stream, const struct mntent *mnt) __yl_nonnull(1, 2);
/* where OPT starts in mnt_opts, or null when it is not there */
char *hasmntopt(const struct mntent *mnt, const char *opt) __yl_nonnull(1, 2);
#endif /* __YASC__ */

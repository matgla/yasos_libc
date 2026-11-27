/* Written by `tcc -yasc-c-header` from yasc/grp.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "yasc/grp.h"
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

#include <sys/_attrs.h>
#include <sys/types.h>


struct group {
  char *gr_name __yl_view;   /* group name */
  char *gr_passwd __yl_view; /* group password */
  gid_t gr_gid;                        /* group ID */
  /* NULL-terminated array of pointers to names of group members */
  char **gr_mem __yl_view_maybe;
};

/* Not found: 0 and *result null (_r), or null with errno unchanged. */
struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name) __yl_nonnull(1);
/* ERANGE: buf too small.  inout, not out buf[size]: tinycc
 * docs/bugs/yasc-c-header-forward-bound.md */
int getgrgid_r(gid_t gid, struct group *grp, char *buf, size_t size,
               struct group **result);
int getgrnam_r(const char *name, struct group *grp, char *buf, size_t size,
               struct group **result) __yl_nonnull(1);

struct group *getgrent(void);
void setgrent(void);
void endgrent(void);

/* not POSIX (BSD, glibc) */
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups) __yl_nonnull(1, 4);
int setgroups(size_t size, const gid_t *list) __yl_access(read_only, 2, 1);
int initgroups(const char *user, gid_t group) __yl_nonnull(1);
#endif /* __YASC__ */

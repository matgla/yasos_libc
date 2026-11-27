#pragma yasc
#pragma tcc c_begin
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

#pragma tcc c_end

struct group {
  const char *c_api(char *) gr_name;   /* group name */
  const char *c_api(char *) gr_passwd; /* group password */
  gid_t gr_gid;                        /* group ID */
  /* NULL-terminated array of pointers to names of group members */
  const char *const *?c_api(char **) gr_mem;
};

/* Not found: 0 and *result null (_r), or null with errno unchanged. */
struct group *?borrows(static) getgrgid(gid_t gid);
struct group *?borrows(static) getgrnam(char *name) __yl_nonnull(1);
/* ERANGE: buf too small.  inout, not out buf[size]: tinycc
 * docs/bugs/yasc-c-header-forward-bound.md */
int getgrgid_r(gid_t gid, out struct group *grp, inout char *buf, size_t size,
               out struct group *?borrows(grp) *result);
int getgrnam_r(char *name, out struct group *grp, inout char *buf, size_t size,
               out struct group *?borrows(grp) *result) __yl_nonnull(1);

struct group *?borrows(static) getgrent(void);
void setgrent(void);
void endgrent(void);

/* not POSIX (BSD, glibc) */
int getgrouplist(char *user, gid_t group, inout gid_t *?groups, inout int *ngroups) __yl_nonnull(1, 4);
int setgroups(size_t size, gid_t list[?size]);
int initgroups(char *user, gid_t group) __yl_nonnull(1);

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

struct passwd {
  const char *c_api(char *) pw_name;   /* user name */
  const char *c_api(char *) pw_passwd; /* password */
  uid_t pw_uid;                        /* user ID */
  gid_t pw_gid;                        /* group ID */
  const char *c_api(char *) pw_gecos;  /* real name */
  const char *c_api(char *) pw_dir;    /* home directory */
  const char *c_api(char *) pw_shell;  /* shell program */
};

/* Not found: 0 and *result null (_r), or null with errno unchanged. */
struct passwd *?borrows(static) getpwnam(char *name) __yl_nonnull(1);
struct passwd *?borrows(static) getpwuid(uid_t uid);
/* ERANGE: buf too small.  inout, not out buf[size]: tinycc
 * docs/bugs/yasc-c-header-forward-bound.md */
int getpwnam_r(char *name, out struct passwd *pwd, inout char *buf, size_t size,
               out struct passwd *?borrows(pwd) *result) __yl_nonnull(1);
int getpwuid_r(uid_t uid, out struct passwd *pwd, inout char *buf, size_t size,
               out struct passwd *?borrows(pwd) *result);

struct passwd *?borrows(static) getpwent(void);
void setpwent(void);
void endpwent(void);

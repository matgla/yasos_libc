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

/* <pwd.h> over /etc/passwd (name:password:uid:gid:gecos:home:shell); without
 * the file, root is the only user. */

#include <pwd.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "internal/dbfile.h"

static const char passwd_path[] = "/etc/passwd";
static const char builtin_users[] = "root:x:0:0:root:/root:/bin/sh\n";

/* field offsets in a line split in place */
struct fields {
  size_t passwd, gecos, dir, shell, end;
  uid_t uid;
  gid_t gid;
};

static int pf_open(out struct db_file *pf) {
  return __db_open(pf, passwd_path, builtin_users);
}

static bool parse(inout char *line, out struct fields *f) {
  *f = (struct fields){};
  size_t at[7] = {};
  size_t end = 0;
  unsigned long uid = 0;
  unsigned long gid = 0;
  if (!__db_fields(line, 7, at, &end) || !__db_id(line + at[2], cast<trunc(uid_t)>(-1), &uid) ||
      !__db_id(line + at[3], cast<trunc(gid_t)>(-1), &gid))
    return false;
  f->passwd = at[1];
  f->gecos = at[4];
  f->dir = at[5];
  f->shell = at[6];
  f->end = end;
  f->uid = cast<int(uid_t)>(uid);
  f->gid = cast<int(gid_t)>(gid);
  return true;
}

/* 0, or ERANGE */
static int fill(char *line, struct fields *f, out struct passwd *pwd, inout char buf[size], size_t size) {
  if (size < f->end + 1) {
    *pwd = (struct passwd){};
    return ERANGE;
  }
  memcpy(buf, line, f->end + 1);
  /* yasc cannot say that *pwd borrows from buf */
  unsafe {
    pwd->pw_name = buf;
    pwd->pw_passwd = buf + f->passwd;
    pwd->pw_gecos = buf + f->gecos;
    pwd->pw_dir = buf + f->dir;
    pwd->pw_shell = buf + f->shell;
  }
  pwd->pw_uid = f->uid;
  pwd->pw_gid = f->gid;
  return 0;
}

/* by NAME, or by UID when NAME is null: 1, 0 not found, or -errno */
static int find(inout struct db_file *pf, char *?name, uid_t uid, out struct fields *f) {
  *f = (struct fields){};
  for (;;) {
    const int r = __db_next_line(pf);
    if (r <= 0)
      return r;
    char *?line = pf->line;
    if (!line || !parse(line, f))
      continue;
    if (name ? strcmp(line, name) == 0 : f->uid == uid)
      return 1;
  }
}

static int lookup_r(char *?name, uid_t uid, out struct passwd *pwd, inout char *buf, size_t size,
                    out struct passwd *?borrows(pwd) *result) {
  *result = nullptr;
  *pwd = (struct passwd){};
  const int saved = errno;
  struct db_file pf = undefined;
  int err = pf_open(&pf);
  if (err) {
    errno = saved;
    return err;
  }
  struct fields f = undefined;
  const int r = find(&pf, name, uid, &f);
  if (r < 0)
    err = -r;
  else if (r > 0 && pf.line)
    err = fill(pf.line, &f, pwd, buf, size);
  if (r > 0 && err == 0)
    *result = pwd;
  __db_close(&pf);
  errno = saved;
  return err;
}

int getpwnam_r(char *name, out struct passwd *pwd, inout char *buf, size_t size,
               out struct passwd *?borrows(pwd) *result) {
  *result = nullptr;
  return lookup_r(name, 0, pwd, buf, size, result);
}

int getpwuid_r(uid_t uid, out struct passwd *pwd, inout char *buf, size_t size,
               out struct passwd *?borrows(pwd) *result) {
  *result = nullptr;
  return lookup_r(nullptr, uid, pwd, buf, size, result);
}

/* the entry getpwnam, getpwuid and getpwent return */
static struct passwd static_passwd = {
    .pw_name = "", .pw_passwd = "", .pw_uid = 0, .pw_gid = 0, .pw_gecos = "", .pw_dir = "", .pw_shell = ""};
static char *?owned(free) static_buf;
static size_t static_size;

static struct passwd *?borrows(static) keep(char *line, struct fields *f) {
  const size_t need = f->end + 1;
  if (need > static_size || !static_buf) {
    char *?owned(free) old = move(static_buf);
    static_buf = nullptr;
    char *?owned(free) bigger = realloc(move(old), need);
    if (!bigger) {
      static_buf = move(old);
      errno = ENOMEM;
      return nullptr;
    }
    static_buf = move(bigger);
    static_size = need;
  }
  char *?buf = static_buf;
  if (!buf || fill(line, f, &static_passwd, buf, static_size) != 0) {
    errno = ENOMEM;
    return nullptr;
  }
  return &static_passwd;
}

static struct passwd *?borrows(static) lookup(char *?name, uid_t uid) {
  const int saved = errno;
  struct db_file pf = undefined;
  int err = pf_open(&pf);
  if (err) {
    errno = err;
    return nullptr;
  }
  struct fields f = undefined;
  struct passwd *?found = nullptr;
  const int r = find(&pf, name, uid, &f);
  if (r < 0)
    err = -r;
  else if (r > 0 && pf.line) {
    found = keep(pf.line, &f);
    if (!found)
      err = errno;
  }
  __db_close(&pf);
  errno = err ? err : saved;
  return found;
}

struct passwd *?borrows(static) getpwnam(char *name) {
  return lookup(name, 0);
}

struct passwd *?borrows(static) getpwuid(uid_t uid) {
  return lookup(nullptr, uid);
}

static struct db_file ent_file = {.fd = -1};
static bool ent_open;

void endpwent(void) {
  if (!ent_open)
    return;
  const int saved = errno;
  __db_close(&ent_file);
  ent_open = false;
  errno = saved;
}

void setpwent(void) {
  endpwent();
}

struct passwd *?borrows(static) getpwent(void) {
  const int saved = errno;
  if (!ent_open) {
    const int err = pf_open(&ent_file);
    if (err) {
      errno = err;
      return nullptr;
    }
    ent_open = true;
  }
  for (;;) {
    const int r = __db_next_line(&ent_file);
    if (r < 0) {
      errno = -r;
      return nullptr;
    }
    if (r == 0) {
      errno = saved;
      return nullptr;
    }
    char *?line = ent_file.line;
    struct fields f = undefined;
    if (!line || !parse(line, &f))
      continue;
    struct passwd *?p = keep(line, &f);
    if (p)
      errno = saved;
    return p;
  }
}

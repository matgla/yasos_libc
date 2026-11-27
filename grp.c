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

/* <grp.h> over /etc/group (name:password:gid:member,...); without the file,
 * root is the only group.  setgroups is the kernel's (ENOSYS for now). */

#include <grp.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/syscall/proc.h>

#include "internal/dbfile.h"

static const char group_path[] = "/etc/group";
static const char builtin_groups[] = "root:x:0:\n";

/* field offsets in a line split in place */
struct fields {
  size_t passwd, members, end;
  gid_t gid;
};

static int gf_open(out struct db_file *gf) {
  return __db_open(gf, group_path, builtin_groups);
}

static bool parse(inout char *line, out struct fields *f) {
  *f = (struct fields){};
  size_t at[4] = {};
  size_t end = 0;
  unsigned long gid = 0;
  if (!__db_fields(line, 4, at, &end) || !__db_id(line + at[2], cast<trunc(gid_t)>(-1), &gid))
    return false;
  f->passwd = at[1];
  f->members = at[3];
  f->end = end;
  f->gid = cast<int(gid_t)>(gid);
  return true;
}

/* "a,b,,c" has three */
static size_t count_members(char *s) {
  size_t n = 0;
  bool in = false;
  for (; *s; ++s) {
    if (*s == ',') {
      in = false;
    } else if (!in) {
      in = true;
      ++n;
    }
  }
  return n;
}

/* worst case, buffer misaligned */
static size_t entry_size(char *line, struct fields *f) {
  return alignof(char *view) - 1 + (count_members(line + f->members) + 1) * sizeof(char *view) + f->end + 1;
}

/* gr_mem array, then the strings: 0, or ERANGE */
static int fill(char *line, struct fields *f, out struct group *grp, inout char buf[size], size_t size) {
  const size_t nmem = count_members(line + f->members);
  const size_t misalign = cast<addr(uintptr_t)>(buf) % alignof(char *view);
  const size_t pad = misalign ? alignof(char *view) - misalign : 0;
  const size_t ptrs = (nmem + 1) * sizeof(char *view);
  if (size < pad || size - pad < ptrs || size - pad - ptrs < f->end + 1) {
    *grp = (struct group){};
    return ERANGE;
  }
  char *strs = buf + pad + ptrs;
  memcpy(strs, line, f->end + 1);
  const char *?view *mem = cast<ptr(const char *?view *), align>(buf + pad);
  size_t n = 0;
  bool in = false;
  for (size_t i = f->members; i < f->end; ++i) {
    if (strs[i] == ',') {
      strs[i] = '\0';
      in = false;
    } else if (!in) {
      in = true;
      mem[n++] = strs + i;
    }
  }
  mem[n] = nullptr;
  /* yasc cannot say that *grp borrows from buf */
  unsafe {
    grp->gr_name = strs;
    grp->gr_passwd = strs + f->passwd;
    grp->gr_mem = mem;
  }
  grp->gr_gid = f->gid;
  return 0;
}

/* by NAME, or by GID when NAME is null: 1, 0 not found, or -errno */
static int find(inout struct db_file *gf, char *?name, gid_t gid, out struct fields *f) {
  *f = (struct fields){};
  for (;;) {
    const int r = __db_next_line(gf);
    if (r <= 0)
      return r;
    char *?line = gf->line;
    if (!line || !parse(line, f))
      continue;
    if (name ? strcmp(line, name) == 0 : f->gid == gid)
      return 1;
  }
}

static int lookup_r(char *?name, gid_t gid, out struct group *grp, inout char *buf, size_t size,
                    out struct group *?borrows(grp) *result) {
  *result = nullptr;
  *grp = (struct group){};
  const int saved = errno;
  struct db_file gf = undefined;
  int err = gf_open(&gf);
  if (err) {
    errno = saved;
    return err;
  }
  struct fields f = undefined;
  const int r = find(&gf, name, gid, &f);
  if (r < 0)
    err = -r;
  else if (r > 0 && gf.line)
    err = fill(gf.line, &f, grp, buf, size);
  if (r > 0 && err == 0)
    *result = grp;
  __db_close(&gf);
  errno = saved;
  return err;
}

int getgrnam_r(char *name, out struct group *grp, inout char *buf, size_t size,
               out struct group *?borrows(grp) *result) {
  *result = nullptr;
  return lookup_r(name, 0, grp, buf, size, result);
}

int getgrgid_r(gid_t gid, out struct group *grp, inout char *buf, size_t size,
               out struct group *?borrows(grp) *result) {
  *result = nullptr;
  return lookup_r(nullptr, gid, grp, buf, size, result);
}

/* the entry getgrnam, getgrgid and getgrent return */
static const char *?no_members[] = {nullptr};
static struct group static_group = {.gr_name = "", .gr_passwd = "", .gr_gid = 0, .gr_mem = no_members};
static char *?owned(free) static_buf;
static size_t static_size;

static struct group *?borrows(static) keep(char *line, struct fields *f) {
  const size_t need = entry_size(line, f);
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
  if (!buf || fill(line, f, &static_group, buf, static_size) != 0) {
    errno = ENOMEM;
    return nullptr;
  }
  return &static_group;
}

static struct group *?borrows(static) lookup(char *?name, gid_t gid) {
  const int saved = errno;
  struct db_file gf = undefined;
  int err = gf_open(&gf);
  if (err) {
    errno = err;
    return nullptr;
  }
  struct fields f = undefined;
  struct group *?found = nullptr;
  const int r = find(&gf, name, gid, &f);
  if (r < 0)
    err = -r;
  else if (r > 0 && gf.line) {
    found = keep(gf.line, &f);
    if (!found)
      err = errno;
  }
  __db_close(&gf);
  errno = err ? err : saved;
  return found;
}

struct group *?borrows(static) getgrnam(char *name) {
  return lookup(name, 0);
}

struct group *?borrows(static) getgrgid(gid_t gid) {
  return lookup(nullptr, gid);
}

static struct db_file ent_file = {.fd = -1};
static bool ent_open;

void endgrent(void) {
  if (!ent_open)
    return;
  const int saved = errno;
  __db_close(&ent_file);
  ent_open = false;
  errno = saved;
}

void setgrent(void) {
  endgrent();
}

struct group *?borrows(static) getgrent(void) {
  const int saved = errno;
  if (!ent_open) {
    const int err = gf_open(&ent_file);
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
    struct group *?g = keep(line, &f);
    if (g)
      errno = saved;
    return g;
  }
}

/* glibc's semantics; a read error ends the list early */
int getgrouplist(char *user, gid_t group, inout gid_t *?groups, inout int *ngroups) {
  const int room = *ngroups;
  int n = 0;
  if (groups && room > 0)
    groups[0] = group;
  ++n;
  const int saved = errno;
  struct db_file gf = undefined;
  if (gf_open(&gf) == 0) {
    for (;;) {
      if (__db_next_line(&gf) <= 0)
        break;
      char *?line = gf.line;
      struct fields f = undefined;
      if (!line || !parse(line, &f) || f.gid == group)
        continue;
      bool seen = false;
      for (int i = 1; i < n && i < room && groups; ++i)
        seen = seen || groups[i] == f.gid;
      if (seen)
        continue;
      bool member = false;
      for (size_t i = f.members; i < f.end && !member;) {
        size_t e = i;
        while (e < f.end && line[e] != ',')
          ++e;
        member = e - i == strlen(user) && strncmp(line + i, user, e - i) == 0;
        i = e + 1;
      }
      if (!member)
        continue;
      if (groups && n < room)
        groups[n] = f.gid;
      ++n;
    }
    __db_close(&gf);
  }
  errno = saved;
  *ngroups = n;
  return n > room ? -1 : n;
}

int setgroups(size_t size, gid_t list[?size]) {
  if (size > NGROUPS_MAX) {
    errno = EINVAL;
    return -1;
  }
  const setgroups_context context = {.size = size, .list = list};
  return trigger_syscall(sys_setgroups, &context);
}

/* truncated to NGROUPS_MAX */
int initgroups(char *user, gid_t group) {
  gid_t groups[NGROUPS_MAX] = {};
  int n = NGROUPS_MAX;
  if (getgrouplist(user, group, groups, &n) < 0)
    n = NGROUPS_MAX;
  return setgroups(cast<int(size_t)>(n), groups);
}

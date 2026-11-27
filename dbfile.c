// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
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

#include "internal/dbfile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int __db_open(out struct db_file *db, char *path, char *builtin) {
  *db = (struct db_file){.fd = -1};
  int ?owned(close) fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    if (errno != ENOENT)
      return errno;
    size_t n = strlen(builtin);
    if (n > sizeof db->chunk)
      n = sizeof db->chunk;
    memcpy(db->chunk, builtin, n);
    db->end = n;
    return 0;
  }
  db->fd = move(fd);
  return 0;
}

void __db_close(inout struct db_file *db) {
  if (db->fd >= 0)
    close(move(db->fd));
  db->fd = -1;
  free(move(db->line));
  db->line = nullptr;
  db->cap = 0;
  db->pos = db->end = 0;
}

static bool room(inout struct db_file *db, size_t len) {
  if (len < db->cap && db->line)
    return true;
  const size_t cap = db->cap ? db->cap * 2 : 64;
  /* not realloc: tinycc docs/bugs/yasc-takes-member-origin.md */
  char *?owned(free) bigger = malloc(cap);
  if (!bigger)
    return false;
  if (db->line)
    memcpy(borrow(bigger), db->line, db->cap);
  free(move(db->line));
  db->line = move(bigger);
  db->cap = cap;
  return true;
}

int __db_next_line(inout struct db_file *db) {
  size_t len = 0;
  bool any = false;
  for (;;) {
    if (db->pos == db->end) {
      ssize_t n = 0;
      if (db->fd >= 0) {
        n = read(db->fd, db->chunk, sizeof db->chunk);
        if (n < 0) {
          if (errno == EINTR)
            continue;
          return -errno;
        }
      }
      if (n == 0) {
        if (!any)
          return 0;
        break; /* last line, no newline */
      }
      db->pos = 0;
      db->end = cast<int(size_t)>(n);
    }
    any = true;
    const char c = db->chunk[db->pos++];
    if (c == '\n')
      break;
    if (!room(db, len))
      return -ENOMEM;
    db->line[len++] = c;
  }
  if (!room(db, len))
    return -ENOMEM;
  db->line[len] = '\0';
  return 1;
}

bool __db_fields(inout char *line, size_t n, inout size_t at[n], out size_t *end) {
  *end = 0;
  for (size_t i = 0; i < n; ++i)
    at[i] = 0;
  if (n == 0 || line[0] == '\0' || line[0] == '#' || line[0] == '+' || line[0] == '-')
    return false;
  size_t i = 0;
  for (size_t f = 1; f < n; ++f) {
    while (line[i] != ':') {
      if (line[i] == '\0')
        return false;
      ++i;
    }
    line[i++] = '\0';
    at[f] = i;
  }
  while (line[i] != '\0') {
    if (line[i] == ':')
      return false;
    ++i;
  }
  *end = i;
  return true;
}

bool __db_id(char *s, unsigned long max, out unsigned long *value) {
  *value = 0;
  if (s[0] == '\0')
    return false;
  unsigned long v = 0;
  for (size_t i = 0; s[i]; ++i) {
    if (s[i] < '0' || s[i] > '9')
      return false;
    v = v * 10 + cast<int(unsigned long)>(s[i] - '0');
    if (v > max)
      return false;
  }
  *value = v;
  return true;
}

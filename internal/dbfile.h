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

#pragma yasc
#pragma once

/* /etc/passwd and /etc/group reader for pwd.c and grp.c (not installed) */

#include <stdbool.h>
#include <stddef.h>

struct db_file {
  int ?owned(close) fd; /* < 0: builtin text in chunk */
  size_t pos, end;
  char chunk[128];
  char *?owned(free) line;
  size_t cap;
};

/* BUILTIN stands in for a missing PATH: 0, or an errno */
int __db_open(out struct db_file *db, char *path, char *builtin);
void __db_close(inout struct db_file *db);
/* 1, 0 at the end, or -errno */
int __db_next_line(inout struct db_file *db);
/* exactly N ':' fields, split in place; false also for comments and NIS lines */
bool __db_fields(inout char *line, size_t n, inout size_t at[n], out size_t *end);
/* decimal, at most MAX */
bool __db_id(char *s, unsigned long max, out unsigned long *value);

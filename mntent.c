#include <errno.h>
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

#include <mntent.h>

#include <stdio.h>
#include <string.h>

/* fstab(5) / /proc/mounts reader. One static entry, like glibc's getmntent:
 * each call overwrites the previous result. */

static char mntent_line[256];
static struct mntent mntent_entry;

FILE *setmntent(const char *filename, const char *type) {
  return fopen(filename, type);
}

/* Undo the octal escapes fstab uses for blanks in a field: \040 (space),
 * \011 (tab), \012 (newline), \134 (backslash). In place. */
static void unescape_field(char *field) {
  char *out = field;
  for (char *in = field; *in; ++in) {
    if (in[0] == '\\' && in[1] >= '0' && in[1] <= '7' && in[2] >= '0' &&
        in[2] <= '7' && in[3] >= '0' && in[3] <= '7') {
      *out++ = (char)(((in[1] - '0') << 6) | ((in[2] - '0') << 3) | (in[3] - '0'));
      in += 3;
    } else {
      *out++ = *in;
    }
  }
  *out = '\0';
}

static char *next_field(char **cursor) {
  char *p = *cursor;
  while (*p == ' ' || *p == '\t') ++p;
  if (*p == '\0') {
    *cursor = p;
    return NULL;
  }
  char *start = p;
  while (*p && *p != ' ' && *p != '\t') ++p;
  if (*p) *p++ = '\0';
  *cursor = p;
  unescape_field(start);
  return start;
}

static int parse_number(const char *text) {
  int value = 0;
  if (!text) return 0;
  while (*text >= '0' && *text <= '9') value = value * 10 + (*text++ - '0');
  return value;
}

struct mntent *getmntent(FILE *stream) {
  while (fgets(mntent_line, sizeof(mntent_line), stream)) {
    size_t len = strlen(mntent_line);
    while (len && (mntent_line[len - 1] == '\n' || mntent_line[len - 1] == '\r'))
      mntent_line[--len] = '\0';
    char *cursor = mntent_line;
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    if (*cursor == '\0' || *cursor == '#') continue;

    char *fsname = next_field(&cursor);
    char *dir = next_field(&cursor);
    char *type = next_field(&cursor);
    char *opts = next_field(&cursor);
    char *freq = next_field(&cursor);
    char *passno = next_field(&cursor);
    if (!fsname || !dir) continue; /* malformed: skip, as glibc does */

    mntent_entry.mnt_fsname = fsname;
    mntent_entry.mnt_dir = dir;
    mntent_entry.mnt_type = type ? type : "auto";
    mntent_entry.mnt_opts = opts ? opts : "defaults";
    mntent_entry.mnt_freq = parse_number(freq);
    mntent_entry.mnt_passno = parse_number(passno);
    return &mntent_entry;
  }
  return NULL;
}

static void write_escaped(FILE *stream, const char *field) {
  for (; *field; ++field) {
    switch (*field) {
    case ' ': fputs("\\040", stream); break;
    case '\t': fputs("\\011", stream); break;
    case '\n': fputs("\\012", stream); break;
    case '\\': fputs("\\134", stream); break;
    default: fputc(*field, stream); break;
    }
  }
}

int addmntent(FILE *stream, const struct mntent *mnt) {
  if (fseek(stream, 0, SEEK_END) != 0) return 1;
  write_escaped(stream, mnt->mnt_fsname);
  fputc(' ', stream);
  write_escaped(stream, mnt->mnt_dir);
  fputc(' ', stream);
  write_escaped(stream, mnt->mnt_type);
  fputc(' ', stream);
  write_escaped(stream, mnt->mnt_opts);
  fprintf(stream, " %d %d\n", mnt->mnt_freq, mnt->mnt_passno);
  return ferror(stream) ? 1 : 0;
}

int endmntent(FILE *streamp) {
  if (streamp) fclose(streamp);
  return 1; /* always 1, per the man page */
}

/* The option `opt` in the comma-separated list, as a whole word ("ro" does not
 * match "errors=remount-ro"); a match may carry "=value". */
char *hasmntopt(const struct mntent *mnt, const char *opt) {
  size_t len = strlen(opt);
  char *p = mnt->mnt_opts;
  while (p && *p) {
    if (strncmp(p, opt, len) == 0 && (p[len] == '\0' || p[len] == ',' || p[len] == '='))
      return p;
    p = (char *)strchr(p, ',');
    if (p) ++p;
  }
  return NULL;
}

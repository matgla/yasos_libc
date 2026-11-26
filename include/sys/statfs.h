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

#include <sys/types.h>

struct statfs;

int statfs(char *path, struct statfs *buf);
int fstatfs(int fd, struct statfs *buf);

typedef long __fsword_t;
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
typedef long fsid_t;

struct statfs {
  __fsword_t f_type;    /* Type of filesystem (see below) */
  __fsword_t f_bsize;   /* Optimal transfer block size */
  fsblkcnt_t f_blocks;  /* Total data blocks in filesystem */
  fsblkcnt_t f_bfree;   /* Free blocks in filesystem */
  fsblkcnt_t f_bavail;  /* Free blocks available to
                           unprivileged user */
  fsfilcnt_t f_files;   /* Total inodes in filesystem */
  fsfilcnt_t f_ffree;   /* Free inodes in filesystem */
  fsid_t f_fsid;        /* Filesystem ID */
  __fsword_t f_namelen; /* Maximum length of filenames */
  __fsword_t f_frsize;  /* Fragment size (since Linux 2.6) */
  __fsword_t f_flags;   /* Mount flags of filesystem
                           (since Linux 2.6.36) */
};
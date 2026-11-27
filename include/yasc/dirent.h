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

#include <sys/_attrs.h>

typedef struct __dirent_dir DIR;

#define DT_BLK 1
#define DT_CHR 2
#define DT_DIR 3
#define DT_FIFO 4
#define DT_LNK 5
#define DT_REG 6
#define DT_SOCK 7
#define DT_UNKNOWN 0

struct dirent {
  unsigned long d_ino;
  unsigned long d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[256];
};

/* closedir first: opendir names it as its deallocator */
int closedir(drop DIR *?owned dir);
DIR *?owned(closedir) opendir(char *path);
/* on success the DIR took FD over (closedir closes it); on null FD is still the caller's */
DIR *?@(owned(closedir), takes(fd)) fdopendir(int owned(close) fd);
/* null at the end of the directory or on an error */
struct dirent *?borrows(dir) readdir(inout DIR *dir);

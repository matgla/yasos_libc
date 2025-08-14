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

#include <stdint.h>

int inotify_init(void);
int inotify_init1(int flags);
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);

#define IN_ACCESS 0x00000001
#define IN_ATTRIB 0x00000002
#define IN_CLOSE_WRITE 0x00000004
#define IN_CLOSE_NOWRITE 0x00000008
#define IN_CREATE 0x00000010
#define IN_DELETE 0x00000020
#define IN_DELETE_SELF 0x00000040
#define IN_MODIFY 0x00000080
#define IN_MOVE_SELF 0x00000100
#define IN_MOVED_FROM 0x00000200
#define IN_MOVED_TO 0x00000400
#define IN_OPEN 0x00000800

struct inotify_event {
  int wd;          /* Watch descriptor */
  uint32_t mask;   /* Mask describing event */
  uint32_t cookie; /* Unique cookie associating related
                      events (for rename(2)) */
  uint32_t len;    /* Size of name field */
  char name[];     /* Optional null-terminated name */
};
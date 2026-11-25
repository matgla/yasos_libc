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

/* yasc declarations for <fcntl.h> (how and why: sys/_attrs.h): what opens a
 * file descriptor gives an owned handle, invalid (< 0) on failure. */

#ifndef _SYS_YASC_FCNTL_H
#define _SYS_YASC_FCNTL_H

#include <sys/_attrs.h>

#ifdef __YASC__
#pragma yasc

int owned(close) maybe open(const char * view path, int flags, ...);
int owned(close) maybe creat(const char * view path, int mode);
/* dirfd may be AT_FDCWD */
int owned(close) maybe openat(int view maybe dirfd, const char * view pathname, int flags, ...);
#endif /* __YASC__ */

#endif /* _SYS_YASC_FCNTL_H */

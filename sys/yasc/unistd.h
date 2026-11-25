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

/* yasc declarations for <unistd.h> (how and why: sys/_attrs.h): close releases
 * an owned handle; the descriptors read and written are borrowed. */

#ifndef _SYS_YASC_UNISTD_H
#define _SYS_YASC_UNISTD_H

#include <sys/_attrs.h>
#include <sys/types.h>

#ifdef __YASC__
#pragma yasc

/* an invalid handle is fine (EBADF), as free(NULL) is */
int close(int drop owned(close) maybe fd);
/* buf may be null with n == 0 */
ssize_t read(int view fd, out void * view maybe buf, size_t n);
ssize_t write(int view fd, const void * view maybe buf, size_t n);
int owned(close) maybe dup(int view fd);
/* both written on success (0) only */
int pipe(out int owned(close) maybe * view fds);
int pipe2(out int owned(close) maybe * view fds, int flags);
/* The result is FD2 again (or -1): a number the caller picked and already
   accounts for -- usually 0, 1 or 2, which no variable owns -- so it is not an
   owned handle, or every dup2(p[1], 1) would leak. FD is only borrowed. */
int dup2(int view fd, int fd2);
/* null when there is no login name (here always "root", but code written for
   any libc tests it) */
char * borrows(static) maybe getlogin(void);
#endif /* __YASC__ */

#endif /* _SYS_YASC_UNISTD_H */

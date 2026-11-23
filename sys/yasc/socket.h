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

/* yasc declarations for <sys/socket.h> (how and why: sys/_attrs.h): a socket
 * is an owned handle, invalid (< 0) on failure. */

#ifndef _SYS_YASC_SOCKET_H
#define _SYS_YASC_SOCKET_H

#include <sys/_attrs.h>

#ifdef __YASC__
#pragma yasc

int owned(close) maybe socket(int domain, int type, int protocol);
/* a new connection, which the listening socket does not own: closing one
   leaves the other open */
int owned(close) maybe accept(int view sockfd, struct sockaddr * view maybe addr, socklen_t * view maybe addrlen);
#endif /* __YASC__ */

#endif /* _SYS_YASC_SOCKET_H */

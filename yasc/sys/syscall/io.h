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

#pragma yasc
#pragma once

/* Open descriptors: read, write, seek, control, poll, pipe, dup, truncate. */

#include <poll.h>

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

typedef struct read_context {
  int fd;
  void * view maybe buf;
  size_t count;
  ssize_t * view result;
} read_context;

typedef struct write_context {
  int fd;
  const void * view maybe buf;
  size_t count;
  ssize_t * view result;
} write_context;

typedef struct lseek_context {
  int fd;
  off_t offset;
  int whence;
  off_t * view result;
} lseek_context;

typedef struct lseek64_context {
  int fd;
  long long offset;
  int whence;
  long long * view result;
} lseek64_context;

typedef struct ioctl_context {
  int fd;
  int op;
  ssize_t arg; // int or void* //
} ioctl_context;

typedef struct fcntl_context {
  int fd;
  int op;
  ssize_t arg; // int or flock* or void*
} fcntl_context;

/* `fds` stays in user memory: the kernel validates it and writes `revents`
 * back through it in place, the same shape `sys_read` uses for its buffer. */
typedef struct poll_context {
  struct pollfd * view maybe fds;
  nfds_t nfds;
  int timeout;
} poll_context;

typedef struct pipe_context {
  /* Where the kernel puts the two descriptors: [0] read end, [1] write end. */
  int * view maybe fds;
  int flags;
} pipe_context;

typedef struct dup_context {
  int fd;
  int newfd;
} dup_context;

typedef struct ftruncate_context {
  int fd;
  off_t length;
} ftruncate_context;

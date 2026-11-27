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

/* Written by `tcc -yasc-c-header` from yasc/sys/syscall/proc.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "../../yasc/sys/syscall/proc.h"
#else
#pragma once

/* Processes: signals, waiting, exec, vfork, ptrace, resource limits. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

struct rlimit;

typedef struct kill_context {
  pid_t pid;
  int sig;
} kill_context;

/* sigaction(2), as much of it as the kernel keeps: what a signal does to the
 * process -- 0 its default action, 1 nothing (SIG_IGN), 2 run a handler, which
 * on YasOS means nothing either: there are no handlers to deliver to.  The
 * previous disposition comes back through `old` when it is not NULL; a
 * negative `disposition` only asks. */
typedef struct sigaction_context {
  int sig;
  int disposition;
  int *old __yl_view;
} sigaction_context;

typedef struct waitpid_context {
  pid_t pid;
  int *status __yl_view_maybe;
  int options;
} waitpid_context;

typedef struct execve_context {
  const char *filename __yl_view_maybe;
  char **const argv __yl_view_maybe;
  char **const envp __yl_view_maybe;
} execve_context;

typedef struct prlimit_context {
  pid_t pid;
  int resource;
  const struct rlimit *new_limit __yl_view_maybe;
  struct rlimit *old_limit __yl_view_maybe;
} prlimit_context;

/* user and group ids: ENOSYS until the kernel keeps them */
typedef struct setgid_context {
  gid_t gid;
} setgid_context;

typedef struct getgroups_context {
  int size; /* 0: count only */
  gid_t *list __yl_view_maybe;
} getgroups_context;

typedef struct setgroups_context {
  size_t size;
  const gid_t *list __yl_view_maybe;
} setgroups_context;

typedef struct setuid_context {
  uid_t uid;
} setuid_context;

/* Registers and ptrace's arguments are raw words, not objects. */
typedef struct vfork_context {
  pid_t *pid __yl_view;
  void *lr;
  void *r9;
  void *sp;
  uint32_t is_fpu_used;
} vfork_context;

/* ptrace(2): the request and its three arguments, unused ones whatever the
 * caller passed (PTRACE_TRACEME takes none). */
typedef struct ptrace_context {
  int request;
  pid_t pid;
  void *addr;
  void *data;
} ptrace_context;
#endif /* __YASC__ */

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

/* Written by `tcc -yasc-c-header` from yasc/sys/syscall/core.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "../../yasc/sys/syscall/core.h"
#else
#pragma once

/* What every system call needs: the SystemCall numbers, the result pair and
 * trigger_syscall.  The argument contexts are in the other headers next to this
 * one, each of which includes this. */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/* Relative in C: the libc unit tests build this with the host's gcc and no -I
 * for libc.  A yasc unit is always tcc's, with libc on its include path. */
#include "../_attrs.h"
#include "../syscall_ids.h"

typedef struct syscall_result {
  int err; // errno returned by OS
  int result;
} syscall_result;

typedef enum SystemCall {
  sys_start_root_process = 5,
  sys_stop_root_process,
  sys_create_process,
  sys_vfork,
  sys_semaphore_acquire,
  sys_semaphore_release,
  sys_getpid,
  sys_mkdir,
  sys_fstat,
  sys_isatty,
  sys_open,
  sys_close,
  sys_exit,
  sys_read,
  sys_kill,
  sys_write,
  sys_unlink,
  sys_link,
  sys_stat,
  sys_getentropy,
  sys_lseek,
  sys_wait,
  sys_times,
  sys_getdents,
  sys_ioctl,
  sys_gettimeofday,
  sys_waitpid,
  sys_execve,
  sys_nanosleep,
  sys_mmap,
  sys_munmap,
  sys_getcwd,
  sys_chdir,
  sys_time,
  sys_fcntl,
  sys_remove,
  sys_realpath,
  sys_mprotect,
  sys_dlopen,
  sys_dlclose,
  sys_dlsym,
  sys_renameat2,
  sys_geteuid,
  sys_getuid,
  sys_dup,
  sys_sysinfo,
  sys_sysconf,
  sys_access,
  sys_prlimit,
  sys_mremap,
  sys_klog_ctl,
  sys_ftruncate,
  sys_perf_dump,
  sys_pipe,
  sys_poll,
  sys_settimeofday,
  sys_utimensat,
  sys_readlink,
  sys_uname,
  sys_fchdir,
  sys_mount,
  sys_umount,
  sys_statvfs,
  sys_lseek64,
  sys_symlink,
  sys_chmod,
  sys_chown,
  sys_ptrace,
  sys_sigaction,
  sys_sync,
  sys_reboot,
  sys_getgid,
  sys_getegid,
  sys_setgid,
  sys_setegid,
  sys_getgroups,
  sys_setgroups,
  sys_setuid,
  sys_seteuid,
  SYSCALL_COUNT,
} SystemCall;

/* Tie the enum to YASOS_SYSCALL_COUNT (sys/syscall_ids.h), which the SVCall stub
 * bounds-checks against and the assembler can read where this enum cannot. A
 * mismatch is a negative array size, which every C compiler rejects. A typedef
 * rather than _Static_assert because tcc compiles this header too. */
typedef char yasos_syscall_count_is_in_sync
    [(SYSCALL_COUNT == YASOS_SYSCALL_COUNT) ? 1 : -1];

/* ARGS is the call's context, null for the calls that take none (getpid, sync) */
int trigger_syscall(int number, const void *args);

#define SYS_renameat2 sys_renameat2

long syscall(long number, ...);
#endif /* __YASC__ */

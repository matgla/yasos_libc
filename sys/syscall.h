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

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include <poll.h>

#include <stdint.h>

#include "_attrs.h"

#include "syscall_ids.h"

/* The contexts' pointers, in yasc code (sys/_attrs.h): __yl_view where libc
 * always points it at an object of its own (a result), __yl_view_maybe where it
 * passes on the caller's pointer, which may be null -- the kernel checks every
 * one.  Raw addresses and handles (mmap's addr, dlsym's handle, vfork's
 * registers) say nothing: they are not objects to dereference. */

typedef struct syscall_result {
  int err; // errno returned by OS
  int result;
} syscall_result;

typedef struct read_context {
  int fd;
  void *buf __yl_view_maybe;
  size_t count;
  ssize_t *result __yl_view;
} read_context;

typedef struct kill_context {
  pid_t pid;
  int sig;
} kill_context;

typedef struct write_context {
  int fd;
  const void *buf __yl_view_maybe;
  size_t count;
  ssize_t *result __yl_view;
} write_context;

typedef struct link_context {
  int olddirfd;
  const char *oldpath __yl_view_maybe;
  int newdirfd;
  const char *newpath __yl_view_maybe;
  int flags;
} link_context;

/* renameat2(2) flags. Same values as Linux; stdio.h repeats the definitions for
   programs that only include it. Only NOREPLACE changes what the kernel does. */
#define RENAME_NOREPLACE (1 << 0)
#define RENAME_EXCHANGE (1 << 1)
#define RENAME_WHITEOUT (1 << 2)

/* renameat2(2) arguments. */
typedef struct rename_context {
  int olddirfd;
  const char *oldpath __yl_view_maybe;
  int newdirfd;
  const char *newpath __yl_view_maybe;
  unsigned int flags;
} rename_context;

typedef struct mkdir_context {
  const char *path __yl_view_maybe;
  mode_t mode;
  int fd;
} mkdir_context;

typedef struct lseek_context {
  int fd;
  off_t offset;
  int whence;
  off_t *result __yl_view;
} lseek_context;

typedef struct getentropy_context {
  void *buffer __yl_view_maybe;
  size_t length;
} getentropy_context;

typedef struct stat_context {
  const char *pathname __yl_view_maybe;
  struct stat *statbuf __yl_view_maybe;
  int fd;
  uint8_t follow_links;
} stat_context;

typedef struct open_context {
  const char *path __yl_view_maybe;
  int flags;
  int mode;
  int fd;
} open_context;

typedef struct fstat_context {
  int fd;
  struct stat *buf __yl_view_maybe;
} fstat_context;

typedef struct getdents_context {
  int fd;
  struct dirent *dirp __yl_view_maybe;
  size_t count;
  ssize_t *result __yl_view;
} getdents_context;

typedef struct ioctl_context {
  int fd;
  int op;
  ssize_t arg; // int or void* //
} ioctl_context;

/* `fds` stays in user memory: the kernel validates it and writes `revents`
 * back through it in place, the same shape `sys_read` uses for its buffer. */
typedef struct poll_context {
  struct pollfd *fds __yl_view_maybe;
  nfds_t nfds;
  int timeout;
} poll_context;

typedef struct gettimeofday_context {
  struct timeval *tv __yl_view_maybe;
  struct timezone *tz __yl_view_maybe;
} gettimeofday_context;

/* readlink(2)/readlinkat(2). `buf` is not NUL-terminated on return, and a
 * target longer than `bufsiz` is silently truncated -- both as POSIX has it. */
typedef struct readlink_context {
  const char *pathname __yl_view_maybe;
  char *buf __yl_view_maybe;
  size_t bufsiz;
  int fd;
} readlink_context;

typedef struct settimeofday_context {
  const struct timeval *tv __yl_view_maybe;
  const struct timezone *tz __yl_view_maybe;
} settimeofday_context;

/* `times` is NULL for "now", otherwise two timespecs: [0] access, [1]
 * modification, either of which may carry UTIME_NOW or UTIME_OMIT in tv_nsec.
 *
 * Shaped like stat_context: `pathname` NULL means "the file `fd` is open on"
 * (futimens), and otherwise `fd` is the directory a relative path resolves
 * against. */
typedef struct utimensat_context {
  const char *pathname __yl_view_maybe;
  const struct timespec *times __yl_view_maybe;
  int fd;
  int flags;
} utimensat_context;

typedef struct waitpid_context {
  pid_t pid;
  int *status __yl_view_maybe;
  int options;
} waitpid_context;

typedef struct nanosleep_context {
  const struct timespec *req __yl_view_maybe;
  struct timespec *rem __yl_view_maybe;
} nanosleep_context;

typedef struct execve_context {
  const char *filename __yl_view_maybe;
  char **const argv __yl_view_maybe;
  char **const envp __yl_view_maybe;
} execve_context;

typedef struct mmap_context {
  void *addr;
  int length;
  int prot;
  int flags;
  int fd;
  int offset;
  void **result __yl_view;
} mmap_context;

typedef struct munmap_context {
  void *addr;
  int length;
} munmap_context;

typedef struct mremap_context {
  void *addr;
  int old_length;
  int new_length;
  int flags;
  void **result __yl_view;
} mremap_context;

typedef struct getcwd_context {
  char *buf __yl_view_maybe;
  size_t size;
  char **result __yl_view;
} getcwd_context;

typedef struct chdir_context {
  const char *path __yl_view_maybe;
} chdir_context;

typedef struct time_context {
  time_t *timep __yl_view_maybe;
  time_t *result __yl_view;
} time_context;

typedef struct fcntl_context {
  int fd;
  int op;
  ssize_t arg; // int or flock* or void*
} fcntl_context;

typedef struct remove_context {
  const char *pathname __yl_view_maybe;
} remove_context;

typedef struct realpath_context {
  const char *path __yl_view_maybe;
  char *resolved_path __yl_view_maybe;
} realpath_context;

typedef struct mprotect_context {
  void *addr;
  size_t length;
  int prot;
} mprotect_context;

typedef struct dlopen_context {
  const char *path __yl_view_maybe;
  int flags;
  void **result __yl_view;
} dlopen_context;

typedef struct dlclose_context {
  void *handle;
} dlclose_context;

typedef struct dlsym_context {
  void *handle;
  const char *symbol __yl_view_maybe;
  void **result __yl_view;
} dlsym_context;

typedef struct vfork_context {
  pid_t *pid __yl_view;
  void *lr;
  void *r9;
  void *sp;
  uint32_t is_fpu_used;
} vfork_context;

typedef struct dup_context {
  int fd;
  int newfd;
} dup_context;

typedef struct sysinfo_context {
  struct sysinfo *info __yl_view_maybe;
} sysinfo_context;

/* uname(2). The kernel writes the whole struct through `buf`. */
typedef struct uname_context {
  struct utsname *buf __yl_view_maybe;
} uname_context;

typedef struct fchdir_context {
  int fd;
} fchdir_context;

typedef struct mount_context {
  const char *source __yl_view_maybe;
  const char *target __yl_view_maybe;
  const char *filesystemtype __yl_view_maybe;
  unsigned long mountflags;
  /* The option string ("nofail,spill=/var/tmp"), or NULL. */
  const void *data __yl_view_maybe;
} mount_context;

typedef struct umount_context {
  const char *target __yl_view_maybe;
  int flags;
} umount_context;

typedef struct symlink_context {
  const char *target __yl_view_maybe;
  int dirfd;
  const char *linkpath __yl_view_maybe;
} symlink_context;

/* fchmodat(2); fchmod(2) is path NULL with fd the open file. */
typedef struct chmod_context {
  const char *path __yl_view_maybe;
  int fd;
  mode_t mode;
  int flags; /* AT_SYMLINK_NOFOLLOW */
} chmod_context;

/* fchownat(2); fchown(2) is path NULL with fd the open file. -1 leaves an id
 * unchanged. */
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

/* reboot(2): one of the RB_* commands from sys/reboot.h. sync(2) takes no
 * argument. */
typedef struct reboot_context {
  int cmd;
} reboot_context;

/* ptrace(2): the request and its three arguments, unused ones whatever the
 * caller passed (PTRACE_TRACEME takes none). */
typedef struct ptrace_context {
  int request;
  pid_t pid;
  void *addr;
  void *data;
} ptrace_context;

typedef struct chown_context {
  const char *path __yl_view_maybe;
  int fd;
  uid_t uid;
  gid_t gid;
  int flags; /* AT_SYMLINK_NOFOLLOW */
} chown_context;

typedef struct lseek64_context {
  int fd;
  long long offset;
  int whence;
  long long *result __yl_view;
} lseek64_context;

struct statvfs;
typedef struct statvfs_context {
  /* Path to describe, or NULL to describe the file `fd` is open on. */
  const char *path __yl_view_maybe;
  int fd;
  struct statvfs *buf __yl_view_maybe;
} statvfs_context;

typedef struct sysconf_context {
  int name;
  long *result __yl_view;
} sysconf_context;

typedef struct prlimit_context {
  pid_t pid;
  int resource;
  const struct rlimit *new_limit __yl_view_maybe;
  struct rlimit *old_limit __yl_view_maybe;
} prlimit_context;

typedef struct unlink_context {
  int dirfd;
  const char *pathname __yl_view_maybe;
  int flags;
} unlink_context;

typedef struct pipe_context {
  /* Where the kernel puts the two descriptors: [0] read end, [1] write end. */
  int *fds __yl_view_maybe;
  int flags;
} pipe_context;

typedef struct access_context {
  const char *pathname __yl_view_maybe;
  int mode;
  int dirfd;
  int flags;
} access_context;

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
  SYSCALL_COUNT,
} SystemCall;

/* Tie the enum to YASOS_SYSCALL_COUNT (sys/syscall_ids.h), which the SVCall stub
 * bounds-checks against and the assembler can read where this enum cannot. A
 * mismatch is a negative array size, which every C compiler rejects. A typedef
 * rather than _Static_assert because tcc compiles this header too. */
typedef char yasos_syscall_count_is_in_sync
    [(SYSCALL_COUNT == YASOS_SYSCALL_COUNT) ? 1 : -1];

typedef struct klog_ctl_context {
  int enable;
} klog_ctl_context;

typedef struct ftruncate_context {
  int fd;
  off_t length;
} ftruncate_context;

/* Per-syscall accounting produced by the kernel's perf profiler.
 *
 * `total_cycles` covers the whole kernel-side syscall: the SVC entry stamp
 * taken in the exception handler through the end of the handler, so it
 * includes exception entry and (for non-fast syscalls) the trampoline into
 * thread mode. `handler_cycles` covers only the handler body, so
 * total - handler is the fixed dispatch overhead. Neither includes the return
 * trampoline back to user mode.
 *
 * `bytes` is the payload actually transferred (read/write return value), so
 * IO throughput can be derived without instrumenting every filesystem. */
typedef struct perf_syscall_entry {
  unsigned int syscall_id;
  unsigned int call_count;
  unsigned long long total_cycles;
  unsigned long long handler_cycles;
  unsigned int max_cycles;
  unsigned int bytes;
} perf_syscall_entry;

typedef struct perf_dump_context {
  /* in */
  perf_syscall_entry *entries __yl_view_maybe;
  int max_entries;
  int *num_entries __yl_view_maybe;
  int reset;
  /* out, filled by the kernel */
  unsigned int cycles_per_us;
  /* Calls whose measured window was implausibly long (the 32-bit cycle counter
   * wraps every few seconds) and were therefore left out of the totals. */
  unsigned int dropped;
  /* Time this process spent being loaded and relocated by the dynamic loader,
   * i.e. before main() ran. 0 when the process was never exec'd. */
  unsigned long long load_us;
} perf_dump_context;

int trigger_syscall(int number, const void *args);

#define SYS_renameat2 sys_renameat2

long syscall(long number, ...);

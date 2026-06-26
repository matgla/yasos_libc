/*
 Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "sys/perf.h"
#include "sys/syscall.h"

#include <stdio.h>

int perf_dump(perf_syscall_entry *entries, int max_entries, int *num_entries, int reset) {
  perf_dump_context context = {
      .entries = entries,
      .max_entries = max_entries,
      .num_entries = num_entries,
      .reset = reset,
  };
  return trigger_syscall(sys_perf_dump, &context);
}

static const char *syscall_name(unsigned int id) {
  switch (id) {
    case sys_open: return "open";
    case sys_close: return "close";
    case sys_read: return "read";
    case sys_write: return "write";
    case sys_fstat: return "fstat";
    case sys_stat: return "stat";
    case sys_lseek: return "lseek";
    case sys_mmap: return "mmap";
    case sys_munmap: return "munmap";
    case sys_mremap: return "mremap";
    case sys_access: return "access";
    case sys_getcwd: return "getcwd";
    case sys_realpath: return "realpath";
    case sys_fcntl: return "fcntl";
    case sys_execve: return "execve";
    case sys_exit: return "exit";
    case sys_vfork: return "vfork";
    case sys_waitpid: return "waitpid";
    case sys_getpid: return "getpid";
    case sys_dlopen: return "dlopen";
    case sys_dlclose: return "dlclose";
    case sys_dlsym: return "dlsym";
    case sys_times: return "times";
    case sys_ioctl: return "ioctl";
    case sys_mkdir: return "mkdir";
    case sys_unlink: return "unlink";
    case sys_isatty: return "isatty";
    case sys_getdents: return "getdents";
    case sys_gettimeofday: return "gettimeofday";
    case sys_nanosleep: return "nanosleep";
    case sys_chdir: return "chdir";
    case sys_time: return "time";
    case sys_mprotect: return "mprotect";
    case sys_dup: return "dup";
    case sys_ftruncate: return "ftruncate";
    default: return NULL;
  }
}

void perf_dump_print(int reset) {
  perf_syscall_entry entries[64];
  int num = 0;
  if (perf_dump(entries, 64, &num, reset) != 0) {
    fprintf(stderr, "perf_dump: syscall failed\n");
    return;
  }
  if (num == 0) {
    fprintf(stderr, "# perf: no syscall data (profiling disabled?)\n");
    return;
  }
  fprintf(stderr, "# perf: syscall profile (%d entries)%s\n", num, reset ? " [reset]" : "");
  fprintf(stderr, "# %-16s %8s %12s %12s %12s\n", "syscall", "calls", "total_cyc", "max_cyc", "avg_cyc");
  for (int i = 0; i < num; i++) {
    const char *name = syscall_name(entries[i].syscall_id);
    unsigned int avg = entries[i].call_count ? entries[i].total_cycles / entries[i].call_count : 0;
    if (name) {
      fprintf(stderr, "# %-16s %8u %12u %12u %12u\n",
              name, entries[i].call_count, entries[i].total_cycles, entries[i].max_cycles, avg);
    } else {
      fprintf(stderr, "# syscall_%-7u %8u %12u %12u %12u\n",
              entries[i].syscall_id, entries[i].call_count, entries[i].total_cycles, entries[i].max_cycles, avg);
    }
  }
}

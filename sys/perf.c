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

int perf_dump_ctx(perf_dump_context *context) {
  context->cycles_per_us = 0;
  context->dropped = 0;
  context->load_us = 0;
  return trigger_syscall(sys_perf_dump, context);
}

int perf_dump(perf_syscall_entry *entries, int max_entries, int *num_entries, int reset) {
  perf_dump_context context = {
      .entries = entries,
      .max_entries = max_entries,
      .num_entries = num_entries,
      .reset = reset,
  };
  return perf_dump_ctx(&context);
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

#define PERF_MAX_ENTRIES 64

/* Cycles -> microseconds. cycles_per_us comes from the kernel (it knows
   clk_sys), so this stays correct across the board's clock settings. */
static unsigned long long cycles_to_us(unsigned long long cycles,
                                       unsigned int cycles_per_us) {
  if (cycles_per_us == 0) {
    return 0;
  }
  return cycles / cycles_per_us;
}

void perf_dump_print(int reset) {
  perf_syscall_entry entries[PERF_MAX_ENTRIES];
  int num = 0;
  perf_dump_context context = {
      .entries = entries,
      .max_entries = PERF_MAX_ENTRIES,
      .num_entries = &num,
      .reset = reset,
  };
  if (perf_dump_ctx(&context) != 0) {
    fprintf(stderr, "perf_dump: syscall failed\n");
    return;
  }
  if (num == 0) {
    fprintf(stderr, "# perf: no syscall data (profiling disabled?)\n");
    return;
  }
  fprintf(stderr, "# perf: syscall profile (%d entries)%s\n", num, reset ? " [reset]" : "");
  fprintf(stderr, "# %-16s %8s %12s %12s %12s %12s\n", "syscall", "calls", "total_us",
          "handler_us", "max_us", "bytes");
  for (int i = 0; i < num; i++) {
    const char *name = syscall_name(entries[i].syscall_id);
    char fallback[16];
    if (name == NULL) {
      snprintf(fallback, sizeof(fallback), "syscall_%u", entries[i].syscall_id);
      name = fallback;
    }
    fprintf(stderr, "# %-16s %8u %12llu %12llu %12llu %12u\n", name,
            entries[i].call_count,
            cycles_to_us(entries[i].total_cycles, context.cycles_per_us),
            cycles_to_us(entries[i].handler_cycles, context.cycles_per_us),
            cycles_to_us(entries[i].max_cycles, context.cycles_per_us),
            entries[i].bytes);
  }
}

/* How many per-syscall entries the compact form spells out; the rest are
   folded into `other=`. Every extra entry is serial time billed to whoever is
   being measured, so this stays small. */
#define PERF_COMPACT_TOP 10

void perf_dump_print_compact(int reset) {
  perf_syscall_entry entries[PERF_MAX_ENTRIES];
  int num = 0;
  perf_dump_context context = {
      .entries = entries,
      .max_entries = PERF_MAX_ENTRIES,
      .num_entries = &num,
      .reset = reset,
  };
  if (perf_dump_ctx(&context) != 0) {
    fprintf(stderr, "# perf: dump failed\n");
    return;
  }

  unsigned int calls = 0;
  unsigned long long total_cycles = 0;
  unsigned long long handler_cycles = 0;
  unsigned long long read_cycles = 0;
  unsigned long long write_cycles = 0;
  unsigned int read_bytes = 0;
  unsigned int write_bytes = 0;
  for (int i = 0; i < num; i++) {
    calls += entries[i].call_count;
    total_cycles += entries[i].total_cycles;
    handler_cycles += entries[i].handler_cycles;
    if (entries[i].syscall_id == sys_read) {
      read_cycles = entries[i].total_cycles;
      read_bytes = entries[i].bytes;
    } else if (entries[i].syscall_id == sys_write) {
      write_cycles = entries[i].total_cycles;
      write_bytes = entries[i].bytes;
    }
  }

  fprintf(stderr,
          "# perf: sys calls=%u us=%llu handler_us=%llu load_us=%llu dropped=%u\n",
          calls, cycles_to_us(total_cycles, context.cycles_per_us),
          cycles_to_us(handler_cycles, context.cycles_per_us), context.load_us,
          context.dropped);
  fprintf(stderr, "# perf: io read=%u/%llu write=%u/%llu\n", read_bytes,
          cycles_to_us(read_cycles, context.cycles_per_us), write_bytes,
          cycles_to_us(write_cycles, context.cycles_per_us));

  if (num == 0) {
    fprintf(stderr, "# perf: top (none)\n");
    return;
  }

  /* Selection sort by total cycles: `num` is bounded by the syscall count and
     only the first PERF_COMPACT_TOP positions are needed. */
  int order[PERF_MAX_ENTRIES];
  for (int i = 0; i < num; i++) {
    order[i] = i;
  }
  int shown = num < PERF_COMPACT_TOP ? num : PERF_COMPACT_TOP;
  for (int i = 0; i < shown; i++) {
    int best = i;
    for (int j = i + 1; j < num; j++) {
      if (entries[order[j]].total_cycles > entries[order[best]].total_cycles) {
        best = j;
      }
    }
    int tmp = order[i];
    order[i] = order[best];
    order[best] = tmp;
  }

  fprintf(stderr, "# perf: top");
  unsigned long long shown_cycles = 0;
  unsigned int shown_calls = 0;
  for (int i = 0; i < shown; i++) {
    const perf_syscall_entry *entry = &entries[order[i]];
    const char *name = syscall_name(entry->syscall_id);
    shown_cycles += entry->total_cycles;
    shown_calls += entry->call_count;
    if (name) {
      fprintf(stderr, " %s=%u/%llu", name, entry->call_count,
              cycles_to_us(entry->total_cycles, context.cycles_per_us));
    } else {
      fprintf(stderr, " syscall_%u=%u/%llu", entry->syscall_id, entry->call_count,
              cycles_to_us(entry->total_cycles, context.cycles_per_us));
    }
  }
  if (shown < num) {
    fprintf(stderr, " other=%u/%llu", calls - shown_calls,
            cycles_to_us(total_cycles - shown_cycles, context.cycles_per_us));
  }
  fprintf(stderr, "\n");
}

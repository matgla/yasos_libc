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

/* The kernel log and the syscall profiler. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

typedef struct klog_ctl_context {
  int enable;
} klog_ctl_context;

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
  perf_syscall_entry *?entries;
  int max_entries;
  int *?num_entries;
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

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

/* ptrace(2), with Linux's request numbers and register layouts.
 *
 * A tracee stops, and its tracer's waitpid reports WIFSTOPPED with:
 *   SIGTRAP  at exec (before the first instruction) and at a breakpoint, which
 *            is the Thumb instruction UDF #1 (0xde01, YASOS_PTRACE_BREAKPOINT);
 *            pc is the breakpoint's address
 *   SIGILL, SIGSEGV, SIGBUS, SIGFPE  at a fault, instead of being killed
 *   SIGSTOP  after PTRACE_ATTACH or PTRACE_INTERRUPT
 *
 * A traced process runs its executable from RAM, even from the XIP romfs, so
 * POKETEXT can plant breakpoints in it; code still in flash (its shared
 * libraries) cannot be written.  PTRACE_ATTACH is limited to descendants of
 * the caller.  There are no signal handlers: a signal passed to PTRACE_CONT or
 * PTRACE_DETACH kills the tracee with it, and PTRACE_KILL is SIGKILL.
 * PTRACE_SINGLESTEP is not there (ENOSYS); a debugger steps with breakpoints.
 * A tracee blocked in a system call stops for PTRACE_ATTACH/INTERRUPT only
 * once that call returns. */

#pragma once

#include <sys/types.h>

enum __ptrace_request {
  PTRACE_TRACEME = 0,
  PTRACE_PEEKTEXT = 1,
  PTRACE_PEEKDATA = 2,
  PTRACE_PEEKUSER = 3,
  PTRACE_POKETEXT = 4,
  PTRACE_POKEDATA = 5,
  PTRACE_POKEUSER = 6,
  PTRACE_CONT = 7,
  PTRACE_KILL = 8,
  PTRACE_SINGLESTEP = 9,
  PTRACE_GETREGS = 12,
  PTRACE_SETREGS = 13,
  PTRACE_ATTACH = 16,
  PTRACE_DETACH = 17,
  PTRACE_GETVFPREGS = 27,
  PTRACE_SETVFPREGS = 28,
  PTRACE_INTERRUPT = 0x4207,
};

/* The breakpoint instruction: UDF #1, a 16-bit Thumb encoding. */
#define YASOS_PTRACE_BREAKPOINT 0xde01

/* PTRACE_GETREGS / SETREGS, and PEEKUSER / POKEUSER at byte offsets into it:
 * r0-r12, sp, lr, pc, xpsr and orig_r0 (always 0 here), as on Linux/ARM.
 * Writing pc keeps the Thumb state; writing sp moves the stopped frame. */
struct user_regs {
  unsigned long uregs[18];
};

/* PTRACE_GETVFPREGS / SETVFPREGS: d0-d15 (s0-s31) of the FPv5 single
 * precision unit, d16-d31 always 0, and FPSCR.  A process with no FP context
 * reads all zeros, and SETVFPREGS on it fails with EIO. */
struct user_vfp {
  unsigned long long fpregs[32];
  unsigned long fpscr;
};

/* PEEKTEXT, PEEKDATA and PEEKUSER return the word read: clear errno first to
 * tell a word of -1 from an error, as on Linux. */
long ptrace(int request, ...);

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

/* Written by `tcc -yasc-c-header` from yasc/sys/syscall.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "../yasc/sys/syscall.h"
#else
#pragma once

/* The system call interface: the SystemCall numbers and trigger_syscall
 * (syscall/core.h), and one header per group of argument contexts.  A unit
 * that needs one group includes it alone (<sys/syscall/fs.h>); this header is
 * all of them, for the kernel's translate-c (source/libc_imports.h) and
 * programs that want the lot.
 *
 * The contexts' pointers: `view` where libc always points it at an object of
 * its own (a result), `*?view` where it passes on the caller's pointer,
 * which may be null -- the kernel checks every one.  Raw addresses and handles
 * (mmap's addr, dlsym's handle, vfork's registers, ptrace's arguments) are not
 * objects to dereference and yasc has no word for them, so their contexts are
 * in C sections and say nothing. */

#include "syscall/core.h"

#include "syscall/io.h"
#include "syscall/fs.h"
#include "syscall/proc.h"
#include "syscall/mem.h"
#include "syscall/clock.h"
#include "syscall/dl.h"
#include "syscall/system.h"
#include "syscall/diag.h"
#endif /* __YASC__ */

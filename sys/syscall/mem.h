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

/* Written by `tcc -yasc-c-header` from yasc/sys/syscall/mem.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "../../yasc/sys/syscall/mem.h"
#else
#pragma once

/* Mappings.  Every context carries an address that is not an object (a hint,
 * a mapping's start), so they are all C. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

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

typedef struct mprotect_context {
  void *addr;
  size_t length;
  int prot;
} mprotect_context;
#endif /* __YASC__ */

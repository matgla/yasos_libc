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

/* Written by `tcc -yasc-c-header` from yasc/sys/syscall/system.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "../../yasc/sys/syscall/system.h"
#else
#pragma once

/* The system as a whole: its name, load, configuration, entropy, reboot. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

struct sysinfo;
struct utsname;

typedef struct sysinfo_context {
  struct sysinfo *info __yl_view_maybe;
} sysinfo_context;

/* uname(2). The kernel writes the whole struct through `buf`. */
typedef struct uname_context {
  struct utsname *buf __yl_view_maybe;
} uname_context;

typedef struct sysconf_context {
  int name;
  long *result __yl_view;
} sysconf_context;

typedef struct getentropy_context {
  void *buffer __yl_view_maybe;
  size_t length;
} getentropy_context;

/* reboot(2): one of the RB_* commands from sys/reboot.h. sync(2) takes no
 * argument. */
typedef struct reboot_context {
  int cmd;
} reboot_context;
#endif /* __YASC__ */

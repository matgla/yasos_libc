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

/* Written by `tcc -yasc-c-header` from yasc/sys/syscall/clock.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "../../yasc/sys/syscall/clock.h"
#else
#pragma once

/* Time of day, the clock and sleeping. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

struct timeval;
struct timezone;
struct timespec;

typedef struct gettimeofday_context {
  struct timeval *tv __yl_view_maybe;
  struct timezone *tz __yl_view_maybe;
} gettimeofday_context;

typedef struct settimeofday_context {
  const struct timeval *tv __yl_view_maybe;
  const struct timezone *tz __yl_view_maybe;
} settimeofday_context;

typedef struct nanosleep_context {
  const struct timespec *req __yl_view_maybe;
  struct timespec *rem __yl_view_maybe;
} nanosleep_context;

typedef struct time_context {
  time_t *timep __yl_view_maybe;
  time_t *result __yl_view;
} time_context;
#endif /* __YASC__ */

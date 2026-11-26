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

/* Time of day, the clock and sleeping. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

struct timeval;
struct timezone;
struct timespec;

typedef struct gettimeofday_context {
  struct timeval * view maybe tv;
  struct timezone * view maybe tz;
} gettimeofday_context;

typedef struct settimeofday_context {
  const struct timeval * view maybe tv;
  const struct timezone * view maybe tz;
} settimeofday_context;

typedef struct nanosleep_context {
  const struct timespec * view maybe req;
  struct timespec * view maybe rem;
} nanosleep_context;

typedef struct time_context {
  time_t * view maybe timep;
  time_t * view result;
} time_context;

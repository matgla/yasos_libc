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

/* The dynamic loader. */

#include "core.h" /* SystemCall, trigger_syscall, sys/_attrs.h */

typedef struct dlopen_context {
  const char *?path;
  int flags;
  void *?view *result;
} dlopen_context;

/* A handle is the loader's, not an object to dereference. */
#pragma tcc c_begin
typedef struct dlclose_context {
  void *handle;
} dlclose_context;

typedef struct dlsym_context {
  const void *handle; /* RTLD_DEFAULT (null) or a dlopen handle; only read */
  const char *symbol __yl_view_maybe;
  void **result __yl_view;
} dlsym_context;
#pragma tcc c_end

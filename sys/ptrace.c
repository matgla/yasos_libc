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

#include <stdarg.h>

#include "sys/ptrace.h"
#include "sys/syscall.h"

long ptrace(int request, ...) {
  /* One statement each: an initializer list evaluates in no set order. */
  ptrace_context context = {.request = request};
  va_list args;
  va_start(args, request);
  context.pid = va_arg(args, pid_t);
  context.addr = va_arg(args, void *);
  context.data = va_arg(args, void *);
  va_end(args);
  /* The kernel stores a peeked word through `data`, like Linux's raw call;
   * the library call returns it. */
  if (request == PTRACE_PEEKTEXT || request == PTRACE_PEEKDATA || request == PTRACE_PEEKUSER) {
    long word = 0;
    context.data = &word;
    if (trigger_syscall(sys_ptrace, &context) < 0)
      return -1;
    return word;
  }
  return trigger_syscall(sys_ptrace, &context);
}

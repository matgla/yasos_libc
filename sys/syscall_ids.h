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

#pragma once

/* Number of system calls, i.e. the value of the SystemCall enum's trailing
 * SYSCALL_COUNT member in sys/syscall.h.
 *
 * It is spelled out here, in the one header the ARMv8-M SVCall stub includes,
 * because that stub bounds-checks the incoming syscall number itself
 * (process_syscall_fast_check in source/arch/armv8-m/context_switch.S) and the
 * assembler cannot see a C enum. sys/syscall.h checks the two against each
 * other at compile time, so adding a syscall without updating this fails the
 * build rather than silently leaving the new call unreachable by the fast
 * path -- or, worse, letting the stub index past the table. */
#define YASOS_SYSCALL_COUNT 58


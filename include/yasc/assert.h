#pragma yasc
#pragma tcc c_begin
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

/* No include guard for assert itself: every inclusion defines it again from
   the NDEBUG in force there (C 7.2), so a file can turn its asserts off and
   back on. */

#ifndef _ASSERT_H_DECLS
#define _ASSERT_H_DECLS

#include <stdbool.h>
#include <sys/_attrs.h>

/* Prints "FILE:LINE: FUNC: Assertion `EXPR' failed." to the standard error
   and abort()s. */
#pragma tcc c_end
__yl_noreturn void __assert_fail(const char *expr, const char *file, unsigned int line,
                                 const char *func);
#pragma tcc c_begin

/* C11 spells it _Static_assert and leaves the short name to this header; C23
   and C++ make static_assert a keyword of their own. */
#if !defined(__cplusplus) && !defined(static_assert) &&                        \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
#define static_assert _Static_assert
#endif

#endif /* _ASSERT_H_DECLS */

#undef assert
/* Variadic as in C23, so a comma inside the expression -- a compound literal,
   a macro's arguments -- is no second argument. */
#ifdef NDEBUG
#define assert(...) ((void)0)
#else
#define assert(...)                                                            \
  ((__VA_ARGS__) ? (void)0 : __assert_fail(#__VA_ARGS__, __FILE__, __LINE__, __func__))
#endif
#pragma tcc c_end

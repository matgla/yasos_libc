// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
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

/* The BSD <sys/cdefs.h> shorthands that newlib and glibc both provide and
 * code written against either takes for granted -- the pico-sdk's compiler.h
 * includes it for __packed, __unused, __aligned and friends. Definitions match
 * newlib's, so code that used to see those sees the same text here. */

#ifndef _SYS_CDEFS_H
#define _SYS_CDEFS_H

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#define __CONCAT1(x, y) x##y
#define __CONCAT(x, y) __CONCAT1(x, y)
#define __STRING(x) #x
#define __XSTRING(x) __STRING(x)

#define __aligned(x) __attribute__((__aligned__(x)))
#define __packed __attribute__((__packed__))
#define __section(x) __attribute__((__section__(x)))
#define __unused __attribute__((__unused__))
#define __used __attribute__((__used__))
#define __always_inline __inline__ __attribute__((__always_inline__))
#define __noinline __attribute__((__noinline__))
#define __dead2 __attribute__((__noreturn__))
#define __pure2 __attribute__((__const__))
#define __printflike(fmtarg, firstvararg)                                      \
  __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#define __scanflike(fmtarg, firstvararg)                                       \
  __attribute__((__format__(__scanf__, fmtarg, firstvararg)))

#define __predict_true(exp) __builtin_expect(!!(exp), 1)
#define __predict_false(exp) __builtin_expect(!!(exp), 0)

#endif /* _SYS_CDEFS_H */

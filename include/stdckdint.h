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

/* C23 checked integer arithmetic: ckd_add(&r, a, b) stores a + b, wrapped to
 * r's type, and returns true when the exact value does not fit there. A, B
 * and *R may be any integer types but bool, plain char and enumerations.
 *
 * tcc, gcc and clang have the __builtin_*_overflow these are, so there is no
 * portable fallback. tcc implements them although its __has_builtin answers
 * no (tccpp.c, tcc_builtins_supported: a code-size choice for zig.h), which
 * is why this tests the compiler, not __has_builtin. */

#ifndef _STDCKDINT_H
#define _STDCKDINT_H

#define __STDC_VERSION_STDCKDINT_H__ 202311L

#if defined(__TINYC__) || defined(__GNUC__)
#define ckd_add(R, A, B) ((_Bool)__builtin_add_overflow((A), (B), (R)))
#define ckd_sub(R, A, B) ((_Bool)__builtin_sub_overflow((A), (B), (R)))
#define ckd_mul(R, A, B) ((_Bool)__builtin_mul_overflow((A), (B), (R)))
#else
#error "<stdckdint.h> needs __builtin_add_overflow and friends"
#endif

#endif /* _STDCKDINT_H */

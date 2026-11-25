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

/* _FORTIFY_SOURCE: is it on, and how big is the object a pointer points to.
 *
 * A program built with -D_FORTIFY_SOURCE=1 (or 2, 3) gets <string.h> and
 * <stdio.h> copies that know the size of their destination: memcpy(d, s, n)
 * becomes __builtin___memcpy_chk(d, s, n, __yl_bos0(d)).  The compiler calls
 * plain memcpy when it can prove the copy fits (or knows nothing of d), warns
 * when it can prove it does not, and otherwise calls __memcpy_chk (fortify.c),
 * which checks at run time and ends the program through __chk_fail with
 * "*** buffer overflow detected ***: terminated".
 *
 * Macros, not inline functions: tcc's __builtin_object_size never looks
 * through a parameter (not even in an inline expansion), so the size has to
 * be asked where the caller names the buffer.  The pointer is written twice,
 * but __builtin_object_size does not evaluate its argument.  `(memcpy)(...)`
 * and `&memcpy` still name the function.
 *
 * Levels, as glibc's: 1 the whole object for every function; 2 the closest
 * member for the string and printf functions (mem* keep the whole object);
 * 3 as 2 with __builtin_dynamic_object_size, which also knows malloc(n) and
 * VLAs at run time.  tcc's answers do not depend on -O, so neither does this
 * (glibc's needs optimisation).  libc itself never defines _FORTIFY_SOURCE:
 * the Makefile undefines it, its own sources stay as they are. */

#ifndef _SYS__FORTIFY_H
#define _SYS__FORTIFY_H

#if defined(_FORTIFY_SOURCE) && defined(__has_builtin) && !defined(__cplusplus)
#if _FORTIFY_SOURCE > 0 && __has_builtin(__builtin_object_size) && __has_builtin(__builtin___memcpy_chk)
#define __YL_FORTIFY 1

#if _FORTIFY_SOURCE > 2 && __has_builtin(__builtin_dynamic_object_size)
#define __yl_bos(p) __builtin_dynamic_object_size(p, 1)
#define __yl_bos0(p) __builtin_dynamic_object_size(p, 0)
#elif _FORTIFY_SOURCE > 1
#define __yl_bos(p) __builtin_object_size(p, 1)
#define __yl_bos0(p) __builtin_object_size(p, 0)
#else
#define __yl_bos(p) __builtin_object_size(p, 0)
#define __yl_bos0(p) __builtin_object_size(p, 0)
#endif

/* the printf functions' flag argument, as glibc passes it (level - 1) */
#define __yl_fortify_flag (_FORTIFY_SOURCE > 1)

/* writes the message to stderr and aborts */
__attribute__((__noreturn__)) void __chk_fail(void);

#endif
#endif

#endif /* _SYS__FORTIFY_H */

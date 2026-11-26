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

/* What the libc's declarations say about pointers, as GCC attributes.
 *
 * tcc's yasc dialect reads them: a pointer from regular C is unchecked, but
 * one these name is believed -- a malloc result is `owned maybe`, the
 * deallocator's argument `owned maybe drop` (tinycc docs/yasc_modules.md,
 * "Legacy C").  gcc and clang use them too: -Wnonnull, and gcc's analyzer
 * pairs allocators with deallocators (-Wmismatched-dealloc).  Other compilers
 * see nothing.  Its own names (__yl_*), so it never clashes with the
 * <sys/cdefs.h> of a host libc.
 *
 * What GCC attributes cannot say -- a result that may be null (yasc `maybe`),
 * where a result points (`borrows`) -- is said once, in yasc: a header with
 * yasc declarations is written in yasc/<header>, and the C <header> is
 * written from it by `tcc -yasc-c-header` (`make headers`; tinycc
 * docs/yasc_c_headers.md).  A yasc unit (__YASC__) reads the yasc header,
 * everything else the C one, where the yasc words are gone and `owned(D)`
 * reads __yl_dealloc(D, n).  A yasc parameter without `maybe` must not get
 * null from a yasc caller; __yl_nonnull is still written by hand, only where
 * null is undefined (getline and posix_memalign check it: EINVAL).  Parts
 * nobody annotated yet stay regular C in `#pragma tcc c_begin` / `c_end`. */

#ifndef _SYS__ATTRS_H
#define _SYS__ATTRS_H

#if defined(__GNUC__) || defined(__TINYC__)
/* a fresh block: nothing else points to it */
#define __yl_malloc __attribute__((__malloc__))
/* these arguments (from 1) must not be null: only where null is undefined */
#define __yl_nonnull(...) __attribute__((__nonnull__(__VA_ARGS__)))
#define __yl_returns_nonnull __attribute__((__returns_nonnull__))
/* the result points to an object of the size argument N (from 1) gives, or
   of the product of N and M: what __builtin_object_size and _FORTIFY_SOURCE
   (sys/_fortify.h) read of a block from it.  tcc knows malloc, calloc,
   realloc, aligned_alloc and memalign by name as well; gcc and clang need it */
#define __yl_alloc_size(...) __attribute__((__alloc_size__(__VA_ARGS__)))
/* never returns (C11 _Noreturn, for C99 and the declarations' own place) */
#define __yl_noreturn __attribute__((__noreturn__))
#else
#define __yl_malloc
#define __yl_nonnull(...)
#define __yl_returns_nonnull
#define __yl_alloc_size(...)
#define __yl_noreturn
#endif

/* An allocator whose result DEALLOC's argument ARG releases (GCC 11's
 * malloc(dealloc, arg)); DEALLOC is declared before it.  clang has no such
 * form. */
#if defined(__TINYC__) || (defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 11)
#define __yl_dealloc(dealloc, arg) __attribute__((__malloc__(dealloc, arg)))
#else
#define __yl_dealloc(dealloc, arg)
#endif

/* Argument N (from 1) is a C string, read up to its NUL.  tcc checks every
 * call (-ffortify, on by default; -ccompat with _FORTIFY_SOURCE): the NUL has
 * to be inside the buffer the caller names, or the program ends
 * (__chk_fail) -- no macro, no _chk entry, the plain function is called.
 * gcc 14 warns with it (-Wstringop-overread). */
#if defined(__TINYC__) || (defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 14)
#define __yl_cstr(n) __attribute__((__null_terminated_string_arg__(n)))
#else
#define __yl_cstr(n)
#endif

/* A struct member's pointer, after its declarator (`ssize_t *result
 * __yl_view;`): yasc code reads it as `view` -- borrowed, never freed through
 * the struct, not null -- or with __yl_view_maybe `view maybe`, instead of a
 * pointer from regular C, which it dereferences only inside `unsafe`.  A
 * struct defined in a yasc header says `view` itself, and its C header gets
 * these.  Only a tcc that knows the attributes (__has_attribute): an older
 * one, gcc and clang see nothing. */
#if defined(__TINYC__) && defined(__has_attribute)
#if __has_attribute(__yasc_view__)
#define __yl_view __attribute__((__yasc_view__))
#define __yl_view_maybe __attribute__((__yasc_view__, __yasc_maybe__))
#endif
#endif
#ifndef __yl_view
#define __yl_view
#define __yl_view_maybe
#endif

#endif /* _SYS__ATTRS_H */

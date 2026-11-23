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
 * What GCC attributes cannot say -- a result that may be null (yasc `maybe`)
 * -- is in sys/yasc/<header>.h: the header includes it before its own
 * prototypes, and in a yasc unit (__YASC__) it says `#pragma yasc` and
 * declares those functions again with yasc qualifiers.  It has to be a file
 * of its own: a header is regular C, where the yasc words are identifiers,
 * also in the expansion of its macros, and the pragma lasts to the end of
 * the file that says it (the file that included it stays C).  tcc keeps the
 * first declaration, so the C prototype after it changes nothing; that is
 * also why the yasc one repeats the nonnull attributes.  There every pointer
 * says `view` (or `owned`), and `maybe` where null is allowed: a parameter
 * without `maybe` is nonnull to tcc's flow checks as well, so a value that
 * may be null passed to it warns.  Regular C (-ccompat, gcc, clang) never
 * sees any of it. */

#ifndef _SYS__ATTRS_H
#define _SYS__ATTRS_H

#if defined(__GNUC__) || defined(__TINYC__)
/* a fresh block: nothing else points to it */
#define __yl_malloc __attribute__((__malloc__))
/* these arguments (from 1) must not be null: only where null is undefined */
#define __yl_nonnull(...) __attribute__((__nonnull__(__VA_ARGS__)))
#define __yl_returns_nonnull __attribute__((__returns_nonnull__))
#else
#define __yl_malloc
#define __yl_nonnull(...)
#define __yl_returns_nonnull
#endif

/* An allocator whose result DEALLOC's argument ARG releases (GCC 11's
 * malloc(dealloc, arg)); DEALLOC is declared before it.  clang has no such
 * form. */
#if defined(__TINYC__) || (defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 11)
#define __yl_dealloc(dealloc, arg) __attribute__((__malloc__(dealloc, arg)))
#else
#define __yl_dealloc(dealloc, arg)
#endif

#endif /* _SYS__ATTRS_H */

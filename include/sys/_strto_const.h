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

/* C23 keeps the caller's const through strchr and its kin (7.26.5); this libc
 * does the same for the number parsers' ENDPTR, from C23 on and in yasc.  An
 * ENDPTR of type `const char **` gets the end back through a const pointer,
 * with no cast at the call (`strtoull(p, &p, 0)` with a `const char *p`).
 * Every other ENDPTR -- `char **`, a null pointer -- is the standard call.
 *
 * The const declarations name the very same functions (an asm label, not a
 * wrapper), so nothing is added to the library.  Off while the libc itself is
 * built (__YL_LIBC_SOURCE, libc/Makefile): its sources define these names.
 *
 * Included at the end of <stdlib.h>: the macros would rewrite the plain
 * declarations there.  A file of its own because, in yasc, a macro is read in
 * the dialect of the file that defines it -- these have to be yasc macros for
 * the yasc spelling of the ENDPTR type below to mean anything. */

#ifndef _SYS_STRTO_CONST_H
#define _SYS_STRTO_CONST_H

#include <sys/_attrs.h>

#if (__STDC_VERSION__ >= 202311L || defined(__YASC__)) && !defined(__YL_LIBC_SOURCE) && !defined(__cplusplus)

#ifdef __YASC__
#pragma yasc
long __yl_strtol_c(const char *s, out const char * borrows(s) * maybe endptr, int base) __asm__("strtol")
    __yl_nonnull(1);
unsigned long __yl_strtoul_c(const char *s, out const char * borrows(s) * maybe endptr, int base)
    __asm__("strtoul") __yl_nonnull(1);
long long __yl_strtoll_c(const char *s, out const char * borrows(s) * maybe endptr, int base)
    __asm__("strtoll") __yl_nonnull(1);
unsigned long long __yl_strtoull_c(const char *s, out const char * borrows(s) * maybe endptr, int base)
    __asm__("strtoull") __yl_nonnull(1);
double __yl_strtod_c(const char *nptr, out const char * borrows(nptr) * maybe endptr) __asm__("strtod")
    __yl_nonnull(1);
float __yl_strtof_c(const char *nptr, out const char * borrows(nptr) * maybe endptr) __asm__("strtof")
    __yl_nonnull(1);
long double __yl_strtold_c(const char *nptr, out const char * borrows(nptr) * maybe endptr) __asm__("strtold")
    __yl_nonnull(1);
/* _Generic matches on const alone: `maybe` and the outer pointer's word do
   not matter */
#define __yl_cendptr const char * view * view
#else
long __yl_strtol_c(const char *s, const char **endptr, int base) __asm__("strtol") __yl_nonnull(1);
unsigned long __yl_strtoul_c(const char *s, const char **endptr, int base) __asm__("strtoul") __yl_nonnull(1);
long long __yl_strtoll_c(const char *s, const char **endptr, int base) __asm__("strtoll") __yl_nonnull(1);
unsigned long long __yl_strtoull_c(const char *s, const char **endptr, int base) __asm__("strtoull")
    __yl_nonnull(1);
double __yl_strtod_c(const char *nptr, const char **endptr) __asm__("strtod") __yl_nonnull(1);
float __yl_strtof_c(const char *nptr, const char **endptr) __asm__("strtof") __yl_nonnull(1);
long double __yl_strtold_c(const char *nptr, const char **endptr) __asm__("strtold") __yl_nonnull(1);
#define __yl_cendptr const char **
#endif

#define strtol(s, endptr, base) _Generic((endptr), __yl_cendptr: __yl_strtol_c, default: strtol)(s, endptr, base)
#define strtoul(s, endptr, base) _Generic((endptr), __yl_cendptr: __yl_strtoul_c, default: strtoul)(s, endptr, base)
#define strtoll(s, endptr, base) _Generic((endptr), __yl_cendptr: __yl_strtoll_c, default: strtoll)(s, endptr, base)
#define strtoull(s, endptr, base) \
  _Generic((endptr), __yl_cendptr: __yl_strtoull_c, default: strtoull)(s, endptr, base)
#define strtod(s, endptr) _Generic((endptr), __yl_cendptr: __yl_strtod_c, default: strtod)(s, endptr)
#define strtof(s, endptr) _Generic((endptr), __yl_cendptr: __yl_strtof_c, default: strtof)(s, endptr)
#define strtold(s, endptr) _Generic((endptr), __yl_cendptr: __yl_strtold_c, default: strtold)(s, endptr)

#endif

#endif /* _SYS_STRTO_CONST_H */

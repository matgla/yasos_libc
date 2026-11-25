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

/* yasc declarations for <stdlib.h> (how and why: sys/_attrs.h): the results that
 * may be null and are not allocations (those are malloc-attributed in
 * <stdlib.h> itself), realloc, whose result takes its block over,
 * mkstemp's descriptor, and the number parsers of atoi.c. */

#ifndef _SYS_YASC_STDLIB_H
#define _SYS_YASC_STDLIB_H

#include <sys/_attrs.h>

#ifdef __YASC__
#pragma yasc

char * borrows(static) maybe getenv(const char * view name);
/* base may be null with nmemb == 0 */
void * borrows(base) maybe bsearch(const void * view key, const void * view maybe base, size_t nmemb, size_t size,
                          int (*compar)(const void * view, const void * view));
/* T itself, or null on failure */
char * borrows(t) maybe mkdtemp(inout char * view t);
/* an open descriptor of the file it made from T, or -1 */
int owned(close) maybe mkstemp(inout char * view t);
/* S, never null */
char * borrows(s) itoa(int n, out char * view s, int base);
/* written on success (0) only */
int posix_memalign(out void * owned(free) maybe * view memptr, size_t alignment, size_t size);
/* the result takes the block over (it may move it); null: v is still the caller's */
void * owned(free) maybe takes(v) realloc(void * owned maybe v, size_t sz) __yl_alloc_size(2);
void * owned(free) maybe takes(p) reallocarray(void * owned maybe p, size_t n, size_t m) __yl_alloc_size(2, 3);

/* atoi.c.  S is read up to the first character that is not part of the
   number.  ENDPTR may be null; when it is not, *endptr is written: where the
   number ended, or S itself when nothing converted -- a pointer into S
   (borrows(s)): the caller's end borrows from what it passed for S. */
int atoi(const char * view s);
long atol(const char * view s);
long strtol(const char * view s, out char * borrows(s) * view maybe endptr, int base);
unsigned long strtoul(const char * view s, out char * borrows(s) * view maybe endptr, int base);
long long strtoll(const char * view s, out char * borrows(s) * view maybe endptr, int base);
unsigned long long strtoull(const char * view s, out char * borrows(s) * view maybe endptr, int base);
#endif /* __YASC__ */

#endif /* _SYS_YASC_STDLIB_H */

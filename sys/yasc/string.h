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

/* yasc declarations for <string.h> (how and why: sys/_attrs.h): the results that
 * may be null, and where the results point. */

#ifndef _SYS_YASC_STRING_H
#define _SYS_YASC_STRING_H

#include <sys/_attrs.h>

#ifdef __YASC__
#pragma yasc

char * borrows(s) maybe strchr(const char * view s, int c) __yl_nonnull(1);
char * borrows(s) maybe strrchr(const char * view s, int c) __yl_nonnull(1);
char * borrows(s) maybe strstr(const char * view s, const char * view r) __yl_nonnull(1, 2);
char * borrows(s) maybe strpbrk(const char * view s, const char * view accept) __yl_nonnull(1, 2);
/* length-bounded: s may be null with n == 0 */
void * borrows(s) maybe memchr(const void * view maybe s, int c, long n);
void * borrows(s) maybe __memchr_c(const void * view maybe s, int c, long n);
void * borrows(s) maybe memrchr(const void * view maybe s, int c, size_t n);
/* null when no token is left; str is null after the first call */
char * borrows(static, str) maybe strtok(char * view maybe str, const char * view delimiters) __yl_nonnull(2);
char * borrows(str, saveptr) maybe strtok_r(char * view maybe str, const char * view delimiters,
                           char * view maybe * view saveptr) __yl_nonnull(2, 3);
/* a fixed message, "Unknown error" for an unknown number */
char * borrows(static) strerror(int errnum);
/* DST (stpcpy: its terminating null), never SRC */
char * borrows(dst) strcpy(char * view dst, const char * view src) __yl_nonnull(1, 2);
char * borrows(dst) strcat(char * view dst, const char * view src) __yl_nonnull(1, 2);
char * borrows(dst) stpcpy(char * view dst, const char * view src) __yl_nonnull(1, 2);
/* length-bounded: dst may be null with size == 0 */
size_t strlcpy(char * view maybe dst, const char * view src, size_t size) __yl_nonnull(2);
size_t strlcat(char * view maybe dst, const char * view src, size_t size) __yl_nonnull(2);
void * borrows(s) maybe memset_explicit(void * view maybe s, int c, size_t n);
void explicit_bzero(void * view maybe s, size_t n);
#endif /* __YASC__ */

#endif /* _SYS_YASC_STRING_H */

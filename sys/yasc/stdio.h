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

/* yasc declarations for <stdio.h> (how and why: sys/_attrs.h): the results that
 * may be null and are not streams (fopen and friends are malloc-attributed in
 * <stdio.h> itself). */

#ifndef _SYS_YASC_STDIO_H
#define _SYS_YASC_STDIO_H

#include <sys/_attrs.h>

#ifdef __YASC__
#pragma yasc

/* S, or null at the end of the input or on an error */
char * borrows(s) maybe fgets(out char * view s, int sz, inout FILE * view fp);
/* BUFFER or a static one; null when no name is left */
char * borrows(static, buffer) maybe tmpnam(out char * view maybe buffer);
/* the line buffer is the caller's before and after (the callee may replace
   it), and must be freed even after -1 */
ssize_t getline(inout char * owned(free) maybe * view lineptr, inout size_t * view n, inout FILE * view fp);
ssize_t getdelim(inout char * owned(free) maybe * view lineptr, inout size_t * view n, int delimiter,
                 inout FILE * view stream);
/* the stream takes the descriptor over; null: fd is still the caller's */
FILE * owned(fclose) maybe takes(fd) fdopen(int owned(close) fd, const char * view mode);
#endif /* __YASC__ */

#endif /* _SYS_YASC_STDIO_H */

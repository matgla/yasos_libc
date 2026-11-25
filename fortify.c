/*
 Copyright (c) 2026 Mateusz Stadnik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/* The runtime of _FORTIFY_SOURCE (sys/_fortify.h): what a fortified call
 * runs when the compiler could not prove that it fits its destination of
 * DSTLEN bytes.  glibc's names and arguments, so objects from gcc or clang
 * link too.  An overflow ends the program before anything is written past
 * the destination.
 *
 * tcc calls the str* ones of its own runtime (libtcc1's __tcc_strcpy_chk
 * ...), which end in this __chk_fail too; these are for other compilers and
 * for direct calls.  Built without _FORTIFY_SOURCE, like all of libc, so the
 * plain functions below are the real ones. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* not in the headers without _FORTIFY_SOURCE (they are here for others) */
__attribute__((noreturn)) void __chk_fail(void);
void *__memcpy_chk(void *dst, const void *src, size_t n, size_t dstlen);
void *__memmove_chk(void *dst, const void *src, size_t n, size_t dstlen);
void *__mempcpy_chk(void *dst, const void *src, size_t n, size_t dstlen);
void *__memset_chk(void *s, int v, size_t n, size_t dstlen);
char *__strcpy_chk(char *dst, const char *src, size_t dstlen);
char *__stpcpy_chk(char *dst, const char *src, size_t dstlen);
char *__strncpy_chk(char *dst, const char *src, size_t n, size_t dstlen);
char *__stpncpy_chk(char *dst, const char *src, size_t n, size_t dstlen);
char *__strcat_chk(char *dst, const char *src, size_t dstlen);
char *__strncat_chk(char *dst, const char *src, size_t n, size_t dstlen);
int __sprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, ...);
int __snprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, ...);
int __vsprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, va_list ap);
int __vsnprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, va_list ap);

/* write(2), not stdio: the overflow may have been into stdio's own state */
__attribute__((noreturn)) void __chk_fail(void)
{
  static const char msg[] = "*** buffer overflow detected ***: terminated\n";
  write(2, msg, sizeof msg - 1);
  abort();
}

void *__memcpy_chk(void *dst, const void *src, size_t n, size_t dstlen)
{
  if (n > dstlen)
    __chk_fail();
  return memcpy(dst, src, n);
}

void *__memmove_chk(void *dst, const void *src, size_t n, size_t dstlen)
{
  if (n > dstlen)
    __chk_fail();
  return memmove(dst, src, n);
}

void *__mempcpy_chk(void *dst, const void *src, size_t n, size_t dstlen)
{
  if (n > dstlen)
    __chk_fail();
  return mempcpy(dst, src, n);
}

void *__memset_chk(void *s, int v, size_t n, size_t dstlen)
{
  if (n > dstlen)
    __chk_fail();
  return memset(s, v, n);
}

char *__strcpy_chk(char *dst, const char *src, size_t dstlen)
{
  size_t len = strlen(src);
  if (len >= dstlen)
    __chk_fail();
  return memcpy(dst, src, len + 1);
}

char *__stpcpy_chk(char *dst, const char *src, size_t dstlen)
{
  size_t len = strlen(src);
  if (len >= dstlen)
    __chk_fail();
  memcpy(dst, src, len + 1);
  return dst + len;
}

char *__strncpy_chk(char *dst, const char *src, size_t n, size_t dstlen)
{
  if (n > dstlen)
    __chk_fail();
  return strncpy(dst, src, (long)n);
}

char *__stpncpy_chk(char *dst, const char *src, size_t n, size_t dstlen)
{
  if (n > dstlen)
    __chk_fail();
  return stpncpy(dst, src, n);
}

/* DST must hold its own string, SRC's and the nul */
char *__strcat_chk(char *dst, const char *src, size_t dstlen)
{
  size_t dl = strnlen(dst, dstlen);
  if (dl == dstlen || strlen(src) >= dstlen - dl)
    __chk_fail();
  return strcat(dst, src);
}

char *__strncat_chk(char *dst, const char *src, size_t n, size_t dstlen)
{
  size_t dl = strnlen(dst, dstlen);
  if (dl == dstlen || strnlen(src, n) >= dstlen - dl)
    __chk_fail();
  return strncat(dst, src, n);
}

int __vsnprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, va_list ap)
{
  (void)flag;
  if (sz > dstlen)
    __chk_fail();
  return vsnprintf(dst, sz, fmt, ap);
}

/* the output is made with vsnprintf into DSTLEN bytes: one that did not fit
   was cut there, never written past */
int __vsprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, va_list ap)
{
  int ret;
  (void)flag;
  if (dstlen == (size_t)-1)
    return vsprintf(dst, fmt, ap);
  if (dstlen == 0)
    __chk_fail();
  ret = vsnprintf(dst, dstlen, fmt, ap);
  if (ret >= 0 && (size_t)ret >= dstlen)
    __chk_fail();
  return ret;
}

int __snprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, ...)
{
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = __vsnprintf_chk(dst, sz, flag, dstlen, fmt, ap);
  va_end(ap);
  return ret;
}

int __sprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, ...)
{
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = __vsprintf_chk(dst, flag, dstlen, fmt, ap);
  va_end(ap);
  return ret;
}

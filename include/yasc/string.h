#pragma yasc
#pragma tcc c_begin
// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <stddef.h>
#include <sys/_attrs.h>

/* memccpy stops at C: n may be more than either buffer holds */
void *memccpy(void *dst, const void *src, int c, size_t n);
#pragma tcc c_end
/* `[n]`: n bytes of the buffer, checked at every call (-ffortify) */
void *borrows(dst) memcpy(out void dst[n], void src[n], size_t n);
/* the byte after the last one written */
void *borrows(dst) mempcpy(out void dst[n], void src[n], size_t n);
void *borrows(dst) memmove(out void dst[n], void src[n], size_t n);
void *borrows(s) memset(out void s[n], int v, size_t n);
/* length-bounded: s may be null with n == 0 */
void *?borrows(s) memchr(void *?s, int c, long n);
void *?borrows(s) memrchr(void *?s, int c, size_t n);
int memcmp(void s1[n], void s2[n], size_t n);
/* DST (stpcpy: its terminating null), never SRC */
char *borrows(dst) strcpy(out char *dst, char *src) __yl_nonnull(1, 2)
    __yl_unsafe("the destination has no size");
char *?borrows(s) strchr(char *s, int c) __yl_nonnull(1);
char *?borrows(s) strrchr(char *s, int c) __yl_nonnull(1);
#pragma tcc c_begin
size_t strlen(const char *s) __yl_nonnull(1);
size_t strnlen(const char *s, size_t n);
#pragma tcc c_end
/* writes all n bytes of d (zeroes after the copy); reads s up to n or its NUL */
char *borrows(d) strncpy(out char d[n], char *s, long n);
char *borrows(dst) strcat(inout char *dst, char *src) __yl_nonnull(1, 2)
    __yl_unsafe("the destination has no size");
#pragma tcc c_begin
char *strncat(char *d, const char *s, size_t n) __yl_unsafe("its bound is the source's, not the destination's");
int strcmp(const char *str1, const char *str2) __yl_nonnull(1, 2);
int strncmp(const char *str1, const char *str2, size_t n);
#pragma tcc c_end
char *?borrows(s) strstr(char *s, char *r) __yl_nonnull(1, 2);
#pragma tcc c_begin

char *strdup(const char *s) __yl_malloc __yl_nonnull(1);
char *strndup(const char *s, size_t n) __yl_malloc;
size_t strspn(const char *s, const char *accept) __yl_nonnull(1, 2);
size_t strcspn(const char *s, const char *reject) __yl_nonnull(1, 2);
#pragma tcc c_end
/* null when no token is left; str is null after the first call */
inout char *?borrows(static, str) strtok(inout char *?str, char *delimiters) __yl_nonnull(2);
char *?borrows(str, saveptr) strtok_r(inout char *?str, char *delimiters,
                           inout char *?view *saveptr) __yl_nonnull(2, 3);
#pragma tcc c_begin
/* faster implementations */
#define memchr(s, c, n) __memchr_c(s, c, n)

#pragma tcc c_end
void *?borrows(s) __memchr_c(void *?s, int c, long n);
/* a fixed message, "Unknown error" for an unknown number */
char *borrows(static) strerror(int errnum);
char *borrows(dst) stpcpy(out char *dst, char *src) __yl_nonnull(1, 2)
    __yl_unsafe("the destination has no size");
char *borrows(dst) stpncpy(out char dst[n], char *src, size_t n);
char *?borrows(s) strpbrk(char *s, char *accept) __yl_nonnull(1, 2);
#pragma tcc c_begin
int strcasecmp(const char *s1, const char *s2) __yl_nonnull(1, 2);
int strncasecmp(const char *s1, const char *s2, size_t n);

/* Bounded copies (POSIX 2024): the result is the length they tried to make,
   >= size when it was cut. dst may be null with size 0. */
#pragma tcc c_end
/* length-bounded: dst may be null with size == 0 */
size_t strlcpy(out char dst[?size], char *src, size_t size) __yl_nonnull(2);
size_t strlcat(inout char dst[?size], char *src, size_t size) __yl_nonnull(2);
#pragma tcc c_begin
/* memset that the optimiser keeps (C23), for secrets; explicit_bzero zeroes */
#pragma tcc c_end
void *?borrows(s) memset_explicit(out void s[?n], int c, size_t n);
void explicit_bzero(out void s[?n], size_t n);

/* Written by `tcc -yasc-c-header` from yasc/string.h: edit that file, not this one.
   A yasc unit reads the yasc header itself. */
#ifdef __YASC__
#include "yasc/string.h"
#else
// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <stddef.h>
#include <sys/_attrs.h>


void *memcpy(void *dst, const void *src, size_t n);
void *mempcpy(void *dst, const void *src, size_t n);
void *memccpy(void *dst, const void *src, int c, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *s, int v, size_t n);
/* length-bounded: s may be null with n == 0 */
void *memchr(const void *s, int c, long n);
void *memrchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

/* DST (stpcpy: its terminating null), never SRC */
char *strcpy(char *dst, const char *src) __yl_nonnull(1, 2);
char *strchr(const char *s, int c) __yl_nonnull(1);
char *strrchr(const char *s, int c) __yl_nonnull(1);
size_t strlen(const char *s) __yl_nonnull(1);
size_t strnlen(const char *s, size_t n);

char *strncpy(char *d, const char *s, long n);
char *strcat(char *dst, const char *src) __yl_nonnull(1, 2);
char *strncat(char *d, const char *s, size_t n);
int strcmp(const char *str1, const char *str2) __yl_nonnull(1, 2);
int strncmp(const char *str1, const char *str2, size_t n);
char *strstr(const char *s, const char *r) __yl_nonnull(1, 2);

char *strdup(const char *s) __yl_malloc __yl_nonnull(1);
char *strndup(const char *s, size_t n) __yl_malloc;
size_t strspn(const char *s, const char *accept) __yl_nonnull(1, 2);
size_t strcspn(const char *s, const char *reject) __yl_nonnull(1, 2);
/* null when no token is left; str is null after the first call */
char *strtok(char *str, const char *delimiters) __yl_nonnull(2);
char *strtok_r(char *str, const char *delimiters,
                           char **saveptr) __yl_nonnull(2, 3);
/* faster implementations */
#define memchr(s, c, n) __memchr_c(s, c, n)

void *__memchr_c(const void *s, int c, long n);
/* a fixed message, "Unknown error" for an unknown number */
char *strerror(int errnum);
char *stpcpy(char *dst, const char *src) __yl_nonnull(1, 2);
char *stpncpy(char *dst, const char *src, size_t n);

char *strpbrk(const char *s, const char *accept) __yl_nonnull(1, 2);
int strcasecmp(const char *s1, const char *s2) __yl_nonnull(1, 2);
int strncasecmp(const char *s1, const char *s2, size_t n);

/* Bounded copies (POSIX 2024): the result is the length they tried to make,
   >= size when it was cut. dst may be null with size 0. */
/* length-bounded: dst may be null with size == 0 */
size_t strlcpy(char *dst, const char *src, size_t size) __yl_nonnull(2);
size_t strlcat(char *dst, const char *src, size_t size) __yl_nonnull(2);
/* memset that the optimiser keeps (C23), for secrets; explicit_bzero zeroes */
void *memset_explicit(void *s, int c, size_t n);
void explicit_bzero(void *s, size_t n);

/* _FORTIFY_SOURCE (sys/_fortify.h): the copies that know their destination's
   size, and the checking functions they call when the compiler cannot prove
   the copy fits (fortify.c; glibc's names and arguments) */
#include <sys/_fortify.h>
#ifdef __YL_FORTIFY
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

#define memcpy(d, s, n) __builtin___memcpy_chk(d, s, n, __yl_bos0(d))
#define memmove(d, s, n) __builtin___memmove_chk(d, s, n, __yl_bos0(d))
#define mempcpy(d, s, n) __builtin___mempcpy_chk(d, s, n, __yl_bos0(d))
#define memset(d, v, n) __builtin___memset_chk(d, v, n, __yl_bos0(d))
#define strcpy(d, s) __builtin___strcpy_chk(d, s, __yl_bos(d))
#define stpcpy(d, s) __builtin___stpcpy_chk(d, s, __yl_bos(d))
#define strncpy(d, s, n) __builtin___strncpy_chk(d, s, n, __yl_bos(d))
#define stpncpy(d, s, n) __builtin___stpncpy_chk(d, s, n, __yl_bos(d))
#define strcat(d, s) __builtin___strcat_chk(d, s, __yl_bos(d))
#define strncat(d, s, n) __builtin___strncat_chk(d, s, n, __yl_bos(d))
#endif
#endif /* __YASC__ */

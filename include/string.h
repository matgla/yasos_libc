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

/* memccpy stops at C: n may be more than either buffer holds */
void *memccpy(void *dst, const void *src, int c, size_t n);
/* `[n]`: n bytes of the buffer, checked at every call (-ffortify) */
void *memcpy(void *dst, const void *src, size_t n) __yl_access(write_only, 1, 3) __yl_access(read_only, 2, 3);
/* the byte after the last one written */
void *mempcpy(void *dst, const void *src, size_t n) __yl_access(write_only, 1, 3) __yl_access(read_only, 2, 3);
void *memmove(void *dst, const void *src, size_t n) __yl_access(write_only, 1, 3) __yl_access(read_only, 2, 3);
void *memset(void *s, int v, size_t n) __yl_access(write_only, 1, 3);
/* length-bounded: s may be null with n == 0 */
void *memchr(const void *s, int c, long n);
void *memrchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n) __yl_access(read_only, 1, 3) __yl_access(read_only, 2, 3);
/* DST (stpcpy: its terminating null), never SRC */
char *strcpy(char *dst, const char *src) __yl_nonnull(1, 2)
    __yl_unsafe("the destination has no size");
char *strchr(const char *s, int c) __yl_nonnull(1);
char *strrchr(const char *s, int c) __yl_nonnull(1);
size_t strlen(const char *s) __yl_nonnull(1);
size_t strnlen(const char *s, size_t n);
/* writes all n bytes of d (zeroes after the copy); reads s up to n or its NUL */
char *strncpy(char *d, const char *s, long n) __yl_access(write_only, 1, 3);
char *strcat(char *dst, const char *src) __yl_nonnull(1, 2)
    __yl_unsafe("the destination has no size");
char *strncat(char *d, const char *s, size_t n) __yl_unsafe("its bound is the source's, not the destination's");
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
char *stpcpy(char *dst, const char *src) __yl_nonnull(1, 2)
    __yl_unsafe("the destination has no size");
char *stpncpy(char *dst, const char *src, size_t n) __yl_access(write_only, 1, 3);
char *strpbrk(const char *s, const char *accept) __yl_nonnull(1, 2);
int strcasecmp(const char *s1, const char *s2) __yl_nonnull(1, 2);
int strncasecmp(const char *s1, const char *s2, size_t n);

/* Bounded copies (POSIX 2024): the result is the length they tried to make,
   >= size when it was cut. dst may be null with size 0. */
/* length-bounded: dst may be null with size == 0 */
size_t strlcpy(char *dst, const char *src, size_t size) __yl_nonnull(2) __yl_access(write_only, 1, 3);
size_t strlcat(char *dst, const char *src, size_t size) __yl_nonnull(2) __yl_access(read_write, 1, 3);
/* memset that the optimiser keeps (C23), for secrets; explicit_bzero zeroes */
void *memset_explicit(void *s, int c, size_t n) __yl_access(write_only, 1, 3);
void explicit_bzero(void *s, size_t n) __yl_access(write_only, 1, 2);
#endif /* __YASC__ */

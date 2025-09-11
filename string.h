// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memccpy(void *dst, const void *src, int c, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *s, int v, size_t n);
void *memchr(void *s, int c, long n);
void *memrchr(void *s, int c, long n);
int memcmp(const void *s1, const void *s2, size_t n);

char *strcpy(char *dst, const char *src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t n);

char *strncpy(char *d, const char *s, long n);
char *strcat(char *d, const char *s);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
char *strstr(char *s, char *r);

char *strdup(const char *s);
char *strndup(const char *s, size_t n);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strtok(char *str, const char *delimiters);
/* faster implementations */
#define memchr(s, c, n) __memchr_c(s, c, n)

void *__memchr_c(void *s, int c, long n);

char *strerror(int errnum);
char *stpcpy(char *dst, const char *src);
char *stpncpy(char *dst, const char *src, size_t n);

char *strpbrk(const char *s, const char *accept);
#pragma once

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *s, int v, size_t n);
void *memchr(void *s, int c, long n);
void *memrchr(void *s, int c, long n);
int memcmp(const void *s1, const void *s2, size_t n);

char *strcpy(char *dst, const char *src);
char *strchr(char *s, int c);
char *strrchr(char *s, int c);
size_t strlen(const char *s);

char *strncpy(char *d, char *s, long n);
char *strcat(char *d, char *s);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
char *strstr(char *s, char *r);

char *strdup(const char *s);
size_t strspn(const char *str1, const char *str2);
size_t strcspn(const char *str1, const char *str2);

/* faster implementations */
#define memchr(s, c, n) __memchr_c(s, c, n)

void *__memchr_c(void *s, int c, long n);

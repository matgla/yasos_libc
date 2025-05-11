#include <string.h>

#include <stdint.h>

#include <stdio.h>

#define LONG01 (0x01010101ul | (0x01010101ul << (sizeof(long) * 8 - 32)))
#define LONG80 (0x80808080ul | (0x80808080ul << (sizeof(long) * 8 - 32)))
#define LONGOFF(s) ((unsigned long)s & (sizeof(long) - 1))
#define HASZERO(x) (((x) - LONG01) & ~(x) & LONG80)

void *__memchr_c(void *src, int c, long n) {
  unsigned char *s = src;
  c = (unsigned char)c;
  unsigned long k = LONG01 * c;
  while (LONGOFF(s) && n && *s != c)
    s++, n--;
  if (n && *s != c) {
    unsigned long *w = (void *)s;
    while (n >= sizeof(long) && !HASZERO(*w ^ k))
      w++, n -= sizeof(long);
    s = (void *)w;
  }
  while (n && *s != c)
    s++, n--;
  return n ? (void *)s : 0;
}

void *memset(void *ptr, int value, size_t num) {
  uint8_t *p = (uint8_t *)ptr;
  for (size_t i = 0; i < num; ++i)
    p[i] = (uint8_t)value;
  return ptr;
}

void *memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (size_t i = 0; i < n; ++i)
    d[i] = s[i];
  return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  if (d < s) {
    for (size_t i = 0; i < n; ++i)
      d[i] = s[i];
  } else {
    for (size_t i = n; i > 0; --i)
      d[i - 1] = s[i - 1];
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;
  for (size_t i = 0; i < n; ++i) {
    if (p1[i] != p2[i])
      return p1[i] - p2[i];
  }
  return 0;
}

size_t strlen(const char *str) {
  size_t r = 0;
  while (str[r] != 0)
    ++r;
  return r;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while ((*d++ = *src++) != 0)
    ;
  return dst;
}

char *strchr(const char *s, int c) {
  while (*s != (char)c) {
    if (*s == 0)
      return NULL;
    ++s;
  }
  return (char *)s;
}

int strncmp(const char *str1, const char *str2, size_t num) {
  size_t i = 0;
  while (i < num && str1[i] != 0 && str2[i] != 0) {
    if (str1[i] != str2[i])
      return str1[i] - str2[i];
    ++i;
  }
  if (i == num)
    return 0;
  return str1[i] - str2[i];
}

int strcmp(const char *str1, const char *str2) {
  while (*str1 != 0 && *str2 != 0) {
    if (*str1 != *str2)
      return *str1 - *str2;
    ++str1;
    ++str2;
  }
  return *str1 - *str2;
}

size_t strspn(const char *str1, const char *str2) {
  size_t count = 0;
  while (*str1 != 0 && strchr((char *)str2, *str1) != NULL) {
    ++count;
    ++str1;
  }
  return count;
}

size_t strcspn(const char *str1, const char *str2) {
  size_t count = 0;
  while (*str1 != 0 && strchr((char *)str2, *str1) == NULL) {
    ++count;
    ++str1;
  }
  return count;
}

char *strtok(char *str, const char *delimiters) {
  static char *last = NULL;

  if (str == NULL) {
    str = last;
  }
  if (str == NULL) {
    return NULL;
  }

  str += strspn(str, delimiters);

  if (*str == '\0') {
    last = NULL;
    return NULL;
  }
  int i = strcspn(str, delimiters);
  char *end = str + i;
  if (*end != '\0') {
    *end = '\0';
    ++end;
  }
  last = end;
  return str;
}

char *strpbrk(const char *str1, const char *str2) {
  while (*str1 != 0) {
    if (strchr((char *)str2, *str1) != NULL)
      return (char *)str1;
    ++str1;
  }
  return NULL;
}

char *strrchr(char *str, int c) {
  const char *last = NULL;
  while (*str != 0) {
    if (*str == (char)c)
      last = str;
    ++str;
  }
  return (char *)last;
}

char *strerror(int errnum) {
  switch (errnum) {
  case 0:
    return "Success";
  case 1:
    return "Operation not permitted";
  case 2:
    return "No such file or directory";
  case 3:
    return "No such process";
  case 4:
    return "Interrupted system call";
  case 5:
    return "Input/output error";
  default:
    return "Unknown error";
  }
}
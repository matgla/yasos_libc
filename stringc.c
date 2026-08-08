#include <stdlib.h>
#include <string.h>

char *strncpy(char *d, const char *s, long n) {
  int len = strlen(s);
  if (len > n)
    len = n;
  memcpy(d, s, len);
  memset(d + len, 0, n - len);
  return d;
}

char *strcat(char *d, const char *s) {
  strcpy(d + strlen(d), s);
  return d;
}

char *strncat(char *d, const char *s, size_t n) {
  char *end = d + strlen(d);
  size_t len = strnlen(s, n);
  memcpy(end, s, len);
  end[len] = '\0';
  return d;
}

char *strstr(const char *s, const char *r) {
  int len = strlen(r);
  if (!len)
    return (char*)s;
  while (s) {
    if (!memcmp(s, r, len))
      return (char*)s;
    s = strchr(s + 1, *r);
  }
  return NULL;
}

char *strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *res = malloc(n);
  if (res)
    memcpy(res, s, n);
  return res;
}

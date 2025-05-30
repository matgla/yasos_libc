#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>

static int ic(FILE *fp) {
  int nr;
  if (fp->back != EOF) {
    int i = fp->back;
    fp->back = EOF;
    return i;
  }
  while (fp->fd >= 0 && fp->icur == fp->ilen) {
    int nr = read(fp->fd, fp->ibuf, fp->isize);
    if (nr <= 0)
      break;
    fp->ilen = nr;
    fp->icur = 0;
  }
  return fp->icur < fp->ilen ? (unsigned char)fp->ibuf[fp->icur++] : EOF;
}

void setbuf(FILE *fp, char *buf) {
}

int fgetc(FILE *fp) {
  return ic(fp);
}

int getchar(void) {
  return ic(stdin);
}

int ungetc(int c, FILE *fp) {
  if (fp->back == EOF)
    fp->back = c;
  return fp->back;
}

static int idig(int c, int hex) {
  static char *digs = "0123456789abcdef";
  char *r = memchr(digs, hex ? tolower(c) : c, hex ? 16 : 10);
  return r == NULL ? -1 : r - digs;
}

/* t is 1 for char, 2 for short, 4 for int, and 8 for long */
static int iint(FILE *fp, void *dst, int t, int hex, int wid) {
  long n = 0;
  int c;
  int neg = 0;
  c = ic(fp);
  if (c == '-')
    neg = 1;
  if ((c == '-' || c == '+') && wid-- > 0)
    c = ic(fp);
  if (c == EOF || idig(c, hex) < 0 || wid <= 0) {
    ungetc(c, fp);
    return 1;
  }
  do {
    n = n * (hex ? 16 : 10) + idig(c, hex);
  } while ((c = ic(fp)) != EOF && idig(c, hex) >= 0 && --wid > 0);
  ungetc(c, fp);
  if (t == 8)
    *(long *)dst = neg ? -n : n;
  else if (t == 4)
    *(int *)dst = neg ? -n : n;
  else if (t == 2)
    *(short *)dst = neg ? -n : n;
  else
    *(char *)dst = neg ? -n : n;
  return 0;
}

static int istr(FILE *fp, char *dst, int wid) {
  char *d = dst;
  int c;
  while ((c = ic(fp)) != EOF && wid-- > 0 && !isspace(c))
    *d++ = c;
  *d = '\0';
  ungetc(c, fp);
  return d == dst;
}

int vfscanf(FILE *fp, char *fmt, va_list ap) {
  int ret = 0;
  int t, c;
  int wid = 1 << 20;
  while (*fmt) {
    while (isspace((unsigned char)*fmt))
      fmt++;
    while (isspace(c = ic(fp)))
      ;
    ungetc(c, fp);
    while (*fmt && *fmt != '%' && !isspace((unsigned char)*fmt))
      if (*fmt++ != ic(fp))
        return ret;
    if (*fmt != '%')
      continue;
    fmt++;
    if (isdigit((unsigned char)*fmt)) {
      wid = 0;
      while (isdigit((unsigned char)*fmt))
        wid = wid * 10 + *fmt++ - '0';
    }
    t = sizeof(int);
    while (*fmt == 'l') {
      t = sizeof(long);
      fmt++;
    }
    while (*fmt == 'h') {
      t = t < sizeof(int) ? sizeof(char) : sizeof(short);
      fmt++;
    }
    switch (*fmt++) {
    case 'u':
    case 'd':
      if (iint(fp, va_arg(ap, long *), t, 0, wid))
        return ret;
      ret++;
      break;
    case 'x':
      if (iint(fp, va_arg(ap, long *), t, 1, wid))
        return ret;
      ret++;
      break;
    case 's':
      if (istr(fp, va_arg(ap, char *), wid))
        return ret;
      ret++;
      break;
    }
  }
  return ret;
}

int fscanf(FILE *fp, char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vfscanf(fp, fmt, ap);
  va_end(ap);
  return ret;
}

int scanf(char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vfscanf(stdin, fmt, ap);
  va_end(ap);
  return ret;
}

int vsscanf(char *s, char *fmt, va_list ap) {
  FILE f = {-1, EOF};
  f.ibuf = s;
  f.ilen = strlen(s);
  return vfscanf(&f, fmt, ap);
}

int sscanf(char *s, char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vsscanf(s, fmt, ap);
  va_end(ap);
  return ret;
}

char *fgets(char *s, int sz, FILE *fp) {
  int i = 0;
  int c;
  while (i + 1 < sz && (c = ic(fp)) != EOF) {
    s[i++] = c;
    if (c == '\n')
      break;
  }
  s[i] = '\0';
  return i ? s : NULL;
}

long fread(void *v, long sz, long n, FILE *fp) {
  char *s = v;
  int i = n * sz;
  while (i-- > 0)
    if ((*s++ = ic(fp)) == EOF)
      return n * sz - i - 1;
  return n * sz;
}

int getline(char **lineptr, size_t *n, FILE *fp) {
  int alloc_size = 128;
  if (n == NULL) {
    return -1;
  }
  if (*lineptr == NULL) {
    *lineptr = malloc(alloc_size);
    if (*lineptr == NULL) {
      *n = 0;
      return -1;
    }
  }
  *n = 0;
  int c = 0;
  int i = 0;
  while (c != '\n' && c != EOF) {
    c = fgetc(fp);
    if (c == EOF) {
      *n = i;
      return i > 0 ? i : -1;
    }

    if (i >= alloc_size - 2) {
      alloc_size <<= 1;
      *lineptr = realloc(*lineptr, alloc_size);
      if (*lineptr == NULL) {
        *n = 0;
        return -1;
      }
    }
    (*lineptr)[i++] = (char)c;
  }
  (*lineptr)[i] = '\0';
  *n = i;
  return i > 0 ?: -1;
}

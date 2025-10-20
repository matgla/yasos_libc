#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>

static int ic(FILE *fp) {
  if (fp->back != EOF) {
    int i = fp->back;
    fp->back = EOF;
    return i;
  }

  if (fp->ibuf == fp->obuf && fp->fd == -1) {
    if (fp->icur < fp->olen) {
      return (unsigned char)fp->ibuf[fp->icur++];
    }
    return EOF;
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

static int idig(int c, int base) {
  static char *digs = "0123456789abcdef";
  char *r = memchr(digs, (base == 16) ? tolower(c) : c, base);
  return r == NULL ? -1 : r - digs;
}

/* t is 1 for char, 2 for short, 4 for int, and 8 for long */
static int iint(FILE *fp, void *dst, int t, int base, int wid, int skip) {
  long n = 0;
  int c;
  int neg = 0;
  if (base < 0) {
    neg = -1;
    base = -base;
  }

  c = ic(fp);
  if (c == '-')
    neg = 1;
  if ((c == '-' || c == '+') && wid-- > 0)
    c = ic(fp);
  if (c == EOF || idig(c, base) < 0 || wid <= 0) {
    ungetc(c, fp);
    if (base == 8) {
      if (!skip) {
        *(long *)dst = 0;
        return 0;
      }
      return 0;
    }
    return 1;
  }
  do {
    n = n * base + idig(c, base);
  } while ((c = ic(fp)) != EOF && idig(c, base) >= 0 && --wid > 0);
  ungetc(c, fp);
  if (!skip) {
    if (t == 8)
      *(long *)dst = neg ? -n : n;
    else if (t == 4)
      *(int *)dst = neg ? -n : n;
    else if (t == 2)
      *(short *)dst = neg ? -n : n;
    else
      *(char *)dst = neg ? -n : n;
  }
  return 0;
}

static int istr(FILE *fp, char *dst, int wid, int skip) {
  char *d = dst;
  int c;
  while ((c = ic(fp)) != EOF && wid-- > 0 && !isspace(c)) {
    if (skip)
      continue;

    *d++ = c;
  }
  if (!skip) {
    *d = '\0';
  }
  ungetc(c, fp);
  return skip ? 0 : d == dst;
}

static int parse_base_from_input(FILE *fp) {
  char c = ic(fp);
  int sign = 1;
  if (c == '-') {
    sign = -1;
    c = ic(fp);
  } else if (c == '+') {
    c = ic(fp);
    sign = 1;
  }

  if (c == '0') {
    char c2 = ic(fp);
    if (c2 == 'x' || c2 == 'X') {
      return 16 * sign;
    } else {
      ungetc(c2, fp);
      return 8 * sign;
    }
  }
  ungetc(c, fp);
  return 10 * sign;
}

int vfscanf(FILE *fp, char *fmt, va_list ap) {
  int ret = 0;
  int t, c;
  int wid = 1 << 20;
  int start_position = fp->icur;
  int skip = 0;
  while (*fmt) {
    while (isspace((unsigned char)*fmt)) {
      fmt++;
      if (!isspace(c = ic(fp))) {
        ungetc(c, fp);
      }
    }
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

    if (*fmt == 'n') {
      const int readed = fp->icur - start_position - (fp->back != EOF ? 1 : 0);
      *(va_arg(ap, int *)) = readed;
      fmt++;
      continue;
    }

    if (*fmt == '[') {
      while (isspace(c = ic(fp)))
        ;
      ungetc(c, fp);
      char scanset[256] = {0};
      int negate = 0;
      fmt++;

      if (*fmt == '^') {
        negate = 1;
        fmt++;
      }

      if (*fmt == ']') {
        fmt++;
      }
      while (*fmt && *fmt != ']') {
        if (fmt[1] == '-') {
          for (int i = fmt[0]; i <= fmt[2]; i++)
            scanset[(unsigned char)i] = 1;
          fmt += 3;
        } else {
          scanset[(unsigned char)*fmt++] = 1;
        }
      }
      if (*fmt != ']') {
        return ret;
      }
      fmt++;

      char *d = va_arg(ap, char *);
      int count = 0;
      while ((c = ic(fp)) != EOF && wid-- > 0 &&
             ((negate && !scanset[(unsigned char)c]) ||
              (!negate && scanset[(unsigned char)c]))) {
        *d++ = c;
        count++;
      }
      ungetc(c, fp);
      *d = '\0';
      if (count == 0) {
        return ret;
      }
      ret++;
      continue;
    }

    skip = 0;
    if (*fmt == '*') {
      skip = 1;
      fmt++;
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

    if (*fmt != 'c') {
      while (isspace(c = ic(fp)))
        ;
      ungetc(c, fp);
    }

    switch (*fmt++) {
    case 'c':
      const char c = ic(fp);
      if (!skip) {
        char *dst = va_arg(ap, char *);
        if (c == EOF) {
          return ret;
        }
        *dst = c;
      }
      ret += !skip;
      break;
    case 'i': {
      // const int sign = parse_sign_from_input(fp);
      const int base = parse_base_from_input(fp);
      long *dst = skip ? NULL : va_arg(ap, long *);
      if (iint(fp, dst, t, base, wid, skip)) {
        return ret;
      }
      ret += !skip;
      break;
    }
    case 'u':
    case 'd': {
      long *dst = skip ? NULL : va_arg(ap, long *);
      if (iint(fp, dst, t, 10, wid, skip))
        return ret;
      ret += !skip;
    } break;
    case 'x': {
      parse_base_from_input(fp);
      long *dst = skip ? NULL : va_arg(ap, long *);
      if (iint(fp, dst, t, 16, wid, skip))
        return ret;
      ret += !skip;
    } break;
    case 'o': {
      parse_base_from_input(fp);
      long *dst = skip ? NULL : va_arg(ap, long *);
      if (iint(fp, dst, t, 8, wid, skip))
        return ret;
      ret += !skip;
    } break;
    case 's': {
      char *dst = skip ? NULL : va_arg(ap, char *);

      if (istr(fp, dst, wid, skip))
        return ret;
      ret += !skip;
    } break;
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
  return i > 0 ? i : -1;
}

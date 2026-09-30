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
    fp->istat |= _FILE_EOF;
    return EOF;
  }

  while (fp->fd >= 0 && fp->icur == fp->ilen) {
    int nr = read(fp->fd, fp->ibuf, fp->isize);
    if (nr <= 0) {
      fp->istat |= nr ? _FILE_ERR : _FILE_EOF;
      break;
    }
    fp->ilen = nr;
    fp->icur = 0;
  }
  if (fp->icur < fp->ilen)
    return (unsigned char)fp->ibuf[fp->icur++];
  if (fp->fd < 0)
    fp->istat |= _FILE_EOF;
  return EOF;
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
  if (c == EOF || fp->back != EOF)
    return EOF;
  fp->back = (unsigned char)c;
  /* a byte to read again means the stream is no longer at its end */
  fp->istat &= ~_FILE_EOF;
  return fp->back;
}

static int idig(int c, int base) {
  static char *digs = "0123456789abcdef";
  char *r = memchr(digs, (base == 16) ? tolower(c) : c, base);
  return r == NULL ? -1 : r - digs;
}

/* Scanner state for one vfscanf call: the stream plus how many characters
   the call has consumed, which is what %n reports (the stream's own buffer
   offsets reset whenever it refills). */
struct scan {
  FILE *fp;
  int count;
};

static int sget(struct scan *s) {
  int c = ic(s->fp);
  if (c != EOF)
    s->count++;
  return c;
}

static void sunget(struct scan *s, int c) {
  if (c != EOF) {
    ungetc(c, s->fp);
    s->count--;
  }
}

static void skip_space(struct scan *s) {
  int c;
  while (isspace(c = sget(s)))
    ;
  sunget(s, c);
}

/* Store v into an integer object of `size` bytes. */
static void store_int(void *dst, int size, unsigned long long v) {
  switch (size) {
  case 1: *(unsigned char *)dst = (unsigned char)v; break;
  case 2: *(unsigned short *)dst = (unsigned short)v; break;
  case 8: *(unsigned long long *)dst = v; break;
  default: *(unsigned int *)dst = (unsigned int)v; break;
  }
}

/* An integer in `base` (0: C prefixes decide, 16: an optional 0x prefix), at
   most `wid` characters. 0 when no digits could be read. */
static int scan_int(struct scan *s, int base, int wid, unsigned long long *out) {
  unsigned long long n = 0;
  int neg = 0, digits = 0;
  int c = sget(s);
  if ((c == '-' || c == '+') && wid > 0) {
    neg = c == '-';
    wid--;
    c = sget(s);
  }
  if (wid > 0 && c == '0' && (base == 0 || base == 16)) {
    wid--;
    c = sget(s);
    if (wid > 0 && (c == 'x' || c == 'X')) {
      int c2 = sget(s);
      if (idig(c2, 16) < 0) { /* "0x" with no hex digit is not a number */
        sunget(s, c2);
        return 0;
      }
      base = 16;
      wid--;
      c = c2;
    } else {
      digits = 1; /* the 0 itself */
      if (base == 0)
        base = 8;
    }
  }
  if (base == 0)
    base = 10;
  while (wid > 0 && c != EOF && idig(c, base) >= 0) {
    n = n * base + idig(c, base);
    digits++;
    wid--;
    c = sget(s);
  }
  sunget(s, c);
  if (!digits)
    return 0;
  *out = neg ? -n : n;
  return 1;
}

int vfscanf(FILE *fp, const char *fmt, va_list ap) {
  struct scan sc = {fp, 0};
  struct scan *s = &sc;
  int ret = 0;
  int converted_any = 0; /* a conversion was attempted with input available */
  int c;

  while (*fmt) {
    /* whitespace in the format matches any amount of input whitespace */
    if (isspace((unsigned char)*fmt)) {
      while (isspace((unsigned char)*fmt))
        fmt++;
      skip_space(s);
      continue;
    }
    /* ordinary characters and %% match themselves */
    if (*fmt != '%' || fmt[1] == '%') {
      int want = (unsigned char)*fmt;
      if (*fmt == '%') {
        fmt++;
        skip_space(s);
      }
      fmt++;
      c = sget(s);
      if (c != want) {
        sunget(s, c);
        return (c == EOF && !converted_any) ? EOF : ret;
      }
      continue;
    }
    fmt++;

    int skip = 0, wid = 0, size = sizeof(int);
    if (*fmt == '*') {
      skip = 1;
      fmt++;
    }
    while (isdigit((unsigned char)*fmt))
      wid = wid * 10 + *fmt++ - '0';
    if (*fmt == 'h') {
      size = fmt[1] == 'h' ? 1 : 2;
      fmt += fmt[1] == 'h' ? 2 : 1;
    } else if (*fmt == 'l') {
      size = fmt[1] == 'l' ? (int)sizeof(long long) : (int)sizeof(long);
      fmt += fmt[1] == 'l' ? 2 : 1;
    } else if (*fmt == 'j' || *fmt == 'q' || *fmt == 'L') {
      size = sizeof(long long);
      fmt++;
    } else if (*fmt == 'z' || *fmt == 't') {
      size = sizeof(size_t);
      fmt++;
    }
    int conv = (unsigned char)*fmt++;
    if (conv == '\0')
      break;

    if (conv == 'n') {
      if (!skip)
        store_int(va_arg(ap, void *), size, (unsigned long long)s->count);
      continue;
    }

    /* every conversion but %c, %[ and %n skips leading whitespace */
    if (conv != 'c' && conv != '[')
      skip_space(s);
    c = sget(s);
    if (c == EOF)
      return (!converted_any && ret == 0) ? EOF : ret;
    sunget(s, c);
    converted_any = 1;

    switch (conv) {
    case 'c': {
      char *d = skip ? NULL : va_arg(ap, char *);
      int n = wid ? wid : 1, got = 0;
      while (got < n && (c = sget(s)) != EOF) {
        if (d)
          d[got] = (char)c;
        got++;
      }
      if (got < n)
        return ret;
      ret += !skip;
      break;
    }
    case 's': {
      char *d = skip ? NULL : va_arg(ap, char *);
      int n = wid ? wid : 1 << 30, got = 0;
      while (got < n && (c = sget(s)) != EOF && !isspace(c)) {
        if (d)
          d[got] = (char)c;
        got++;
      }
      if (got < n)
        sunget(s, c);
      if (d)
        d[got] = '\0';
      ret += !skip;
      break;
    }
    case '[': {
      char set[256] = {0};
      int negate = 0;
      if (*fmt == '^') {
        negate = 1;
        fmt++;
      }
      if (*fmt == ']') /* a leading ] is a member, not the end */
        set[(unsigned char)*fmt++] = 1;
      while (*fmt && *fmt != ']') {
        if (fmt[1] == '-' && fmt[2] && fmt[2] != ']') {
          for (int i = (unsigned char)fmt[0]; i <= (unsigned char)fmt[2]; i++)
            set[i] = 1;
          fmt += 3;
        } else {
          set[(unsigned char)*fmt++] = 1;
        }
      }
      if (*fmt != ']')
        return ret;
      fmt++;
      char *d = skip ? NULL : va_arg(ap, char *);
      int n = wid ? wid : 1 << 30, got = 0;
      while (got < n && (c = sget(s)) != EOF && (set[(unsigned char)c] != negate)) {
        if (d)
          d[got] = (char)c;
        got++;
      }
      if (got < n)
        sunget(s, c);
      if (got == 0) /* matching failure: nothing assigned */
        return ret;
      if (d)
        d[got] = '\0';
      ret += !skip;
      break;
    }
    case 'd':
    case 'u':
    case 'i':
    case 'x':
    case 'X':
    case 'o':
    case 'p': {
      int base = conv == 'i' ? 0 : (conv == 'x' || conv == 'X' || conv == 'p') ? 16 : conv == 'o' ? 8 : 10;
      unsigned long long v;
      if (conv == 'p')
        size = sizeof(void *);
      if (!scan_int(s, base, wid ? wid : 1 << 30, &v))
        return ret;
      if (!skip)
        store_int(va_arg(ap, void *), size, v);
      ret += !skip;
      break;
    }
    default: /* unsupported conversion */
      return ret;
    }
  }
  return ret;
}

int fscanf(FILE *fp, const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vfscanf(fp, fmt, ap);
  va_end(ap);
  return ret;
}

int scanf(const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vfscanf(stdin, fmt, ap);
  va_end(ap);
  return ret;
}

int vsscanf(const char *s, const char *fmt, va_list ap) {
  FILE f = {-1, EOF};
  f.ibuf = (char *)s;
  f.ilen = strlen(s);
  return vfscanf(&f, fmt, ap);
}

int sscanf(const char *s, const char *fmt, ...) {
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

/* Chunked: drain the pushback byte and any buffered bytes, then read()
   straight into the caller's buffer when the remainder is at least one
   buffer's worth. String sources (sscanf's ibuf==obuf FILEs) and anything
   else unusual keep the ic() path. */
long fread(void *v, long sz, long n, FILE *fp) {
  char *s = v;
  long total = n * sz;
  long got = 0;
  if (total <= 0)
    return 0;
  while (got < total) {
    if (fp->back != EOF || fp->fd < 0 || fp->ibuf == NULL) {
      int c = ic(fp);
      if (c == EOF)
        break;
      s[got++] = c;
      continue;
    }
    if (fp->icur < fp->ilen) {
      long avail = fp->ilen - fp->icur;
      if (avail > total - got)
        avail = total - got;
      memcpy(s + got, fp->ibuf + fp->icur, avail);
      fp->icur += avail;
      got += avail;
      continue;
    }
    if (total - got >= (long)fp->isize) {
      long r = read(fp->fd, s + got, total - got);
      if (r <= 0) {
        fp->istat |= r ? _FILE_ERR : _FILE_EOF;
        break;
      }
      got += r;
    } else {
      int c = ic(fp);
      if (c == EOF)
        break;
      s[got++] = c;
    }
  }
  return got / sz;
}

/* Whether a read has already found the end, as C says: not a peek. The old
   peek kept the byte in a `char`, unsigned on ARM, so EOF never compared equal
   -- feof() was never true and pushed back a 0xFF for fread() to return, and
   make's "while (!feof(stdin)) fread()" copied `make -f -` input forever (a
   pipe peek would block, too). */
int feof(FILE *fp) {
  return (fp->istat & _FILE_EOF) != 0;
}

/* `getline` is `getdelim` with the delimiter spelled out -- and it lives here
 * only because it always has. The implementation that used to be here had its
 * own buffer growth, its own idea of what `*n` meant, and a heap overflow to
 * go with it; see the note on getdelim in stdio.c. */
ssize_t getline(char **lineptr, size_t *n, FILE *fp) {
  return getdelim(lineptr, n, '\n', fp);
}

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

static char _ibuf[BUFSIZ], _obuf[BUFSIZ], _ebuf[BUFSIZ];
static FILE _stdin = {0, EOF, _ibuf, NULL, BUFSIZ, 0};
static FILE _stdout = {1, EOF, NULL, _obuf, 0, BUFSIZ};
static FILE _stderr = {2, EOF, NULL, _ebuf, 0, 1};
FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

FILE *tmpfile(void) {
  printf("TODO: implement tmpfile()\n");
  return NULL;
}

FILE *fopen(const char *path, const char *mode) {
  FILE *fp;
  int flags;

  if (strchr(mode, '+'))
    flags = O_RDWR;
  else
    flags = *mode == 'r' ? O_RDONLY : O_WRONLY;
  if (*mode != 'r')
    flags |= O_CREAT;
  if (*mode == 'w')
    flags |= O_TRUNC;
  if (*mode == 'a')
    flags |= O_APPEND;

  fp = malloc(sizeof(*fp));
  memset(fp, 0, sizeof(*fp));
  fp->fd = open(path, flags, 0600);
  if (fp->fd < 0) {
    free(fp);
    return NULL;
  }
  fp->back = EOF;
  fp->ibuf = malloc(BUFSIZ);
  fp->obuf = malloc(BUFSIZ);
  fp->isize = BUFSIZ;
  fp->osize = BUFSIZ;
  fp->iown = 1;
  fp->oown = 1;
  return fp;
}

FILE *fdopen(int fd, const char *mode) {
  FILE *fp;
  int flags;

  if (strchr(mode, '+'))
    flags = O_RDWR;
  else
    flags = *mode == 'r' ? O_RDONLY : O_WRONLY;
  if (*mode != 'r')
    flags |= O_CREAT;
  if (*mode == 'w')
    flags |= O_TRUNC;
  if (*mode == 'a')
    flags |= O_APPEND;

  fp = malloc(sizeof(*fp));
  memset(fp, 0, sizeof(*fp));
  if (fcntl(fd, F_GETFL) < 0) {
    free(fp);
    return NULL;
  }
  fp->fd = fd;
  fp->back = EOF;
  fp->ibuf = malloc(BUFSIZ);
  fp->obuf = malloc(BUFSIZ);
  fp->isize = BUFSIZ;
  fp->osize = BUFSIZ;
  fp->iown = 1;
  fp->oown = 1;
  return fp;
}

int fclose(FILE *fp) {
  fflush(fp);
  int ret = close(fp->fd);
  if (fp->iown)
    free(fp->ibuf);
  if (fp->oown)
    free(fp->obuf);
  free(fp);
  return ret;
}

int fflush(FILE *fp) {
  if (fp->fd < 0)
    return 0;
  if (write(fp->fd, fp->obuf, fp->olen) != fp->olen)
    return EOF;
  fp->olen = 0;
  return 0;
}

int fputc(int c, FILE *fp) {
  if (fp->osize == 0) {
    if (write(fp->fd, (char *)&c, 1) != 1)
      return EOF;
    return c;
  }
  if (fp->olen < fp->osize) {
    fp->obuf[fp->olen++] = c;
    fp->ostat++;
  }
  if (c == '\n' || fp->olen == fp->osize)
    if (fflush(fp))
      return EOF;
  return c;
}

int putchar(int c) {
  return fputc(c, stdout);
}

static int ostr(FILE *fp, char *s, int wid, int left, int max_len,
                char fill_character) {
  int str_len = strlen(s);
  str_len = str_len < max_len ? str_len : max_len;
  int fill = wid - str_len;
  if (fill < 0)
    fill = 0;
  int n = 0;
  if (max_len < INT_MAX - fill)
    max_len += fill;
  if (!left)
    while (n < fill) {
      fputc(fill_character, fp);
      ++n;
    }
  while (*s && n < max_len) {
    fputc((unsigned char)*s++, fp);
    ++n;
  }
  if (left)
    while (fill-- > 0) {
      fputc(fill_character, fp);
      ++n;
    }
  return n;
}

static int digits(unsigned long long n, int base) {
  int i;
  for (i = 0; n; i++)
    n /= base;
  return i ? i : 1;
}

static char *digs = "0123456789abcdef";
static char *digs_uc = "0123456789ABCDEF";

typedef union {
  double d;
  unsigned long long u;
} fmt_double_bits;

#define FMT_LEFT 0001   /* flag '-' */
#define FMT_PLUS 0002   /* flag '+' */
#define FMT_BLANK 0004  /* flag ' ' */
#define FMT_ALT 0010    /* flag '#' */
#define FMT_ZERO 0020   /* flag '0' */
#define FMT_SIGNED 0040 /* is the conversion signed? */
#define FMT_UCASE 0100  /* uppercase hex digits? */

static double fmt_pow10(int exp) {
  double value = 1.0;

  while (exp > 0) {
    value *= 10.0;
    --exp;
  }
  while (exp < 0) {
    value /= 10.0;
    ++exp;
  }
  return value;
}

static int fmt_ilog10(double value) {
  int exp = 0;

  if (value <= 0.0)
    return 0;
  while (value >= 10.0) {
    value /= 10.0;
    ++exp;
  }
  while (value < 1.0) {
    value *= 10.0;
    --exp;
  }
  return exp;
}

static char *fmt_append_uint_dec(char *out, unsigned int value,
                                 int min_digits) {
  char tmp[16];
  int len = 0;

  do {
    tmp[len++] = '0' + (value % 10);
    value /= 10;
  } while (value > 0);

  while (len < min_digits)
    tmp[len++] = '0';

  while (len-- > 0)
    *out++ = tmp[len];

  return out;
}

static char *fmt_build_fixed(char *out, double value, int precision,
                             int alt_form) {
  int mag;

  if (value != 0.0)
    value += fmt_pow10(-precision) / 2.0;

  mag = value == 0.0 ? 0 : fmt_ilog10(value);
  if (mag < 0)
    mag = 0;

  do {
    double scale;
    int digit;

    if (mag == -1 && (precision > 0 || alt_form))
      *out++ = '.';
    scale = fmt_pow10(mag);
    digit = scale == 0.0 ? 0 : (int)(value / scale);
    if (digit < 0)
      digit = 0;
    if (digit > 9)
      digit = 9;
    value -= digit * scale;
    if (value < 0.0)
      value = 0.0;
    *out++ = '0' + digit;
  } while (--mag >= -precision);

  if (precision == 0 && alt_form)
    *out++ = '.';

  *out = '\0';
  return out;
}

static char *fmt_build_scientific(char *out, double value, int precision,
                                  int alt_form, int upper) {
  int exp = value == 0.0 ? 0 : fmt_ilog10(value);
  unsigned int abs_exp;

  if (value != 0.0)
    value /= fmt_pow10(exp);
  if (value != 0.0)
    value += fmt_pow10(-precision) / 2.0;
  if (value >= 10.0) {
    value /= 10.0;
    ++exp;
  }

  {
    int mag = 0;

    do {
      double scale;
      int digit;

      if (mag == -1 && (precision > 0 || alt_form))
        *out++ = '.';
      scale = fmt_pow10(mag);
      digit = scale == 0.0 ? 0 : (int)(value / scale);
      if (digit < 0)
        digit = 0;
      if (digit > 9)
        digit = 9;
      value -= digit * scale;
      if (value < 0.0)
        value = 0.0;
      *out++ = '0' + digit;
    } while (--mag >= -precision);
  }

  if (precision == 0 && alt_form)
    *out++ = '.';

  *out++ = upper ? 'E' : 'e';
  *out++ = exp < 0 ? '-' : '+';
  abs_exp = (unsigned int)(exp < 0 ? -exp : exp);
  out = fmt_append_uint_dec(out, abs_exp, 2);
  *out = '\0';

  return out;
}

static void fmt_trim_trailing_zeros(char *buf, int alt_form) {
  const char *dot;
  const char *exp;
  const char *end;
  char *trim_end;

  if (alt_form)
    return;

  dot = strchr(buf, '.');
  if (!dot)
    return;

  exp = strchr(buf, 'e');
  if (!exp)
    exp = strchr(buf, 'E');
  end = exp ? exp - 1 : buf + strlen(buf) - 1;

  while (end > dot && *end == '0')
    --end;
  if (end == dot)
    --end;

  trim_end = buf + (end - buf);

  if (exp)
    memmove(trim_end + 1, exp, strlen(exp) + 1);
  else
    trim_end[1] = '\0';
}

static int fmt_emit_float(FILE *fp, const char *buf, int wid, int flags,
                          int zero_flag, char sign) {
  int fill_zero = zero_flag && !(flags & FMT_LEFT);
  int left = flags & FMT_LEFT;
  int len = strlen(buf) + (sign ? 1 : 0);
  int size = 0;

  if (!fill_zero && !left)
    while (len < wid) {
      fputc(' ', fp);
      ++size;
      ++len;
    }
  if (sign) {
    fputc(sign, fp);
    ++size;
  }
  if (fill_zero && !left)
    while (len < wid) {
      fputc('0', fp);
      ++size;
      ++len;
    }
  size += ostr(fp, (char *)buf, 0, 0, INT_MAX, ' ');
  if (left)
    while (len < wid) {
      fputc(' ', fp);
      ++size;
      ++len;
    }

  return size;
}

static int fmt_general_exponent(double value, int precision) {
  int exp;

  if (value == 0.0)
    return 0;

  exp = fmt_ilog10(value);
  value /= fmt_pow10(exp);
  value += fmt_pow10(1 - precision) / 2.0;
  if (value >= 10.0)
    ++exp;

  return exp;
}

static int ofloat(FILE *fp, double value, int wid, int flags, int precision,
                  int zero_flag, int upper, int general) {
  char buf[384];
  char sign = '\0';
  fmt_double_bits bits = {.d = value};
  unsigned long long frac_mask = (1ULL << 52) - 1;
  unsigned long long exponent = (bits.u >> 52) & 0x7ff;

  if (bits.u >> 63) {
    sign = '-';
    bits.u &= ~(1ULL << 63);
    value = bits.d;
  } else if (flags & FMT_PLUS) {
    sign = '+';
  } else if (flags & FMT_BLANK) {
    sign = ' ';
  }

  if (precision == INT_MAX || precision < 0)
    precision = 6;
  if (general && precision == 0)
    precision = 1;
  if (precision > 120)
    precision = 120;

  if (exponent == 0x7ff) {
    int is_nan = (bits.u & frac_mask) != 0;
    const char *special =
        is_nan ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");

    if (is_nan && !(flags & (FMT_PLUS | FMT_BLANK)))
      sign = '\0';

    return fmt_emit_float(fp, special, wid, flags, zero_flag, sign);
  }

  if (general) {
    int exp = fmt_general_exponent(value, precision);

    if (exp < -4 || exp >= precision)
      fmt_build_scientific(buf, value, precision - 1, flags & FMT_ALT, upper);
    else
      fmt_build_fixed(buf, value, precision - (exp + 1), flags & FMT_ALT);
    fmt_trim_trailing_zeros(buf, flags & FMT_ALT);
  } else {
    fmt_build_fixed(buf, value, precision, flags & FMT_ALT);
  }

  return fmt_emit_float(fp, buf, wid, flags, zero_flag, sign);
}

static int oint(FILE *fp, unsigned long long n, int base, int wid, int bytes,
                int flags, int max_len) {
  char buf[64];
  char *s = buf;
  int sign = '\0';
  char fill;
  int left = flags & FMT_LEFT;
  int alt_form = flags & FMT_ALT;
  int ucase = flags & FMT_UCASE;
  int prefix_len = 0; /* length of sign or base prefix */
  int d;
  int i;
  int size = 0;

  if (flags & FMT_SIGNED) {
    if ((bytes == 4 && ((int)n < 0)) || (bytes == 8 && ((long long)n < 0)) ||
        (bytes == 2 && ((short)n < 0)) || (bytes == 1 && ((char)n < 0))) {
      sign = '-';
      n = -n;
    } else {
      if (flags & FMT_PLUS)
        sign = '+';
      else if (flags & FMT_BLANK)
        sign = ' ';
    }
    prefix_len = !!sign;
  } else if (n > 0 && alt_form) {
    prefix_len = base == 16 ? 2 : 1;
  }
  if (bytes == 1)
    n &= 0x000000ff;
  if (bytes == 2)
    n &= 0x0000ffff;
  if (bytes == 4)
    n &= 0xffffffff;
  d = digits(n, base);
  for (i = 0; i < d; i++) {
    s[d - i - 1] = ucase ? digs_uc[n % base] : digs[n % base];
    n /= base;
  }
  s[d] = '\0';
  fill = (flags & FMT_ZERO) ? '0' : ' ';
  i = d + prefix_len;
  if (max_len < INT_MAX - (sign ? 1 : 0)) {
    max_len += sign ? 1 : 0;
  }

  if (max_len != INT_MAX) {
    wid = max_len;
  }

  if (fill == ' ' && !left)
    while (i++ < wid) {
      fputc(' ', fp);
      ++size;
    }
  if (sign) {
    fputc(sign, fp);
    ++size;
  } else if (prefix_len) {
    fputc('0', fp);
    ++size;
    if (base == 16) {
      fputc(ucase ? 'X' : 'x', fp);
      ++size;
    }
  }
  if (fill == '0' && !left)
    while (i++ < wid) {
      fputc('0', fp);
      ++size;
    }
  size += ostr(fp, buf, 0, 0, max_len, '0');
  if (left)
    while (i++ < wid) {
      fputc(' ', fp);
      ++size;
    }
  return size;
}

static const char *fmt_flags = "-+ #0";

int vfprintf(FILE *fp, const char *fmt, va_list ap) {
  const char *s = fmt;
  int beg = fp->ostat;
  int n = 0;
  while (*s) {
    int c = (unsigned char)*s++;
    int wid = 0;
    int bytes = sizeof(int);
    int flags = 0;
    int left;
    int zero_flag;
    int float_long = 0;
    int max_len = INT_MAX;
    const char *f;
    if (c != '%') {
      fputc(c, fp);
      ++n;
      continue;
    }
    while ((f = strchr(fmt_flags, *s))) {
      flags |= 1 << (f - fmt_flags);
      s++;
    }
    left = flags & FMT_LEFT;
    zero_flag = flags & FMT_ZERO;
    if (*s == '*') {
      wid = va_arg(ap, int);
      if (wid < 0) {
        flags |= FMT_LEFT;
        left = 1;
        wid = -wid;
      }
      s++;
    } else {
      while (isdigit(*s)) {
        wid *= 10;
        wid += *s++ - '0';
      }
    }

    if (*s == '.') {
      ++s;
      flags |= FMT_ZERO;
      if (*s == '*') {
        max_len = va_arg(ap, int);
        s++;
        if (max_len < 0) {
          max_len = INT_MAX;
        }
      } else {
        if (isdigit(*s)) {
          max_len = 0;
        }
        while (isdigit(*s)) {
          max_len *= 10;
          max_len += *s++ - '0';
        }
        if (max_len < 0) {
          max_len = -max_len;
          flags |= FMT_LEFT;
        }
      }
    }

    {
      int l_count = 0;
      while (*s == 'l') {
        l_count++;
        s++;
      }
      if (l_count >= 2)
        bytes = sizeof(long long);
      else if (l_count == 1)
        bytes = sizeof(long);
    }
    while (*s == 'h') {
      bytes = bytes < sizeof(int) ? sizeof(char) : sizeof(short);
      s++;
    }
    if (*s == 'L') {
      float_long = 1;
      s++;
    }
    switch ((c = *s++)) {
    case 'd':
    case 'i':
      flags |= FMT_SIGNED;
      if (bytes >= (int)sizeof(long long))
        n += oint(fp, va_arg(ap, long long), 10, wid, bytes, flags, max_len);
      else
        n += oint(fp, va_arg(ap, long), 10, wid, bytes, flags, max_len);
      break;
    case 'u':
      flags &= ~FMT_ALT;
      if (bytes >= (int)sizeof(long long))
        n += oint(fp, va_arg(ap, unsigned long long), 10, wid, bytes, flags,
                  max_len);
      else
        n +=
            oint(fp, va_arg(ap, unsigned long), 10, wid, bytes, flags, max_len);
      break;
    case 'o':
      if (bytes >= (int)sizeof(long long))
        n += oint(fp, va_arg(ap, unsigned long long), 8, wid, bytes, flags,
                  max_len);
      else
        n += oint(fp, va_arg(ap, unsigned long), 8, wid, bytes, flags, max_len);
      break;
    case 'p':
      flags |= FMT_ALT;
    case 'x':
      if (bytes >= (int)sizeof(long long))
        n += oint(fp, va_arg(ap, unsigned long long), 16, wid, bytes, flags,
                  max_len);
      else
        n +=
            oint(fp, va_arg(ap, unsigned long), 16, wid, bytes, flags, max_len);
      break;
    case 'X':
      flags |= FMT_UCASE;
      if (bytes >= (int)sizeof(long long))
        n += oint(fp, va_arg(ap, unsigned long long), 16, wid, bytes, flags,
                  max_len);
      else
        n +=
            oint(fp, va_arg(ap, unsigned long), 16, wid, bytes, flags, max_len);
      break;
    case 'c':
      if (left) {
        fputc(va_arg(ap, int), fp);
        ++n;
      }
      while (wid-- > 1) {
        fputc(' ', fp);
        ++n;
      }
      if (!left) {
        fputc(va_arg(ap, int), fp);
        ++n;
      }
      break;
    case 's':
      n += ostr(fp, va_arg(ap, char *), wid, left, max_len, ' ');
      break;
    case 'f':
    case 'F':
      if (float_long)
        n += ofloat(fp, (double)va_arg(ap, long double), wid, flags, max_len,
                    zero_flag, c == 'F', 0);
      else
        n += ofloat(fp, va_arg(ap, double), wid, flags, max_len, zero_flag,
                    c == 'F', 0);
      break;
    case 'g':
    case 'G':
      if (float_long)
        n += ofloat(fp, (double)va_arg(ap, long double), wid, flags, max_len,
                    zero_flag, c == 'G', 1);
      else
        n += ofloat(fp, va_arg(ap, double), wid, flags, max_len, zero_flag,
                    c == 'G', 1);
      break;
    case 'n':
      *va_arg(ap, int *) = fp->ostat - beg;
      break;
    case '\0':
      s--;
      break;
    default: {
      fputc(c, fp);
      ++n;
    }
    }
  }
  return n;
}

void perror(const char *s) {
  int idx = errno;
  if (idx >= sys_nerr)
    idx = 0;
  if (s && *s)
    fprintf(stderr, "%s: %s\n", s, sys_errlist[idx]);
  else
    fprintf(stderr, "%s\n", sys_errlist[idx]);
}

int vsnprintf(char *dst, size_t sz, const char *fmt, va_list ap) {
  FILE f = {-1, EOF};
  int ret;
  f.obuf = dst;
  f.osize = sz - 1;
  ret = vfprintf(&f, fmt, ap);
  dst[f.olen] = '\0';
  return ret;
}

int vsprintf(char *dst, const char *fmt, va_list ap) {
  return vsnprintf(dst, 1 << 20, fmt, ap);
}

int printf(const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vfprintf(stdout, fmt, ap);
  va_end(ap);
  return ret;
}

int vprintf(const char *fmt, va_list ap) {
  return vfprintf(stdout, fmt, ap);
}

int fprintf(FILE *fp, const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vfprintf(fp, fmt, ap);
  va_end(ap);
  return ret;
}

int sprintf(char *dst, const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vsprintf(dst, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *dst, size_t sz, const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vsnprintf(dst, sz, fmt, ap);
  va_end(ap);
  return ret;
}

int fputs(const char *s, FILE *fp) {
  if (!fp)
    return EOF;
  int n = 0;
  while (*s) {
    if (fputc((unsigned char)*s++, fp) == EOF)
      return EOF;
    n++;
  }
  return n;
}

int puts(const char *s) {
  int ret = fputs(s, stdout);
  if (ret == EOF)
    return EOF;
  if (fputc('\n', stdout) == EOF)
    return EOF;
  return ret + 1;
}

long fwrite(void *v, long sz, long n, FILE *fp) {
  unsigned char *s = v;
  int i = n * sz;
  while (i-- > 0)
    if (fputc(*s++, fp) == EOF)
      return n * sz - i - 1;
  return n * sz;
}

int fseek(FILE *fp, long offset, int whence) {
  if (fp->fd < 0) {
    return 0;
  }
  fflush(fp);
  if (lseek(fp->fd, offset, whence) < 0)
    return -1;
  fp->ilen = fp->olen = 0;
  fp->icur = 0;
  return 0;
}

long ftell(FILE *fp) {
  if (fp->fd < 0)
    return -1;
  fflush(fp);
  return lseek(fp->fd, 0, SEEK_CUR) - fp->ilen + fp->icur;
}

int ferror(FILE *fp) {
  return 0;
}

int pclose(FILE *stream) {
  int ret = fclose(stream);
  if (ret < 0)
    return EOF;
  return ret;
}

FILE *popen(const char *command, const char *type) {
  return NULL;
}

int rename(const char *oldpath, const char *newpath) {
  printf("TODO: Implement rename\n");
  return -1; // Not implemented
}

FILE *fmemopen(void *buf, size_t size, const char *mode) {
  FILE *fp;
  int flags;

  if (strchr(mode, '+'))
    flags = O_RDWR;
  else
    flags = *mode == 'r' ? O_RDONLY : O_WRONLY;
  if (*mode != 'r')
    flags |= O_CREAT;
  if (*mode == 'w')
    flags |= O_TRUNC;
  if (*mode == 'a')
    flags |= O_APPEND;

  fp = malloc(sizeof(*fp));
  memset(fp, 0, sizeof(*fp));
  fp->fd = -1;
  if (buf == NULL) {
    fp->ibuf = malloc(size);
    fp->iown = 1;
    fp->obuf = fp->ibuf;
  } else {
    fp->ibuf = buf;
    fp->obuf = buf;
  }

  fp->isize = size;

  if (flags & O_WRONLY || flags & O_RDWR) {
    fp->osize = fp->isize;
  }
  fp->ilen = strnlen(buf, size);
  fp->icur = 0;

  fp->olen = fp->ilen;
  fp->back = EOF;
  fp->oown = 0;
  return fp;
}

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream) {
  printf("TODO: Implement getdelim\n");
  return -1; // Not implemented
}

void clearerr(FILE *fp) {
  // printf("TODO: Implement clearerr\n");
}

int setvbuf(FILE *fp, char *buf, int mode, size_t size) {
  if (fp->fd < 0)
    return -1;
  if (buf == NULL) {
    fp->obuf = malloc(size);
    fp->oown = 1;
  } else {
    fp->obuf = buf;
    fp->oown = 0;
  }
  fp->osize = size;
  fp->olen = 0;
  return 0;
}
int vdprintf(int fildes, const char *format, va_list ap) {
  char buf[BUFSIZ];
  FILE f = {fildes, EOF, NULL, buf, 0, sizeof(buf)};
  int ret = vfprintf(&f, format, ap);
  if (f.olen > 0)
    write(fildes, f.obuf, f.olen);
  return ret;
}

int dprintf(int fildes, const char *format, ...) {
  va_list ap;
  int ret;
  va_start(ap, format);
  ret = vdprintf(fildes, format, ap);
  va_end(ap);
  return ret;
}

int fileno(FILE *fp) {
  if (fp == NULL) {
    errno = EBADF;
    return -1;
  }
  return fp->fd;
}

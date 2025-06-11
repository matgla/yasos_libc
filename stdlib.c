#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>

#define ATEXIT_MAX 32

char **environ;

char *itoa(int n, char *s, int base) {
  char *p = s;
  int sign = n < 0 ? -1 : 1;
  if (sign < 0)
    n = -n;
  do {
    *p++ = "0123456789abcdef"[n % base];
    n /= base;
  } while (n);
  if (sign < 0)
    *p++ = '-';
  *p-- = '\0';
  while (s < p) {
    char tmp = *s;
    *s++ = *p;
    *p-- = tmp;
  }
  return s;
}

int abs(int n) {
  return n >= 0 ? n : -n;
}

double atof(const char *str) {
  return strtod(str, NULL);
}

long int labs(long int n) {
  return n >= 0 ? n : -n;
}

long long int llabs(long long int n) {
  return n >= 0 ? n : -n;
}

char *getenv(const char *name) {
  char **p = environ;
  int len = strlen(name);
  for (; *p; p++)
    if (!memcmp(name, *p, len) && (*p)[len] == '=')
      return *p + len + 1;
  return NULL;
}

int putenv(char *string) {
  printf("TODO: Implement putenv\n");
  return -1; // Not implemented
}

int system(char *cmd) {
  char *argv[] = {"/bin/sh", "-c", cmd, NULL};
  pid_t pid;
  int ret;
  pid = vfork();
  if (pid < 0)
    return -1;
  if (!pid) {
    execv(argv[0], argv);
    exit(1);
  }
  if (waitpid(pid, &ret, 0) != pid)
    return -1;
  return ret;
}

/* ISO C forbids direct conversion between function pointers and void *.
 * Use a union to store arbitrary handler types without violating pedantic. */
typedef union {
  void (*as_void)(void);
  void (*as_exit)(int, void *);
  void *as_ptr;
} atexit_handler;

static atexit_handler atexit_func[ATEXIT_MAX];
static void *atexit_arg[ATEXIT_MAX];
static int atexit_cnt;
void *__yasos_fini_table;
void *__yasos_fini_got;

extern void __call_with_got(void (*func)(void), void *got_base);

int on_exit(void (*func)(int, void *), void *arg) {
  if (atexit_cnt >= ATEXIT_MAX)
    return -1;
  atexit_func[atexit_cnt].as_exit = func;
  atexit_arg[atexit_cnt] = arg;
  atexit_cnt++;
  return 0;
}

int atexit(void (*func)(void)) {
  if (atexit_cnt >= ATEXIT_MAX)
    return -1;
  atexit_func[atexit_cnt].as_void = func;
  atexit_arg[atexit_cnt] = 0;
  atexit_cnt++;
  return 0;
}

void __libc_finalize_and_exit(int status) {
  /* Run destructors from the YAFF initfini table.
   * These are raw code addresses (not thunked), so we call them
   * via __call_with_got which switches R9 to the originating
   * module's GOT base. */
  void *ft = __yasos_fini_table;
  __yasos_fini_table = 0;
  if (ft && __yasos_fini_got) {
    unsigned int *tbl = (unsigned int *)ft;
    unsigned int init_count = tbl[0];
    unsigned int fini_count = tbl[1];
    void (**fini_funcs)(void) = (void (**)(void))&tbl[2 + init_count];
    unsigned int i;
    for (i = fini_count; i > 0;) {
      --i;
      __call_with_got(fini_funcs[i], __yasos_fini_got);
    }
  }
  /* Run atexit/on_exit handlers (LIFO) */
  while (atexit_cnt > 0) {
    --atexit_cnt;
    (atexit_func[atexit_cnt].as_exit)(status, atexit_arg[atexit_cnt]);
  }
  fflush(stdout);
  fflush(stderr);
  _exit(status);
}

// TODO: Verify that
unsigned long long int strtoull(const char *nptr, char **endptr, int base) {
  unsigned long long int result = 0;
  while (*nptr && *nptr != ' ' && *nptr != '\t') {
    if (*nptr >= '0' && *nptr <= '9')
      result = result * base + (*nptr - '0');
    else if (*nptr >= 'a' && *nptr <= 'z')
      result = result * base + (*nptr - 'a' + 10);
    else if (*nptr >= 'A' && *nptr <= 'Z')
      result = result * base + (*nptr - 'A' + 10);
    else
      break;
    nptr++;
  }
  if (endptr)
    *endptr = (char *)nptr;
  return result;
}

double strtod(const char *nptr, char **endptr) {
  const char *s = nptr;
  int sign = 1;

  /* Skip leading whitespace */
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
    s++;

  /* Sign */
  if (*s == '-') {
    sign = -1;
    s++;
  } else if (*s == '+') {
    s++;
  }

  /* Accumulate the mantissa as an exact 64-bit integer and track the decimal
   * exponent separately; scale once at the end with correctly-rounded IEEE
   * multiplies/divides by exact powers of ten.  A digit-by-digit
   * `frac *= 0.1` accumulation is ~1 ulp low on most fractions (0.1 is not
   * representable), which broke FP literal parsing in the on-target tcc:
   * "1.25" parsed to the double just below 1.25. */
  unsigned long long mant = 0;
  int dec_exp = 0;
  int digits = 0;

  /* Integer part */
  while (*s >= '0' && *s <= '9') {
    if (mant != 0 || *s != '0') {
      if (digits < 19) {
        mant = mant * 10 + (unsigned long long)(*s - '0');
        digits++;
      } else {
        dec_exp++;
      }
    }
    s++;
  }

  /* Fractional part */
  if (*s == '.') {
    s++;
    while (*s >= '0' && *s <= '9') {
      if (mant == 0 && *s == '0') {
        dec_exp--; /* leading fractional zero: pure scale-down */
      } else if (digits < 19) {
        mant = mant * 10 + (unsigned long long)(*s - '0');
        digits++;
        dec_exp--;
      }
      s++;
    }
  }

  /* Exponent part (only if at least one digit follows the 'e'/sign) */
  if (*s == 'e' || *s == 'E') {
    const char *e_start = s;
    s++;
    int exp_sign = 1;
    if (*s == '-') {
      exp_sign = -1;
      s++;
    } else if (*s == '+') {
      s++;
    }
    if (*s >= '0' && *s <= '9') {
      int exp_val = 0;
      while (*s >= '0' && *s <= '9') {
        if (exp_val < 100000)
          exp_val = exp_val * 10 + (*s - '0');
        s++;
      }
      dec_exp += exp_sign * exp_val;
    } else {
      s = e_start; /* bare 'e' is not part of the number */
    }
  }

  /* 10^0..10^22 are exactly representable as doubles, so a single multiply
   * or divide below is correctly rounded.  Larger exponents step in chunks
   * (rare; may cost an extra rounding). */
  static const double pow10tab[23] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                                      1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                                      1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};
  double result = (double)mant;
  if (result != 0.0) {
    int e = dec_exp;
    while (e > 0) {
      int step = e > 22 ? 22 : e;
      result *= pow10tab[step];
      e -= step;
    }
    while (e < 0) {
      int step = e < -22 ? 22 : -e;
      result /= pow10tab[step];
      e += step;
    }
  }

  if (endptr)
    *endptr = (char *)s;
  return sign < 0 ? -result : result;
}

float strtof(const char *nptr, char **endptr) {
  return (float)strtod(nptr, endptr);
}

long double strtold(const char *nptr, char **endptr) {
  return (long double)strtod(nptr, endptr);
}

long long int strtoll(const char *nptr, char **endptr, int base) {
  long long int result = 0;
  while (*nptr && *nptr != ' ' && *nptr != '\t') {
    if (*nptr >= '0' && *nptr <= '9')
      result = result * base + (*nptr - '0');
    else if (*nptr >= 'a' && *nptr <= 'z')
      result = result * base + (*nptr - 'a' + 10);
    else if (*nptr >= 'A' && *nptr <= 'Z')
      result = result * base + (*nptr - 'A' + 10);
    else
      break;
    nptr++;
  }
  if (endptr)
    *endptr = (char *)nptr;
  return result;
}

long long atoll(const char *nptr) {
  return strtoll(nptr, NULL, 10);
}

static unsigned long seed = 1;

long random(void) {
  seed = (seed * 1103515245 + 12345) & RAND_MAX;
  return seed;
}

void srandom(unsigned int seed) {
  if (seed == 0)
    seed = 1; // Avoid zero seed
  unsigned long long int s = seed;
  s = (s * 1103515245 + 12345) & RAND_MAX;
  seed = (unsigned long)s;
}

int setenv(const char *name, const char *value, int overwrite) {
  printf("TODO: Implement setenv\n");
  return -1; // Not implemented
}

int unsetenv(const char *name) {
  printf("TODO: Implement unsetenv\n");
  return 0; // Not implemented
}

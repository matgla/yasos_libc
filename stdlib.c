#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>

/* 16 atexit handlers is plenty for the programs we run; each unused slot is
 * a function pointer + an arg pointer in per-process .bss. */
#define ATEXIT_MAX 16

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

/* The environment. `environ` is whatever the start code handed over, or
 * whatever the program put there itself; setenv/putenv/unsetenv change it in
 * place while it is the array they allocated, and copy it into a new one the
 * first time they meet an array they did not.
 *
 * The strings setenv builds are remembered, so replacing or removing one frees
 * it; the ones putenv was given and those inherited from exec belong to
 * someone else and are left alone. */
static char **env_array;
static size_t env_capacity;
static char **env_owned;
static size_t env_owned_count;
static size_t env_owned_capacity;

/* Length of a variable name: up to '=', or the whole string. */
static size_t env_name_length(const char *s) {
  size_t n = 0;
  while (s[n] && s[n] != '=')
    n++;
  return n;
}

static int env_matches(const char *entry, const char *name, size_t len) {
  return !memcmp(entry, name, len) && entry[len] == '=';
}

static size_t env_count(void) {
  size_t n = 0;
  if (environ)
    while (environ[n])
      n++;
  return n;
}

static int env_remember(char *s) {
  if (env_owned_count == env_owned_capacity) {
    size_t capacity = env_owned_capacity ? env_owned_capacity * 2 : 8;
    char **grown = realloc(env_owned, capacity * sizeof(*grown));
    if (!grown)
      return -1;
    env_owned = grown;
    env_owned_capacity = capacity;
  }
  env_owned[env_owned_count++] = s;
  return 0;
}

/* Free `s` if setenv built it; leave it alone otherwise. */
static void env_release(char *s) {
  for (size_t i = 0; i < env_owned_count; i++) {
    if (env_owned[i] == s) {
      env_owned[i] = env_owned[--env_owned_count];
      free(s);
      return;
    }
  }
}

/* Make room for `count` entries plus the terminator in an array this file
 * owns, copying the current one over when it is someone else's. */
static int env_reserve(size_t count) {
  size_t needed = count + 1;
  if (environ == env_array && env_array && needed <= env_capacity)
    return 0;
  size_t capacity = needed < 8 ? 8 : needed + needed / 2;
  char **grown;
  if (environ == env_array && env_array) {
    grown = realloc(env_array, capacity * sizeof(*grown));
    if (!grown)
      return -1;
  } else {
    /* Someone else's array (the start code's, or one the program set): copy
     * it and never free it. An array of ours the program swapped out is not
     * freed either -- it may still hold it. */
    grown = malloc(capacity * sizeof(*grown));
    if (!grown)
      return -1;
    for (size_t i = 0; i < count; i++)
      grown[i] = environ[i];
    grown[count] = NULL;
  }
  env_array = grown;
  env_capacity = capacity;
  environ = grown;
  return 0;
}

/* Put "name=value" `entry` (name `len` bytes long) in the environment,
 * replacing a variable of that name. */
static int env_put(char *entry, size_t len) {
  size_t n = env_count();
  for (size_t i = 0; i < n; i++) {
    if (env_matches(environ[i], entry, len)) {
      char *old = environ[i];
      environ[i] = entry;
      if (old != entry)
        env_release(old);
      return 0;
    }
  }
  if (env_reserve(n + 1) < 0) {
    errno = ENOMEM;
    return -1;
  }
  environ[n] = entry;
  environ[n + 1] = NULL;
  return 0;
}

static int env_valid_name(const char *name) {
  return name && *name && !strchr(name, '=');
}

char *getenv(const char *name) {
  if (!environ || !name)
    return NULL;
  size_t len = strlen(name);
  for (char **p = environ; *p; p++)
    if (env_matches(*p, name, len))
      return *p + len + 1;
  return NULL;
}

int setenv(const char *name, const char *value, int overwrite) {
  if (!env_valid_name(name)) {
    errno = EINVAL;
    return -1;
  }
  if (!overwrite && getenv(name))
    return 0;
  if (!value)
    value = "";
  size_t len = strlen(name);
  size_t value_len = strlen(value);
  char *entry = malloc(len + value_len + 2);
  if (!entry) {
    errno = ENOMEM;
    return -1;
  }
  memcpy(entry, name, len);
  entry[len] = '=';
  memcpy(entry + len + 1, value, value_len + 1);
  if (env_remember(entry) < 0) {
    free(entry);
    errno = ENOMEM;
    return -1;
  }
  if (env_put(entry, len) < 0) {
    env_release(entry);
    return -1;
  }
  return 0;
}

int unsetenv(const char *name) {
  if (!env_valid_name(name)) {
    errno = EINVAL;
    return -1;
  }
  if (!environ)
    return 0;
  size_t len = strlen(name);
  char **out = environ;
  for (char **in = environ; *in; in++) {
    if (env_matches(*in, name, len))
      env_release(*in);
    else
      *out++ = *in;
  }
  *out = NULL;
  return 0;
}

/* `string` itself goes into the environment, so later changes to it show
 * there. Without an '=' it removes the variable, as glibc and musl do. */
int putenv(char *string) {
  size_t len = env_name_length(string);
  if (!string[len])
    return unsetenv(string);
  if (len == 0) {
    errno = EINVAL;
    return -1;
  }
  return env_put(string, len);
}

/* The no-OS build has no processes to run a shell in. */
#ifndef YASLIBC_NOOS
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
#endif

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
#ifndef YASLIBC_NOOS
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
#endif
  /* Run atexit/on_exit handlers (LIFO) */
  while (atexit_cnt > 0) {
    --atexit_cnt;
    (atexit_func[atexit_cnt].as_exit)(status, atexit_arg[atexit_cnt]);
  }
  fflush(stdout);
  fflush(stderr);
  _exit(status);
}

/* strtoull lives in atoi.c, beside strtoul. */

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

/* strtoll lives in atoi.c, beside strtol. */

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


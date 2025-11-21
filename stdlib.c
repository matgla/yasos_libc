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
  double result = 0.0;
  int sign = 1;
  while (*str && *str != ' ' && *str != '\t') {
    if (*str == '-')
      sign = -1;
    else if (*str >= '0' && *str <= '9')
      result = result * 10.0 + (*str - '0');
    else
      break;
    str++;
  }
  return result * sign;
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

static void (*atexit_func[ATEXIT_MAX])(void);
static int atexit_cnt;

int atexit(void (*func)(void)) {
  if (atexit_cnt >= ATEXIT_MAX)
    return -1;
  atexit_func[atexit_cnt++] = func;
  return 0;
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

float strtof(const char *nptr, char **endptr) {
  float result = 0.0f;
  int sign = 1;
  while (*nptr && *nptr != ' ' && *nptr != '\t') {
    if (*nptr == '-')
      sign = -1;
    else if (*nptr >= '0' && *nptr <= '9')
      result = result * 10.0f + (*nptr - '0');
    else
      break;
    nptr++;
  }
  if (endptr)
    *endptr = (char *)nptr;
  return result * sign;
}

long double strtold(const char *nptr, char **endptr) {
  long double result = 0.0L;
  int sign = 1;
  while (*nptr && *nptr != ' ' && *nptr != '\t') {
    if (*nptr == '-')
      sign = -1;
    else if (*nptr >= '0' && *nptr <= '9')
      result = result * 10.0L + (*nptr - '0');
    else
      break;
    nptr++;
  }
  if (endptr)
    *endptr = (char *)nptr;
  return result * sign;
}

double strtod(const char *nptr, char **endptr) {
  double result = 0.0;
  int sign = 1;
  while (*nptr && *nptr != ' ' && *nptr != '\t') {
    if (*nptr == '-')
      sign = -1;
    else if (*nptr >= '0' && *nptr <= '9')
      result = result * 10.0 + (*nptr - '0');
    else
      break;
    nptr++;
  }
  if (endptr)
    *endptr = (char *)nptr;
  return result * sign;
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
  return -1; // Not implemented
}

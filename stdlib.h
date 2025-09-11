#include <stddef.h>

#define RAND_MAX 0x7fffffff
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

void *malloc(long n);
void free(void *m);
void *calloc(long n, long sz);
void *realloc(void *v, long sz);

int atoi(const char *s);
long atol(const char *s);
long long atoll(const char *s);
char *itoa(int n, char *s, int base);
long strtol(const char *s, char **endptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
int abs(int n);
long int labs(long n);
long long int llabs(long long int n);

void exit(int status);
void abort(void);
int atexit(void (*func)(void));

char *getenv(char *name);
void qsort(void *a, int n, int sz, int (*cmp)(const void *, const void *));
int mkstemp(char *t);
int system(char *cmd);

void srand(unsigned int seed);
int rand(void);

double strtod(const char *nptr, char **endptr);
long long strtoll(const char *str, char **endptr, int base);
unsigned long long int strtoull(const char *nptr, char **endptr, int base);

/* for examining heap memory allocation */
#ifdef MEMTST
void *memtst_malloc(long n);
void memtst_free(void *v);
void *memtst_calloc(long n, long sz);
void *memtst_realloc(void *v, long sz);
#define malloc memtst_malloc
#define free memtst_free
#define calloc memtst_calloc
#define realloc memtst_realloc
#endif

int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

#define MB_CUR_MAX 1

long random(void);
void srandom(unsigned int seed);

char *realpath(const char *path, char *resolved_path);
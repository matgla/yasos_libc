#include <stddef.h>

#define RAND_MAX 0x7fffffff
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

void *malloc(size_t n);
/* Over-aligned allocation. malloc itself only promises 8-byte alignment; these
   take an explicit one (a power of two, and for posix_memalign a multiple of
   sizeof(void *)). Blocks from either are released with plain free(). */
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void *memalign(size_t alignment, size_t size);
/* Bytes usable through a malloc'd pointer: the size it was asked for. */
size_t malloc_usable_size(void *ptr);
void free(void *m);
void *calloc(size_t n, size_t sz);
void *realloc(void *v, size_t sz);

#define alloca(sz) __builtin_alloca(sz)

int atoi(const char *s);
long atol(const char *s);
long long atoll(const char *s);
char *itoa(int n, char *s, int base);
long strtol(const char *s, char **endptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
int abs(int n);
long int labs(long n);
long long int llabs(long long int n);
double atof(const char *str);

__attribute__((noreturn)) void exit(int status);
__attribute__((noreturn)) void abort(void);
int atexit(void (*func)(void));
int on_exit(void (*func)(int, void *), void *arg);

char *getenv(const char *name);
int putenv(char *string);

void qsort(void *a, int n, int sz, int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
int mkstemp(char *t);
char *mkdtemp(char *t);
int system(char *cmd);

void srand(unsigned int seed);
int rand(void);

double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
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
/* The only locale is the single-byte C locale (MB_CUR_MAX 1): every byte is
   one character whose wide value is the byte. */
int mbtowc(wchar_t *pwc, const char *s, size_t n);

long random(void);
void srandom(unsigned int seed);

char *realpath(const char *path, char *resolved_path);
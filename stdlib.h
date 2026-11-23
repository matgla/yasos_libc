#include <stddef.h>
#include <sys/_attrs.h>

/* yasc's reading of the results that may be null (sys/_attrs.h) */
#include <sys/yasc/stdlib.h>

#define RAND_MAX 0x7fffffff
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

/* free first: the allocators name it as their deallocator (sys/_attrs.h) */
void free(void *m);
void *malloc(size_t n) __yl_malloc __yl_dealloc(free, 1);
/* Over-aligned allocation. malloc itself only promises 8-byte alignment; these
   take an explicit one (a power of two, and for posix_memalign a multiple of
   sizeof(void *)). Blocks from either are released with plain free(). */
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size) __yl_malloc __yl_dealloc(free, 1);
void *memalign(size_t alignment, size_t size) __yl_malloc __yl_dealloc(free, 1);
/* Bytes usable through a malloc'd pointer: the size it was asked for. */
size_t malloc_usable_size(void *ptr);
void *calloc(size_t n, size_t sz) __yl_malloc __yl_dealloc(free, 1);
/* may return the block it was given: an allocator, but not `malloc` */
void *realloc(void *v, size_t sz) __yl_dealloc(free, 1);
/* realloc(p, n * m), but null (ENOMEM) when n * m overflows */
void *reallocarray(void *p, size_t n, size_t m) __yl_dealloc(free, 1);

#define alloca(sz) __builtin_alloca(sz)

int atoi(const char *s) __yl_nonnull(1);
long atol(const char *s) __yl_nonnull(1);
long long atoll(const char *s) __yl_nonnull(1);
char *itoa(int n, char *s, int base) __yl_nonnull(2);
long strtol(const char *s, char **endptr, int base) __yl_nonnull(1);
unsigned long strtoul(const char *s, char **endptr, int base) __yl_nonnull(1);
int abs(int n);
long int labs(long n);
long long int llabs(long long int n);
double atof(const char *str) __yl_nonnull(1);

__attribute__((noreturn)) void exit(int status);
__attribute__((noreturn)) void abort(void);
int atexit(void (*func)(void)) __yl_nonnull(1);
int on_exit(void (*func)(int, void *), void *arg) __yl_nonnull(1);

char *getenv(const char *name) __yl_nonnull(1);
int putenv(char *string) __yl_nonnull(1);

void qsort(void *a, int n, int sz, int (*cmp)(const void *, const void *)) __yl_nonnull(4);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) __yl_nonnull(1, 5);
int mkstemp(char *t) __yl_nonnull(1);
char *mkdtemp(char *t) __yl_nonnull(1);
int system(char *cmd);

void srand(unsigned int seed);
int rand(void);

double strtod(const char *nptr, char **endptr) __yl_nonnull(1);
float strtof(const char *nptr, char **endptr) __yl_nonnull(1);
long double strtold(const char *nptr, char **endptr) __yl_nonnull(1);
long long strtoll(const char *str, char **endptr, int base) __yl_nonnull(1);
unsigned long long int strtoull(const char *nptr, char **endptr, int base) __yl_nonnull(1);

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
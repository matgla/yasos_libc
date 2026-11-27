#pragma yasc
#pragma tcc c_begin
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include <sys/_attrs.h>


#define RAND_MAX 0x7fffffff
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

/* free first: the allocators name it as their deallocator (sys/_attrs.h) */
void free(void *m);
void *malloc(size_t n) __yl_malloc __yl_alloc_size(1) __yl_dealloc(free, 1);
/* Over-aligned allocation. malloc itself only promises 8-byte alignment; these
   take an explicit one (a power of two, and for posix_memalign a multiple of
   sizeof(void *)). Blocks from either are released with plain free(). */
#pragma tcc c_end
/* written on success (0) only */
int posix_memalign(out void *?owned(free) *memptr, size_t alignment, size_t size);
#pragma tcc c_begin
void *aligned_alloc(size_t alignment, size_t size) __yl_malloc __yl_alloc_size(2) __yl_dealloc(free, 1);
void *memalign(size_t alignment, size_t size) __yl_malloc __yl_alloc_size(2) __yl_dealloc(free, 1);
/* Bytes usable through a malloc'd pointer: the size it was asked for. */
size_t malloc_usable_size(void *ptr);
void *calloc(size_t n, size_t sz) __yl_malloc __yl_alloc_size(1, 2) __yl_dealloc(free, 1);
/* may return the block it was given: an allocator, but not `malloc` */
#pragma tcc c_end
/* the result takes the block over (it may move it); null: v is still the caller's */
void *?@(owned(free), takes(v)) realloc(void *?owned v, size_t sz) __yl_alloc_size(2);
#pragma tcc c_begin
/* realloc(p, n * m), but null (ENOMEM) when n * m overflows */
#pragma tcc c_end
void *?@(owned(free), takes(p)) reallocarray(void *?owned p, size_t n, size_t m) __yl_alloc_size(2, 3);
#pragma tcc c_begin

#define alloca(sz) __builtin_alloca(sz)

#pragma tcc c_end
/* atoi.c.  S is read up to the first character that is not part of the
   number.  ENDPTR may be null; when it is not, *endptr is written: where the
   number ended, or S itself when nothing converted -- a pointer into S
   (borrows(s)): the caller's end borrows from what it passed for S. */
int atoi(char *s) __yl_nonnull(1);
long atol(char *s) __yl_nonnull(1);
#pragma tcc c_begin
long long atoll(const char *s) __yl_nonnull(1);
#pragma tcc c_end
/* S, never null */
char *borrows(s) itoa(int n, out char *s, int base) __yl_nonnull(2);
long strtol(char *s, out char *borrows(s) *?endptr, int base) __yl_nonnull(1);
unsigned long strtoul(char *s, out char *borrows(s) *?endptr, int base) __yl_nonnull(1);
#pragma tcc c_begin
int abs(int n);
long int labs(long n);
long long int llabs(long long int n);
double atof(const char *str) __yl_nonnull(1);

__attribute__((noreturn)) void exit(int status);
__attribute__((noreturn)) void abort(void);
int atexit(void (*func)(void)) __yl_nonnull(1);
int on_exit(void (*func)(int, void *), void *arg) __yl_nonnull(1);

#pragma tcc c_end
char *?borrows(static) getenv(char *name) __yl_nonnull(1);
#pragma tcc c_begin
int putenv(char *string) __yl_nonnull(1);

void qsort(void *a, int n, int sz, int (*cmp)(const void *, const void *)) __yl_nonnull(4);
#pragma tcc c_end
/* base may be null with nmemb == 0 */
void *?borrows(base) bsearch(void *key, void *?base, size_t nmemb, size_t size,
                          int (*view compar)(void *, void *)) __yl_nonnull(1, 5);
/* an open descriptor of the file it made from T, or -1 */
int ?owned(close) mkstemp(inout char *t) __yl_nonnull(1);
/* T itself, or null on failure */
char *?borrows(t) mkdtemp(inout char *t) __yl_nonnull(1);
#pragma tcc c_begin
int system(char *cmd);

void srand(unsigned int seed);
int rand(void);

double strtod(const char *nptr, char **endptr) __yl_nonnull(1);
float strtof(const char *nptr, char **endptr) __yl_nonnull(1);
long double strtold(const char *nptr, char **endptr) __yl_nonnull(1);
#pragma tcc c_end
long long strtoll(char *s, out char *borrows(s) *?endptr, int base) __yl_nonnull(1);
unsigned long long strtoull(char *s, out char *borrows(s) *?endptr, int base) __yl_nonnull(1);
#pragma tcc c_begin

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

/* C23-style const kept through the strto* ENDPTR: after every declaration
   above, which its macros would otherwise rewrite */
#include <sys/_strto_const.h>

#endif /* _STDLIB_H */
#pragma tcc c_end

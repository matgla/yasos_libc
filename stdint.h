#ifndef _STDINT_H
#define _STDINT_H

#include <limits.h>

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;

#ifdef __x86_64__
typedef unsigned long uint64_t;
typedef long int64_t;
#else
typedef unsigned long long uint64_t;
typedef long long int64_t;
#endif

typedef uint64_t uint_least64_t;
typedef int64_t int_least64_t;
typedef uint64_t uint_fast64_t;
typedef int64_t int_fast64_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __PTRDIFF_TYPE__ intptr_t;
typedef __SIZE_TYPE__ uintptr_t;

#ifndef INT8_MIN
#define INT8_MIN (-127 - 1)
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32767 - 1)
#endif
#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif
#ifndef INT64_MIN
#define INT64_MIN (-9223372036854775807LL - 1)
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif
#ifndef INT16_MAX
#define INT16_MAX 32767
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 0xffu
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xffffu
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffffu
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif

#ifndef SIZE_MAX
#ifdef __SIZE_MAX__
#define SIZE_MAX __SIZE_MAX__
#else
#define SIZE_MAX 0xffffffffUL
#endif
#endif

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INT_FAST8_MIN INT8_MIN
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST8_MAX INT8_MAX
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX
#define UINT_FAST8_MAX UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

/* intptr_t and ptrdiff_t are __PTRDIFF_TYPE__, uintptr_t is __SIZE_TYPE__. */
#ifdef __PTRDIFF_MAX__
#define PTRDIFF_MAX __PTRDIFF_MAX__
#else
#define PTRDIFF_MAX INT32_MAX
#endif
#define PTRDIFF_MIN (-PTRDIFF_MAX - 1)
#define INTPTR_MAX PTRDIFF_MAX
#define INTPTR_MIN PTRDIFF_MIN
#define UINTPTR_MAX SIZE_MAX

/* sig_atomic_t is char (signal.h). */
#define SIG_ATOMIC_MIN CHAR_MIN
#define SIG_ATOMIC_MAX CHAR_MAX

/* wchar_t is a 32-bit int. */
#ifndef WCHAR_MAX
#ifdef __WCHAR_MAX__
#define WCHAR_MAX __WCHAR_MAX__
#define WCHAR_MIN __WCHAR_MIN__
#else
#define WCHAR_MAX INT32_MAX
#define WCHAR_MIN INT32_MIN
#endif
#endif

/* The same tokens as wchar.h, so either may come first. */
#ifndef WINT_MAX
#define WINT_MAX 0xffffffffU
#endif
#ifndef WINT_MIN
#define WINT_MIN 0U
#endif

#define INT8_C(c) c
#define INT16_C(c) c
#define INT32_C(c) c
#define UINT8_C(c) c
#define UINT16_C(c) c
#define UINT32_C(c) c##U
#ifdef __x86_64__
#define INT64_C(c) c##L
#define UINT64_C(c) c##UL
#else
#define INT64_C(c) c##LL
#define UINT64_C(c) c##ULL
#endif
#define INTMAX_C(c) INT64_C(c)
#define UINTMAX_C(c) UINT64_C(c)

#endif

#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <limits.h>

typedef char int8_t;
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
typedef uint64_t uint_least64_t;
typedef int64_t int_least64_t;
typedef uint64_t uint_fast64_t;
typedef int64_t int_fast64_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
#endif

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __PTRDIFF_TYPE__ intptr_t;
typedef __SIZE_TYPE__ uintptr_t;

#endif

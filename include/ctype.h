// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
#pragma once

int isascii(int c);
int isblank(int c);
int isspace(int c);
int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isupper(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);

int tolower(int c);
int toupper(int c);

int iscntrl(int c);
int isgraph(int c);
int isxdigit(int c);

/* Only the C locale exists, so each classifier is a range test: inline them
 * rather than make every isdigit() a PLT call into libc.so (which also costs
 * the caller an R9 reload). The functions above stay for anyone who takes
 * their address or #undefs the macro; ctype.c defines them. */
#ifndef __YASLIBC_CTYPE_IMPL
static inline int __yas_isdigit(int c) { return (unsigned)(c - '0') < 10u; }
static inline int __yas_isalpha(int c) { return (unsigned)((c | 0x20) - 'a') < 26u; }
static inline int __yas_isalnum(int c) { return __yas_isalpha(c) || __yas_isdigit(c); }
static inline int __yas_isupper(int c) { return (unsigned)(c - 'A') < 26u; }
static inline int __yas_islower(int c) { return (unsigned)(c - 'a') < 26u; }
static inline int __yas_isspace(int c) { return (unsigned)(c - 9) < 5u || c == ' '; }
static inline int __yas_isblank(int c) { return c == ' ' || c == '\t'; }
static inline int __yas_isxdigit(int c) { return __yas_isdigit(c) || (unsigned)((c | 0x20) - 'a') < 6u; }
static inline int __yas_isprint(int c) { return (unsigned)(c - 32) < 95u; }
static inline int __yas_isgraph(int c) { return (unsigned)(c - 33) < 94u; }
static inline int __yas_ispunct(int c) { return __yas_isgraph(c) && !__yas_isalnum(c); }
static inline int __yas_iscntrl(int c) { return (unsigned)c < 32u || c == 127; }
static inline int __yas_isascii(int c) { return (unsigned)c < 128u; }
static inline int __yas_tolower(int c) { return __yas_isupper(c) ? c + ('a' - 'A') : c; }
static inline int __yas_toupper(int c) { return __yas_islower(c) ? c - ('a' - 'A') : c; }
#define isdigit(c) __yas_isdigit(c)
#define isalpha(c) __yas_isalpha(c)
#define isalnum(c) __yas_isalnum(c)
#define isupper(c) __yas_isupper(c)
#define islower(c) __yas_islower(c)
#define isspace(c) __yas_isspace(c)
#define isblank(c) __yas_isblank(c)
#define isxdigit(c) __yas_isxdigit(c)
#define isprint(c) __yas_isprint(c)
#define isgraph(c) __yas_isgraph(c)
#define ispunct(c) __yas_ispunct(c)
#define iscntrl(c) __yas_iscntrl(c)
#define isascii(c) __yas_isascii(c)
#define tolower(c) __yas_tolower(c)
#define toupper(c) __yas_toupper(c)
#endif

#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL ((void *)0)
#define offsetof(type, field) ((int)(&((type *)0)->field))

typedef unsigned long size_t;

#if __STDC_VERSION__ >= 201112L
typedef union {
  long long __ll;
  long double __ld;
} max_align_t;
#endif

#endif

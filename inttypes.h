#include <stdint.h>

#if defined(__x86_64__) || defined(__aarch64__)
#define PRIi64 "lld"
#define PRIx32 "lx"
#define PRIu32 "lu"
#define PRIx16 "hx"
#define PRIu16 "hu"
#else
#define PRIi64 "ld"
#define PRIx32 "x"
#define PRIu32 "u"
#define PRIx16 "x"
#define PRIu16 "u"
#endif


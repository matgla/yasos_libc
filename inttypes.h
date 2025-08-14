#include <stdint.h>

#if defined(__x86_64__) || defined(__aarch64__)
#define PRIi64 "lld"
#else
#define PRIi64 "ld"
#endif

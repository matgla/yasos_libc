#include <stdlib.h>

/* counted by libtcc1's __tcc_*_chk functions (the gcc builtins chk tests
   read it); __chk_fail itself is in fortify.c */
volatile int chk_calls = 0;

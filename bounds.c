#include <stdlib.h>

volatile int chk_calls = 0;

void __chk_fail(void) {
  abort();
}
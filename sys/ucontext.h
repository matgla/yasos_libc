#pragma once

#include <stdint.h>

typedef struct mcontext {
  uint32_t arm_pc;
  uint32_t arm_lr;
  uint32_t arm_fp;
} mcontext_t;

typedef struct ucontext {
  mcontext_t uc_mcontext;
} ucontext_t;
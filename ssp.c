/*
 Copyright (c) 2026 Mateusz Stadnik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/* The runtime of tcc's -fstack-protector[-strong|-all]: the canary every
 * protected frame copies, and what its epilogue calls when the copy changed.
 *
 * YasOS has no entropy source for user programs yet, so the guard is mixed
 * at startup (crt1 calls __stack_chk_init before the constructors) from what
 * differs between runs: the monotonic clock and where the kernel put argv.
 * Its low byte stays 0, as glibc's does: a string overrun copying up to a
 * NUL cannot write the canary back unchanged. */

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

uintptr_t __stack_chk_guard = 0xff0a0d00u;

void __stack_chk_init(void *seed)
{
  struct timespec ts = {0, 0};
  uintptr_t g = __stack_chk_guard;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  g ^= (uintptr_t)ts.tv_nsec * 2654435761u;
  g ^= (uintptr_t)ts.tv_sec << 16;
  g ^= (uintptr_t)seed * 40503u;
  g &= ~(uintptr_t)0xff;
  if (!(g >> 8))
    g = 0xff0a0d00u;
  __stack_chk_guard = g;
}

__attribute__((noreturn)) void __stack_chk_fail(void)
{
  static const char msg[] = "*** stack smashing detected ***: terminated\n";
  write(2, msg, sizeof msg - 1);
  abort();
}

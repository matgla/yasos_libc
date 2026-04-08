/*
 Copyright (c) 2025 Mateusz Stadnik

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

#include <stdio.h>
#include <stdlib.h>

extern char **environ;

/* Struct appended to .data by the YAFF linker (tcc_yaff_prepare_init_fini).
 * Layout:
 * [init_count:u32][fini_count:u32][init_func_ptrs...][fini_func_ptrs...]
 * Accessed via a single LOCAL symbol through a GOT local relocation.
 * Declared weak so that ELF-format builds (which skip the YAFF linker
 * step) link without error — the pointer resolves to NULL/0. */
extern unsigned int __yaff_initfini[] __attribute__((weak));
extern void *__yasos_fini_table;
extern void *__yasos_fini_got;

int main(int argc, char *argv[]);

/* Pure-assembly _start: capture R9 (GOT base) into R2 before any
 * GOT-relative C code runs, then tail-call __crt1_main. */
__attribute__((naked, used))
void _start(void) {
  __asm__ volatile(
    ".syntax unified\n"
    "mov r2, r9\n"
    "b __crt1_main\n"
  );
}

void __crt1_main(int argc, char *argv[], void *r9_value) {
  environ = (char **)malloc(sizeof(char *));
  environ[0] = 0;

  __yasos_fini_got = r9_value;

  if (__yaff_initfini) {
    unsigned int init_count = __yaff_initfini[0];
    unsigned int fini_count = __yaff_initfini[1];
    void (**init_funcs)(void) = (void (**)(void))&__yaff_initfini[2];

    /* Run constructors (forward order) */
    for (unsigned int i = 0; i < init_count; i++) {
      init_funcs[i]();
    }
  }

  __yasos_fini_table = __yaff_initfini;

  int ret = main(argc, argv);
  exit(ret);
}

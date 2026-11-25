// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/* The compiler's runtime entry points this libc defines: what a compiler
 * calls on its own for -fstack-protector and -fsanitize=undefined, and a
 * program never names.  They are the libc's because they report and end the
 * program (write, abort) and the canary is seeded at startup (crt1); the
 * pure-computation part of the runtime -- libgcc's bit helpers, complex
 * division -- is the compiler's (tinycc's libtcc1 and FP libraries), and the
 * libc defines none of it.
 *
 * No public header declares these (gcc keeps such declarations in libgcc's
 * internal libgcc2.h, LLVM in compiler-rt's), so neither do this libc's: the
 * declarations here are for the sources that define or call them, which is
 * what -Wmissing-prototypes asks for.  Private: in internal/, which `make
 * install` does not copy, and included by a relative path.  The signatures
 * are the ABI the compilers call (libgcc's, clang's UBSan runtime).
 *
 * yasc: in a yasc unit (__YASC__) the header says `#pragma yasc` and its
 * pointers are `view` -- borrowed for the call, never freed or kept, not null:
 * a UBSan data record is the compiler's static, the stack-protector seed is
 * only mixed in as a number.  RT_VIEW carries that to regular C, where it is
 * nothing, so one declaration serves both, and the defining sources write it
 * as well.  It is defined after the pragma: the yasc words of a regular C
 * header stay identifiers, also in its macros (sys/_attrs.h). */

#ifndef _LIBC_INTERNAL_COMPILER_RT_H
#define _LIBC_INTERNAL_COMPILER_RT_H

#include <stdint.h>

#ifdef __YASC__
#pragma yasc
#define RT_VIEW view
#else
#define RT_VIEW
#endif

/* ssp.c: -fstack-protector's canary, read in every protected prologue and
 * epilogue; crt1 seeds it (__stack_chk_init) before the constructors, and a
 * changed canary ends the program (__stack_chk_fail) */
extern uintptr_t __stack_chk_guard;
void __stack_chk_init(void *RT_VIEW seed);
__attribute__((noreturn)) void __stack_chk_fail(void);

/* ubsan.c: clang's -fsanitize=undefined handlers.  Each takes clang's data
 * record for the check (its layout is ubsan.c's business) and the operands as
 * ValueHandles; the plain ones report once and return, the _abort ones
 * (-fno-sanitize-recover) report and abort. */
struct ubsan_overflow;
struct ubsan_shift;
struct ubsan_mismatch;
struct ubsan_vla;
struct ubsan_oob;
struct ubsan_unreachable;

void __ubsan_handle_add_overflow(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_add_overflow_abort(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_sub_overflow(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_sub_overflow_abort(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_mul_overflow(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_mul_overflow_abort(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_negate_overflow(struct ubsan_overflow *RT_VIEW d, uintptr_t a);
void __ubsan_handle_negate_overflow_abort(struct ubsan_overflow *RT_VIEW d, uintptr_t a);
void __ubsan_handle_divrem_overflow(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_divrem_overflow_abort(struct ubsan_overflow *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_shift_out_of_bounds(struct ubsan_shift *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_shift_out_of_bounds_abort(struct ubsan_shift *RT_VIEW d, uintptr_t a, uintptr_t b);
void __ubsan_handle_type_mismatch_v1(struct ubsan_mismatch *RT_VIEW d, uintptr_t p);
void __ubsan_handle_type_mismatch_v1_abort(struct ubsan_mismatch *RT_VIEW d, uintptr_t p);
void __ubsan_handle_vla_bound_not_positive(struct ubsan_vla *RT_VIEW d, uintptr_t v);
void __ubsan_handle_vla_bound_not_positive_abort(struct ubsan_vla *RT_VIEW d, uintptr_t v);
void __ubsan_handle_out_of_bounds(struct ubsan_oob *RT_VIEW d, uintptr_t i);
void __ubsan_handle_out_of_bounds_abort(struct ubsan_oob *RT_VIEW d, uintptr_t i);
/* __builtin_unreachable() under -fsanitize=unreachable, and yasc's
   `unreachable` in handler mode: always fatal (clang has no _abort form) */
void __ubsan_handle_builtin_unreachable(struct ubsan_unreachable *RT_VIEW d);

#endif /* _LIBC_INTERNAL_COMPILER_RT_H */

#pragma once

/* A const table of string pointers normally needs a per-process relocated copy.
 * This keeps it in shared rodata, as offsets, where the toolchain can do that;
 * the kernel's copy of libc is linked without it. */
#if defined(__TINYC_RODATA_RELATIVE__) && !defined(YASLIBC_NOOS)
#define ROREL __rodata_relative
#else
#define ROREL
#endif

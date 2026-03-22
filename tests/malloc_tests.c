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

#include "utest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/*
 * Include malloc.c directly so we can test internal helpers (msize)
 * and avoid the SUT library symbol-rename complexity.
 *
 * Rename entry points to avoid colliding with the host libc.
 * Suppress fprintf debug output.
 * Headers above already satisfied the include-guards, so the
 * #includes inside malloc.c are harmless no-ops.
 */
#define fprintf(...) (0)
#define malloc test_malloc
#define free test_free
#define calloc test_calloc
#define realloc test_realloc
#include "../malloc.c"
#undef fprintf
#undef malloc
#undef free
#undef calloc
#undef realloc

/* ---- msize correctness ---- */

UTEST(malloc_tests, msize_large_alloc_returns_user_size) {
  /* Large alloc (>= MSETMAX=4096) uses mmap with a header page.
     msize() must return the *user* size, not size+PGSIZE. */
  void *p = test_malloc(8136);
  ASSERT_TRUE(p != NULL);
  ASSERT_EQ(msize(p), 8136);
  test_free(p);
}

UTEST(malloc_tests, msize_large_alloc_exact_page) {
  void *p = test_malloc(4096);
  ASSERT_TRUE(p != NULL);
  ASSERT_EQ(msize(p), 4096);
  test_free(p);
}

UTEST(malloc_tests, msize_large_alloc_multi_page) {
  void *p = test_malloc(16384);
  ASSERT_TRUE(p != NULL);
  ASSERT_EQ(msize(p), 16384);
  test_free(p);
}

UTEST(malloc_tests, msize_small_alloc) {
  void *p = test_malloc(128);
  ASSERT_TRUE(p != NULL);
  ASSERT_EQ(msize(p), 128);
  test_free(p);
}

/* ---- realloc data preservation ---- */

UTEST(malloc_tests, realloc_shrink_preserves_prefix) {
  /* Allocate a large buffer, fill it, shrink via realloc.
     Only the first new_size bytes must be copied. */
  char *p = (char *)test_malloc(8192);
  ASSERT_TRUE(p != NULL);
  for (int i = 0; i < 8192; i++)
    p[i] = (char)(i & 0xff);

  char *q = (char *)test_realloc(p, 100);
  ASSERT_TRUE(q != NULL);

  for (int i = 0; i < 100; i++)
    ASSERT_EQ((unsigned char)q[i], (unsigned char)(i & 0xff));

  test_free(q);
}

UTEST(malloc_tests, realloc_grow_preserves_data) {
  char *p = (char *)test_malloc(5000);
  ASSERT_TRUE(p != NULL);
  memset(p, 'X', 5000);

  char *q = (char *)test_realloc(p, 10000);
  ASSERT_TRUE(q != NULL);

  for (int i = 0; i < 5000; i++)
    ASSERT_EQ(q[i], 'X');

  test_free(q);
}

UTEST(malloc_tests, realloc_large_to_small_no_overflow) {
  /* Exact scenario that caused the heap corruption:
     malloc(8136) → stored_len = 12232, then realloc to < 4096.
     Old msize() returned 12232 instead of 8136,
     and old realloc() copied msize() bytes unconditionally. */
  char *p = (char *)test_malloc(8136);
  ASSERT_TRUE(p != NULL);
  memset(p, 0x42, 8136);

  char *q = (char *)test_realloc(p, 256);
  ASSERT_TRUE(q != NULL);

  for (int i = 0; i < 256; i++)
    ASSERT_EQ((unsigned char)q[i], 0x42);

  test_free(q);
}

UTEST(malloc_tests, realloc_null_acts_as_malloc) {
  void *p = test_realloc(NULL, 1000);
  ASSERT_TRUE(p != NULL);
  test_free(p);
}

/* ---- calloc ---- */

UTEST(malloc_tests, calloc_returns_zeroed_memory) {
  unsigned char *p = (unsigned char *)test_calloc(100, 1);
  ASSERT_TRUE(p != NULL);
  for (int i = 0; i < 100; i++)
    ASSERT_EQ(p[i], 0);
  test_free(p);
}

/* ---- basic malloc/free round-trip ---- */

UTEST(malloc_tests, malloc_free_large_roundtrip) {
  void *p = test_malloc(8192);
  ASSERT_TRUE(p != NULL);
  /* writing to the full range must not fault */
  memset(p, 0xAA, 8192);
  test_free(p);
}

UTEST(malloc_tests, malloc_free_small_roundtrip) {
  void *p = test_malloc(64);
  ASSERT_TRUE(p != NULL);
  memset(p, 0xBB, 64);
  test_free(p);
}

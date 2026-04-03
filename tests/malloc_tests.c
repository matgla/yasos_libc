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

#define _GNU_SOURCE

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

static void reset_allocator_state(void) {
  struct mset *op;
  while (old_pools) {
    op = old_pools;
    old_pools = op->next_pool;
    munmap(op, MSETLEN);
  }
  if (pool1 != NULL) {
    munmap(pool1, MSETLEN);
    pool1 = NULL;
  }
  if (pool != NULL) {
    munmap(pool, MSETLEN);
    pool = NULL;
  }
  memset(large_table, 0, sizeof(large_table));
  prof_current = 0;
  prof_peak = 0;
  prof_total_mapped = 0;
  prof_malloc_calls = 0;
  prof_free_calls = 0;
  prof_realloc_calls = 0;
  prof_mmap_calls = 0;
  prof_munmap_calls = 0;
  prof_inplace = 0;
  prof_pool_allocs = 0;
  prof_large_allocs = 0;
}

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

UTEST(malloc_tests, small_alloc_never_returns_page_aligned_pointer) {
  reset_allocator_state();

  void *filler = test_malloc(4072);
  ASSERT_TRUE(filler != NULL);

  void *p = test_malloc(1);
  ASSERT_TRUE(p != NULL);
  ASSERT_NE((unsigned long)p & PGMASK, 0UL);

  test_free(p);
  test_free(filler);
  reset_allocator_state();
}

/* ---- free list coalescing ---- */

UTEST(malloc_tests, freelist_coalesces_adjacent_blocks) {
  /* Bug: without coalescing, freeing many small adjacent blocks produced a
     free list full of tiny entries (16-24 bytes) that could never satisfy
     larger allocations (88-144 bytes), causing pool-full → new mmap. */
  reset_allocator_state();

  /* Allocate many small blocks consecutively */
  #define NBLOCKS 40
  void *ptrs[NBLOCKS];
  for (int i = 0; i < NBLOCKS; i++) {
    ptrs[i] = test_malloc(16); /* alloc_size = 24 each */
    ASSERT_TRUE(ptrs[i] != NULL);
  }
  /* Pin some blocks so pool doesn't reach refs=0 */
  void *pin = test_malloc(32);
  ASSERT_TRUE(pin != NULL);

  /* Free all the small blocks — they should coalesce */
  for (int i = 0; i < NBLOCKS; i++)
    test_free(ptrs[i]);

  /* The coalesced free space should serve a larger allocation
     without needing the bump pointer */
  int initial_mmap_calls = prof_mmap_calls;
  void *big = test_malloc(256);
  ASSERT_TRUE(big != NULL);
  /* No new mmap should have occurred — served from free list */
  ASSERT_EQ(prof_mmap_calls, initial_mmap_calls);

  memset(big, 0xCC, 256);

  test_free(big);
  test_free(pin);
  reset_allocator_state();
  #undef NBLOCKS
}

UTEST(malloc_tests, freelist_coalesces_with_both_neighbors) {
  /* Free B, then A, then C where A-B-C are adjacent — should form one block */
  reset_allocator_state();

  void *a = test_malloc(32);
  void *b = test_malloc(32);
  void *c = test_malloc(32);
  void *pin = test_malloc(16);
  ASSERT_TRUE(a && b && c && pin);

  /* Free middle, then left, then right */
  test_free(b);
  test_free(a);
  test_free(c);

  /* Coalesced block should serve a 128-byte alloc from free list */
  int initial_mmap_calls = prof_mmap_calls;
  void *big = test_malloc(100);
  ASSERT_TRUE(big != NULL);
  ASSERT_EQ(prof_mmap_calls, initial_mmap_calls);

  memset(big, 0xDD, 100);

  test_free(big);
  test_free(pin);
  reset_allocator_state();
}

/* ---- block splitting ---- */

UTEST(malloc_tests, split_does_not_create_unusable_fragments) {
  /* Bug: splitting with threshold sizeof(struct free_node)=8 created 8-byte
     free list entries that could never be reused (min alloc_size=16).
     Fix: only split when remainder >= 2*sizeof(struct mhdr) = 16. */
  reset_allocator_state();

  /* Allocate and free a 128-byte block */
  void *p = test_malloc(128);
  ASSERT_TRUE(p != NULL);
  void *pin = test_malloc(16);
  ASSERT_TRUE(pin != NULL);
  test_free(p);

  /* Now allocate from the free list with a size that would leave a small
     remainder. alloc_size for malloc(120) = 128 (120+8 aligned).
     The freed block is 136 bytes (128+8 aligned). Remainder = 8.
     With the fix, no split occurs — the 8 bytes stay as internal frag. */
  void *q = test_malloc(120);
  ASSERT_TRUE(q != NULL);
  memset(q, 0xEE, 120);

  /* Free and check: free list should have at most 1 entry (the block we just
     freed), not 1 + a useless 8-byte fragment */
  test_free(q);
  ASSERT_LE(freelist_count(pool), 1);

  test_free(pin);
  reset_allocator_state();
}

UTEST(malloc_tests, split_creates_usable_remainder) {
  /* When remainder is large enough (>=16), split should produce a usable
     second block */
  reset_allocator_state();

  /* Allocate and free a 256-byte block */
  void *p = test_malloc(256);
  ASSERT_TRUE(p != NULL);
  void *pin = test_malloc(16);
  ASSERT_TRUE(pin != NULL);
  test_free(p);

  /* Allocate 64 bytes from the 264-byte freed block.
     alloc_size = 72. Remainder = 264-72 = 192 >= 16, so split occurs. */
  void *q = test_malloc(64);
  ASSERT_TRUE(q != NULL);
  memset(q, 0xAA, 64);

  /* The remainder should be on the free list and usable */
  ASSERT_GE(freelist_count(pool), 1);

  /* Allocate from the split remainder — no new mmap */
  int initial_mmap_calls = prof_mmap_calls;
  void *r = test_malloc(100);
  ASSERT_TRUE(r != NULL);
  ASSERT_EQ(prof_mmap_calls, initial_mmap_calls);
  memset(r, 0xBB, 100);

  test_free(q);
  test_free(r);
  test_free(pin);
  reset_allocator_state();
}

/* ---- old pool free list reuse ---- */

UTEST(malloc_tests, old_pool_drains_via_free_not_alloc) {
  /* Old pools must NOT be searched for new allocations — doing so adds refs
     and prevents the pool from ever reaching refs=0 for reclaim.
     Instead, old pools drain naturally as their live allocations are freed. */
  reset_allocator_state();

  #define FILL_SIZE 48
  #define MAX_PTRS 700
  void *ptrs[MAX_PTRS];
  int count = 0;

  /* Fill pool to the brim until a second pool is created */
  while (count < MAX_PTRS) {
    void *p = test_malloc(FILL_SIZE);
    if (!p)
      break;
    ptrs[count++] = p;
    if (prof_mmap_calls >= 2)
      break;
  }
  ASSERT_GE(prof_mmap_calls, 2);
  ASSERT_TRUE(old_pools != NULL);

  struct mset *old = old_pools;
  int refs_after_fill = old->refs;

  /* Free half the blocks from the old pool — refs should decrease */
  for (int i = 0; i < count / 2 && i < 50; i++) {
    test_free(ptrs[i]);
    ptrs[i] = NULL;
  }
  ASSERT_LT(old->refs, refs_after_fill);

  /* New allocations should NOT come from old pool — they use current pool */
  int old_refs_before = old->refs;
  void *new_alloc = test_malloc(FILL_SIZE);
  ASSERT_TRUE(new_alloc != NULL);
  ASSERT_EQ(old->refs, old_refs_before); /* old pool refs unchanged */
  test_free(new_alloc);

  /* Clean up */
  for (int i = 0; i < count; i++)
    if (ptrs[i])
      test_free(ptrs[i]);
  reset_allocator_state();
  #undef FILL_SIZE
  #undef MAX_PTRS
}

UTEST(malloc_tests, old_pool_removed_from_list_when_refs_zero) {
  /* When all allocations from an old pool are freed, it should be removed
     from old_pools and recycled (to pool1 or munmap'd). */
  reset_allocator_state();

  /* Create allocations that fill the first pool */
  #define FILL_SIZE2 48
  #define MAX_PTRS2 700
  void *ptrs[MAX_PTRS2];
  int count = 0;
  while (count < MAX_PTRS2) {
    ptrs[count] = test_malloc(FILL_SIZE2);
    if (!ptrs[count])
      break;
    count++;
    if (prof_mmap_calls >= 2)
      break;
  }
  ASSERT_GE(prof_mmap_calls, 2);
  ASSERT_TRUE(old_pools != NULL);

  struct mset *old = old_pools;

  /* Free ALL allocations that belong to the old pool.
     We can identify them by the pool pointer: old pool was the first mmap,
     current pool is the second. Pointers within old pool satisfy:
     ptr >= (char*)old && ptr < (char*)old + MSETLEN */
  for (int i = 0; i < count; i++) {
    if (ptrs[i] &&
        (char *)ptrs[i] >= (char *)old &&
        (char *)ptrs[i] < (char *)old + MSETLEN) {
      test_free(ptrs[i]);
      ptrs[i] = NULL;
    }
  }

  /* old pool should have been removed from old_pools (refs hit 0) */
  struct mset *op;
  int found = 0;
  for (op = old_pools; op; op = op->next_pool)
    if (op == old)
      found = 1;
  ASSERT_EQ(found, 0);

  /* Clean up remaining */
  for (int i = 0; i < count; i++)
    if (ptrs[i])
      test_free(ptrs[i]);
  reset_allocator_state();
  #undef FILL_SIZE2
  #undef MAX_PTRS2
}

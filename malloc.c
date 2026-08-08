#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define MALLOC_DEBUG 0



#if MALLOC_DEBUG
/* GDB: break malloc_debug_trap */
static volatile int malloc_debug_trap_hit;
void __attribute__((noinline)) malloc_debug_trap(void *block, int size, int alloc_size) {
  malloc_debug_trap_hit++;
#if defined(__arm__) || defined(__thumb__)
  asm volatile("bkpt #1");
#endif
  (void)block; (void)size; (void)alloc_size;
}
#endif

/* malloc returns uninitialised memory, as C says it does.
 *
 * It used to return zeroed memory by accident and then on purpose: fresh pool
 * pages arrive zeroed from the kernel, so the bump path never had to clear
 * anything, and the free-list path memset to match. Both halves of that are
 * now paid for:
 *
 *   - the kernel's clear is the single most expensive thing in a page
 *     allocation (76% of pool time; 26 MB/s against PSRAM), and it only exists
 *     to stop one process reading another's data -- which is why the kernel now
 *     hands a process its *own* freed pages back unzeroed;
 *   - the free-list memset is proportional to the allocation and runs on every
 *     reuse, for callers that overwrite the memory immediately.
 *
 * calloc still zeroes explicitly, which is what a caller that needs zeroes
 * should have been calling. Set this back to 1 to restore the old behaviour if
 * something in the rootfs turns out to have been relying on it. */
#define MALLOC_ZEROES 0

/* Freed large mappings are parked here instead of being unmapped, and reused
 * by the next large allocation that fits. tcc frees and re-requests the same
 * few sizes over and over -- 39 mappings for a hello-world compile -- and each
 * round trip costs an mmap, a munmap and a kernel-side page clear. */
#define LARGE_CACHE_SLOTS 8
#define LARGE_CACHE_MAX_BYTES (128 * 1024)

#define PGSIZE 4096
#define PGMASK (PGSIZE - 1)

/* ── Pool cascade ─────────────────────────────────────────────────────────
   Pool size and the pooled/mapped threshold are not constants: they start at
   the old fixed values and widen as the process proves it is memory-hungry.

   Why: on this target an mmap cost ~537 us (measured over 151,008 of them in
   one suite run). That was read at the time as a flat per-call cost, and it is
   not -- it is the kernel's page clear, and it scales with the bytes: measured
   later, 423 us for the ~20 KiB average mapping and 1102 us for a 64 KiB one,
   both landing on the PSRAM clear rate of 55 MB/s (SRAM: 777 MB/s). Read every
   number below with that correction: what a mapping costs is the memory it
   clears and the tier it clears it in, so the levers are fewer BYTES mapped and
   a freer fast tier -- not fewer calls. With a fixed
   2 KiB threshold *every* allocation at or above it was a mapping, and tcc's
   averaged exactly 8.0 KiB: 34 mmaps per compile, 82 s over a suite run, the
   largest single item left in the compile profile.

   Simply raising the threshold would tax every light applet with a big pool it
   never fills -- the 32 KiB inherited default used to waste ~24 KiB on
   processes whose live small-object set is 2-6 KiB, which is why it was cut to
   one page. So instead the class widens on evidence:

     - a fresh pool had to be mapped (the small-object set outgrew a pool), or
     - LARGE_PER_CLASS more allocations took the mapping path.

   The second is what catches tcc: a process that has already paid for a
   handful of mappings is exactly the one whose next mid-size request should
   come out of a pool instead. A process that allocates a few KiB and stops
   never leaves the first class, and sees byte-for-byte the old behaviour.

   Do not try to skip the ladder. Letting a process declare its appetite up
   front (malloc_pool_hint(64 KiB), called from the compiler's main) was built
   and measured: it does what it says -- mappings per compile 12.2 -> 4.3 -- and
   it is 5.5% SLOWER over the corpus (compile bucket 596 -> 661 s). Two reasons,
   both worth remembering. A mapping's cost here is the bytes the kernel clears,
   not a per-call constant (see below), so trading twelve small mappings for
   four large ones moves the same bytes; and a 64 KiB pool spreads a small
   compile's working set over sixteen times the address range, which a 16 KiB
   XIP cache notices. The ladder is not overhead to be optimised away -- it is
   what keeps a small compile's heap small. */
#define MSETLEN_MIN (1 << 12)      /* first pool: one page, as before */
#define MSETLEN_MAX (64 * 1024)
#define MSETMAX_MIN 2048           /* first threshold, as before */
#define MSETMAX_CAP (32 * 1024)
#define LARGE_PER_CLASS 4

/* Size of the next pool to map. The threshold is derived from it as half,
   clamped -- half guarantees the largest pooled object still fits a fresh pool
   alongside the pool header, which matters because malloc must never hand back
   a block that overruns its mapping. */
static int pool_len_next = MSETLEN_MIN;
static unsigned large_allocs;

static void widen_pool_class(void) {
  if (pool_len_next < MSETLEN_MAX)
    pool_len_next <<= 1;
}

/* Largest object served from a bump pool; >= this takes the mapping path. */
static int cur_msetmax(void) {
  int m = pool_len_next >> 1;
  if (m > MSETMAX_CAP)
    m = MSETMAX_CAP;
  if (m < MSETMAX_MIN)
    m = MSETMAX_MIN;
  return m;
}

/* Smallest pool in the current class that can hold `want` bytes in total
   (payload + pool header + the page-skip slack the bump path may insert). */
static int pool_len_for(int want) {
  int len = pool_len_next;
  while (len < want && len < MSETLEN_MAX)
    len <<= 1;
  if (len < want) /* a request larger than the class ceiling: map it exactly */
    len = (want + PGMASK) & ~PGMASK;
  return len;
}

/* Large allocations (>= the current threshold) are mmap'd with a small
   self-describing header in the mapping itself, so they are UNBOUNDED — no
   fixed-size tracking table
   (which capped concurrent large allocs and returned NULL / "memory full" once
   full) and zero per-process .bss. A central table was historically used to
   dodge a 4 KiB header page per alloc, but the kernel page grain is now 256 B,
   so an in-mapping header rounds the mapping up by at most one grain.

   Header (16 B; the user pointer is 16 B past the mmap base):
     +0  long mapped     total mmap'd bytes (for munmap / mremap)
     +8  int  moff = -1   LARGE_MOFF sentinel. A pool block's moff is its offset
                          into the pool, always in [0, pool->len), so -1 can't
                          collide; free()/realloc() read it (at user-8, the
                          mhdr.moff slot) to tell large from small.
     +12 int  size        user-visible size (at user-4, the mhdr.size slot, so
                          msize() is uniform for both small and large). */
#define LARGE_MOFF (-1)
#define LARGE_HDR 16

/* placed at the beginning of regions for small allocations */
struct mset {
  int refs;     /* number of allocations */
  int size;     /* bump pointer (next free offset) */
  void *freelist; /* singly-linked list of freed blocks for reuse */
  struct mset *next_pool; /* linked list of old pools with refs > 0 */
  int len;      /* this pool's mapped length -- pools are no longer one size */
  int pad;      /* keeps sizeof(struct mset) a multiple of 8, so the first
                   block's user pointer stays 8-aligned (ARM LDRD, doubles) */
};

/* placed before each small allocation */
struct mhdr {
  int moff; /* mset offset */
  int size; /* allocation size */
};

/* overlays mhdr in freed blocks (must be >= sizeof(mhdr)) */
struct free_node {
  struct free_node *next;
  int block_size; /* total block size including mhdr, aligned */
};

static struct mset *pool;
static struct mset *pool1; /* a freed pool */
static struct mset *old_pools; /* linked list of full pools with refs > 0 */

/* ── Mapping cache ────────────────────────────────────────────────────────
   Freed mappings are parked here rather than returned to the kernel, and the
   next request for the same mapped size takes one back. Exact-size matching
   only: the kernel frees a run by the page count it is handed, so reusing an
   oversized mapping for a smaller request would strand the difference.

   Bounded by LARGE_CACHE_MAX_BYTES, so a program that frees a large working
   set still gives it back. */
static struct {
  void *base;
  long mapped;
} large_cache[LARGE_CACHE_SLOTS];
static long large_cache_bytes;

/* A vfork child shares the parent's address space, so a mapping it parks (or
   takes) is one the parent may still consider its own. Rather than reason
   about which side owns what, the reuse paths are simply inert between vfork
   and the child's exec -- a window of a few syscalls. That covers the mapping
   cache and, for a sharper reason, promoting a retired pool: __malloc_vfork_restore
   unmaps whatever `pool` points at if it is not the one the parent saved, so a
   child that promoted one of the parent's old pools would have it unmapped out
   from under the parent's live blocks. */
static int in_vfork_window;

static void *cache_map(long total) {
  if (!in_vfork_window) {
    for (int i = 0; i < LARGE_CACHE_SLOTS; i++) {
      if (large_cache[i].base && large_cache[i].mapped == total) {
        void *base = large_cache[i].base;
        large_cache[i].base = NULL;
        large_cache_bytes -= total;
        return base;
      }
    }
  }
  {
    void *m = mmap(NULL, total, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return m == MAP_FAILED ? NULL : m;
  }
}

static void cache_unmap(void *base, long total) {
  if (!in_vfork_window && large_cache_bytes + total <= LARGE_CACHE_MAX_BYTES) {
    for (int i = 0; i < LARGE_CACHE_SLOTS; i++) {
      if (large_cache[i].base == NULL) {
        large_cache[i].base = base;
        large_cache[i].mapped = total;
        large_cache_bytes += total;
        return;
      }
    }
  }
  munmap(base, total);
}

/* Give back everything this process is holding but not using: parked mappings
   and the spare pool. Called when the process is about to hand the machine to
   a vfork child, which on this system is a fresh exec that wants the fast
   memory tier more than the parent -- the parent is suspended until the child
   execs, so anything it is sitting on is pure hold. */
static void trim_slack(void) {
  int i;
  for (i = 0; i < LARGE_CACHE_SLOTS; i++) {
    if (large_cache[i].base != NULL) {
      munmap(large_cache[i].base, large_cache[i].mapped);
      large_cache[i].base = NULL;
    }
  }
  large_cache_bytes = 0;
  if (pool1 != NULL) {
    munmap(pool1, pool1->len);
    pool1 = NULL;
  }
}

/* vfork save/restore — prevents child from corrupting parent's pool state */
static struct mset *vfork_saved_pool;
static struct mset *vfork_saved_pool1;
static struct mset *vfork_saved_old_pools;
static int vfork_saved_pool_size;
static int vfork_saved_pool_refs;
static void *vfork_saved_pool_freelist;

void __malloc_vfork_save(void) {
  /* Hand back the slack before the child runs, not after: the child is about
     to exec, and on this system the memory it gets depends on what is free at
     that moment (the pool serves fast SRAM first and falls back to PSRAM). */
  trim_slack();
  in_vfork_window = 1;
  vfork_saved_pool = pool;
  vfork_saved_pool1 = pool1;
  vfork_saved_old_pools = old_pools;
  vfork_saved_pool_size = pool ? pool->size : 0;
  vfork_saved_pool_refs = pool ? pool->refs : 0;
  vfork_saved_pool_freelist = pool ? pool->freelist : 0;
}

void __malloc_vfork_restore(void) {
  /* munmap any pools the vfork child allocated that differ from the saved
     parent state — otherwise they become orphaned and leak. */
  if (pool && pool != vfork_saved_pool && pool != vfork_saved_pool1)
    munmap(pool, pool->len);
  if (pool1 && pool1 != vfork_saved_pool && pool1 != vfork_saved_pool1)
    munmap(pool1, pool1->len);
  pool = vfork_saved_pool;
  pool1 = vfork_saved_pool1;
  old_pools = vfork_saved_old_pools;
  if (pool) {
    pool->size = vfork_saved_pool_size;
    pool->refs = vfork_saved_pool_refs;
    pool->freelist = vfork_saved_pool_freelist;
  }
  in_vfork_window = 0;
}

/* Insert a freed block into a pool's free list, sorted by address.
   Coalesces with adjacent free blocks to combat fragmentation. */
static void freelist_insert(struct mset *mset, void *ptr, int block_size) {
  struct free_node *fn = (struct free_node *)ptr;
  fn->block_size = block_size;

#if MALLOC_DEBUG
  /* Validate block is within the pool */
  if ((char *)ptr < (char *)mset || (char *)ptr + block_size > (char *)mset + mset->len) {
    fprintf(stderr, "[malloc] BAD FREE: ptr=%p blk=%d pool=%p..%p\n",
            ptr, block_size, mset, (char *)mset + mset->len);
  }
#endif

  struct free_node **prev = (struct free_node **)&mset->freelist;
  struct free_node *prev_node = NULL;
  struct free_node *cur;

  /* Find sorted position by address */
  while ((cur = *prev) && cur < fn) {
    prev_node = cur;
    prev = &cur->next;
  }

  /* Insert */
  fn->next = cur;
  *prev = fn;

  /* Coalesce with next block if adjacent */
  if (cur && (char *)fn + fn->block_size == (char *)cur) {
    fn->block_size += cur->block_size;
    fn->next = cur->next;
#if MALLOC_DEBUG
    /* Re-poison merged area so stale free_node headers don't cause false positives */
    memset((char *)fn + sizeof(struct free_node), 0xDE,
           fn->block_size - sizeof(struct free_node));
#endif
  }

  /* Coalesce with previous block if adjacent */
  if (prev_node && (char *)prev_node + prev_node->block_size == (char *)fn) {
    prev_node->block_size += fn->block_size;
    prev_node->next = fn->next;
#if MALLOC_DEBUG
    memset((char *)prev_node + sizeof(struct free_node), 0xDE,
           prev_node->block_size - sizeof(struct free_node));
#endif
  }
}

/* Take a first-fit block of at least `alloc_size` bytes off `ms`'s free list,
   splitting the remainder back onto the list. NULL when nothing on the list
   fits. The caller fills in the header and bumps refs, because it is the one
   that knows which pool the block came from. */
static void *freelist_take(struct mset *ms, size_t alloc_size) {
  struct free_node **prev = (struct free_node **)&ms->freelist, *fn;
  while ((fn = *prev)) {
    if (fn->block_size >= (int)alloc_size) {
      /* Skip blocks where user pointer would be page-aligned —
         page-aligned pointers are mistaken for large allocations
         by msize()/free()/realloc(). */
      if (!((unsigned long)((char *)fn + sizeof(struct mhdr)) & PGMASK)) {
        prev = &fn->next;
        continue;
      }
      int remainder = fn->block_size - (int)alloc_size;
      *prev = fn->next;
      /* Split if remainder can hold a minimum-sized allocation (16 bytes).
         sizeof(struct free_node) is only 8, but min alloc_size is 16,
         so 8-byte splits would create permanently unusable fragments. */
      if (remainder >= 2 * (int)sizeof(struct mhdr)) {
        struct free_node *split = (struct free_node *)((char *)fn + alloc_size);
        split->block_size = remainder;
        split->next = *prev;
        *prev = split;
        fn->block_size = alloc_size;
#if MALLOC_DEBUG
        /* Poison the split remainder's data area */
        memset((char *)split + sizeof(struct free_node), 0xDE,
               remainder - sizeof(struct free_node));
#endif
      }
#if MALLOC_DEBUG
      /* Check poison pattern to detect use-after-free */
      {
        unsigned char *data = (unsigned char *)fn + sizeof(struct mhdr);
        int data_len = fn->block_size - sizeof(struct mhdr);
        int corrupted = 0;
        for (int j = 0; j < data_len && j < 64; j++) {
          if (data[j] != 0xDE) { corrupted = 1; break; }
        }
        if (corrupted) {
          fprintf(stderr, "[malloc] POISON VIOLATED: reuse %p blk=%d for %d\n",
                  (void *)fn, fn->block_size, (int)alloc_size);
          fprintf(stderr, "[malloc] data: ");
          for (int j = 0; j < 32 && j < data_len; j++)
            fprintf(stderr, "%02x ", data[j]);
          fprintf(stderr, "\n");
          malloc_debug_trap(fn, fn->block_size, (int)alloc_size);
        }
      }
#endif
      return (void *)fn;
    }
    prev = &fn->next;
  }
  return NULL;
}

/* Serve `alloc_size` from a retired pool, and make that pool current again.
   NULL when no retired pool has room.

   A pool is retired when its *bump pointer* runs out, but blocks inside it go
   on being freed long afterwards, and freelist_insert coalesces them — so a
   long-lived process ends up owning a chain of pools that are nearly empty and
   completely unreachable, while every replacement pool is twice the size of
   the one before. Measured on the smoke rig: the shell had ratcheted to 188 KiB
   of pools holding almost nothing, in the fast SRAM tier, which is exactly the
   memory the compiler it then spawns could not have (and a compile's page
   clears cost ~30x more against PSRAM). Reaching into those pools before
   mapping a new one is what stops the ratchet.

   The pool is promoted rather than merely borrowed from, so the next
   allocation finds its free list on the normal path instead of walking the
   chain again. Only reached when the current pool cannot serve the request,
   i.e. on the path whose alternative is an mmap. */
static void *old_pool_take(size_t alloc_size) {
  struct mset **link = &old_pools;
  struct mset *old;
  if (in_vfork_window)
    return NULL;
  while ((old = *link) != NULL) {
    void *m = freelist_take(old, alloc_size);
    if (m == NULL) {
      link = &old->next_pool;
      continue;
    }
    *link = old->next_pool;
    /* Retire the current pool exactly as mk_pool would. It normally still has
       live blocks -- a drained one would have had its whole body on its free
       list and served this request itself -- but a request too big for an
       older, smaller-class pool can reach here with a drained one in hand, and
       chaining that would strand it: nothing is ever freed into it again, so
       the refs==0 check in free() would never run for it. */
    if (pool != NULL) {
      if (pool->refs > 0) {
        pool->next_pool = old_pools;
        old_pools = pool;
      } else if (pool1 == NULL) {
        pool1 = pool;
      } else {
        cache_unmap(pool, pool->len);
      }
    }
    pool = old;
    return m;
  }
  return NULL;
}

/* Make `pool` a pool with room for `want` total bytes (the caller's block plus
   whatever the bump path needs around it). Pools vary in size now, so every
   reuse path has to check the candidate actually fits -- the old code could
   assume it, and malloc still does not re-check after this returns. */
static int mk_pool(int want) {
  struct mset *cur = pool;
  /* Every process maps a first pool, so that one is not evidence of anything.
     Only having to *replace* a pool says the small-object set outgrew it. */
  const int replacing = (cur != NULL);

  /* Drained and still big enough: rewind it in place, no mapping at all. */
  if (cur != NULL && cur->refs == 0 && cur->len >= want) {
    cur->size = sizeof(*cur);
    cur->freelist = 0;
    return 0;
  }
  /* Otherwise this pool is retired: chained if blocks are still live in it,
     kept as the spare if it has drained, released if there is already one. */
  if (cur != NULL) {
    if (cur->refs > 0) {
      cur->next_pool = old_pools;
      old_pools = cur;
    } else if (pool1 == NULL) {
      pool1 = cur;
    } else if (cur->len > pool1->len) {
      /* Keep the larger of the two spares: a small one satisfies fewer future
         requests, and discarding the big one only to re-map it is the round
         trip this whole change exists to avoid. */
      cache_unmap(pool1, pool1->len);
      pool1 = cur;
    } else {
      cache_unmap(cur, cur->len);
    }
    pool = NULL;
  }
  /* The spare, if it fits. */
  if (pool1 != NULL && pool1->len >= want) {
    pool = pool1;
    pool1 = NULL;
    pool->size = sizeof(*pool);
    pool->refs = 0;
    pool->freelist = 0;
    return 0;
  }
  {
    int len = pool_len_for(want);
    pool = cache_map(len);
    if (pool == NULL) {
      return 1;
    }
    pool->len = len;
    pool->size = sizeof(*pool);
    pool->refs = 0;
    pool->freelist = 0;
    pool->next_pool = 0;
    pool->pad = 0;
  }
  if (replacing)
    widen_pool_class();
  return 0;
}

void *malloc(size_t n) {
  void *m;
  size_t alloc_size;
  int offset;
  if (n >= (size_t)cur_msetmax()) {
    /* Self-tracking large alloc: header in the mapping, no table, unbounded. */
    long total = (n + LARGE_HDR + PGMASK) & ~PGMASK;
    char *lm = cache_map(total);
    if (lm == NULL) {
      return NULL;
    }
    /* Every one of these is a mapping. Enough of them and the pool class
       widens, so later requests of this size come out of a pool instead. */
    if (++large_allocs % LARGE_PER_CLASS == 0)
      widen_pool_class();
    *(long *)lm = total;
    ((struct mhdr *)(lm + 8))->moff = LARGE_MOFF;
    ((struct mhdr *)(lm + 8))->size = (int)n;
#if MALLOC_ZEROES
    /* A mapping can now come from the cache above or from a kernel page the
       process itself last wrote, so neither path is zero by construction. */
    memset(lm + LARGE_HDR, 0, n);
#endif
    return lm + LARGE_HDR;
  }
  alloc_size = (n + sizeof(struct mhdr) + 7) & ~7;
  if (!pool)
    if (mk_pool((int)alloc_size + (int)sizeof(struct mset) + 8))
      return NULL;
  /* Search current pool's free list for a reusable block */
  m = freelist_take(pool, alloc_size);
  if (m != NULL) {
    ((struct mhdr *)m)->moff = (char *)m - (char *)pool;
    ((struct mhdr *)m)->size = n;
    pool->refs++;
#if MALLOC_ZEROES
    /* Historic behaviour: match the clean state of a fresh bump allocation,
       which used to be guaranteed by the kernel zeroing every page it handed
       out. */
    memset((char *)m + sizeof(struct mhdr), 0, n);
#endif
    return (char *)m + sizeof(struct mhdr);
  }
  offset = pool->size;
  if (!((unsigned long)((char *)pool + offset + sizeof(struct mhdr)) & PGMASK))
    offset += sizeof(long);
  if (offset + alloc_size > (size_t)pool->len) {
    /* The current pool is out of bump room. Retired pools are searched only
       here, on the path whose alternative is mapping more memory: a walk of a
       few free lists is cheap against an mmap, and it is what keeps a
       long-lived process from accumulating pools it cannot reach. */
    m = old_pool_take(alloc_size);
    if (m != NULL) {
      ((struct mhdr *)m)->moff = (char *)m - (char *)pool;
      ((struct mhdr *)m)->size = n;
      pool->refs++;
#if MALLOC_ZEROES
      memset((char *)m + sizeof(struct mhdr), 0, n);
#endif
      return (char *)m + sizeof(struct mhdr);
    }
    if (mk_pool((int)alloc_size + (int)sizeof(struct mset) + 8))
      return NULL;
    offset = pool->size;
    if (!((unsigned long)((char *)pool + offset + sizeof(struct mhdr)) &
          PGMASK))
      offset += sizeof(long);
  }
  m = (char *)pool + offset;
  ((struct mhdr *)m)->moff = offset;
  ((struct mhdr *)m)->size = n;
  pool->refs++;
  pool->size = offset + alloc_size;
  if (!((unsigned long)((char *)pool + pool->size + sizeof(struct mhdr)) &
        PGMASK))
    pool->size += sizeof(long);
#if MALLOC_ZEROES
  memset((char *)m + sizeof(struct mhdr), 0, n);
#endif
  return m + sizeof(struct mhdr);
}

void free(void *v) {
  if (!v)
    return;
  struct mhdr *mhdr = (struct mhdr *)((char *)v - sizeof(struct mhdr));
  /* Large allocs carry the LARGE_MOFF sentinel in the mhdr.moff slot (a pool
     block's moff is its in-pool offset, always < its pool's len). */
  if (mhdr->moff == LARGE_MOFF) {
    char *base = (char *)v - LARGE_HDR;
    cache_unmap(base, *(long *)base);
    return;
  }
  {
    struct mset *mset = (void *)mhdr - mhdr->moff;
#if MALLOC_DEBUG
    if ((unsigned long)v < 0x10000000UL || ((unsigned long)mset & PGMASK) ||
        mhdr->moff < (int)sizeof(struct mset) || mhdr->moff >= MSETLEN_MAX) {
      fprintf(stderr, "[malloc] WILD FREE v=%p moff=%d size=%d mset=%p ra=%p\n",
              v, mhdr->moff, mhdr->size, (void *)mset,
              __builtin_return_address(0));
      return;
    }
#endif
    mset->refs--;
    if (mset->refs == 0 && mset != pool) {
      /* Remove from old_pools list if present */
      struct mset **pp = &old_pools;
      while (*pp) {
        if (*pp == mset) { *pp = mset->next_pool; break; }
        pp = &(*pp)->next_pool;
      }
      if (pool1 != NULL) {
        cache_unmap(mset, mset->len);
      } else {
        pool1 = mset;
      }
    } else if (mset != pool && mset->refs > 0) {
      /* Block freed into non-current pool - these are unreachable by malloc */
      int block_size = ((mhdr->size + sizeof(struct mhdr) + 7) & ~7);
#if MALLOC_DEBUG
      memset((char *)mhdr + sizeof(struct mhdr), 0xDE, block_size - sizeof(struct mhdr));
#endif
      freelist_insert(mset, mhdr, block_size);
    } else {
      /* Add freed block to current pool's free list for reuse */
      int block_size = ((mhdr->size + sizeof(struct mhdr) + 7) & ~7);
#if MALLOC_DEBUG
      memset((char *)mhdr + sizeof(struct mhdr), 0xDE, block_size - sizeof(struct mhdr));
#endif
      freelist_insert(mset, mhdr, block_size);
    }
  }
}

void *calloc(size_t n, size_t sz) {
  void *r = malloc(n * sz);
  if (r)
    memset(r, 0, n * sz);
  return r;
}

static long msize(void *v) {
  /* Both small and large store the user size in the mhdr.size slot (user-4). */
  return ((struct mhdr *)((char *)v - sizeof(struct mhdr)))->size;
}

void *realloc(void *v, size_t sz) {
  void *r;
  long old_size;

  if (!v)
    return malloc(sz);
  if (!sz) {
    free(v);
    return NULL;
  }

  old_size = msize(v);

  /* If shrinking or same size, return as-is (user size is in mhdr.size for
     both small and large). */
  if ((long)sz <= old_size) {
    ((struct mhdr *)((char *)v - sizeof(struct mhdr)))->size = sz;
    return v;
  }

  /* For large allocations, try to grow in-place */
  {
    struct mhdr *mhdr = (struct mhdr *)((char *)v - sizeof(struct mhdr));
    if (mhdr->moff == LARGE_MOFF) {
      char *base = (char *)v - LARGE_HDR;
      long mapped = *(long *)base;
      /* New size still fits in the existing mapping (minus the header)? */
      if ((long)sz + LARGE_HDR <= mapped) {
        mhdr->size = sz;
        return v;
      }
      /* Try kernel mremap to extend the mapping in-place. */
      long new_mapped = (sz + LARGE_HDR + PGMASK) & ~PGMASK;
      r = mremap(base, mapped, new_mapped, 0);
      if (r != NULL && r != (void *)-1) {
        *(long *)r = new_mapped;
        ((struct mhdr *)((char *)r + 8))->moff = LARGE_MOFF;
        ((struct mhdr *)((char *)r + 8))->size = sz;
        return (char *)r + LARGE_HDR;
      }
    }
  }

  r = malloc(sz);
  if (!r) {
    return NULL;
  }
  {
    int copy_len = old_size < (long)sz ? old_size : sz;
    memcpy(r, v, copy_len);
  }
  free(v);
  return r;
}
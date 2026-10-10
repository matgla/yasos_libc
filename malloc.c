#include <errno.h>
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
   what keeps a small compile's heap small.

   A pool that runs out of bump room first tries to grow in place (mremap into
   the free pages after it, one page at a time, up to MSETLEN_MAX), and a pool
   that has to be mapped fresh is mapped at the size its first request needs,
   then grows the same way. The ladder still sets the pooled/mapped threshold,
   and growth counts as the same evidence a replacement pool did; but a pool is
   no longer mapped at the class size up front. That had left most of the
   newest pool unused at a compile's peak. Replaying tcc's own allocation
   traces against a port of the device's page allocator, the 64 KiB pool
   mapped last held 16-20 KiB, and hello world peaked at 212 KiB mapped for
   141 KiB live; with growth in place it peaks at 152 KiB, and the peaks of
   50 libc files fall 18% in sum at -O0 and 9% at -O2. The kernel places a
   mapping at the lowest free run, so the newest pool is normally at the top
   and the pages after it are free; when they are not (a large mapping landed
   there), a fresh pool is mapped instead. The cap keeps a pool's free list as
   short as it was: unbounded growth took only ~3% more off the peak and
   doubled the free-list walks. */
#define MSETLEN_MIN (1 << 12)      /* first pool: one page, as before */
#define MSETLEN_MAX (64 * 1024)
#define MSETMAX_MIN 2048           /* first threshold, as before */
#define MSETMAX_CAP (32 * 1024)
#define LARGE_PER_CLASS 4
/* Pooled requests at least this big look in the retired pools first; see
   old_pool_best_fit. */
#define RETIRED_FIT_MIN 1024

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
/* An aligned block (posix_memalign/aligned_alloc): the user pointer has been
   moved forward to meet an alignment malloc does not promise, so neither of
   free()'s two ways of finding the block's start applies. The pointer malloc
   actually returned is parked in the word directly below this header, and
   free() releases that instead. */
#define ALIGNED_MOFF (-2)
#define LARGE_HDR 16

/* placed at the beginning of regions for small allocations */
struct mset {
  int refs;     /* number of allocations */
  int size;     /* bump pointer (next free offset) */
  void *freelist; /* singly-linked list of freed blocks for reuse */
  struct mset *next_pool; /* linked list of old pools with refs > 0 */
  int len;      /* this pool's mapped length -- pools are no longer one size */
  int max_free; /* retired pools: no block on the free list is bigger than
                   this -- an upper bound, made exact by a full walk -- so a
                   search can pass over the pool without walking it. Also keeps
                   sizeof(struct mset) a multiple of 8, so the first block's
                   user pointer stays 8-aligned (ARM LDRD, doubles). */
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

static void release_parked(void) {
  int i;
  for (i = 0; i < LARGE_CACHE_SLOTS; i++) {
    if (large_cache[i].base != NULL) {
      munmap(large_cache[i].base, large_cache[i].mapped);
      large_cache[i].base = NULL;
    }
  }
  large_cache_bytes = 0;
}

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
    /* No exact fit, so this mapping is fresh memory, and a process's peak is
       reached exactly here. Whatever is parked would be counted on top of it
       while serving nothing, so it goes back first. Measured on the device: up
       to 123 KB of a Zig compile's peak was parked mappings. */
    release_parked();
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
  release_parked();
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

/* Give the whole pages of a retired pool from byte offset `from` to its end back
   to the kernel (mremap shrinks a mapping in place), keeping `from` itself.
   Nothing bumps a retired pool, so free space at its end serves only the
   retired-pool paths, and until then it was mapped memory holding nothing: 85 KB
   of a Zig compile's peak on the device. Only for a retired pool -- the current
   one's end is its bump room. Trimming a pool's leftover bump room as it retires
   too was worse for tcc (+29 KB): what it returns, later requests then map anew.
   Returns the pool's length. */
static int pool_trim_end(struct mset *ms, int from) {
  int keep = (from + PGMASK) & ~PGMASK;
  if (in_vfork_window || keep >= ms->len)
    return ms->len;
  if (mremap(ms, ms->len, keep, 0) == (void *)ms) {
    ms->len = keep;
    if (ms->size > keep)
      ms->size = keep;
  }
  return ms->len;
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
    fn = prev_node;
  }
  /* A retired pool's free space reaching its end: hand its whole pages back. */
  if (mset != pool && (char *)fn + fn->block_size == (char *)mset + mset->len &&
      fn->block_size >= PGSIZE) {
    int off = (char *)fn - (char *)mset;
    int len = pool_trim_end(mset, off + (int)sizeof(struct free_node));
    if (len < off + fn->block_size)
      fn->block_size = len - off;
  }
  if (fn->block_size > mset->max_free)
    mset->max_free = fn->block_size;
}

/* Whether a free block can be handed out for `alloc_size` bytes. Blocks whose
   user pointer would be page-aligned are skipped -- page-aligned pointers are
   mistaken for large allocations by msize()/free()/realloc(). */
static int block_fits(struct free_node *fn, size_t alloc_size) {
  return fn->block_size >= (int)alloc_size &&
         ((unsigned long)((char *)fn + sizeof(struct mhdr)) & PGMASK);
}

/* Unlink the free block `*prev` points at, splitting what `alloc_size` does not
   need back onto the same list. */
static void *take_block(struct free_node **prev, size_t alloc_size) {
  struct free_node *fn = *prev;
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
  return fn;
}

/* Take a first-fit block of at least `alloc_size` bytes off `ms`'s free list,
   splitting the remainder back onto the list. NULL when nothing on the list
   fits. The caller fills in the header and bumps refs, because it is the one
   that knows which pool the block came from. */
static void *freelist_take(struct mset *ms, size_t alloc_size) {
  struct free_node **prev = (struct free_node **)&ms->freelist, *fn;
  while ((fn = *prev)) {
    if (block_fits(fn, alloc_size)) {
      take_block(prev, alloc_size);
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

/* Chain a pool that still has live blocks onto the retired list. Its free list
   has not been measured, so the bound starts at the whole pool.

   A pool retires because a request did not fit its remaining bump room, and a
   pool serves requests up to MSETMAX_CAP, so that room can be most of 32 KB.
   Nothing bumps a retired pool again, so it goes on the free list, where the
   retired-pool paths can reach it; the bump pointer moves to the end so a
   pool promoted back to current cannot hand the same bytes out twice. Left
   where it was, it was stranded for the pool's lifetime: ~150 KB across the
   14 retired pools of a Zig compile's peak on the device. */
static void retire(struct mset *ms) {
  int tail = (ms->size + 7) & ~7;
  ms->size = ms->len;
  ms->max_free = ms->len;
  if (ms->len - tail >= 2 * (int)sizeof(struct mhdr))
    freelist_insert(ms, (char *)ms + tail, ms->len - tail);
  ms->next_pool = old_pools;
  old_pools = ms;
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
    void *m = old->max_free >= (int)alloc_size ? freelist_take(old, alloc_size) : NULL;
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
        retire(pool);
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

/* Serve a mid-size `alloc_size` from the best-fitting free block in any retired
   pool, leaving that pool where it is. NULL when none fits.

   old_pool_take only runs once the current pool's bump pointer is exhausted,
   and the current pool is always the one being bumped -- so the holes a
   retired pool accumulates sit unused while fresh pool space is spent. What
   makes the holes is growth: a buffer grown by doubling leaves its previous
   copy behind at every step. A Zig compile on the device had 476 KB free
   inside 967 KB of retired pools at its peak, 371 KB of it in holes of 4 KB
   and up, while only ~0.5 MB of small objects were live. Filling those holes
   first took 205 KB off malloc's mapped peak there, and 197 KB off tcc
   compiling the result;
   a compile small enough never to retire a pool does not reach this.

   Best fit rather than first fit, so a big hole is not spent on a small
   request that a small hole could have served. Only requests of at least
   RETIRED_FIT_MIN come here; they are the ones that fragment. Walking every
   retired free list for each of them made tcc ~20% slower on a large file
   (thousands of 1-4 KB requests against dozens of pools full of small
   fragments), so a pool whose max_free rules out a fit is passed over, and a
   full walk leaves max_free exact. */
static void *old_pool_best_fit(size_t alloc_size, struct mset **owner) {
  struct mset *ms;
  struct free_node **prev, **best = NULL;
  struct mset *best_pool = NULL;
  if (in_vfork_window)
    return NULL;
  for (ms = old_pools; ms != NULL; ms = ms->next_pool) {
    int largest = 0;
    if (ms->max_free < (int)alloc_size)
      continue;
    for (prev = (struct free_node **)&ms->freelist; *prev; prev = &(*prev)->next) {
      if ((*prev)->block_size > largest)
        largest = (*prev)->block_size;
      if (block_fits(*prev, alloc_size) &&
          (best == NULL || (*prev)->block_size < (*best)->block_size)) {
        best = prev;
        best_pool = ms;
        if ((*prev)->block_size == (int)alloc_size)
          goto found;
      }
    }
    ms->max_free = largest;
  }
  if (best == NULL)
    return NULL;
found:
  *owner = best_pool;
  return take_block(best, alloc_size);
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
      retire(cur);
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
    /* Just what the request needs: the pool grows in place from here (see
       pool_grow). Not inside the vfork window, where nothing grows and the
       child should keep the old sizing. */
    int len = in_vfork_window ? pool_len_for(want) : (want + PGMASK) & ~PGMASK;
    pool = cache_map(len);
    if (pool == NULL) {
      return 1;
    }
    pool->len = len;
    pool->size = sizeof(*pool);
    pool->refs = 0;
    pool->freelist = 0;
    pool->next_pool = 0;
    pool->max_free = 0;
  }
  if (replacing)
    widen_pool_class();
  return 0;
}

/* Extend the current pool in place so its bump room reaches `need` bytes,
   growing by at least a page. Nonzero on success. Refused past MSETLEN_MAX,
   where a fresh pool is mapped instead, and inside the vfork window, where the
   child would be growing the parent's mapping. A pool that grows has outgrown
   its class just as surely as one that had to be replaced, so the class
   follows it (that is what moves mid-size requests into pools). */
static int pool_grow(int need) {
  int len = pool->len + PGSIZE;
  if (len < need)
    len = (need + PGMASK) & ~PGMASK;
  if (in_vfork_window || len > MSETLEN_MAX)
    return 0;
  if (mremap(pool, pool->len, len, 0) != (void *)pool)
    return 0;
  pool->len = len;
  while (pool_len_next < len && pool_len_next < MSETLEN_MAX)
    widen_pool_class();
  return 1;
}

/* A request of whole pages (4096, 8192, ... -- tcc's arenas and buffers are
   sized like that) would take a mapping one page longer than itself, for the
   16-byte header alone. A pool block costs it 8 bytes, so below the pooled
   ceiling such a request is served from a pool whatever the class: on hello
   world, before the class widened, they were 15 KiB of the 36 KiB mapped for
   21 KiB of large requests. */
static int header_spills_page(size_t n) {
  size_t rem = (n + LARGE_HDR) & PGMASK;
  return n < MSETMAX_CAP && rem != 0 && rem <= LARGE_HDR;
}

void *malloc(size_t n) {
  void *m;
  size_t alloc_size;
  int offset;
  if (n >= (size_t)cur_msetmax() && !header_spills_page(n)) {
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
  if (alloc_size >= RETIRED_FIT_MIN && old_pools != NULL) {
    struct mset *owner;
    m = old_pool_best_fit(alloc_size, &owner);
    if (m != NULL) {
      ((struct mhdr *)m)->moff = (char *)m - (char *)owner;
      ((struct mhdr *)m)->size = n;
      owner->refs++;
#if MALLOC_ZEROES
      memset((char *)m + sizeof(struct mhdr), 0, n);
#endif
      return (char *)m + sizeof(struct mhdr);
    }
  }
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
    if (!pool_grow(offset + (int)alloc_size + 8) &&
        mk_pool((int)alloc_size + (int)sizeof(struct mset) + 8))
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
  /* An over-aligned block hands back the pointer malloc gave us; everything
     about the real block (pool or mapping) hangs off that one. */
  if (mhdr->moff == ALIGNED_MOFF) {
    free(*(void **)((char *)mhdr - sizeof(void *)));
    return;
  }
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

/* realloc to a smaller size. A shrink has to give something back to be worth
   asking for: callers that trim a grown buffer (Zig's allocator does it for
   every toOwnedSlice) keep the block for as long as it lives.

   The size recorded in the header is what free() sizes the block by, so it may
   only change when the bytes it no longer covers go somewhere. Recording the
   smaller size without releasing anything -- what this used to do -- stranded
   the tail of a pool block outside every free list until its pool drained. */
static void *shrink(void *v, size_t sz, long old_size) {
  struct mhdr *mhdr = (struct mhdr *)((char *)v - sizeof(struct mhdr));

  if (mhdr->moff == LARGE_MOFF) {
    /* A block that stays large keeps its mapping and gives the tail pages back
       through mremap: moving it would hold old and new copies at once, which
       is exactly the peak a trimmed buffer (Zig trims every grown one) is
       meant to avoid. A kernel that cannot shrink a mapping refuses, and a
       block small enough for a pool moves into one, where it frees at least
       a page; otherwise the mapping stays as it is. */
    char *base = (char *)v - LARGE_HDR;
    long mapped = *(long *)base;
    long need = (sz >= (size_t)cur_msetmax())
                    ? (long)((sz + LARGE_HDR + PGMASK) & ~PGMASK)
                    : (long)sz + (long)sizeof(struct mhdr);
    if (sz >= (size_t)cur_msetmax() && mapped - need >= PGSIZE &&
        !in_vfork_window) {
      void *r = mremap(base, mapped, need, 0);
      if (r == base) {
        *(long *)base = need;
        mhdr->size = sz;
        return v;
      }
    }
    if (mapped - need >= PGSIZE) {
      void *r = malloc(sz);
      if (r != NULL) {
        memcpy(r, v, sz);
        free(v);
        return r;
      }
    }
    return v;
  }

  if (mhdr->moff >= 0) {
    /* A pool block: split the tail off onto its pool's free list, as long as it
       is big enough to be a block of its own (freelist_take's split rule). */
    int old_block = (old_size + sizeof(struct mhdr) + 7) & ~7;
    int new_block = (sz + sizeof(struct mhdr) + 7) & ~7;
    if (old_block - new_block >= 2 * (int)sizeof(struct mhdr)) {
      struct mset *mset = (struct mset *)((char *)mhdr - mhdr->moff);
      mhdr->size = sz;
      freelist_insert(mset, (char *)mhdr + new_block, old_block - new_block);
    }
    return v;
  }

  /* An over-aligned block: free() goes through the pointer malloc returned, so
     this header's size is only what msize() reports. */
  mhdr->size = sz;
  return v;
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

  if ((long)sz <= old_size)
    return shrink(v, sz, old_size);

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

/* ── Over-aligned allocation ──────────────────────────────────────────────
 *
 * malloc promises 8-byte alignment (see the mset padding note above), which is
 * all C requires. A caller wanting more -- 32-byte buffers are ordinary in
 * languages with explicit alignment, and the Zig compiler asks for them -- goes
 * through here.
 *
 * The block is over-allocated, the user pointer moved forward to the alignment,
 * and a header written directly in front of it carrying the ALIGNED_MOFF
 * sentinel; the pointer malloc returned is parked in the word below that
 * header. free() recognises the sentinel and releases the original, so nothing
 * is leaked and the pool/mapping bookkeeping is untouched. */
static void *alloc_aligned(size_t alignment, size_t size) {
  char *raw;
  char *user;
  size_t addr;
  struct mhdr *hdr;
  /* Room for the worst-case shift plus the header and the parked pointer. */
  size_t extra = alignment + sizeof(struct mhdr) + sizeof(void *);

  if (size > (size_t)-1 - extra)
    return NULL;
  raw = malloc(size + extra);
  if (!raw)
    return NULL;

  addr = (size_t)raw + sizeof(void *) + sizeof(struct mhdr);
  addr = (addr + alignment - 1) & ~(alignment - 1);
  user = (char *)addr;

  hdr = (struct mhdr *)(user - sizeof(struct mhdr));
  hdr->moff = ALIGNED_MOFF;
  hdr->size = (int)size;
  *(void **)(user - sizeof(struct mhdr) - sizeof(void *)) = raw;
  return user;
}

int posix_memalign(void **out, size_t alignment, size_t size) {
  void *p;
  if (out == NULL)
    return EINVAL;
  /* POSIX: a power of two, and a multiple of sizeof(void *). */
  if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0)
    return EINVAL;
  if (size == 0) {
    *out = NULL;
    return 0;
  }
  /* Nothing to do when malloc's own guarantee already covers it. */
  p = (alignment <= 8) ? malloc(size) : alloc_aligned(alignment, size);
  if (!p)
    return ENOMEM;
  *out = p;
  return 0;
}

void *aligned_alloc(size_t alignment, size_t size) {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    errno = EINVAL;
    return NULL;
  }
  if (size == 0)
    return NULL;
  return (alignment <= 8) ? malloc(size) : alloc_aligned(alignment, size);
}

void *memalign(size_t alignment, size_t size) {
  return aligned_alloc(alignment, size);
}

/* Bytes usable through this pointer. Both the small and the large path record
 * the requested size in the header word below the user pointer, and an
 * over-aligned block carries its own copy, so all three answer the same way.
 * Reporting the requested size rather than the block's full capacity is the
 * safe direction: a caller uses this to decide whether something fits without
 * reallocating. */
size_t malloc_usable_size(void *v) {
  if (!v)
    return 0;
  return (size_t)((struct mhdr *)((char *)v - sizeof(struct mhdr)))->size;
}

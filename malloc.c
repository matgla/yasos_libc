#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Only ours has it: a host build of these sources just gets zeroed pages. */
#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0
#endif

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
#define LARGE_CACHE_MAX_BYTES (256 * 1024)

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
/* Size bins of the free blocks in retired pools (see bin_take): 8-byte steps
   up to 256 bytes, then four per octave up to the largest pool. */
#define NBINS 68

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
  unsigned flags; /* MS_BINNED, and the pages given back (MS_HOLE); also
                     keeps sizeof(struct mset) a multiple of 8, so the first
                     block's user pointer stays 8-aligned (ARM LDRD, doubles) */
};

/* A retired pool whose free blocks are in the size bins. */
#define MS_BINNED 1u
/* Page k of the pool (k >= 1: page 0 holds this header) was given back to the
   kernel (pool_punch); only pools of up to MS_HOLE_PAGES pages get holes. */
#define MS_HOLE(k) (1u << (k))
#define MS_HOLE_PAGES 32
#define MS_HOLES(ms) ((ms)->flags & ~MS_BINNED)
/* The fewest whole pages a free block gives back (punch_retired).  Replayed
   over 13 tcc compiles (Zig kernel.c at several tcc revisions, -O0/-O1/-O2,
   with and without the internal spill): 4 lowered the peak in 12, by 0.15 MB
   on average (worst +0.13); 1 raised it in most, 2 and 3 by up to 0.4 / 0.25.
   A smaller run splits free space a later request would have fit, for a page
   or two.  The stress tests set 1, to make holes as often as they can. */
#ifndef PUNCH_MIN_PAGES
#define PUNCH_MIN_PAGES 4
#endif

/* placed before each small allocation */
struct mhdr {
  int moff; /* mset offset, | MOFF_SLACK8 */
  int size; /* allocation size */
};

/* A free block 8 bytes larger than a request is handed out whole: 8 bytes
   cannot be a free block of their own.  The header records the requested size
   (msize() reports it exactly), so this bit in moff says the block is 8 bytes
   longer than that size implies.  Without it free() gave back 8 bytes less,
   which then sat between two blocks for good, and each later reuse of the hole
   beside them added 8 more: at the peak of a Zig kernel compile 367 KB of pool
   space belonged to nothing, in gaps of up to a kilobyte.  A pool offset is
   below MSETLEN_MAX, so the bit is free; the large and aligned sentinels are
   negative and never carry it. */
#define MOFF_SLACK8 (1 << 30)
#define MOFF_OFFSET(moff) ((moff) & ~MOFF_SLACK8)

/* The pool block a header describes, in bytes, header included. */
static int mhdr_block_size(const struct mhdr *h) {
  return ((h->size + (int)sizeof(struct mhdr) + 7) & ~7) + ((h->moff & MOFF_SLACK8) ? 8 : 0);
}

/* overlays mhdr in freed blocks (must be >= sizeof(mhdr)) */
struct free_node {
  struct free_node *next; /* the pool's free blocks, in address order */
  int block_size; /* total block size including mhdr, aligned */
  struct free_node *bin_next; /* retired pools: the block's size bin, */
  struct free_node *bin_prev; /* doubly linked (16 bytes: the smallest block) */
};

static struct mset *pool;
static struct mset *pool1; /* a freed pool */
static struct mset *old_pools; /* linked list of full pools with refs > 0 */

/* ── Hole reuse across retired pools ──────────────────────────────────────
   A pool retires when its bump room runs out, and the blocks freed in it
   afterwards are holes between the ones still live.  They used to be reached
   only by requests of at least 1 KB, and the rest only when the current pool
   was exhausted, so every smaller request bumped fresh memory past them: at
   the peak of a Zig kernel compile (12.4 MB mapped for 8.8 MB live) 2.1 MB of
   pool space was free, 1.2 MB of it in holes under 1 KB.  Every free block of a
   retired pool is now in a size bin, and any request takes the best fitting
   hole -- smallest bin first -- before the current pool's free list, its bump
   room or a new mapping.  Replayed against the device page allocator
   (tests/malloc_replay.c) that compile maps 10.2 MB at its peak.  A binned
   block's pool is found by address in retired[], which is mapped (not .bss)
   the first time a process retires a pool -- a light process never pays for
   it.  The pools' address ordered free lists stay, for coalescing.  Every
   block is binned on the device: the smallest block is exactly a free_node,
   and the 16-byte holes are the ones the smallest requests need (leaving
   them out kept 1.4 MB more mapped in that compile).  A 64-bit host build has
   16-byte blocks and 32-byte nodes, and leaves those out. */
extern int __malloc_map_flags;
static struct free_node *bins[NBINS];
static unsigned binmap[(NBINS + 31) / 32];
static int bin_of(int size) {
  if (size <= 256)
    return (size >> 3) - 2;
  int o = 31 - __builtin_clz((unsigned)size) - 8;
  int b = 31 + o * 4 + ((size >> (o + 6)) & 3);
  return b < NBINS ? b : NBINS - 1;
}

#define BINNABLE(fn) ((fn)->block_size >= (int)sizeof(struct free_node))

static void bin_insert(struct free_node *fn) {
  if (!BINNABLE(fn))
    return;
  int b = bin_of(fn->block_size);
  fn->bin_prev = NULL;
  fn->bin_next = bins[b];
  if (bins[b])
    bins[b]->bin_prev = fn;
  bins[b] = fn;
  binmap[b >> 5] |= 1u << (b & 31);
}

static void bin_remove(struct free_node *fn) {
  if (!BINNABLE(fn))
    return;
  int b = bin_of(fn->block_size);
  if (fn->bin_prev)
    fn->bin_prev->bin_next = fn->bin_next;
  else
    bins[b] = fn->bin_next;
  if (fn->bin_next)
    fn->bin_next->bin_prev = fn->bin_prev;
  if (!bins[b])
    binmap[b >> 5] &= ~(1u << (b & 31));
}

/* The first non-empty bin at or above b, or NBINS. */
static int bin_next_used(int b) {
  while (b < NBINS) {
    unsigned w = binmap[b >> 5] >> (b & 31);
    if (w)
      return b + __builtin_ctz(w);
    b = (b | 31) + 1;
  }
  return NBINS;
}

/* retired[]: the binned pools, by address. */
static struct mset **retired;
static int nbinned_pools, cap_retired;

static int retired_index(const void *p) {
  int lo = 0, hi = nbinned_pools - 1, r = -1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    if ((const void *)retired[mid] <= p) {
      r = mid;
      lo = mid + 1;
    } else
      hi = mid - 1;
  }
  return r;
}

/* The retired pool holding block P.  Usually the last pool starting at or
   below it, but a pool can lie in another one's hole (pool_punch gives pages
   back, and the next mapping may land there), so a block past that hole
   belongs to an earlier pool. */
static struct mset *retired_owner(const void *p) {
  for (int i = retired_index(p); i >= 0; i--) {
    struct mset *ms = retired[i];
    const long off = (const char *)p - (const char *)ms;
    if (off < ms->len && !(MS_HOLES(ms) & MS_HOLE(off / PGSIZE)))
      return ms;
  }
  return NULL;
}

static void retired_add(struct mset *ms) {
  if (nbinned_pools == cap_retired) {
    long bytes = ((long)(cap_retired ? cap_retired * 2 : 64) * (long)sizeof(*retired) + PGMASK) & ~PGMASK;
    struct mset **n = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED | __malloc_map_flags, -1, 0);
    if (n == MAP_FAILED)
      return; /* its holes just stay out of the bins */
    if (retired) {
      memcpy(n, retired, nbinned_pools * sizeof(*retired));
      munmap(retired, ((long)cap_retired * (long)sizeof(*retired) + PGMASK) & ~PGMASK);
    }
    retired = n;
    cap_retired = (int)(bytes / (long)sizeof(*retired));
  }
  int i = retired_index(ms) + 1;
  memmove(retired + i + 1, retired + i, (nbinned_pools - i) * sizeof(*retired));
  retired[i] = ms;
  nbinned_pools++;
  ms->flags |= MS_BINNED;
  for (struct free_node *fn = ms->freelist; fn != NULL; fn = fn->next)
    bin_insert(fn);
}

static void retired_remove(struct mset *ms) {
  if (!(ms->flags & MS_BINNED))
    return;
  int i = retired_index(ms);
  for (struct free_node *fn = ms->freelist; fn != NULL; fn = fn->next)
    bin_remove(fn);
  memmove(retired + i, retired + i + 1, (nbinned_pools - i - 1) * sizeof(*retired));
  nbinned_pools--;
  ms->flags &= ~MS_BINNED;
}


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

/* Extra mmap flags for this process's heap: MAP_SLOW keeps malloc out of the fast
 * tier for a program that places its own hot data there. */
int __malloc_map_flags;

/* Map at least *total bytes; *total comes back as what the caller really got.
   Any parked mapping up to twice the request will do: a fresh one costs a page
   clear of every byte through PSRAM, 6.9% of a compile before this took near fits
   as well as exact ones (456 MB cleared over eight units, 8311 mappings, all for
   a heap of a few MB).  */
static void punch_retired(void);

/* The parked mapping cache_map would take for TOTAL bytes, or -1. */
static int cache_fit(long total) {
  int best = -1;
  for (int i = 0; i < LARGE_CACHE_SLOTS; i++) {
    if (large_cache[i].base && large_cache[i].mapped >= total && large_cache[i].mapped <= total * 2 &&
        (best < 0 || large_cache[i].mapped < large_cache[best].mapped))
      best = i;
  }
  return best;
}

/* Fresh pages are about to be mapped: a process's peak is reached exactly
   here, and whatever is parked or punchable would count on top of it while
   serving nothing. */
static void before_fresh_pages(void) {
  if (!in_vfork_window) {
    release_parked();
    punch_retired();
  }
}

static void *cache_map(long *total) {
  if (!in_vfork_window) {
    int best = cache_fit(*total);
    if (best >= 0) {
      void *base = large_cache[best].base;
      *total = large_cache[best].mapped;
      large_cache[best].base = NULL;
      large_cache_bytes -= *total;
      return base;
    }
    /* No exact fit, so this mapping is fresh memory, and a process's peak is
       reached exactly here. Whatever is parked would be counted on top of it
       while serving nothing, so it goes back first. Measured on the device: up
       to 123 KB of a Zig compile's peak was parked mappings. */
    before_fresh_pages();
  }
  {
    /* Nothing here needs zeroes: the parked mappings above are not zero either. */
    void *m = mmap(NULL, *total, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED | __malloc_map_flags, -1, 0);
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

/* The pages pool_punch would give back from free block FN of MS, [*p0, *p1)
   from the pool's start, and the pieces left of it (sizes in 8s, 0 = none):
   zero when there is nothing worth giving. */
static int punch_range(const struct mset *ms, const struct free_node *fn, int *p0, int *p1, int *left, int *right) {
  const int lo = (const char *)fn - (const char *)ms, hi = lo + fn->block_size;
  if (ms->len > MS_HOLE_PAGES * PGSIZE)
    return 0;
  *p0 = (lo + PGMASK) & ~PGMASK;
  *p1 = hi & ~PGMASK;
  /* Every block is a multiple of 8 bytes (a block handed out whole is taken
     to be exactly 8 over its request, MOFF_SLACK8), but after the bump path's
     page skip a block can start 4 bytes off: the pieces keep their sizes in
     8s, the left one from lo and the right one to hi, and the 4 bytes between
     a piece and the hole belong to nothing, as a page skip's do. */
  *left = (*p0 - lo) & ~7;
  *right = (hi - *p1) & ~7;
  if (*p0 > lo && *left < (int)sizeof(struct free_node)) {
    *p0 += PGSIZE;
    *left = (*p0 - lo) & ~7;
  }
  if (hi > *p1 && *right < (int)sizeof(struct free_node)) {
    *p1 -= PGSIZE;
    *right = (hi - *p1) & ~7;
  }
  return *p1 - *p0 >= PUNCH_MIN_PAGES * PGSIZE;
}

/* Give the whole pages inside free block FN of the retired pool MS back to the
   kernel, as a hole in the pool (MS_HOLE).  Nothing bumps a retired pool, so a
   free page there serves only a later request that happens to fit it.  What is
   left of FN on either side stays a free block; the two never coalesce again,
   as they are not adjacent.  Returns the block left in FN's place, or NULL; a
   block after the hole is put on the free list (and in its bin, if BINNED)
   here.  A pool with holes is unmapped piece by piece (pool_unmap_holes) and
   never reused whole. */
static struct free_node *pool_punch(struct mset *ms, struct free_node *fn, int binned) {
  char *base = (char *)ms;
  int p0, p1, left, right;
  if (!punch_range(ms, fn, &p0, &p1, &left, &right))
    return fn;
  const int hi = ((char *)fn - base) + fn->block_size;
  /* FN itself may be in the pages going: read it first. */
  struct free_node *const after = fn->next, **prev = NULL;
  if (left == 0) {
    prev = (struct free_node **)&ms->freelist;
    while (*prev != fn)
      prev = &(*prev)->next;
  }
  if (munmap(base + p0, p1 - p0) != 0)
    return fn;
  for (int k = p0 / PGSIZE; k < p1 / PGSIZE; k++)
    ms->flags |= MS_HOLE(k);
  struct free_node *rn = NULL;
  if (right > 0) {
    rn = (struct free_node *)(base + hi - right);
    rn->block_size = right;
    rn->next = after;
  }
  if (prev) {
    *prev = rn ? rn : after;
    fn = NULL;
  } else {
    fn->block_size = left;
    fn->next = rn ? rn : after;
  }
  if (rn && binned)
    bin_insert(rn);
  return fn;
}

/* Unmap a drained pool with holes: each run of pages it still holds is a
   mapping of its own now, and a range across a hole could take pages the
   kernel has since handed to another mapping of this process. */
static void pool_unmap_holes(struct mset *ms) {
  const unsigned holes = MS_HOLES(ms);
  const int pages = ms->len / PGSIZE;
  char *base = (char *)ms;
  for (int k = 0; k < pages;) {
    if (holes & MS_HOLE(k)) {
      k++;
      continue;
    }
    int e = k;
    while (e < pages && !(holes & MS_HOLE(e)))
      e++;
    munmap(base + (long)k * PGSIZE, (long)(e - k) * PGSIZE);
    k = e;
  }
}

static struct mset *retired_owner(const void *p);

/* About to map fresh memory -- where a process's peak grows: first give back
   the whole free pages of the retired pools' big blocks (pool_punch).  Only
   then: given back as soon as they were free, they split runs a later request
   would have fit, and the pieces under a page kept more mapped than the pages
   saved (Zig kernel compile: 7.65 -> 8.20 MB at the peak; 7.33 this way).
   The blocks of a page or more are all in the bins from bin_of(PGSIZE) up. */
static void punch_retired(void) {
  if (in_vfork_window || !nbinned_pools)
    return;
  for (int b = bin_next_used(bin_of(PGSIZE)); b < NBINS; b = bin_next_used(b + 1)) {
  again:
    for (struct free_node *fn = bins[b]; fn != NULL; fn = fn->bin_next) {
      int p0, p1, left, right;
      struct mset *ms;
      if (fn->block_size < PGSIZE || !(ms = retired_owner(fn)) || !punch_range(ms, fn, &p0, &p1, &left, &right))
        continue;
      const unsigned before = ms->flags;
      bin_remove(fn);
      struct free_node *rest = pool_punch(ms, fn, 1);
      if (rest != NULL)
        bin_insert(rest);
      if (ms->flags == before)
        return; /* the kernel refused: leave the rest for the next time */
      goto again; /* the bins changed; what is left in this one is smaller */
    }
  }
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
  /* A retired pool's free blocks are in the size bins (see bin_take). */
  const int binned = mset != pool && (mset->flags & MS_BINNED);

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
    if (binned)
      bin_remove(cur);
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
    if (binned)
      bin_remove(prev_node);
    prev_node->block_size += fn->block_size;
    prev_node->next = fn->next;
#if MALLOC_DEBUG
    memset((char *)prev_node + sizeof(struct free_node), 0xDE,
           prev_node->block_size - sizeof(struct free_node));
#endif
    fn = prev_node;
  }
  /* A retired pool's free space reaching its end: hand its whole pages back. */
  if (mset != pool && !MS_HOLES(mset) && (char *)fn + fn->block_size == (char *)mset + mset->len &&
      fn->block_size >= PGSIZE) {
    int off = (char *)fn - (char *)mset;
    int len = pool_trim_end(mset, off + (int)sizeof(struct free_node));
    if (len < off + fn->block_size)
      fn->block_size = len - off;
  }
  if (binned)
    bin_insert(fn);
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
  /* A retired pool's chain of big blocks is the caller's to keep. */
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
  if (ms->len - tail >= 2 * (int)sizeof(struct mhdr))
    freelist_insert(ms, (char *)ms + tail, ms->len - tail);
  ms->next_pool = old_pools;
  old_pools = ms;
  /* Not between vfork and exec: __malloc_vfork_restore puts the parent's pools
     back as they were, and a binned block would outlive that. */
  if (!in_vfork_window)
    retired_add(ms);
}

/* Take the best fitting hole of a retired pool for `alloc_size`: the
   smallest non-empty bin that can hold it, the first block there that fits
   (bins above the request's own hold only blocks that are big enough).  The
   pool keeps it, and stays where it is.  NULL when no hole fits. */
static void *bin_take(size_t alloc_size, struct mset **owner) {
  struct free_node *fn = NULL;
  for (int b = bin_next_used(bin_of((int)alloc_size)); b < NBINS; b = bin_next_used(b + 1)) {
    for (fn = bins[b]; fn != NULL; fn = fn->bin_next)
      if (block_fits(fn, alloc_size))
        goto found;
  }
  return NULL;
found:
  {
    struct mset *ms = retired_owner(fn);
    struct free_node **prev = (struct free_node **)&ms->freelist, *after;
    bin_remove(fn);
    while (*prev != fn)
      prev = &(*prev)->next;
    after = fn->next;
    take_block(prev, alloc_size);
    if (*prev != after) /* the split-off remainder */
      bin_insert(*prev);
    *owner = ms;
  }
  return fn;
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
    long len = in_vfork_window ? pool_len_for(want) : (want + PGMASK) & ~PGMASK;
    pool = cache_map(&len);
    if (pool == NULL) {
      return 1;
    }
    pool->len = len;
    pool->size = sizeof(*pool);
    pool->refs = 0;
    pool->freelist = 0;
    pool->next_pool = 0;
    pool->flags = 0;
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
  before_fresh_pages(); /* growing in place maps fresh pages as well */
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
    char *lm = cache_map(&total);
    if (lm == NULL) {
      errno = ENOMEM;
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
    if (mk_pool((int)alloc_size + (int)sizeof(struct mset) + 8)) {
      errno = ENOMEM;
      return NULL;
    }
  if (nbinned_pools && !in_vfork_window) {
    struct mset *owner;
    m = bin_take(alloc_size, &owner);
    if (m != NULL) {
      const int slack = ((struct free_node *)m)->block_size != (int)alloc_size ? MOFF_SLACK8 : 0;
      ((struct mhdr *)m)->moff = ((char *)m - (char *)owner) | slack;
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
    const int slack = ((struct free_node *)m)->block_size != (int)alloc_size ? MOFF_SLACK8 : 0;
    ((struct mhdr *)m)->moff = ((char *)m - (char *)pool) | slack;
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
    if (!pool_grow(offset + (int)alloc_size + 8) &&
        mk_pool((int)alloc_size + (int)sizeof(struct mset) + 8)) {
      errno = ENOMEM;
      return NULL;
    }
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
    struct mset *mset = (void *)mhdr - MOFF_OFFSET(mhdr->moff);
#if MALLOC_DEBUG
    if ((unsigned long)v < 0x10000000UL || ((unsigned long)mset & PGMASK) ||
        MOFF_OFFSET(mhdr->moff) < (int)sizeof(struct mset) || MOFF_OFFSET(mhdr->moff) >= MSETLEN_MAX) {
      fprintf(stderr, "[malloc] WILD FREE v=%p moff=%d size=%d mset=%p ra=%p\n",
              v, mhdr->moff, mhdr->size, (void *)mset,
              __builtin_return_address(0));
      return;
    }
#endif
    /* Between vfork and exec the child leaves the parent's retired pools
       alone: their blocks are binned, and restore does not undo bin changes. */
    if (mset != pool && in_vfork_window)
      return;
    mset->refs--;
    if (mset->refs == 0 && mset != pool) {
      retired_remove(mset);
      /* Remove from old_pools list if present */
      struct mset **pp = &old_pools;
      while (*pp) {
        if (*pp == mset) { *pp = mset->next_pool; break; }
        pp = &(*pp)->next_pool;
      }
      if (MS_HOLES(mset)) {
        pool_unmap_holes(mset); /* not reusable whole */
      } else if (pool1 != NULL) {
        cache_unmap(mset, mset->len);
      } else {
        pool1 = mset;
      }
    } else if (mset != pool && mset->refs > 0) {
      /* Block freed into non-current pool - these are unreachable by malloc */
      int block_size = mhdr_block_size(mhdr);
#if MALLOC_DEBUG
      memset((char *)mhdr + sizeof(struct mhdr), 0xDE, block_size - sizeof(struct mhdr));
#endif
      freelist_insert(mset, mhdr, block_size);
    } else {
      /* Add freed block to current pool's free list for reuse */
      int block_size = mhdr_block_size(mhdr);
#if MALLOC_DEBUG
      memset((char *)mhdr + sizeof(struct mhdr), 0xDE, block_size - sizeof(struct mhdr));
#endif
      freelist_insert(mset, mhdr, block_size);
    }
  }
}

void *calloc(size_t n, size_t sz) {
  void *r;
  if (sz != 0 && n > (size_t)-1 / sz) {
    errno = ENOMEM;
    return NULL;
  }
  r = malloc(n * sz);
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
    int old_block = mhdr_block_size(mhdr);
    int new_block = (sz + sizeof(struct mhdr) + 7) & ~7;
    struct mset *mset = (struct mset *)((char *)mhdr - MOFF_OFFSET(mhdr->moff));
    (void)old_size;
    if (old_block - new_block >= 2 * (int)sizeof(struct mhdr)) {
      if (!(mset != pool && in_vfork_window)) {
        mhdr->size = sz;
        mhdr->moff &= ~MOFF_SLACK8;
        freelist_insert(mset, (char *)mhdr + new_block, old_block - new_block);
      }
    } else {
      /* Too little to give back: the block keeps it, the size stays exact. */
      mhdr->size = sz;
      mhdr->moff = MOFF_OFFSET(mhdr->moff) | (old_block != new_block ? MOFF_SLACK8 : 0);
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
      /* Try kernel mremap to extend the mapping in-place -- unless a parked
         mapping fits: the copy below takes it, no fresh pages.  Growing maps
         fresh pages, so the parked ones go first (cache_map does the same);
         kept, up to 245 KB of them sat on a Zig compile's peak. */
      long new_mapped = (sz + LARGE_HDR + PGMASK) & ~PGMASK;
      r = NULL;
      if (in_vfork_window || cache_fit(new_mapped) < 0) {
        before_fresh_pages();
        r = mremap(base, mapped, new_mapped, 0);
      }
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

  if (size > (size_t)-1 - extra) {
    errno = ENOMEM;
    return NULL;
  }
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
/* Where the pooled memory is, for profiling a process's peak (tcc's
   TCC_LIVEPROF calls this at its snapshots).  Fills up to n of:
     [0] pools mapped (current + spare + retired)   [1] their bytes
     [2] free bytes on their free lists              [3] of that, in whole pages
     [4] current pool's unused bump room             [5] spare pool bytes
     [6] retired pools                               [7] retired pool bytes
     [8] retired free bytes                          [9] of that, in whole pages
     [10] parked large mappings, bytes
     [11..18] free bytes by block size: <64 <256 <1K <2K <4K <8K <16K >=16K
   Whole pages: the pages a free block covers entirely past its header, i.e.
   what could go back to the kernel if a mapping could be split. */
static long stats_hist[8]; /* free bytes by block size: <64 <256 <1K <2K <4K <8K <16K >=16K */
static void stats_pool(struct mset *ms, long *free_b, long *free_pg) {
  for (struct free_node *fn = ms->freelist; fn != NULL; fn = fn->next) {
    int b = fn->block_size, k = b < 64 ? 0 : b < 256 ? 1 : b < 1024 ? 2 : b < 2048 ? 3 : b < 4096 ? 4 : b < 8192 ? 5 : b < 16384 ? 6 : 7;
    stats_hist[k] += b;
    unsigned long a = ((unsigned long)fn + sizeof(struct free_node) + PGMASK) & ~(unsigned long)PGMASK;
    unsigned long e = ((unsigned long)fn + fn->block_size) & ~(unsigned long)PGMASK;
    *free_b += fn->block_size;
    if (e > a)
      *free_pg += (long)(e - a);
  }
}

void __malloc_stats(long *out, int n) {
  long v[19] = {0};
  for (int i = 0; i < 8; i++)
    stats_hist[i] = 0;
  if (pool) {
    v[0]++, v[1] += pool->len;
    stats_pool(pool, &v[2], &v[3]);
    v[4] = pool->len - pool->size;
  }
  if (pool1) {
    v[0]++, v[1] += pool1->len;
    v[5] = pool1->len;
  }
  for (struct mset *ms = old_pools; ms != NULL; ms = ms->next_pool) {
    long fb = 0, fp = 0;
    const long held = ms->len - (long)__builtin_popcount(MS_HOLES(ms)) * PGSIZE;
    v[0]++, v[1] += held;
    v[6]++, v[7] += held;
    stats_pool(ms, &fb, &fp);
    v[2] += fb, v[3] += fp, v[8] += fb, v[9] += fp;
  }
  v[10] = large_cache_bytes;
  for (int i = 0; i < 8; i++)
    v[11 + i] = stats_hist[i];
  for (int i = 0; i < n && i < 19; i++)
    out[i] = v[i];
}

size_t malloc_usable_size(void *v) {
  if (!v)
    return 0;
  return (size_t)((struct mhdr *)((char *)v - sizeof(struct mhdr)))->size;
}

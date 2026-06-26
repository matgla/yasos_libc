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

#define PGSIZE 4096
#define PGMASK (PGSIZE - 1)
/* Largest object served from the bump pool; >= this goes to a direct mmap.
 * Must stay below MSETLEN minus pool overhead (sizeof(struct mset) + per-alloc
 * header + page-skip slack, ~48 B): malloc does NOT recheck fit after mk_pool()
 * (see ~line 286), so a pooled object bigger than the pool would overflow the
 * mmap. With a 4 KiB pool that caps the threshold at ~4048; 2048 leaves margin
 * and keeps medium objects (which are rare) on the direct-mmap path. */
#define MSETMAX 2048
/* Bump-pool chunk size. The kernel process pool is page-granular (4 KiB), so
 * the first malloc maps this many pages up front. 32 KiB (the inherited 2010
 * "neat" default) wasted ~24 KiB on light applets whose live small-object set
 * is only 2-6 KiB. 4 KiB = one page: the minimum that still holds the largest
 * pooled object (MSETMAX-1 + overhead). Allocations >= MSETMAX bypass the pool
 * via direct mmap, so this never bounds large objects (e.g. tcc's). */
#define MSETLEN (1 << 12)

/* Large allocations (>= MSETMAX) are mmap'd with a small self-describing header
   in the mapping itself, so they are UNBOUNDED — no fixed-size tracking table
   (which capped concurrent large allocs and returned NULL / "memory full" once
   full) and zero per-process .bss. A central table was historically used to
   dodge a 4 KiB header page per alloc, but the kernel page grain is now 256 B,
   so an in-mapping header rounds the mapping up by at most one grain.

   Header (16 B; the user pointer is 16 B past the mmap base):
     +0  long mapped     total mmap'd bytes (for munmap / mremap)
     +8  int  moff = -1   LARGE_MOFF sentinel. A pool block's moff is its offset
                          into the pool, always in [0, MSETLEN), so -1 can't
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

/* vfork save/restore — prevents child from corrupting parent's pool state */
static struct mset *vfork_saved_pool;
static struct mset *vfork_saved_pool1;
static struct mset *vfork_saved_old_pools;
static int vfork_saved_pool_size;
static int vfork_saved_pool_refs;
static void *vfork_saved_pool_freelist;

void __malloc_vfork_save(void) {
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
    munmap(pool, MSETLEN);
  if (pool1 && pool1 != vfork_saved_pool && pool1 != vfork_saved_pool1)
    munmap(pool1, MSETLEN);
  pool = vfork_saved_pool;
  pool1 = vfork_saved_pool1;
  old_pools = vfork_saved_old_pools;
  if (pool) {
    pool->size = vfork_saved_pool_size;
    pool->refs = vfork_saved_pool_refs;
    pool->freelist = vfork_saved_pool_freelist;
  }
}

/* Insert a freed block into a pool's free list, sorted by address.
   Coalesces with adjacent free blocks to combat fragmentation. */
static void freelist_insert(struct mset *mset, void *ptr, int block_size) {
  struct free_node *fn = (struct free_node *)ptr;
  fn->block_size = block_size;

#if MALLOC_DEBUG
  /* Validate block is within the pool */
  if ((char *)ptr < (char *)mset || (char *)ptr + block_size > (char *)mset + MSETLEN) {
    fprintf(stderr, "[malloc] BAD FREE: ptr=%p blk=%d pool=%p..%p\n",
            ptr, block_size, mset, (char *)mset + MSETLEN);
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

static int mk_pool(void) {
  struct mset *old_pool = pool;
  if ((pool == NULL || pool->refs > 0) && pool1 != NULL) {
    pool = pool1;
    pool1 = NULL;
  }
  if (pool != NULL && pool->refs == 0) {
    /* Chain the old full pool before recycling pool1 */
    if (old_pool && old_pool != pool && old_pool->refs > 0) {
      old_pool->next_pool = old_pools;
      old_pools = old_pool;
    }
    pool->size = sizeof(*pool);
    pool->freelist = 0;
    return 0;
  }
  /* Chain old pool with live refs onto old_pools list */
  if (old_pool && old_pool->refs > 0) {
    old_pool->next_pool = old_pools;
    old_pools = old_pool;
  }
  pool = mmap(NULL, MSETLEN, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (pool == MAP_FAILED) {
    pool = NULL;
    return 1;
  }
  pool->size = sizeof(*pool);
  pool->refs = 0;
  pool->freelist = 0;
  pool->next_pool = 0;
  return 0;
}

void *malloc(size_t n) {
  void *m;
  size_t alloc_size;
  int offset;
  if (n >= MSETMAX) {
    /* Self-tracking large alloc: header in the mapping, no table, unbounded. */
    long total = (n + LARGE_HDR + PGMASK) & ~PGMASK;
    char *lm = mmap(NULL, total, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (lm == MAP_FAILED) {
      return NULL;
    }
    *(long *)lm = total;
    ((struct mhdr *)(lm + 8))->moff = LARGE_MOFF;
    ((struct mhdr *)(lm + 8))->size = (int)n;
    return lm + LARGE_HDR;
  }
  alloc_size = (n + sizeof(struct mhdr) + 7) & ~7;
  if (!pool)
    if (mk_pool())
      return NULL;
  /* Search current pool's free list for a reusable block */
  {
    struct free_node **prev = (struct free_node **)&pool->freelist, *fn;
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
        m = (void *)fn;
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
            fprintf(stderr, "[malloc] POISON VIOLATED: reuse %p blk=%d for n=%d\n",
                    fn, fn->block_size, (int)n);
            fprintf(stderr, "[malloc] data: ");
            for (int j = 0; j < 32 && j < data_len; j++)
              fprintf(stderr, "%02x ", data[j]);
            fprintf(stderr, "\n");
            malloc_debug_trap(fn, fn->block_size, (int)n);
          }
        }
#endif
        ((struct mhdr *)m)->moff = (char *)m - (char *)pool;
        ((struct mhdr *)m)->size = n;
        pool->refs++;
        /* Zero the returned block so callers see the same clean state
           as from fresh bump allocation (pages come pre-zeroed). */
        memset((char *)m + sizeof(struct mhdr), 0, n);
        return (char *)m + sizeof(struct mhdr);
      }
      prev = &fn->next;
    }
  }
  /* NOTE: We deliberately do NOT search old_pools' freelists here.
     Allocating from old pools adds refs, preventing them from ever reaching
     zero and being reclaimed.  Let old pools drain naturally via free(). */
  offset = pool->size;
  if (!((unsigned long)((char *)pool + offset + sizeof(struct mhdr)) & PGMASK))
    offset += sizeof(long);
  if (offset + alloc_size > MSETLEN) {
    if (mk_pool())
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
  return m + sizeof(struct mhdr);
}

void free(void *v) {
  if (!v)
    return;
  struct mhdr *mhdr = (struct mhdr *)((char *)v - sizeof(struct mhdr));
  /* Large allocs carry the LARGE_MOFF sentinel in the mhdr.moff slot (a pool
     block's moff is its in-pool offset, always < MSETLEN). */
  if (mhdr->moff == LARGE_MOFF) {
    char *base = (char *)v - LARGE_HDR;
    munmap(base, *(long *)base);
    return;
  }
  {
    struct mset *mset = (void *)mhdr - mhdr->moff;
#if MALLOC_DEBUG
    if ((unsigned long)v < 0x10000000UL || ((unsigned long)mset & PGMASK) ||
        mhdr->moff < (int)sizeof(struct mset) || mhdr->moff >= MSETLEN) {
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
        munmap(mset, MSETLEN);
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
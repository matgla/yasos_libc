#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define PGSIZE 4096
#define PGMASK (PGSIZE - 1)
#define MSETMAX 4096
#define MSETLEN (1 << 15)

/* malloc profiling - prints stats to stderr */
static long prof_current;      /* bytes currently mapped via mmap */
static long prof_peak;         /* high water mark */
static long prof_total_mapped; /* cumulative bytes mmapped */
static int prof_malloc_calls;
static int prof_free_calls;
static int prof_realloc_calls;
static int prof_mmap_calls;
static int prof_munmap_calls;
static int prof_inplace; /* realloc served in-place */
static int prof_pool_allocs;
static int prof_large_allocs;

/* Tracking table for large allocations - eliminates the 4KB header page
   overhead per allocation. Each entry is 8 bytes vs 4096 bytes wasted. */
#define MAX_LARGE_ALLOCS 256
struct large_alloc {
  void *addr;       /* mmap base address (NULL = free slot) */
  long mapped_size; /* total mapped bytes (page-aligned) */
  long user_size;   /* requested user-visible allocation size */
};
static struct large_alloc large_table[MAX_LARGE_ALLOCS];

static struct large_alloc *find_large(void *ptr) {
  for (int i = 0; i < MAX_LARGE_ALLOCS; i++)
    if (large_table[i].addr == ptr)
      return &large_table[i];
  return NULL;
}

static void prof_map(long sz) {
  prof_current += sz;
  prof_total_mapped += sz;
  prof_mmap_calls++;
  if (prof_current > prof_peak)
    prof_peak = prof_current;
}

static void prof_unmap(long sz) {
  prof_current -= sz;
  prof_munmap_calls++;
}

void malloc_dump_profile(void) {
  fprintf(stderr, "[malloc] current=%ldK peak=%ldK total_mapped=%ldK\n",
          prof_current / 1024, prof_peak / 1024, prof_total_mapped / 1024);
  fprintf(stderr,
          "[malloc] calls: malloc=%d(pool=%d,large=%d) free=%d "
          "realloc=%d(inplace=%d)\n",
          prof_malloc_calls, prof_pool_allocs, prof_large_allocs,
          prof_free_calls, prof_realloc_calls, prof_inplace);
  fprintf(stderr, "[malloc] mmap=%d munmap=%d\n", prof_mmap_calls,
          prof_munmap_calls);
  /* dump active large allocations */
  int active = 0;
  long active_bytes = 0;
  for (int i = 0; i < MAX_LARGE_ALLOCS; i++) {
    if (large_table[i].addr) {
      active++;
      active_bytes += large_table[i].mapped_size;
    }
  }
  fprintf(stderr, "[malloc] large: active=%d active_bytes=%ldK\n", active,
          active_bytes / 1024);
}

/* placed at the beginning of regions for small allocations */
struct mset {
  int refs; /* number of allocations */
  int size; /* remaining size */
};

/* placed before each small allocation */
struct mhdr {
  int moff; /* mset offset */
  int size; /* allocation size */
};

static struct mset *pool;
static struct mset *pool1; /* a freed pool */

/* vfork save/restore — prevents child from corrupting parent's pool state */
static struct mset *vfork_saved_pool;
static struct mset *vfork_saved_pool1;
static int vfork_saved_pool_size;
static int vfork_saved_pool_refs;

void __malloc_vfork_save(void) {
  vfork_saved_pool = pool;
  vfork_saved_pool1 = pool1;
  vfork_saved_pool_size = pool ? pool->size : 0;
  vfork_saved_pool_refs = pool ? pool->refs : 0;
}

void __malloc_vfork_restore(void) {
  pool = vfork_saved_pool;
  pool1 = vfork_saved_pool1;
  if (pool) {
    pool->size = vfork_saved_pool_size;
    pool->refs = vfork_saved_pool_refs;
  }
}

static int mk_pool(void) {
  if ((pool == NULL || pool->refs > 0) && pool1 != NULL) {
    pool = pool1;
    pool1 = NULL;
  }
  if (pool != NULL && pool->refs == 0) {
    pool->size = sizeof(*pool);
    return 0;
  }
  pool = mmap(NULL, MSETLEN, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (pool == MAP_FAILED) {
    pool = NULL;
    return 1;
  }
  prof_map(MSETLEN);
  pool->size = sizeof(*pool);
  pool->refs = 0;
  return 0;
}

void *malloc(size_t n) {
  void *m;
  size_t alloc_size;
  int offset;
  prof_malloc_calls++;
  if (n >= MSETMAX) {
    long total = (n + PGMASK) & ~PGMASK; /* page-align, no header page */
    m = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
             -1, 0);
    if (m == MAP_FAILED) {
      fprintf(stderr,
              "[malloc] FAIL large n=%ld total=%ld cur=%ldK peak=%ldK\n",
              (long)n, total, prof_current / 1024, prof_peak / 1024);
      malloc_dump_profile();
      return NULL;
    }
    prof_map(total);
    prof_large_allocs++;
    for (int i = 0; i < MAX_LARGE_ALLOCS; i++) {
      if (!large_table[i].addr) {
        large_table[i].addr = m;
        large_table[i].mapped_size = total;
        large_table[i].user_size = n;
        return m;
      }
    }
    prof_unmap(total);
    munmap(m, total);
    return NULL;
  }
  prof_pool_allocs++;
  alloc_size = (n + sizeof(struct mhdr) + 7) & ~7;
  if (!pool)
    if (mk_pool())
      return NULL;
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
  {
    void *ret = m + sizeof(struct mhdr);
    return ret;
  }
}

void free(void *v) {
  if (!v)
    return;
  prof_free_calls++;
  if ((unsigned long)v & PGMASK) {
    struct mhdr *mhdr = v - sizeof(struct mhdr);
    struct mset *mset = (void *)mhdr - mhdr->moff;
    mset->refs--;
    if (mset->refs == 0 && mset != pool) {
      if (pool1 != NULL) {
        prof_unmap(MSETLEN);
        munmap(mset, MSETLEN);
      } else {
        pool1 = mset;
      }
    }
  } else {
    struct large_alloc *e = find_large(v);
    if (e) {
      prof_unmap(e->mapped_size);
      munmap(e->addr, e->mapped_size);
      e->addr = NULL;
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
  if ((unsigned long)v & PGMASK)
    return ((struct mhdr *)(v - sizeof(struct mhdr)))->size;
  struct large_alloc *e = find_large(v);
  return e ? e->user_size : 0;
}

void *realloc(void *v, size_t sz) {
  void *r;
  long old_size;

  prof_realloc_calls++;
  if (!v)
    return malloc(sz);
  if (!sz) {
    free(v);
    return NULL;
  }

  old_size = msize(v);

  /* If shrinking or same size, return as-is */
  if ((long)sz <= old_size) {
    if ((unsigned long)v & PGMASK) {
      ((struct mhdr *)(v - sizeof(struct mhdr)))->size = sz;
    } else {
      struct large_alloc *e = find_large(v);
      if (e)
        e->user_size = sz;
    }
    return v;
  }

  /* For large allocations, try to grow in-place */
  if (!((unsigned long)v & PGMASK)) {
    struct large_alloc *e = find_large(v);
    if (e) {
      /* Check if new size fits in already-mapped pages */
      if ((long)sz <= e->mapped_size) {
        prof_inplace++;
        e->user_size = sz;
        return v;
      }
      /* Try kernel mremap to extend the mapping in-place */
      long new_mapped = (sz + PGMASK) & ~PGMASK;
      r = mremap(v, e->mapped_size, new_mapped, 0);
      if (r != NULL && r != (void *)-1) {
        prof_inplace++;
        prof_map(new_mapped - e->mapped_size);
        e->addr = r;
        e->mapped_size = new_mapped;
        e->user_size = sz;
        return r;
      }
    }
  }

  r = malloc(sz);
  if (!r) {
    fprintf(stderr, "[malloc] realloc FAIL old=%ld new=%ld cur=%ldK\n",
            old_size, (long)sz, prof_current / 1024);
    return NULL;
  }
  memcpy(r, v, old_size < (long)sz ? old_size : sz);
  free(v);
  return r;
}
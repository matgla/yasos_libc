// Host stress test of the pools: random work against the allocator, checking
// that every retired pool's free blocks are exactly what its size bins hold,
// that a hole of a retired pool that fits is used before fresh space, and that
// every pool is tiled -- header, live blocks, free blocks and bump room cover
// it with no byte belonging to nothing (a block freed short of its real length
// used to strand its tail for good, MOFF_SLACK8 in malloc.c).
#define _GNU_SOURCE
#include <sys/mman.h>
#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0
#endif
#define malloc t_malloc
#define free t_free
#define calloc t_calloc
#define realloc t_realloc
#define posix_memalign t_posix_memalign
#define aligned_alloc t_aligned_alloc
#define memalign t_memalign
#define malloc_usable_size t_malloc_usable_size
#include "../malloc.c"
#undef malloc
#undef free
#undef calloc
#undef realloc

#include <stdint.h>

#define SLOTS 6000
static struct { unsigned char *p; size_t n; unsigned char tag; } slot[SLOTS];
static unsigned long checks, tilings, searches, binned_seen, list_seen, retired_seen;

static void die(const char *what)
{
  fprintf(stderr, "FAIL: %s (after %lu checks)\n", what, checks);
  abort();
}

static int in_pool(const struct mset *ms, const void *p)
{
  return (const char *)p >= (const char *)ms && (const char *)p < (const char *)ms + ms->len;
}

/* Every retired pool's free list against the bins. */
static void check(void)
{
  checks++;
  unsigned long in_lists = 0, in_bins = 0;
  for (struct mset *ms = old_pools; ms; ms = ms->next_pool)
  {
    struct free_node *last = NULL;
    retired_seen++;
    if (ms == pool) die("the current pool is on the retired list");
    for (struct free_node *fn = ms->freelist; fn; fn = fn->next)
    {
      list_seen++;
      if ((char *)fn < (char *)ms + sizeof(*ms) || (char *)fn + fn->block_size > (char *)ms + ms->len)
        die("a free block outside its pool");
      if (last && !((char *)last + last->block_size < (char *)fn)) die("list out of order or not coalesced");
      if (ms->binned && BINNABLE(fn))
      {
        in_lists++;
        int b = bin_of(fn->block_size);
        struct free_node *x = bins[b];
        while (x && x != fn)
          x = x->bin_next;
        if (!x) die("a free block of a binned pool is not in its bin");
      }
      last = fn;
    }
  }
  for (int b = 0; b < NBINS; b++)
  {
    if (!!bins[b] != !!(binmap[b >> 5] & (1u << (b & 31)))) die("binmap disagrees with a bin");
    for (struct free_node *fn = bins[b]; fn; fn = fn->bin_next)
    {
      in_bins++;
      binned_seen++;
      if (bin_of(fn->block_size) != b) die("a block in the wrong bin");
      if (fn->bin_next && fn->bin_next->bin_prev != fn) die("a bin's links are broken");
    }
  }
  if (in_lists != in_bins) die("a bin holds a block no retired free list has");
}

/* The pool tiling: sorted pieces must abut, except for the bump path's skip
   of a long so a user pointer is never page-aligned. */
static int cmp_piece(const void *a, const void *b)
{
  uintptr_t x = *(const uintptr_t *)a, y = *(const uintptr_t *)b;
  return (x > y) - (x < y);
}

static uintptr_t (*pieces)[2];

static void check_tiling(struct mset *ms, uintptr_t end)
{
  long k = 0;
  pieces[k][0] = (uintptr_t)ms, pieces[k][1] = sizeof(struct mset), k++;
  for (struct free_node *fn = ms->freelist; fn; fn = fn->next)
    pieces[k][0] = (uintptr_t)fn, pieces[k][1] = fn->block_size, k++;
  for (int i = 0; i < SLOTS; i++)
    if (slot[i].p && in_pool(ms, slot[i].p))
    {
      struct mhdr *h = (struct mhdr *)(slot[i].p - sizeof(struct mhdr));
      if (h->moff < 0) die("a pool block with a negative moff");
      pieces[k][0] = (uintptr_t)h, pieces[k][1] = mhdr_block_size(h), k++;
    }
  qsort(pieces, k, sizeof(*pieces), cmp_piece);
  uintptr_t at = (uintptr_t)ms;
  for (long i = 0; i < k; i++)
  {
    uintptr_t gap = pieces[i][0] - at;
    if (pieces[i][0] < at) die("two pieces of a pool overlap");
    if (gap && !(gap <= sizeof(long) && !((at + sizeof(struct mhdr)) & PGMASK)) &&
        !(gap < 8 && i == 0))
    {
      fprintf(stderr, "gap %lu at offset %lu of %s pool (len %d size %d), before a piece of %lu at %lu (prev end %lu)\n",
              (unsigned long)gap, (unsigned long)(at - (uintptr_t)ms), ms == pool ? "current" : "retired", ms->len,
              ms->size, (unsigned long)pieces[i][1], (unsigned long)(pieces[i][0] - (uintptr_t)ms),
              (unsigned long)(at - (uintptr_t)ms));
      die("pool bytes that belong to nothing");
    }
    at = pieces[i][0] + pieces[i][1];
  }
  if (at > end) die("a piece runs past its pool");
  /* a retired pool's tail too small for a free block (< 16 bytes), or the
     8-byte rounding of its bump pointer */
  if (end - at >= 2 * sizeof(struct mhdr) && end != (uintptr_t)ms + ms->size) die("a pool's tail belongs to nothing");
  tilings++;
}

static void check_all_tilings(void)
{
  for (struct mset *ms = old_pools; ms; ms = ms->next_pool)
    check_tiling(ms, (uintptr_t)ms + ms->len);
  if (pool)
    check_tiling(pool, (uintptr_t)pool + pool->size);
}

/* Does any binned hole fit? */
static int binned_hole_fits(size_t alloc_size)
{
  for (struct mset *ms = old_pools; ms; ms = ms->next_pool)
    if (ms->binned)
      for (struct free_node *fn = ms->freelist; fn; fn = fn->next)
        if (BINNABLE(fn) && block_fits(fn, alloc_size))
          return 1;
  return 0;
}

static int in_retired(const void *p)
{
  for (struct mset *ms = old_pools; ms; ms = ms->next_pool)
    if (in_pool(ms, p))
      return 1;
  return 0;
}

static size_t pick_size(unsigned r)
{
  switch (r % 10)
  {
  case 0: case 1: case 2: case 3: return 8 + (r >> 8) % 64;
  case 4: case 5: return 64 + (r >> 8) % 512;
  case 6: case 7: return 1000 + (r >> 8) % 3000;
  case 8: return 4096 * (1 + (r >> 8) % 3) - 8;
  default: return 2048 + (r >> 8) % 14000;
  }
}

static void fill(int i, size_t n, unsigned char tag)
{
  slot[i].n = n; slot[i].tag = tag;
  memset(slot[i].p, tag, n);
}

static void verify(int i)
{
  for (size_t k = 0; k < slot[i].n; k++)
    if (slot[i].p[k] != slot[i].tag) die("a live block was overwritten");
}

static void run(unsigned seed, long steps)
{
  unsigned x = seed;
  for (long s = 0; s < steps; s++)
  {
    x = x * 1664525u + 1013904223u;
    int i = (x >> 8) % SLOTS;
    unsigned r = x * 2654435761u ^ (x >> 13);
    if (slot[i].p == NULL)
    {
      size_t n = pick_size(r);
      size_t alloc_size = (n + sizeof(struct mhdr) + 7) & ~7ul;
      int searched = n < (size_t)cur_msetmax() && !header_spills_page(n) && nbinned_pools && pool &&
                     binned_hole_fits(alloc_size);
      slot[i].p = t_malloc(n);
      if (slot[i].p == NULL) die("out of memory");
      if (searched)
      {
        searches++;
        if (!in_retired(slot[i].p)) die("a fitting hole was passed over for fresh space");
      }
      fill(i, n, (unsigned char)(r >> 24));
    }
    else if (r % 8 == 0)
    {
      verify(i);
      size_t n = (r >> 3) % 2 ? slot[i].n / 2 + 1 : pick_size(r >> 5);
      size_t keep = n < slot[i].n ? n : slot[i].n;
      unsigned char *q = t_realloc(slot[i].p, n);
      if (q == NULL) die("realloc failed");
      for (size_t k = 0; k < keep; k++)
        if (q[k] != slot[i].tag) die("realloc lost data");
      if ((size_t)msize(q) != n) die("msize is not the requested size");
      slot[i].p = q;
      fill(i, n, (unsigned char)(r >> 16));
    }
    else
    {
      verify(i);
      t_free(slot[i].p);
      slot[i].p = NULL;
    }
    if (s % 16 == 0 || s < 2000) check();
    if (s % 1024 == 0) check_all_tilings();
  }
  check();
  check_all_tilings();
  for (int i = 0; i < SLOTS; i++)
    if (slot[i].p) { verify(i); t_free(slot[i].p); slot[i].p = NULL; }
  check();
}

int main(void)
{
  _Static_assert(sizeof(struct mset) % 8 == 0, "first block's user pointer must stay 8-aligned");
  pieces = t_calloc(SLOTS + (1 << 16), sizeof(*pieces));
  for (unsigned seed = 1; seed <= 6; seed++)
    run(seed * 2654435761u, 600000);
  printf("ok: %lu checks, %lu tilings, %lu hole searches, %lu retired pool visits, %lu free blocks, %lu binned\n",
         checks, tilings, searches, retired_seen, list_seen, binned_seen);
  if (searches == 0 || binned_seen == 0 || retired_seen == 0 || tilings == 0) die("the test never reached the bins");
  return 0;
}

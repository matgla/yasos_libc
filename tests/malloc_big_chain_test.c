// Host test of the retired pools' chain of big free blocks: random work against
// the allocator, checking after every step that the chain is exactly the list's
// big blocks and that the search picks what a walk of the whole list would.
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
static unsigned long checks, searches, chained_seen, list_seen, retired_seen, same_size;

static void die(const char *what)
{
  fprintf(stderr, "FAIL: %s (after %lu checks)\n", what, checks);
  abort();
}

/* The whole structure of every retired pool. */
static void check(void)
{
  checks++;
  for (struct mset *ms = old_pools; ms; ms = ms->next_pool)
  {
    struct free_node *big = ms->big, *last = NULL;
    retired_seen++;
    if (ms == pool) die("the current pool is on the retired list");
    for (struct free_node *fn = ms->freelist; fn; fn = fn->next)
    {
      list_seen++;
      if ((char *)fn < (char *)ms + sizeof(*ms) || (char *)fn + fn->block_size > (char *)ms + ms->len)
        die("a free block outside its pool");
      if (last && !((char *)last + last->block_size < (char *)fn)) die("list out of order or not coalesced");
      if (fn->block_size > ms->max_free) die("max_free is below a free block");
      if (fn->block_size >= RETIRED_FIT_MIN)
      {
        chained_seen++;
        if (big != fn) die("a big block is missing from the chain");
        big = fn->next_big;
      }
      last = fn;
    }
    if (big != NULL) die("the chain has a block the list does not");
  }
}

/* What a walk of every free block would choose. */
static struct free_node *full_walk(size_t alloc_size)
{
  struct free_node *best = NULL;
  for (struct mset *ms = old_pools; ms; ms = ms->next_pool)
    for (struct free_node *fn = ms->freelist; fn; fn = fn->next)
      if (block_fits(fn, alloc_size) && (best == NULL || fn->block_size < best->block_size))
      {
        best = fn;
        if (fn->block_size == (int)alloc_size) return best;
      }
  return best;
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
      int searched = n < (size_t)cur_msetmax() && alloc_size >= RETIRED_FIT_MIN && pool && old_pools;
      struct free_node *want = searched ? full_walk(alloc_size) : NULL;
      slot[i].p = t_malloc(n);
      if (slot[i].p == NULL) die("out of memory");
      if (searched)
      {
        searches++;
        if (want && (char *)want + sizeof(struct mhdr) != (char *)slot[i].p) die("the search picked another block");
        if (want && want->block_size == (int)alloc_size) same_size++;
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
  }
  check();
  for (int i = 0; i < SLOTS; i++)
    if (slot[i].p) { verify(i); t_free(slot[i].p); slot[i].p = NULL; }
  check();
}

int main(void)
{
  _Static_assert(sizeof(struct mset) % 8 == 0, "first block's user pointer must stay 8-aligned");
  for (unsigned seed = 1; seed <= 6; seed++)
    run(seed * 2654435761u, 600000);
  printf("ok: %lu checks, %lu searches, %lu retired pool visits, %lu free blocks, %lu of them chained\n", checks, searches, retired_seen, list_seen, chained_seen);
  if (searches == 0 || chained_seen == 0 || retired_seen == 0) die("the test never reached the chain");
  return 0;
}

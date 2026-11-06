/*
 * Replay a process's allocation trace through malloc.c against a port of the
 * device's page allocator, and report what the device would have mapped.
 *
 * The trace is what tcc's TCC_ALLOCTRACE=FILE writes (a -DTCC_ALLOC_PROFILE
 * native build, libtcc.c): one record of three 32-bit words per call it made
 * into libc -- {in, size, out}: in 0 is a malloc, size 0 a free, otherwise a
 * realloc of `in`; `out` is what the device's malloc returned, used here only
 * to name the block.  The page allocator is the one malloc_overlap_devbackend.c
 * models (lowest free run, in-place mremap growth and shrink), so a change to
 * malloc.c's placement policy shows up here as it would on the device, in
 * seconds instead of an image rebuild.
 *
 *   gcc -m32 -O2 -o /tmp/malloc_replay malloc_replay.c && /tmp/malloc_replay alloc.trace
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- device-like page arena (as malloc_overlap_devbackend.c) ---- */
#define DEV_PAGE 4096
#define DEV_PAGES 16384 /* 64 MiB */
static unsigned char dev_arena[(long)DEV_PAGES * DEV_PAGE] __attribute__((aligned(DEV_PAGE)));
static unsigned char dev_bitmap[DEV_PAGES];
static long dev_used, dev_peak, dev_maps, dev_fresh; /* pages newly mapped or grown into: what the kernel clears */
static long dev_low; /* no free page below this index */

static long dev_pages_for(long len) { return (len + DEV_PAGE - 1) / DEV_PAGE; }

static void dev_set(long from, long n, unsigned char v) {
  memset(dev_bitmap + from, v, n);
  dev_used += v ? n : -n;
  if (dev_used > dev_peak)
    dev_peak = dev_used;
  if (!v && from < dev_low)
    dev_low = from;
}

static void *dev_mmap(long len) {
  long need = dev_pages_for(len), i = dev_low;
  dev_maps++;
  while (i < DEV_PAGES && dev_bitmap[i])
    i++;
  dev_low = i;
  while (i + need <= DEV_PAGES) {
    long j = 0;
    while (j < need && !dev_bitmap[i + j])
      j++;
    if (j == need) {
      dev_set(i, need, 1);
      dev_fresh += need;
      return dev_arena + i * DEV_PAGE;
    }
    i += j + 1;
  }
  return (void *)-1;
}

static long dev_index(void *addr) { return ((unsigned char *)addr - dev_arena) / DEV_PAGE; }

static int dev_munmap(void *addr, long len) {
  dev_set(dev_index(addr), dev_pages_for(len), 0);
  return 0;
}

static void *dev_mremap(void *addr, long old_len, long new_len, int flags) {
  (void)flags;
  long idx = dev_index(addr), oldn = dev_pages_for(old_len), newn = dev_pages_for(new_len);
  if (newn == oldn)
    return addr;
  if (newn < oldn) {
    if (newn <= 0)
      return (void *)-1;
    dev_set(idx + newn, oldn - newn, 0);
    return addr;
  }
  if (idx + newn > DEV_PAGES)
    return (void *)-1;
  for (long k = oldn; k < newn; k++)
    if (dev_bitmap[idx + k])
      return (void *)-1;
  dev_set(idx + oldn, newn - oldn, 1);
  dev_fresh += newn - oldn;
  return addr;
}

#define MAP_PRIVATE 0
#define MAP_ANONYMOUS 0
#define PROT_READ 0
#define PROT_WRITE 0
#define MAP_FAILED ((void *)-1)
#define mmap(addr, len, prot, flags, fd, off) dev_mmap((long)(len))
#define munmap(addr, len) dev_munmap((addr), (long)(len))
#define mremap(addr, old, new, flags) dev_mremap((addr), (long)(old), (long)(new), (flags))
#define _SYS_MMAN_H 1

#define fprintf(...) (0)
#define malloc sut_malloc
#define free sut_free
#define calloc sut_calloc
#define realloc sut_realloc
#ifndef MALLOC_C
#define MALLOC_C "../malloc.c"
#endif
#include MALLOC_C
#undef fprintf
#undef malloc
#undef free
#undef calloc
#undef realloc

/* ---- device pointer -> replay pointer ---- */
static uint32_t *map_key;
static void **map_val;
static uint32_t map_cap = 1u << 22;

static uint32_t map_slot(uint32_t k) {
  uint32_t m = map_cap - 1, h = (k >> 3) * 2654435761u & m;
  while (map_key[h] && map_key[h] != k)
    h = (h + 1) & m;
  return h;
}

static void map_del(uint32_t k) {
  uint32_t m = map_cap - 1, h = map_slot(k);
  if (!map_key[h])
    return;
  map_key[h] = 0;
  for (uint32_t j = (h + 1) & m; map_key[j]; j = (j + 1) & m) {
    uint32_t kk = map_key[j];
    void *vv = map_val[j];
    map_key[j] = 0;
    uint32_t s = map_slot(kk);
    map_key[s] = kk;
    map_val[s] = vv;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s TRACE\n", argv[0]);
    return 2;
  }
  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  map_key = calloc(map_cap, sizeof *map_key);
  map_val = calloc(map_cap, sizeof *map_val);
  /* live requested bytes, by the device's own sizes */
  uint32_t *size_of = calloc(map_cap, sizeof *size_of);
  long live = 0, live_peak = 0, live_at_peak = 0, events = 0;
  uint32_t rec[3 * 4096];
  size_t n;
  clock_t t0 = clock();
  while ((n = fread(rec, sizeof(uint32_t) * 3, 4096, f)) > 0) {
    for (size_t i = 0; i < n; i++) {
      uint32_t in = rec[3 * i], size = rec[3 * i + 1], out = rec[3 * i + 2];
      events++;
      if (size == 0) {
        uint32_t s = map_slot(in);
        if (map_key[s]) {
          sut_free(map_val[s]);
          live -= size_of[s];
          map_del(in);
        }
        continue;
      }
      void *p;
      if (in) {
        uint32_t s = map_slot(in);
        void *old = map_key[s] ? map_val[s] : NULL;
        if (map_key[s]) {
          live -= size_of[s];
          map_del(in);
        }
        p = sut_realloc(old, size);
      } else
        p = sut_malloc(size);
      if (!p) {
        printf("out of arena at event %ld\n", events);
        return 1;
      }
      uint32_t s = map_slot(out);
      map_key[s] = out;
      map_val[s] = p;
      size_of[s] = size;
      live += size;
      if (live > live_peak)
        live_peak = live;
      if (dev_used * DEV_PAGE >= dev_peak * DEV_PAGE && live > live_at_peak)
        live_at_peak = live;
    }
  }
  printf("events %ld  mapped peak %.3f MB  live peak %.3f MB  mappings %ld  mapped in total %.1f MB  (%.1f s)\n",
         events, dev_peak * (double)DEV_PAGE / 1e6, live_peak / 1e6, dev_maps, dev_fresh * (double)DEV_PAGE / 1e6,
         (double)(clock() - t0) / CLOCKS_PER_SEC);
  return 0;
}

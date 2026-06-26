/*
 * Host reproducer #2 — models the DEVICE memory backend, not host mmap.
 *
 * The first reproducer used the host kernel's mmap/mremap, which spread
 * allocations far apart (ASLR) and refuse in-place growth onto busy pages.
 * The RP2350 yasos backend instead serves pages from a single fixed arena
 * with a page bitmap, lowest-free-fit reuse, and in-place mremap extension
 * (source/kernel/memory/heap/process_memory_pool.zig). That aggressive
 * page reuse is the regime in which a freed region silently aliases a live
 * one — the symptom seen on device (put_elf_sym writing into a live regalloc
 * interval array).
 *
 * Here we shim mmap/munmap/mremap to a faithful port of that page allocator
 * so the SAME malloc.c logic runs against the SAME backend semantics, and we
 * assert no two live allocations ever overlap and content is never clobbered.
 *
 *   gcc -m32 -O2 -g -o /tmp/mor_dev malloc_overlap_devbackend.c && /tmp/mor_dev
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- device-like page arena (mirrors process_memory_pool.zig) ---- */
#define DEV_PAGE 4096
#define DEV_PAGES 8192            /* 32 MiB arena, like a generous PSRAM heap */
static unsigned char dev_arena[DEV_PAGES * DEV_PAGE] __attribute__((aligned(DEV_PAGE)));
static unsigned char dev_bitmap[DEV_PAGES];

static long dev_pages_for(long len) { return (len + DEV_PAGE - 1) / DEV_PAGE; }

/* lowest-free-fit, exactly like get_next_free_slot + allocate_pages */
static void *dev_mmap(long len) {
  long need = dev_pages_for(len);
  if (need <= 0) return (void *)-1;
  long i = 0;
  while (i + need <= DEV_PAGES) {
    long j = 0;
    while (j < need && !dev_bitmap[i + j]) j++;
    if (j == need) {
      for (long k = 0; k < need; k++) dev_bitmap[i + k] = 1;
      void *p = dev_arena + i * DEV_PAGE;
      memset(p, 0, need * DEV_PAGE);
      return p;
    }
    i += j + 1; /* skip past the busy page */
  }
  return (void *)-1; /* MAP_FAILED */
}

static long dev_index(void *addr) {
  return ((unsigned char *)addr - dev_arena) / DEV_PAGE;
}

static int dev_munmap(void *addr, long len) {
  long idx = dev_index(addr);
  long n = dev_pages_for(len);
  for (long k = 0; k < n; k++) dev_bitmap[idx + k] = 0;
  return 0;
}

/* in-place extend only (flags ignored), mirrors try_extend_pages */
static void *dev_mremap(void *addr, long old_len, long new_len, int flags) {
  (void)flags;
  long idx = dev_index(addr);
  long oldn = dev_pages_for(old_len);
  long newn = dev_pages_for(new_len);
  if (newn <= oldn) return addr;
  if (idx + newn > DEV_PAGES) return (void *)-1;
  for (long k = oldn; k < newn; k++)
    if (dev_bitmap[idx + k]) return (void *)-1;
  for (long k = oldn; k < newn; k++) dev_bitmap[idx + k] = 1;
  memset((unsigned char *)addr + oldn * DEV_PAGE, 0, (newn - oldn) * DEV_PAGE);
  return addr;
}

/* shim the names malloc.c calls (it includes <sys/mman.h>; we pre-empt it) */
#define MAP_PRIVATE 0
#define MAP_ANONYMOUS 0
#define PROT_READ 0
#define PROT_WRITE 0
#define MAP_FAILED ((void *)-1)
#define mmap(addr, len, prot, flags, fd, off) dev_mmap((long)(len))
#define munmap(addr, len) dev_munmap((addr), (long)(len))
#define mremap(addr, old, new, flags) dev_mremap((addr), (long)(old), (long)(new), (flags))
#define _SYS_MMAN_H 1 /* block the real header if pulled in */

#define fprintf(...) (0)
#define malloc  sut_malloc
#define free    sut_free
#define calloc  sut_calloc
#define realloc sut_realloc
#include "../malloc.c"
#undef fprintf
#undef malloc
#undef free
#undef calloc
#undef realloc

/* ---- live-allocation tracking + audit (same as reproducer #1) ---- */
#define MAXLIVE 4096
struct live { unsigned char *p; size_t n; unsigned char tag; };
static struct live live[MAXLIVE];
static int nlive;
static unsigned char next_tag = 1;
static unsigned long long ops;
static int violations;

static void audit(void) {
  for (int i = 0; i < nlive; i++) {
    for (size_t k = 0; k < live[i].n; k++) {
      if (live[i].p[k] != live[i].tag) {
        violations++;
        printf("  !! CONTENT CLOBBER op %llu live[%d]=%p+%zu byte %zu=%02x exp %02x\n",
               ops, i, (void *)live[i].p, live[i].n, k, live[i].p[k], live[i].tag);
        break;
      }
    }
    for (int j = i + 1; j < nlive; j++) {
      unsigned char *a0 = live[i].p, *a1 = a0 + live[i].n;
      unsigned char *b0 = live[j].p, *b1 = b0 + live[j].n;
      if (a0 < b1 && b0 < a1) {
        violations++;
        printf("  !! OVERLAP op %llu live[%d]=%p+%zu vs live[%d]=%p+%zu\n",
               ops, i, (void *)a0, live[i].n, j, (void *)b0, live[j].n);
      }
    }
  }
}

static void do_alloc(size_t n) {
  if (nlive >= MAXLIVE || n == 0) return;
  unsigned char *p = sut_malloc(n);
  if (!p) return;
  unsigned char tag = next_tag++; if (!next_tag) next_tag = 1;
  memset(p, tag, n);
  live[nlive++] = (struct live){p, n, tag};
  ops++; audit();
}
static void do_free(int idx) {
  if (idx < 0 || idx >= nlive) return;
  sut_free(live[idx].p);
  live[idx] = live[--nlive];
  ops++; audit();
}
static void do_realloc(int idx, size_t n) {
  if (idx < 0 || idx >= nlive || n == 0) return;
  unsigned char tag = live[idx].tag; size_t old = live[idx].n;
  unsigned char *q = sut_realloc(live[idx].p, n);
  if (!q) return;
  size_t chk = old < n ? old : n;
  for (size_t k = 0; k < chk; k++)
    if (q[k] != tag) { violations++;
      printf("  !! REALLOC LOST DATA op %llu byte %zu=%02x exp %02x\n", ops, k, q[k], tag);
      break; }
  memset(q, tag, n);
  live[idx].p = q; live[idx].n = n;
  ops++; audit();
}

static unsigned long rng_state = 1;
static unsigned rng(void) { rng_state = rng_state * 1103515245 + 12345; return (rng_state >> 16) & 0x7fff; }

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("device-backend reproducer (void*=%zu mhdr=%zu free_node=%zu arena=%dMiB)\n",
         sizeof(void *), sizeof(struct mhdr), sizeof(struct free_node),
         (DEV_PAGES * DEV_PAGE) >> 20);

  /* Phase 1: random mixed, biased small with periodic large (interval arrays,
     symtab grow into the >=4096 large path). */
  for (int seed = 1; seed <= 6 && violations < 20; seed++) {
    rng_state = 0x1000 * seed + 7;
    for (int it = 0; it < 8000 && violations < 20; it++) {
      unsigned r = rng() % 100;
      if (r < 45) {
        size_t n = (rng() % 100 < 85) ? 8 + rng() % 400 : 4097 + rng() % 24000;
        do_alloc(n);
      } else if (r < 70 && nlive) {
        do_realloc(rng() % nlive, 8 + rng() % 30000); /* span small<->large */
      } else if (nlive) {
        do_free(rng() % nlive);
      }
    }
    while (nlive) do_free(nlive - 1);
  }

  /* Phase 2: tcc-like — long-lived growers (large, realloc-extended) amid a
     churn of small allocs and frees, the exact shape that aliased on device. */
  {
    rng_state = 0xBEEF;
    for (int g = 0; g < 5; g++) do_alloc(2048);
    for (int round = 0; round < 12000 && violations < 20; round++) {
      int burst = 1 + rng() % 6;
      for (int b = 0; b < burst; b++) do_alloc(8 + rng() % 300);
      if (nlive > 12 && (rng() & 1)) do_free(5 + rng() % (nlive - 5));
      if (rng() % 3 == 0 && nlive > 0)
        do_realloc(rng() % (nlive < 5 ? nlive : 5), 2048 + (round / 20) * 64 + rng() % 256);
    }
    while (nlive) do_free(nlive - 1);
  }

  printf("\nTotal ops: %llu   Violations: %d\n", ops, violations);
  if (violations) { printf("REPRODUCED on host (device-backend model).\n"); return 1; }
  printf("No overlap/corruption with device-backend model + this pattern.\n");
  return 0;
}

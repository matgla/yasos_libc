/*
 * Standalone host reproducer for the device heap corruption seen on RP2350
 * while self-hosted armv8m-tcc compiles 90_struct-init.c:
 *
 *   A live regalloc interval array was found (via DWT watchpoint) being
 *   overwritten by put_elf_sym (symtab section) and the IR operand pool —
 *   i.e. two *distinct, simultaneously-live* tcc_malloc/tcc_realloc
 *   allocations physically overlapped on the PSRAM heap.
 *
 * tcc_malloc just wraps the libc allocator (libs/libc/malloc.c). This test
 * compiles that exact allocator and checks the invariant the device violated:
 *   - no two live allocations may overlap
 *   - a live allocation's bytes must never change unless we touch it
 *
 * Build 32-bit to match the device's pointer/struct sizes (mhdr=8, free_node=8):
 *   gcc -m32 -O2 -g -o /tmp/mor malloc_overlap_repro.c && /tmp/mor
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* pull in the allocator under test, renamed to avoid host-libc clash */
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

/* ---- tracking of live allocations ---- */
#define MAXLIVE 4096
struct live {
  unsigned char *p;
  size_t n;
  unsigned char tag; /* fill byte that identifies this allocation */
};
static struct live live[MAXLIVE];
static int nlive;
static unsigned char next_tag = 1;

static unsigned long long ops;
static int violations;

static void fail(const char *what, int i, int j) {
  violations++;
  printf("  !! VIOLATION (%s) at op %llu: live[%d]=%p+%zu",
         what, ops, i, (void *)live[i].p, live[i].n);
  if (j >= 0)
    printf(" vs live[%d]=%p+%zu", j, (void *)live[j].p, live[j].n);
  printf("\n");
}

/* full audit: pairwise overlap + content integrity */
static void audit(void) {
  for (int i = 0; i < nlive; i++) {
    /* content integrity: every byte must still equal the tag */
    for (size_t k = 0; k < live[i].n; k++) {
      if (live[i].p[k] != live[i].tag) {
        fail("content-clobbered", i, -1);
        printf("     byte %zu = %02x, expected tag %02x\n",
               k, live[i].p[k], live[i].tag);
        break;
      }
    }
    /* pairwise overlap */
    for (int j = i + 1; j < nlive; j++) {
      unsigned char *a0 = live[i].p, *a1 = a0 + live[i].n;
      unsigned char *b0 = live[j].p, *b1 = b0 + live[j].n;
      if (a0 < b1 && b0 < a1)
        fail("overlap", i, j);
    }
  }
}

static void do_alloc(size_t n) {
  if (nlive >= MAXLIVE || n == 0)
    return;
  unsigned char *p = sut_malloc(n);
  if (!p)
    return;
  unsigned char tag = next_tag++;
  if (next_tag == 0)
    next_tag = 1;
  memset(p, tag, n);
  live[nlive].p = p;
  live[nlive].n = n;
  live[nlive].tag = tag;
  nlive++;
  ops++;
  audit();
}

static void do_free(int idx) {
  if (idx < 0 || idx >= nlive)
    return;
  sut_free(live[idx].p);
  live[idx] = live[--nlive];
  ops++;
  audit();
}

static void do_realloc(int idx, size_t n) {
  if (idx < 0 || idx >= nlive || n == 0)
    return;
  unsigned char tag = live[idx].tag;
  size_t old = live[idx].n;
  unsigned char *q = sut_realloc(live[idx].p, n);
  if (!q)
    return;
  /* preserved prefix must survive */
  size_t chk = old < n ? old : n;
  for (size_t k = 0; k < chk; k++) {
    if (q[k] != tag) {
      violations++;
      printf("  !! VIOLATION (realloc-lost-data) at op %llu: byte %zu=%02x "
             "expected %02x\n", ops, k, q[k], tag);
      break;
    }
  }
  memset(q, tag, n); /* refill whole (grown region included) */
  live[idx].p = q;
  live[idx].n = n;
  ops++;
  audit();
}

/* deterministic PRNG (no host rand dependency) */
static unsigned long rng_state = 0x12345678;
static unsigned rng(void) {
  rng_state = rng_state * 1103515245 + 12345;
  return (rng_state >> 16) & 0x7fff;
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("malloc overlap reproducer  (sizeof void*=%zu mhdr=%zu free_node=%zu "
         "MSETMAX=%d MSETLEN=%d)\n",
         sizeof(void *), sizeof(struct mhdr), sizeof(struct free_node),
         MSETMAX, MSETLEN);

  /* --- Phase 1: random mixed stress, several seeds --- */
  for (int seed = 1; seed <= 8; seed++) {
    rng_state = 0x1000 * seed + 1;
    for (int it = 0; it < 8000 && violations < 20; it++) {
      unsigned r = rng() % 100;
      if (r < 45) {
        /* bias toward small (IR operands/intervals), some large */
        size_t n = (rng() % 100 < 90) ? 8 + rng() % 400 : 4097 + rng() % 16000;
        do_alloc(n);
      } else if (r < 70 && nlive) {
        do_realloc(rng() % nlive, 8 + rng() % 2000);
      } else if (nlive) {
        do_free(rng() % nlive);
      }
    }
    /* drain */
    while (nlive)
      do_free(nlive - 1);
  }

  /* --- Phase 2: tcc-like growth pattern --- */
  /* Mimic: a few arrays that grow via realloc (symtab section, interval
     arrays) while a steady churn of small allocations (IR operands, strings)
     happens around them. */
  {
    rng_state = 0xC0FFEE;
    int growers[6];
    for (int g = 0; g < 6; g++) {
      do_alloc(64);
      growers[g] = nlive - 1;
    }
    for (int round = 0; round < 8000 && violations < 20; round++) {
      /* churn small allocations */
      int burst = 1 + rng() % 6;
      for (int b = 0; b < burst; b++)
        do_alloc(8 + rng() % 200);
      /* free some random small ones (not the growers) */
      if (nlive > 12 && (rng() & 1)) {
        int idx = 6 + rng() % (nlive - 6);
        do_free(idx);
        /* growers[] indices may have shifted due to swap-remove; recompute */
        for (int g = 0; g < 6; g++) {
          /* find grower by walking — they are the long-lived large-tagged
             ones; simplest: re-pin by scanning is overkill, just grow the
             first 6 live slots which remain the growers in practice */
        }
      }
      /* grow a grower */
      if (rng() % 3 == 0) {
        int g = rng() % 6;
        if (g < nlive)
          do_realloc(g, 64 + (round / 100) * 8 + rng() % 128);
      }
    }
    while (nlive)
      do_free(nlive - 1);
  }

  printf("\nTotal ops: %llu   Violations: %d\n", ops, violations);
  if (violations) {
    printf("REPRODUCED heap corruption on host.\n");
    return 1;
  }
  printf("No overlap/corruption detected with this pattern.\n");
  return 0;
}

/* What `make qemu`'s bare-metal newlib lacks and the tests use (qemu_suite.sh).
 *
 * timespec_get: utest.h times each test with it -- a clock that stays at zero
 * (the times print as 0).
 *
 * mmap, munmap, mremap: the tests serve the SUT's faked sys_mmap from the
 * "host", as they do with the host kernel's.  Anonymous pages from a static
 * arena, placed at the lowest free run as the device's page allocator places
 * them; any range can be unmapped (malloc_tests leaves room after a mapping by
 * unmapping its tail); mremap shrinks in place, grows in place when the pages
 * after the mapping are free, and moves only with MREMAP_MAYMOVE. */

#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

int timespec_get(struct timespec *ts, int base)
{
  ts->tv_sec = 0;
  ts->tv_nsec = 0;
  return base;
}

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

/* 2 MiB.  The harness's linker script loads .data from flash, and .bss rides
   in that segment (as GNU ld places it too): QEMU zero-fills the .bss at the
   flash address, and past the 4 MiB code memory that fill lands on the
   program's own code.  So .bss has to end below 4 MiB of flash. */
enum { PAGE = 4096, ARENA_PAGES = 512 };
static unsigned char arena[ARENA_PAGES * PAGE] __attribute__((aligned(PAGE)));
static unsigned char used[ARENA_PAGES];

static int pages(int len)
{
  return (int)(((unsigned)len + PAGE - 1) / PAGE);
}

/* first page of ADDR in the arena, or -1 */
static int page_of(void *addr)
{
  unsigned char *p = addr;
  if (p < arena || p >= arena + sizeof arena || (p - arena) % PAGE)
    return -1;
  return (int)((p - arena) / PAGE);
}

static int run_free(int first, int n)
{
  if (first < 0 || n < 0 || first + n > ARENA_PAGES)
    return 0;
  for (int i = first; i < first + n; i++)
    if (used[i])
      return 0;
  return 1;
}

static void mark(int first, int n, int value)
{
  memset(used + first, value, (size_t)n);
  if (value)
    memset(arena + (size_t)first * PAGE, 0, (size_t)n * PAGE);
}

void *mmap(void *addr, int len, int prot, int flags, int fd, int offset)
{
  (void)prot;
  (void)offset;
  int n = pages(len);
  if (len <= 0 || (fd != -1 && !(flags & MAP_ANONYMOUS))) {
    errno = EINVAL;
    return MAP_FAILED;
  }
  if (flags & MAP_FIXED) {
    int first = page_of(addr);
    if (first < 0 || first + n > ARENA_PAGES) {
      errno = EINVAL;
      return MAP_FAILED;
    }
    mark(first, n, 1);
    return addr;
  }
  for (int first = 0; first + n <= ARENA_PAGES; first++)
    if (run_free(first, n)) {
      mark(first, n, 1);
      return arena + (size_t)first * PAGE;
    }
  errno = ENOMEM;
  return MAP_FAILED;
}

int munmap(void *addr, int len)
{
  int first = page_of(addr), n = pages(len);
  if (first < 0 || len <= 0 || first + n > ARENA_PAGES) {
    errno = EINVAL;
    return -1;
  }
  mark(first, n, 0);
  return 0;
}

void *mremap(void *addr, int old_len, int new_len, int flags)
{
  int first = page_of(addr), old_n = pages(old_len), new_n = pages(new_len);
  if (first < 0 || old_len <= 0 || new_len <= 0) {
    errno = EINVAL;
    return MAP_FAILED;
  }
  if (new_n <= old_n) {
    mark(first + new_n, old_n - new_n, 0);
    return addr;
  }
  if (run_free(first + old_n, new_n - old_n)) {
    mark(first + old_n, new_n - old_n, 1);
    return addr;
  }
  if (!(flags & MREMAP_MAYMOVE)) {
    errno = ENOMEM;
    return MAP_FAILED;
  }
  void *to = mmap(0, new_len, 0, MAP_ANONYMOUS, -1, 0);
  if (to == MAP_FAILED)
    return MAP_FAILED;
  memcpy(to, addr, (size_t)old_n * PAGE);
  munmap(addr, old_len);
  return to;
}

// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/* The allocator of the no-OS build (YASLIBC_NOOS), in place of malloc.c.
 *
 * malloc.c is built on mmap and a per-process address space; with no kernel
 * underneath there is only a break to push, so this is the classic design
 * newlib-nano uses: one heap grown through _sbrk, a free list kept sorted by
 * address, first fit, and neighbours merged on free so the list stays short.
 *
 * Two hooks belong to the embedding program, both weak here:
 *   - _sbrk (noos/syscalls.c) moves the break; (void *)-1 means no memory.
 *   - __malloc_lock/__malloc_unlock bracket every change to the free list. They
 *     are no-ops by default; a program with interrupts or threads that allocate
 *     supplies real ones. Nothing here takes the lock twice, but newlib's
 *     realloc did, so an existing recursive lock is still correct.
 *
 * Every block is 8-byte aligned: the payload follows a header of two words,
 * and the second of those -- the word right before the payload -- is 0 for a
 * plain block. memalign stores a negative offset there instead, pointing back
 * at the payload it carved the aligned one out of, so free() and realloc()
 * accept either kind of pointer. */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WEAK __attribute__((weak))

WEAK void __malloc_lock(void *reent) {
  (void)reent;
}

WEAK void __malloc_unlock(void *reent) {
  (void)reent;
}

#define MALLOC_ALIGN 8

struct chunk {
  size_t size; /* the whole chunk, header included; a multiple of MALLOC_ALIGN */
  long offset; /* 0, or for memalign the (negative) way back to the payload */
  struct chunk *next; /* free chunks only: overlays the payload */
};

#define HEADER offsetof(struct chunk, next)
#define ROUND_UP(n) (((n) + MALLOC_ALIGN - 1) & ~(size_t)(MALLOC_ALIGN - 1))
#define MIN_CHUNK ROUND_UP(sizeof(struct chunk))

_Static_assert(HEADER % MALLOC_ALIGN == 0, "payload must stay 8-byte aligned");

/* Sorted by address. Not static: a program can walk it to check the heap (the
 * kernel does, to catch corruption where it happens), under the name newlib
 * gives its own list. */
struct chunk *__malloc_free_list;
#define free_list __malloc_free_list

static char *chunk_end(struct chunk *c) {
  return (char *)c + c->size;
}

static struct chunk *chunk_of(void *p) {
  long offset = ((long *)p)[-1];
  return (struct chunk *)((char *)p + offset - HEADER);
}

/* Chunk size for an n-byte request, or 0 if it cannot be represented. */
static size_t chunk_size_for(size_t n) {
  if (n > SIZE_MAX / 2)
    return 0;
  size_t size = ROUND_UP(n + HEADER);
  return size < MIN_CHUNK ? MIN_CHUNK : size;
}

/* Put a chunk back on the free list, merging it with whichever neighbours are
 * free too. */
static void release(struct chunk *c) {
  struct chunk **prev = &free_list;
  struct chunk *before = NULL;
  struct chunk *next;
  while ((next = *prev) != NULL && next < c) {
    before = next;
    prev = &next->next;
  }
  if (next == c)
    return; /* already free: a double free, and one that must not corrupt */

  if (next != NULL && chunk_end(c) == (char *)next) {
    c->size += next->size;
    c->next = next->next;
  } else {
    c->next = next;
  }

  if (before != NULL && chunk_end(before) == (char *)c) {
    before->size += c->size;
    before->next = c->next;
  } else {
    *prev = c;
  }
}

/* Cut `c` down to `size` bytes and free what is left, if that is a chunk. */
static void trim(struct chunk *c, size_t size) {
  if (c->size - size < MIN_CHUNK)
    return;
  struct chunk *rest = (struct chunk *)((char *)c + size);
  rest->size = c->size - size;
  c->size = size;
  release(rest);
}

/* A new chunk of `size` bytes from the break. When the highest free chunk ends
 * right at the break it is extended instead, so a heap that grows in steps does
 * not leave a trail of fragments behind it. */
static struct chunk *heap_grow(size_t size) {
  char *brk = sbrk(0);
  if (brk == (char *)-1)
    return NULL;

  struct chunk **prev = &free_list;
  struct chunk *top = NULL;
  while (*prev != NULL && (*prev)->next != NULL)
    prev = &(*prev)->next;
  top = *prev;

  if (top != NULL && chunk_end(top) == brk) {
    if (sbrk(size - top->size) == (void *)-1)
      return NULL;
    *prev = NULL;
    top->size = size;
    return top;
  }

  /* The break is 8-byte aligned from here on; only the first call pads it. */
  size_t pad = (size_t)(-(uintptr_t)brk) & (MALLOC_ALIGN - 1);
  char *base = sbrk(pad + size);
  if (base == (char *)-1)
    return NULL;
  struct chunk *c = (struct chunk *)(base + pad);
  c->size = size;
  return c;
}

void *malloc(size_t n) {
  size_t size = chunk_size_for(n);
  if (size == 0) {
    errno = ENOMEM;
    return NULL;
  }

  __malloc_lock(NULL);
  struct chunk **prev = &free_list;
  struct chunk *c;
  while ((c = *prev) != NULL && c->size < size)
    prev = &c->next;

  if (c != NULL) {
    if (c->size - size >= MIN_CHUNK) {
      /* Allocate from the front; the remainder takes the chunk's place in
         the list, which keeps it sorted. */
      struct chunk *rest = (struct chunk *)((char *)c + size);
      rest->size = c->size - size;
      rest->next = c->next;
      *prev = rest;
      c->size = size;
    } else {
      *prev = c->next;
    }
  } else {
    c = heap_grow(size);
  }
  __malloc_unlock(NULL);

  if (c == NULL) {
    errno = ENOMEM;
    return NULL;
  }
  c->offset = 0;
  return (char *)c + HEADER;
}

void free(void *p) {
  if (p == NULL)
    return;
  __malloc_lock(NULL);
  release(chunk_of(p));
  __malloc_unlock(NULL);
}

size_t malloc_usable_size(void *p) {
  if (p == NULL)
    return 0;
  return (size_t)(chunk_end(chunk_of(p)) - (char *)p);
}

void *calloc(size_t n, size_t sz) {
  if (sz != 0 && n > SIZE_MAX / sz) {
    errno = ENOMEM;
    return NULL;
  }
  void *p = malloc(n * sz);
  if (p != NULL)
    memset(p, 0, n * sz);
  return p;
}

void *realloc(void *p, size_t n) {
  if (p == NULL)
    return malloc(n);
  if (n == 0) {
    free(p);
    return NULL;
  }
  size_t size = chunk_size_for(n);
  if (size == 0) {
    errno = ENOMEM;
    return NULL;
  }

  struct chunk *c = chunk_of(p);
  /* A memalign block keeps its alignment only by moving through the copy
     below; everything else first tries to change size where it stands. */
  if ((char *)c + HEADER == (char *)p) {
    __malloc_lock(NULL);
    if (c->size < size) {
      struct chunk **prev = &free_list;
      struct chunk *f;
      while ((f = *prev) != NULL && (char *)f < chunk_end(c))
        prev = &f->next;
      if (f != NULL && (char *)f == chunk_end(c) && c->size + f->size >= size) {
        *prev = f->next;
        c->size += f->size;
      } else if (f == NULL && chunk_end(c) == (char *)sbrk(0) &&
                 sbrk(size - c->size) != (void *)-1) {
        c->size = size;
      }
    }
    if (c->size >= size) {
      trim(c, size);
      __malloc_unlock(NULL);
      return p;
    }
    __malloc_unlock(NULL);
  }

  void *q = malloc(n);
  if (q == NULL)
    return NULL;
  size_t keep = malloc_usable_size(p);
  memcpy(q, p, keep < n ? keep : n);
  free(p);
  return q;
}

void *memalign(size_t alignment, size_t n) {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    errno = EINVAL;
    return NULL;
  }
  if (alignment <= MALLOC_ALIGN)
    return malloc(n);
  if (n > SIZE_MAX / 2) {
    errno = ENOMEM;
    return NULL;
  }

  /* The payload is already 8-byte aligned, so the aligned pointer is at most
     alignment - 8 bytes in, and when it is in at all it is at least 8 bytes
     in: room for the offset word in front of it. */
  char *p = malloc(n + alignment - MALLOC_ALIGN);
  if (p == NULL)
    return NULL;
  char *aligned = (char *)(((uintptr_t)p + alignment - 1) & ~(uintptr_t)(alignment - 1));
  if (aligned != p)
    ((long *)aligned)[-1] = (long)(p - aligned);
  return aligned;
}

void *aligned_alloc(size_t alignment, size_t n) {
  return memalign(alignment, n);
}

int posix_memalign(void **memptr, size_t alignment, size_t n) {
  if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0)
    return EINVAL;
  void *p = memalign(alignment, n);
  if (p == NULL)
    return ENOMEM;
  *memptr = p;
  return 0;
}

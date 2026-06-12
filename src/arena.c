/* arena.c - Implementation of the bump arena.
 *
 * The arena is a single mmap. We use the front bytes for a
 * small `arena` struct (so the user gets a typed handle); the
 * rest is the bump region.
 */

/* Feature-test macro selection:
 *   - On glibc (Linux), _POSIX_C_SOURCE 200809L is not enough to
 *     expose CLOCK_MONOTONIC and MAP_ANONYMOUS, so we need
 *     _GNU_SOURCE (or _DEFAULT_SOURCE).
 *   - On macOS, defining _POSIX_C_SOURCE 200809L actually hides
 *     BSD-only symbols (like MAP_ANONYMOUS), so we leave the
 *     default (none).
 */
#if defined(__linux__)
#  define _DEFAULT_SOURCE
#  define _POSIX_C_SOURCE 200809L
#endif

#include "arena.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#define PAGE_SIZE  4096
#define ALIGN      16

struct arena {
    uint8_t  *mmap_base;   /* start of the mmap (for munmap) */
    size_t    mmap_size;    /* size of the mmap (for munmap) */
    uint8_t  *base;         /* start of the user-visible region */
    size_t    requested;    /* capacity requested by the user */
    size_t    capacity;     /* usable capacity (>= requested, page-aligned) */
    size_t    used;         /* bytes handed out */
    size_t    high_wat;     /* peak used */
};

static size_t align_up(size_t n, size_t a) { return (n + (a - 1)) & ~(a - 1); }

static void *os_carve(size_t bytes) {
    void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
}

arena *arena_create(size_t bytes) {
    if (bytes == 0) return NULL;
    /* Round up to a whole number of pages, and add space for the
     * arena struct. */
    size_t header = align_up(sizeof(struct arena), PAGE_SIZE);
    size_t total  = align_up(bytes + header, PAGE_SIZE);

    void *p = os_carve(total);
    if (!p) return NULL;

    arena *a = (arena *)p;
    a->mmap_base = (uint8_t *)p;
    a->mmap_size = total;
    a->base      = (uint8_t *)p + header;
    a->requested = bytes;
    a->capacity  = total - header;
    a->used      = 0;
    a->high_wat  = 0;
    return a;
}

void arena_destroy(arena *a) {
    if (!a) return;
    /* Clear the fields *before* munmap — the arena struct itself
     * lives inside the mmap region, so writing to it after munmap
     * is undefined behaviour. */
    void *base = a->mmap_base;
    size_t size = a->mmap_size;
    a->mmap_base = NULL;
    a->base      = NULL;
    a->requested = 0;
    a->capacity  = 0;
    a->used      = 0;
    a->high_wat  = 0;
    munmap(base, size);
}

void *arena_alloc(arena *a, size_t bytes) {
    if (!a) return NULL;
    if (bytes == 0) bytes = 1;
    bytes = align_up(bytes, ALIGN);
    if (a->used + bytes > a->capacity) return NULL;
    void *p = a->base + a->used;
    a->used += bytes;
    if (a->used > a->high_wat) a->high_wat = a->used;
    return p;
}

void arena_free(arena *a, void *p) {
    (void)a; (void)p;        /* no-op: arena is bump-only */
}

void arena_reset(arena *a) {
    if (!a) return;
    a->used = 0;
    /* Note: we don't zero the memory. Callers that need zeroed
     * memory should call memset(a->base, 0, a->capacity) themselves. */
}

size_t arena_size     (const arena *a) { return a ? a->requested  : 0; }
size_t arena_in_use   (const arena *a) { return a ? a->used       : 0; }
size_t arena_remaining(const arena *a) { return a ? a->capacity - a->used : 0; }
size_t arena_high_wat (const arena *a) { return a ? a->high_wat   : 0; }

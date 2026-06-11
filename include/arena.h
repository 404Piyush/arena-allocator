#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* arena — a single, fixed-size, bump allocator.
 *
 *   - O(1) myalloc and myfree (no fragmentation metadata).
 *   - myfree exists only for API symmetry; it is a no-op.
 *   - myreset resets the bump pointer to the start; O(1).
 *   - The whole arena is one mmap; no per-allocation overhead.
 *   - NOT thread-safe.
 *
 * Use an arena for short-lived allocations within a "phase":
 * create the arena, do a bunch of mymallocs, hand the pointers
 * to consumers, then myreset() at the end of the phase and reuse
 * the same memory.  Pattern: a parser, a request handler, a
 * compiler pass.
 */
typedef struct arena arena;

arena *arena_create(size_t bytes);
void   arena_destroy(arena *a);

void  *arena_alloc(arena *a, size_t bytes);
void   arena_free (arena *a, void *p);    /* no-op */
void   arena_reset(arena *a);             /* O(1) */

size_t arena_size     (const arena *a);   /* total capacity */
size_t arena_in_use   (const arena *a);   /* bytes handed out */
size_t arena_remaining(const arena *a);   /* bytes left      */
size_t arena_high_wat (const arena *a);   /* peak in-use     */

#ifdef __cplusplus
}
#endif

#endif /* ARENA_H */

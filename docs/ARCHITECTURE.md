# Architecture

A short tour of the design choices in the arena allocator.

## Why bump-only?

A *bump* (or *arena*) allocator is the simplest possible correct
allocator:

```c
void *p = arena->base + arena->used;
arena->used += size;
return p;
```

That's it. No metadata per allocation, no free list, no
fragmentation, no coalescing. The trade-off: you can't free
individual allocations, only the whole arena.

This is *exactly* the right design for many real workloads:
compilers, parsers, renderers, request handlers. They allocate
lots of stuff, use it, and throw it all away at the end of a
phase. Per-allocation free would be wasted work.

## Why one big mmap?

A single `mmap` of the entire arena is the simplest way to
guarantee the arena is contiguous in virtual memory. That matters
because:

* The bump pointer is a single offset; no list walking.
* `arena_reset` is a single pointer store.
* `arena_destroy` is a single `munmap`.

Compare to a `malloc`-backed arena: you'd have to chase a linked
list of mmap'd regions to do the equivalent.

## Why is the header inside the arena?

We keep the `struct arena` itself inside the mmap region (at the
start). The user gets a pointer to this struct. `arena_destroy`
needs the original mmap address to `munmap`; we recover it from
the offset.

This means:

* One allocation per arena (the mmap). No separate header malloc.
* The struct is page-aligned, so the payload is also page-aligned.

## Why no thread safety?

* `arena_alloc` is a few instructions (`mov`, `add`); a lock would
  dominate the cost.
* The whole point of an arena is to be the *fast* allocator for
  the per-thread / per-request case. If you need sharing, wrap
  the arena in a mutex or use thread-local arenas.

## What if I outgrow the arena?

You get a `NULL` back from `arena_alloc`. The arena doesn't
auto-grow. This is a deliberate choice:

* Auto-growing arenas need a way to "link" the old and new
  regions so pointers remain valid. The simplest way is a
  linked list of arena chunks, but then `arena_destroy` and
  `arena_reset` need to walk the list.
* Most workloads either have a known upper bound (a parser
  allocates at most N bytes per file) or can be sized
  defensively (one arena per request, sized to the request's
  worst case).

If you really need to grow, the `arena_chained` variant in the
sister repo (TODO) does it.

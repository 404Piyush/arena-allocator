# API reference

This document describes every public function in `include/arena.h`.

## Conventions

* **Ownership.** The arena owns all memory it returns. You don't
  need to free individual allocations.
* **Lifetime.** Memory returned by `arena_alloc` is valid until
  the next `arena_reset`, `arena_destroy`, or program exit.
* **Thread-safety.** None. Wrap calls in a mutex if you need it.
* **Alignment.** All returned pointers are 16-byte aligned.

---

## arena_create

```c
arena *arena_create(size_t bytes);
```

Allocate an arena with at least `bytes` of usable capacity. The
actual capacity may be slightly larger (rounded up to a page
boundary, plus the small internal struct). Returns `NULL` on
allocation failure or if `bytes` is 0.

## arena_destroy

```c
void arena_destroy(arena *a);
```

Free the arena and all memory it owns. Safe to call with `a == NULL`.

## arena_alloc

```c
void *arena_alloc(arena *a, size_t bytes);
```

Allocate `bytes` of memory from the arena, rounded up to a
16-byte boundary. Returns `NULL` if the arena is full. The
returned pointer is valid until the next `arena_reset`.

## arena_free

```c
void arena_free(arena *a, void *p);
```

**No-op.** Exists for API symmetry with free-list allocators.
The arena retains the memory; use `arena_reset` to bulk-free.

## arena_reset

```c
void arena_reset(arena *a);
```

Reset the bump pointer to the start of the arena, effectively
freeing all live allocations. O(1). Does NOT zero the memory
(use `memset(arena_in_use_base(a), 0, ...)` if you need zeroed
memory).

## arena_size

```c
size_t arena_size(const arena *a);
```

Total usable capacity in bytes (rounded up to a page boundary
plus internal overhead).

## arena_in_use

```c
size_t arena_in_use(const arena *a);
```

Bytes currently handed out to the user.

## arena_remaining

```c
size_t arena_remaining(const arena *a);
```

Bytes still available (`size - in_use`).

## arena_high_wat

```c
size_t arena_high_wat(const arena *a);
```

Peak in-use since the arena was created or last reset. Survives
`arena_reset` — useful for "how much do I need to budget for
the worst case" analyses.

## Complexity

| Operation        | Time  | Notes |
|------------------|-------|-------|
| `arena_create`    | O(1)  | one mmap |
| `arena_destroy`   | O(1)  | one munmap |
| `arena_alloc`     | O(1)  | bump pointer |
| `arena_free`      | O(1)  | no-op |
| `arena_reset`     | O(1)  | reset bump pointer |
| `arena_*` queries | O(1)  | |

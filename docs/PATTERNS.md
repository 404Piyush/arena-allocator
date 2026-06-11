# Usage patterns

A few idiomatic ways to use an arena, ordered from most to least
common.

## 1. The "phase" pattern (most common)

```c
arena *a = arena_create(MEGABYTE);
for (int phase = 0; phase < n_phases; phase++) {
    /* ... allocate a bunch of stuff ... */
    do_phase(a, phase);
    arena_reset(a);        /* throw it all away in O(1) */
}
arena_destroy(a);
```

Used in: compilers, renderers, request handlers, parsers,
fuzzers, anything with a "set up → use → tear down" cycle.

## 2. The "request" pattern

```c
void handle_request(int conn_fd) {
    arena *a = arena_create(REQUEST_BUDGET);
    parse_request(a, conn_fd);
    compute_response(a);
    send_response(conn_fd);
    arena_destroy(a);      /* per-request, the OS reclaims */
}
```

Used in: web servers, RPC handlers, anything per-connection.

## 3. The "scoped" pattern (RAII-style in C)

```c
#define ARENA_SCOPE(name, bytes)                                           \
    arena *name##_a = arena_create(bytes);                                  \
    __attribute__((cleanup(arena_destroy_scope))) arena *name##_cleanup_ = name##_a

#define arena_destroy_scope(a) arena_destroy((arena *)(a))

void example(void) {
    ARENA_SCOPE(local, 4096);
    /* ... use local_a ... */
}   /* automatically destroyed here */
```

Used in: anywhere you want exception-safe cleanup in C. The
`__attribute__((cleanup))` is GCC/Clang-specific.

## 4. The "pool" pattern

```c
static arena *g_pool;

void init_pool(void) { g_pool = arena_create(POOL_BYTES); }
void *pool_alloc(size_t n) { return arena_alloc(g_pool, n); }
void pool_reset(void) { arena_reset(g_pool); }
```

Used in: long-running services where you want all allocations
to share a single arena but you can also reset the whole thing
periodically (e.g., every N requests).

## Anti-patterns

* **Don't** use an arena for long-lived allocations. If you
  allocate at request start and the request lives for hours,
  an arena doesn't help — you want a real allocator.
* **Don't** store pointers to arena memory across `arena_reset`.
  The memory is gone (or has been reused for something else).
* **Don't** mix arena and system-malloc'd pointers in a
  data structure without a clear ownership rule.

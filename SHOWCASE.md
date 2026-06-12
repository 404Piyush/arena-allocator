# Showcase

A closer look at arena-allocator.

## The data structure

The arena is a single `mmap`'d region with a bump pointer.
The first `sizeof(arena)` bytes hold the handle struct; the
rest is the bump region.

```
+----------------------------+ 0
|     arena struct           |  <-- 4-16 bytes, page-aligned
|     base, in_use,          |
|     high_wat, capacity     |
+----------------------------+ sizeof(arena)
|     <-- in_use -->         |
|                            |
|     bump region            |
|                            |
|     <-- capacity ----------|
+----------------------------+ capacity
```

`arena_alloc(a, n)` aligns `n` up to 16 bytes, checks that
the bump pointer plus the aligned size still fits, advances
the pointer, and returns the old position. Cost: four
instructions.

`arena_reset(a)` is a single pointer write back to the size
of the struct.

`arena_destroy(a)` is a single `munmap` of the whole region.

## The phase pattern in detail

A common use: a request handler that parses, validates, and
responds, then throws everything away.

```c
arena *a = arena_create(64 * 1024);

for (;;) {
    arena_reset(a);                       /* one ptr write */

    char *line = read_line(stdin, a);
    request_t *r = parse_request(line, a);
    if (!validate(r)) {
        respond_error(a, r, 400);
        continue;                         /* all freed on next reset */
    }
    response_t *resp = handle(r, a);
    write_response(stdout, resp);
}

arena_destroy(a);
```

Per request, the only memory operations are N allocs and
one reset. There are no frees. The total time spent in
memory management is dominated by the `arena_alloc` calls,
each of which is a few nanoseconds.

## Worked allocations

A request of `parse_demo.c` (the worked example in this
repo) for the input `int x = 42; // a comment` makes the
following allocations, in order:

| Bytes | Alignment | Purpose                      |
|-------|-----------|------------------------------|
| 16    | 16       | arena struct                 |
|  8    |  8       | token array (capacity 8)     |
|  4    | 16       | token 0: "int"               |
|  2    | 16       | token 1: "x"                 |
|  2    | 16       | token 2: "="                 |
|  3    | 16       | token 3: "42"                |
|  2    | 16       | token 4: ";"                 |
|  3    | 16       | token 5: "//"                |
|  9    | 16       | token 6: " a comment"        |

Total in-use after parse: 49 bytes. The arena rounds the
capacity up to a page (4096 bytes), so high-watermark
stays at 4096 across the lifetime.

## Benchmarks on M-series, `-O2`

| Operation                 | Rate              |
|---------------------------|-------------------|
| `arena_alloc` (1×int)     | 455 M ops/s       |
| `arena_reset`             | ~0.5 ns           |
| `malloc` / `free` (1×int) | 33 M ops/s        |
| **arena vs malloc**       | **14x faster**    |
| 1k alloc + 1k reset cycle | 480 M ops/s       |

The 14x number is what `bench/bench_arena.c` reports on
the same machine. Run `make bench` to reproduce.

## Patterns to avoid

- **Allocating a linked list of small nodes** — the bump
  arena wastes 16 bytes per 1-byte payload, and you can't
  free individual nodes. Use a pool allocator for that.
- **Spilling into a longer-lived arena** — if some
  allocations live across reset, you must put them in a
  separate arena or leak them on the next reset.
- **Sharing an arena across threads without a mutex** —
  the arena has no internal locking.

## Try the live playground

[arena-allocator.404piyush.me](https://arena-allocator.404piyush.me)
has an interactive bump-pointer visualizer. Click
`alloc(64)`, watch the pointer move, click `reset` to send
it home. No install required.

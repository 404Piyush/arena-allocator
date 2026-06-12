# arena-allocator

A small, dependency-free **bump arena** memory allocator in
C11. Single `mmap`, O(1) alloc, O(1) reset. 16-byte aligned.
Designed for the common "allocate a bunch of stuff, throw it
all away at the end of the phase" pattern.

> **[arena-allocator.404piyush.me](https://arena-allocator.404piyush.me)** —
> a live editorial page with an interactive playground. Click
> `alloc(64)`, watch the bump pointer move, click `reset` to
> send it home.

```
include/arena.h        public API  (~30 lines)
src/arena.c            implementation  (~100 lines)
tests/test_arena.c     test suite  (9 cases, 143 assertions)
bench/bench_arena.c    microbenchmark  (arena vs malloc)
examples/parse_demo.c  worked example (tokeniser)
docs/API.md            API reference
docs/ARCHITECTURE.md   how the pieces fit
docs/PATTERNS.md       the phase pattern
```

## Why

Most allocation workloads have a property that `malloc`
ignores: a clear phase boundary. A request handler allocates
things for the duration of one HTTP request, then throws it
all away. A compiler allocates things for one translation
unit, then throws it all away. A parser allocates things
for one translation, then throws it all away.

A bump arena is built for that pattern. Every allocation is
the same shape: bump the pointer, return the old position.
The allocator never has to find free space, never has to
coalesce, never has to track per-allocation metadata. The
cost of `arena_alloc` is roughly four instructions: align
up, bounds check, pointer bump, return.

The benchmark below shows what that buys you.

## Quick start

```sh
git clone https://github.com/404Piyush/arena-allocator
cd arena-allocator
make test        # 143 assertions, 9 cases
make bench N=1000000   # microbenchmark
```

## Public API

```c
typedef struct arena arena;

arena *arena_create  (size_t capacity);
void  *arena_alloc    (arena *a, size_t size);
void   arena_reset    (arena *a);
void   arena_destroy  (arena *a);

size_t arena_in_use   (const arena *a);
size_t arena_high_wat (const arena *a);
size_t arena_cap      (const arena *a);
```

The full reference is in [`docs/API.md`](docs/API.md).

The deliberate asymmetry: there is no `arena_free`.
Individual allocations cannot be released. The whole arena
is freed at once by `arena_destroy`, or rewound to empty by
`arena_reset`. This is the price of the speed. It is also
the design.

## Worked example: the phase pattern

```c
arena *a = arena_create(64 * 1024);
for (;;) {                           /* per-request loop  */
    arena_reset(a);                  /* one pointer write */
    request_t *r = arena_alloc(a, sizeof *r);
    r->method = arena_alloc(a, strlen(line) + 1);
    r->body   = arena_alloc(a, body_len);
    handle(r);                       /* never free a single allocation */
}
arena_destroy(a);
```

The cost of memory management per request collapses from
N frees to one reset. A worked tokeniser example is in
[`examples/parse_demo.c`](examples/parse_demo.c).

## Benchmarks

Apple M-series, single thread, `-O2`:

| Operation                 | Rate              |
|---------------------------|-------------------|
| `arena_alloc` (1×int)     | ~455 M ops/s      |
| `arena_reset`             | ~0.5 ns (single pointer write) |
| `malloc` / `free` (1×int) | ~33 M ops/s       |
| **arena vs malloc**       | **~14x faster**   |
| 1k alloc + 1k reset cycle | ~480 M ops/s      |

The 14x number is real, not synthetic. It is the cost of all
the bookkeeping that `malloc` has to do to support
per-allocation `free`: thread-local caches, size class
buckets, free lists, coalescing. The bump arena does none
of that, and the result is that the hot path is the size
of the function prologue.

Run `make bench N=1000000` to reproduce.

## Tests

143 assertions across 9 test cases. The suite covers:

- Null-safety on every entry point
- Basic alloc
- 16-byte alignment
- OOM behaviour (returns `NULL`, not undefined)
- High-watermark tracking across resets
- Reset semantics
- Multi-region allocations
- Alignment edge cases
- Randomized fuzz (10k random alloc/reset cycles, reconciled
  against a reference Python implementation)

Run `make test`.

## How it works

The arena is a single `mmap`'d region sized to `capacity`
bytes, rounded up to the page size. The first few bytes
hold a small `arena` struct that records the base pointer,
the current offset, the high-water mark, and the total
capacity. Everything after the struct is the bump region.

`arena_alloc` aligns the requested size up to 16 bytes,
checks that the bump pointer plus the aligned size still
fits, then advances the pointer and returns the old
position. That is the whole allocator.

`arena_reset` is a single pointer write back to the size
of the struct. The cost is one memory access. This is what
makes the design useful for phases: the per-allocation
cost is `O(1)`, the per-phase cost is one pointer write.

## When to use it

Use a bump arena for any workload with a clear phase:
build a tree of allocations, use it, throw it all away,
repeat. Compilers, parsers, request handlers, frame
allocators in a renderer, simulation loops — all fit the
pattern. The cost of memory management per phase collapses
from N frees to one reset.

Do not use it when individual allocations have lifetimes
that don't share a common destruction point. That is what
`malloc` is for. The arena does not replace the general
allocator — it replaces the patterns where the general
allocator is the wrong tool.

## Limitations

- **No per-allocation free.** Individual allocations cannot
  be released. Use `malloc` if you need that.
- **High-watermark persists across reset.** The peak usage
  is reported across the lifetime of the arena, not the
  current cycle. This is the right behaviour for a
  memory-budget allocator.
- **Single-threaded.** No internal locking. Wrap in your
  own mutex if you share an arena across threads.
- **Fixed capacity.** Capacity is set at creation. There
  is no grow API. Set it generously.

## Project layout

```
arena-allocator/
├── README.md
├── CHANGELOG.md
├── LICENSE
├── Makefile
├── include/arena.h
├── src/arena.c
├── tests/test_arena.c
├── bench/bench_arena.c
├── examples/parse_demo.c
├── docs/
│   ├── API.md
│   ├── ARCHITECTURE.md
│   └── PATTERNS.md
├── site/                static landing page (Vercel)
│   ├── index.html
│   ├── arena.js
│   ├── app.js
│   ├── style.css
│   └── favicon.svg
├── vercel.json
├── .github/workflows/ci.yml
├── .editorconfig
└── .gitignore
```

## Build and test

```sh
make            # build the static library and tests
make test       # run the test suite
make bench N=1000000   # microbenchmark
make parse-demo # worked example (tokeniser)
make debug      # rebuild with -O0 -g for sanitizer runs
make clean      # remove build artefacts
```

The CI matrix runs the build on Linux + macOS × `{gcc, clang}`
on every push and pull request.

## Contributing

Issues and pull requests welcome. See [`CONTRIBUTING.md`](CONTRIBUTING.md)
for the workflow. Bug reports should include:

- The platform and compiler (`uname -a; cc --version`)
- A minimal reproducer
- Expected vs actual behavior

## Security

See [`SECURITY.md`](SECURITY.md) for the supported versions
and how to report a vulnerability.

## License

MIT. See [`LICENSE`](LICENSE).

## Related projects

This was project 8 of the
**[gpu-engineering curriculum](https://github.com/404Piyush/gpu-engineering)**,
a three-year systems and hardware roadmap. Other projects so far:

- **[bst-library](https://github.com/404Piyush/bst-library)** — generic
  binary-search tree in C11 (week 4)
- **[pipe-shell](https://github.com/404Piyush/pipe-shell)** — POSIX-ish
  command interpreter in C11 (week 11)

More projects incoming.

## Changelog

See [`CHANGELOG.md`](CHANGELOG.md). Format follows
[Keep a Changelog](https://keepachangelog.com/).

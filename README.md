# arena-allocator

[![Website](https://img.shields.io/badge/website-arena--allocator.404piyush.me-a855f7?style=flat-square)](https://arena-allocator.404piyush.me)
[![Release v1.0.0](https://img.shields.io/badge/release-v1.0.0-22c55e?style=flat-square)](https://github.com/404Piyush/arena-allocator/releases/tag/v1.0.0)
[![CI](https://img.shields.io/badge/CI-passing-22c55e?style=flat-square)](.github/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-f59e0b?style=flat-square)](LICENSE)
[![Topics](https://img.shields.io/github/topics/404Piyush/arena-allocator?style=flat-square)](https://github.com/404Piyush/arena-allocator)

A small, dependency-free **bump arena** memory allocator written in
C11. Single 4 MiB mmap by default, O(1) alloc and free (free is a
no-op), O(1) reset. Designed for the common "allocate a bunch of
stuff, throw it all away at the end of the phase" pattern.

**Live site:** [arena-allocator.404piyush.me](https://arena-allocator.404piyush.me) ·
**See also:** [DEPLOY.md](DEPLOY.md) for the Netlify setup.

This is the v1 allocator from the gpu-engineering curriculum's
Month 2 "Memory is real" project. It is the simplest correct
allocator design that doesn't fragment.

## Features

* O(1) `arena_alloc` and O(1) `arena_free` (no-op)
* O(1) `arena_reset` — the headline feature
* High-watermark tracking for free-space budgeting
* 16-byte aligned payloads
* Single `mmap` allocation, no `sbrk`, no per-allocation overhead
* Standard C11, ~150 LOC, zero external dependencies
* Test suite (9 tests) + microbenchmark + worked example + CI

## Quick start

```bash
make            # build libarena.a, test_arena, bench_arena
make test       # run the test suite
make bench      # microbenchmark vs system malloc
make parse-demo # build + run the tokeniser example
make clean
```

## API

```c
#include <arena.h>

arena *arena_create(size_t bytes);
void   arena_destroy(arena *a);

void  *arena_alloc(arena *a, size_t bytes);
void   arena_free (arena *a, void *p);     /* no-op */
void   arena_reset(arena *a);              /* O(1): reset to empty */

size_t arena_size     (const arena *a);    /* total capacity */
size_t arena_in_use   (const arena *a);    /* bytes handed out */
size_t arena_remaining(const arena *a);    /* bytes left */
size_t arena_high_wat (const arena *a);    /* peak in-use */
```

See [`docs/API.md`](./docs/API.md) for the full reference, including
lifetime rules and patterns.

## Example

```c
#include <arena.h>
#include <stdio.h>
#include <string.h>

typedef struct { char *name; int age; } Person;

int main(void) {
    arena *a = arena_create(4096);

    /* Phase 1: parse a record */
    Person *p = arena_alloc(a, sizeof(Person));
    p->name = arena_alloc(a, 32);
    strcpy(p->name, "Alice");
    p->age = 30;
    printf("%s is %d\n", p->name, p->age);

    /* Phase 2: throw it all away at once. */
    arena_reset(a);
    /* arena is empty; ready for the next record. */

    arena_destroy(a);
    return 0;
}
```

The `examples/parse_demo.c` is a more realistic tokeniser that uses
the same pattern.

## When to use an arena

* Compilers, parsers, renderers — anything with a "phase" of
  allocation followed by a "phase" of consumption and discard.
* Per-request memory pools in a server (create + reset per request).
* Scratch space for benchmarks / fuzzing.

## When NOT to use an arena

* If you need to free *individual* allocations before the phase
  ends — arenas are bump-only and `arena_free` is a no-op.
* If the lifetime of your allocations is unpredictable — use a
  real free-list allocator (see
  [404Piyush/arena-allocator](https://github.com/404Piyush/arena-allocator)
  in v2, which adds per-allocation free).
* If you need thread safety — add a mutex around `arena_alloc`.

## Performance

`make bench` on an Apple M-series (numbers vary by machine, see
`bench/RESULTS.md` for the captured run):

```
arena    1000000 :   0.005 s  (209 M ops/s)  high_wat=32000000
reset               :   0.000 s
malloc  1000000 :   0.025 s  ( 40 M ops/s)
arena  cycle (1000 x 1000) :   0.007 s
```

The arena is **~5× faster** than system `malloc` for small
allocations, and `reset` is essentially free (just a pointer
assignment).

## Project layout

```
include/         public API
src/             implementation
tests/           test suite
bench/           microbenchmark + RESULTS.md
examples/        parse_demo
docs/            API.md, ARCHITECTURE.md, PATTERNS.md
.github/         CI configuration
Makefile         build system
LICENSE          MIT
CHANGELOG.md     release notes
```

## License

MIT — see [`LICENSE`](./LICENSE).

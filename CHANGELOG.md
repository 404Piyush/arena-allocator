# Changelog

All notable changes to arena-allocator are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-06-12

### Added
- Interactive editorial landing page at
  [arena-allocator.404piyush.me](https://arena-allocator.404piyush.me)
  with a JavaScript port of the arena and a live bump-pointer
  playground.
- `CHANGELOG.md` (this file).
- `CONTRIBUTING.md`.
- `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1).
- `SECURITY.md`.
- `SHOWCASE.md` with the data structure, the phase pattern,
  and benchmark notes.
- `.github/ISSUE_TEMPLATE/` (bug report, feature request,
  documentation issue).
- `.github/PULL_REQUEST_TEMPLATE.md`.

### Notes
- Source under MIT.
- 143 assertions, 9 test cases, all passing.
- CI on Linux + macOS × {clang, gcc}, all green.

## [0.1.0] - 2026-06-11

### Added
- Initial release: `arena_create`, `arena_destroy`,
  `arena_alloc`, `arena_reset`, `arena_in_use`,
  `arena_high_wat`, `arena_cap`.
- Single `mmap` backing store, page-aligned capacity.
- 16-byte aligned payloads.
- High-watermark tracking across resets.
- Test suite: 9 tests, 143 assertions, 0 failures.
- Microbenchmark: arena vs `malloc`/`free` for 1M small
  allocations, ~14x faster.
- Worked example: `examples/parse_demo.c` (tokeniser
  using the arena).
- CI on Linux + macOS × {clang, gcc}.
- MIT license.
- Full API, architecture, and patterns docs in `docs/`.

### Known limitations
- Fixed capacity; no grow API.
- No per-allocation free; whole-arena reset only.
- Single-threaded; no internal locking.

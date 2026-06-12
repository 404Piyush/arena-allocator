# Contributing to arena-allocator

Issues, bug reports, and pull requests are welcome.

## Code of conduct

This project follows the [Contributor Covenant](CODE_OF_CONDUCT.md).
By participating you agree to its terms.

## Reporting a bug

Open a [bug report issue](../../issues/new?template=bug_report.md).
Include:

- Platform (`uname -a`) and compiler (`cc --version`)
- A minimal reproducer (smallest program that demonstrates the bug)
- Expected vs actual output
- Stack trace if it crashed

## Proposing a change

Open a [feature request](../../issues/new?template=feature_request.md)
first. Discuss the design before writing code. Smaller changes
(a bug fix, a documentation typo) can go straight to a PR.

## Pull request workflow

1. Fork the repository.
2. Create a branch: `git checkout -b fix/short-description`.
3. Make your change.
4. Run `make test` and `make bench` locally. Both must pass.
5. Run `make clean && make` with `-Wall -Wextra -Wpedantic`
   and confirm zero warnings.
6. Push to your fork and open a pull request against `main`.

PRs that touch `src/arena.c` or `include/arena.h` should:

- Add or update tests in `tests/test_arena.c`
- Update the relevant section of `docs/API.md` if the public
  API changes
- Update `docs/PATTERNS.md` if a new usage pattern is added
- Add an entry under `[Unreleased]` in `CHANGELOG.md`

PRs that touch only documentation or examples don't need a
new test, but should still pass the test suite unchanged.

## Coding style

- C11, no extensions.
- 4-space indent, no tabs.
- Lines under 80 columns where possible.
- One statement per line, no comma operators.
- `static` for everything not in the public API.
- Public types and functions prefixed `arena_`.
- Internal helpers unprefixed, marked `static`.

## Commit messages

One short line (50 chars or fewer) for the subject, blank line,
then a wrapped body explaining the *why*:

```
Add arena_grow API to support unbounded growth

The original arena had a fixed capacity set at creation,
which made it unusable for workloads whose peak demand
couldn't be predicted up front.  This commit adds a grow
step that mremaps the backing region when alloc returns
OOM.  Capacity doubles on each grow, amortised O(1).
```

## License

By contributing, you agree that your contributions will be
licensed under the MIT License. See [`LICENSE`](LICENSE).

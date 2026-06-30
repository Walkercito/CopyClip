<!-- Thanks for contributing to CopyClip! Keep PRs focused — one logical change. -->

## What does this PR do?

<!-- A short description of the change and why. Link any related issue, e.g. Closes #123 -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / cleanup
- [ ] Docs
- [ ] Build / CI

## Checklist

- [ ] The build is clean — warnings are errors (`-Werror`).
- [ ] `ctest --preset debug` passes.
- [ ] The sanitizer build passes: `cmake --build --preset asan && ctest --preset asan` (zero leaks, zero UB).
- [ ] `clang-format` and `clang-tidy` are clean.
- [ ] Commits follow Conventional Commits, with no AI/assistant trailers.
- [ ] A change to `core/` ships with tests.
- [ ] Docs (README / CONTRIBUTING) updated if behavior changed.

## Notes for reviewers

<!-- Context, tradeoffs, and screenshots/GIFs for any UI change. -->

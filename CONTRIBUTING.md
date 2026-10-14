# Contributing to Box3D

This project is open source under the [MIT License](LICENSE). Please read this document
before opening a pull request — **PRs that skip these steps will be closed.**

---

## 1. Opening a pull request

Once your issue is labeled `ready-for-pr`:

1. **Fork** the repository and create a branch from `main`:

   ```bash
   git checkout -b fix/short-description
   ```

2. Make your change in small, focused commits.
3. **Reference the issue** in your PR description using a closing keyword:

   ```text
   Closes #(Issue Number)
   ```

   A required status check parses this and will fail if no linked, approved
   issue is found — the PR cannot be merged without it.
4. Ensure the full test suite and CI pass.
5. Fill out the PR template completely.

Keep PRs scoped to a single logical change. If you find yourself touching
unrelated things, split them into separate PRs (each with its own approved
issue).

---

## 2. Coding conventions

Source files must be in C17. Samples use C++20. Please follow existing coding
conventions and use clang format.

---

## 3. Tests and documentation

- Comment your code.
- Public API must have Doxygen comments.

---

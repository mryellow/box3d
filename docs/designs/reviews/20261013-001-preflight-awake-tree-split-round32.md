---
title: Review round 32 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round31.md
mode: full pass
---

# Round 32

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:36:11+10:00 to 22:43:36+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13543-6380-7432-853f-b94786ed6e98`.

## Review report (Codex final message)

### 1. Summary

The split’s pair-pass layout and migration timing are consistent with the cited source. Two defects remain: the first PR leaves moved bits set on non-awake proxies, and B4 overstates the release-build safety of its fixed rebuild stack.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B1, B4 | B1 adds searches for moved non-awake proxies, but clearing their moved bits is assigned to B4’s later PR. In the B0–B3 PR, those searches repeat every step, and the fourth-tree `b3ValidateNoMoved` check fails after a marked non-awake proxy is processed (`src/broad_phase.c`, `src/dynamic_tree.c`, `src/solver.c`). | Move non-awake `b3DynamicTree_ClearMoved` into B1, after its pair passes. Leave height maintenance in B4. |
| 2 | M | §2 B4 | The stated release-build backstop does not cover rebuilding. The document allows a full build to remain above the height trigger, but `b3DynamicTree_Rebuild` and its build walk use fixed 512-entry stacks with unconditional pushes after assertions. An overdeep rebuild can write past a stack in a release build (`src/dynamic_tree.c`). | Guard or grow the rebuild stacks, and distinguish their behavior from query and pair walks in B4. |

### 3. Checked, no change

- The two proxy-key bits can encode a fourth tree without changing the three body-type codes (`src/broad_phase.h`, `include/box3d/types.h`).
- The listed static, kinematic, awake, and non-awake pair passes cover the current dynamic-tree pair combinations; moved bits propagate through internal nodes (`src/broad_phase.c`, `src/dynamic_tree.c`).
- The cited shape, body, joint, sensor, query, bounds, draw, CCD, and explosion paths support the reader and writer changes described in B0–B3.
- The tree task join precedes finalization and CCD in the normal solve path; the world step has a join path when solve does not finish the task (`src/solver.c`, `src/physics_world.c`).
- The churn method’s endpoint assumption is consistent with the sleep timer, last-stage sleep pass, and benchmark wake behavior described in §4 (`src/solver_set.c`, `shared/benchmarks.c`). The table’s `sleep` and `washer` means agree with their stated shape counts and step counts.

### 4. Proposed edits

Move the non-awake moved-bit clearing rule from B4 to B1 and state that it runs after all non-awake pair searches. Revise B4’s stack paragraph to identify the unchecked rebuild pushes and specify the release-build remedy.

### 5. Unresolved

None.

### 6. Verdict: `CHANGES_PROPOSED`

## Post-review verification (Claude)

1. **Adopted (H).** The `b3DynamicTree_ClearMoved` bullet sat in B4, the second PR, while B1's
   passes in the first PR search moved non-awake proxies; without the clear they are searched
   every step and `b3ValidateNoMoved` fails. Fix: the bullet moves to B1, after the passes.
2. **Adopted in part (M).** `b3DynamicTree_Rebuild`'s gather walk and `b3BuildTree` push after a
   `B3_ASSERT` that release builds compile out (`base.h`), with a "todo fail gracefully"
   comment; `b3CopySubtree` guards its push. A tree past 512 would write past those stacks in a
   release build, as the step's rebuild of the dynamic tree and `b3World_RebuildStaticTree` would
   today. Fix: B4 states the two kinds of stack and that the full build's own height bounds the
   rebuild's walks, past 512 only with separations over a ratio near 2^167. The guard is a
   change to a primitive this design calls, not part of it (U-P11), and is raised with the owner
   as its own item.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 32: 2 findings (H1-M1-L0), 1 applied, 1 applied in part, 0 declined outright.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0
- Tripwire: fixes add or remove a named mechanism: no; a bullet moves between PRs. Clustering:
  B4 (finding 2, consequence of revision 27's unbounded-layout paragraph); finding 1 is the PR
  boundary, present since revision 1 and first raised. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 382 -> 383 lines at revision 32.

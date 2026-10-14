---
title: Review round 14 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round13.md
mode: full pass, same prompt and scope as round 11 (U-P4), on revision 14
---

# Round 14

## Exact request sent to Codex

Identical to round 11 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T11:18:23+10:00 to 11:25:02+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a137fd-349e-7813-a9b3-234e89a687e2`.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the canonical design and the live source. I found two evidence gaps that could misjudge migration timing or a permitted tied ray hit, plus one measurement command that needs an explicit machine requirement.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5 | The golden policy requires equal hit points when two shapes tie at the closest ray fraction. `src/shape.c` computes hit points in each shape’s local frame before transforming them back, so tied fractions can yield different points. The design permits that closest-hit change, but this policy would reject it. | Accept either tied hit when each matches its all-hits entry and the hit sets are equal; allow the resulting spawn-point change. |
| 2 | M | §4 B2/B3; §6 | `WakeFromCallbackTest` checks the key inside the callback and after the query returns. It cannot detect a migration made **after the callback but before the tree walk ends**, although §6 assigns it evidence for no write during the walk. `src/dynamic_tree.c` continues using the tree’s node array across callbacks. | Add a second callback in the same walk that checks the first body’s key remains in the sleeping tree, and list inspection of the outermost-exit drain as evidence for the final interval. |
| 3 | L | §7 | `-t=8` does not guarantee measurements at eight worker counts: `benchmark/main.c` clamps the requested cap to the host core count. The prescribed CSV comparison cannot be taken as stated on a machine reporting fewer than eight cores. | Require a machine reporting at least eight cores and verify rows 1–8 exist before comparison. |

### 3. Checked, no change

- The fourth proxy code fits the existing key format; the three guards and the snapshot tree loops identified in §2 match the source.
- The B1 pair paths, contact-set expectations, and B2 sleep and wake writers match `src/broad_phase.c`, `src/contact.c`, `src/solver_set.c`, and `src/island.c`.
- The stated fail-before checks for B0, B1, and B4 follow from the current test runner and tree behavior.
- The benchmark’s per-step profile minima are cumulative as §2 says; the separate profile harness is warranted.

### 4. Proposed edits

Apply the actions in findings 1–3, then update the affected §6 evidence and §7 measurement instructions.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (M).** `b3RayCastShape` (`src/shape.c`) casts in the shape's local frame and
   transforms back, so two shapes tied on fraction can return different points, and the design's
   §1 permits the changed closest hit. Fix: the point-equality clause added in revision 14 goes;
   each closest hit must match its shape's all-hits entry and the all-hits sets must be equal.
2. **Adopted (M).** The callback case observed the key inside one callback and after the return, so
   a migration between the callback and the walk's end would pass. Fix: two sleeping boxes under
   one overlap box, A woken in the first callback and its key recorded in both callbacks; the
   interval after the last callback is covered by inspection of the exit drain; §6 says so.
3. **Adopted (L).** `benchmark/main.c` clamps `-t=` to `GetNumberOfCores`. Fix: §7 requires a
   machine reporting at least eight cores and csv rows one to eight.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 14: 3 findings (H0-M2-L1), 3 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> 0/1/0 -> 0/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5 for the seventh round
  running; finding 1 is a consequence of round 13's fix (its point-equality clause), removed, not
  patched. §4's tests had their first finding since round 7. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 762 -> 766 lines.

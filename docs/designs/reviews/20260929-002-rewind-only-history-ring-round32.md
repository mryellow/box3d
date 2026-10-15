---
title: Codex review - rewind-only history ring, round 32
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round31.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 32
---

# Round 32

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T14:52:04+10:00 to 2026-10-15T14:58:26+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13de7-327f-78a0-ae39-9ac316c7022b`.

## Review report (Codex final message)

## 1. Summary

The design accounts for the current awake and sleeping tree split, migration queue, and sorted collision candidates. I found one concrete error in the cold-state journal specification; the restore and ring rules I checked otherwise align with the cited source.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5.2, §7.1 | `b3Body_Write` is specified to capture `bodyStates[localIndex]` for a non-awake body. Non-awake solver sets have no body states; `b3GetBodyState` returns `NULL` for them (`src/solver_set.h`, `src/body.c`). Implemented literally, a setter on a sleeping or static body would read an empty array. | Journal the non-awake `b3BodySim` only. Keep `b3BodyState` in the awake image, and correct the cold inventory and entry description. |

## 3. Checked, no change

- Pair creation, CCD candidates, and explosion effects use the stated deterministic ordering (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- Parked leaves, migration queue draining, and the four tree roles match the live broad phase (`src/broad_phase.c`, `src/broad_phase.h`).
- The proposed proxy-id undo and moved-bit reconstruction cover the tree cases checked (`src/dynamic_tree.c`, `src/dynamic_tree.h`).
- The documented ownership transfers address the contact, shape-material, sensor, island, solver-set, and shared-hull lifetimes checked in source.
- Ring eviction retains the journal segments needed to reach its oldest imaged tick; the stated budget overruns are explicit.

## 4. Proposed edits

Correct §5.2 and §7.1 so non-awake body entries contain a body record and `b3BodySim`, with no `b3BodyState` lookup or payload.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) a non-awake body has no state: confirmed, applied.** `b3GetBodyState` returns null for a
  body whose set is not awake (`body.c`), and the sleep path removeswaps the state out of the awake
  array without pushing one into the sleeping set (`solver_set.c`), so a non-awake set's
  `bodyStates` array is always empty. The doc's accessor text, the cold inventory row, the array
  container row and the array push tag named `bodyStates` for a non-awake owner, and round 31's
  clause repeated "sim and state". Fix: §5.2 and §7.1 say a body's entry holds its sim only; the
  non-awake set holds no state; the inventory row, the container row and the push tag drop
  `bodyStates` (a joint's sim has no state). The awake `bodyStates` stays in the image (§5.1).

Raw log: the reviewer read no round record or changelog.

## Status

One Medium finding. Category: a payload that named a body state a non-awake set does not hold (#1).
It is in the sentence round 31's fix extended: the original text already said "sim and state" and
the fix repeated it, so partly a consequence of the previous round's fix (U-T2). Not converged.
Next: round 33, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings clustered in §5.2's write accessor
text, which round 31 also changed; two rounds, not three (U-T2); the reviewer read no round
record. Doc length 1415 -> 1415 lines; added lines per fix: zero, one parenthetical (U-T3). Over the
split check (U-T6): still one decision; no split.

Round 32: 1 findings (0H-1M-0L), 1 applied, 0 declined
Series total: 90 findings (15H-46M-29L) across 32 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L)

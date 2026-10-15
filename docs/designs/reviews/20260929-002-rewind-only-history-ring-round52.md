---
title: Codex review - rewind-only history ring, round 52
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round51.md
mode: full pass, same prompt and scope as round 40, on revision 51
---

# Round 52

## Exact request sent to Codex

Identical to round 51 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T21:38:47+10:00 to 2026-10-15T21:46:26+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f5b-910d-7ae3-a00f-f0a0f62f173b`.

## Review report (Codex final message)

## 1. Summary

I found one low severity cost accounting error in §5.1. The current source confirms the awake/sleeping dynamic-tree split and the sorted continuous and explosion candidates; I found no state-restoration defect in the document’s own inventory, journal entries, or restore sequence.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §5.1 | The estimate of “~208 B per awake shape” counts the 208 B `b3Shape` record but omits its separately imaged 24 B `fatAABBs` entry. The image costs about 232 B per awake shape before proxy storage. | Correct the per-shape estimate and any totals derived from it. |

## 3. Checked, no change

- The four-tree inventory and migration queue match `src/broad_phase.c` and `src/broad_phase.h`.
- Continuous candidates and explosion hits are sorted as §3 states (`src/solver.c`, `src/physics_world.c`).
- Sensor `hits` are cleared each step; `overlaps2` is the retained overlap state (`src/sensor.c`).
- The stated scope of the existing recording hash and `ScrubBackward` test matches `src/recording.c` and `test/test_recording.c`.

## 4. Proposed edits

In §5.1, change the per-awake-shape estimate to approximately 232 B and recalculate the associated image estimate.

## 5. Unresolved

None within this review’s scope. The tree rules cited by §§5.4 and 7.4 and the ring rules cited by §8 remain assigned to their own reviews.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (L) the per-shape estimate omits the `fatAABBs` entry: confirmed, applied.** §5.1's shape
  row images the record and `fatAABBs[id]`, a `b3AABB` of two `b3Vec3`, 24 B
  (`math_functions.h`); the estimate counted the 208 B record alone. It now says ~232 B per awake
  shape, the whole record and its `fatAABBs` entry. The scene totals are order-of-magnitude
  figures and stand.

Raw log: the reviewer read no round record or changelog.

## Status

One Low finding, in §5.1, a number. §5.2's two-round run (50, 51) ends here. Fix folded; a fresh
clean round and its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). §5.1 last produced a finding before round
40; #1 is not a consequence of any fix (U-T2); the reviewer read no round record. Doc length
1177 -> 1177 lines; #1 added zero (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 52: 1 findings (0H-0M-1L), 1 applied, 0 declined
Series total: 114 findings (22H-56M-36L) across 52 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L)

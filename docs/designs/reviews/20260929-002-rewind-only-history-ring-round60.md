---
title: Codex review - rewind-only history ring, round 60
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round59.md
mode: full pass, same prompt and scope as round 40, on revision 57 (confirmation round of round 59, U-P9)
---

# Round 60

## Exact request sent to Codex

Identical to round 59 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T23:04:01+10:00 to 2026-10-15T23:12:23+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13fa9-9862-7da0-bba2-d086be53e1b8`.

## Review report (Codex final message)

## 1. Summary

The hot and cold state inventory matches the current awake and sleeping tree split and the sorted continuous and explosion candidates. Two changes are needed: phase 0 leaves later timeline events in the world, and the correction example does not account for history ticks starting at zero when history is enabled.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §13, §9, §10 | Phase 0 claims the same rewind contract as phase 1, but `b3DeserializeIntoShell` in `src/world_snapshot.c` neither clears event arrays nor resets awake bodies’ `bodyMoveIndex`. A restored world can expose events from P as events of a replayed step, and an immediate sleep operation can use a stale move-event index. | Require phase 0’s rewind wrapper to perform §9 step 5’s event clearing and `bodyMoveIndex` reset after deserialization. |
| 2 | L | §14 | History ticks start at zero on `EnableHistory` (§8 of the ring design), while the correction example passes `serverTick` directly to the history API. If history is enabled after network tick zero, the example can rewind to or correct the wrong tick. | State that `serverTick` and `now` are history-local ticks, or show the conversion from network ticks using the enable-time origin. |

## 3. Checked, no change

- The current broad phase has separate awake and sleeping dynamic trees and a migration queue; the design accounts for them.
- Pair creation is sorted by pair key, continuous candidates by sweep key then shape id, and explosion hits by shape id.
- The inventoried world scalars, sensor overlaps, contact blocks, solver sets, pools, pair set, and graph bitsets have a stated image or journal route.
- The reverse undo and ownership rules for destroyed sets, islands, contacts, shapes, sensors, and hull references are internally consistent.

## 4. Proposed edits

Add the phase 0 cleanup to §13 and make the tick origin explicit beside §14’s correction example.

## 5. Unresolved

The tree rules in §§5.4 and 7.4 and ring rules in §8 belong to their cited companion reviews; no further issue is raised here.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) phase 0 leaves P's events and a stale `bodyMoveIndex` in the world: confirmed,
  applied.** `b3DeserializeIntoShell` (`world_snapshot.c`) frees the live sim elements and reads
  the scalars, pools, sets and sparse records; it neither writes nor clears `bodyMoveEvents`, the
  begin-event arrays or the end-event buffers, which the step clears at its start
  (`physics_world.c`), and a body record is read as raw bytes, `bodyMoveIndex` included, which
  `b3TrySleepIsland` uses to index `bodyMoveEvents` (`solver_set.c`). §13 named two things phase 0
  needs beyond the ring plumbing; it now names three, the third being that the phase-0 rewind
  performs §9 step 5 itself after the restore.
- **#2 (L) the correction example's ticks are history ticks: confirmed as wording, applied.**
  The ring doc's §8 counts ticks from the enable-time world as tick 0, and §14's example passed
  `serverTick` straight in. The comment above the example now says `serverTick` and `now` are
  history ticks and that a caller maps a network tick through the tick at which it enabled history.

Raw log: the reviewer listed the `docs/designs/` directory, which names the changelogs, and
opened none of them and no round record.

## Status

Two findings, one Medium and one Low, in sections (§13, §14) that last produced findings in
rounds 47 and 45. Fixes folded as revision 58. Neither is a consequence of the gate revision 57's
fix, which the round's checks of the sleep and wake transitions did not fault. Round 59's clean
run is not confirmed; a fresh clean round and its confirmation are required on revision 58
(U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed; the phase-0 rewind performing §9's existing step 5
is a required behaviour of a phase, not a mechanism (U-T1). No section recurs; neither finding is
a consequence of the previous round's fix (U-T2); the reviewer read no round record or changelog.
Doc length 1188 -> 1193 lines; added lines per fix: #1 three, #2 two (U-T3). Over the split check
(U-T6): unchanged, the tree and ring decisions already delegated.

Round 60: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 121 findings (24H-59M-38L) across 60 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L) -> 1 (1H-0M-0L) -> 1 (0H-0M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 2 (0H-1M-1L)

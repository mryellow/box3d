---
title: Codex review - rewind-only history ring, round 27
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round26.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 29
---

# Round 27

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T13:32:37+10:00 to 2026-10-15T13:40:25+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13d9e-7570-7ee3-81fb-645961dac4d6`.

## Review report (Codex final message)

## 1. Summary

The design accounts for the current awake and sleeping tree split, migration queue, and sorted collision candidates. I found one restore safety defect and one cost claim that still describes the old broad phase.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §9, §10 | The design says locking the world prevents `destroyDebugShape` from re-entering it during restore. In `src/physics_world.c`, `b3GetWorld` ignores the lock, and mutators such as `b3Shape_SetSurfaceMaterial` in `src/shape.c` use it. The callback can therefore change a partially restored world. | Queue renderer handles for destruction after rewind has restored the world, discarded later slots, and unlocked it; or make every reachable mutator enforce the lock. |
| 2 | L | §1, §15 | The claim that pair discovery scans the whole dynamic tree describes the former single-tree design. `src/broad_phase.c` now gathers moved siblings from the awake tree and uses cross passes against the sleeping tree. This overstates the routine cost attributed to the scoped alternative. | Describe the awake-tree scan and conditional sleeping-tree traversal, including their worst case. |

## 3. Checked, no change

- The four-tree proxy arrangement, parked marked leaves, migration queue, and sleeping-tree height trigger match `src/broad_phase.c` and `src/broad_phase.h`.
- Pair creation, continuous candidates, and explosion candidates use the ordering the design claims.
- The sensor pass swaps overlap arrays as described; `overlaps2` is the overlap state needed across steps.
- The proposed proxy-id undo sequence is consistent with the tree’s LIFO free list and ascending growth.

## 4. Proposed edits

- Revise §9’s lock guarantee and specify when renderer destruction callbacks run.
- Update the broad-phase cost statements in §1 and §15 for the split trees.

## 5. Unresolved

None within this design review.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) the lock does not stop `destroyDebugShape` re-entry: confirmed in part, claim corrected,
  deferral declined.** `b3GetWorld` ignores the lock and shape setters such as
  `b3Shape_SetSurfaceMaterial` use it (`physics_world.c`, `shape.c`), so the lock refuses only the
  world-level entries that go through `b3GetUnlockedWorldFromId`. The same holds at an ordinary shape
  destroy, which calls `destroyDebugShape` under the same lock (`shape.c`), so the callback's
  restriction (do not call the API) is the engine's existing contract, not a new exposure. §9 states
  what the lock refuses and the contract; §10's Host handles bullet states it. The proposed
  deferral queue or lock enforcement on every mutator is declined: the queue is a second structure
  holding handles to shape records the restore has replaced, and the enforcement edits a settled
  primitive (U-P11); reasoning added to §17 (U-V10). Not a fork: both options the reviewer named
  change the engine's callback contract, which this design does not.
- **#2 (L) pair discovery cost: confirmed, applied.** `b3UpdateBroadPhasePairs` gathers the awake
  tree's moved siblings and runs cross passes from moved proxies against the other trees, gated at
  the roots by the moved bits (`broad_phase.c`). §1 and §15 say that, with the whole-world worst
  case.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High and one Low, from two in round 26. Categories: an overstated safety claim
about the lock (#1) and a cost claim from the single-tree broad phase (#2). Not converged. Next:
round 28, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings did not cluster in a section that
produced findings last round (§9 was touched by round 26's precondition, but the finding is the
sentence beside it, on the lock, not the precondition) (U-T2); the reviewer read no round record.
Doc length 1404 -> 1409 lines; added lines per fix: #1 five (one Provenance bullet), #2 zero (U-T3).
Over the split check (U-T6): still one decision; no split.

Round 27: 2 findings (1H-0M-1L), 2 applied, 0 declined
Series total: 86 findings (14H-44M-28L) across 27 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L)

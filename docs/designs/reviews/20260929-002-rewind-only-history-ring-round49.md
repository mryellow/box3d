---
title: Codex review - rewind-only history ring, round 49
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round48.md
mode: confirmation round (U-P9) of round 48, same prompt and scope as round 40, on revision 47
session: 01a13f40-def4-7d33-b6bb-8b48531676e7, resumed once to finalize
---

# Round 49

## Exact request sent to Codex

Identical to round 48 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(8,264 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T21:09:38+10:00
to 21:18:38. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 21:18:46 to 21:19:32. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13f40-def4-7d33-b6bb-8b48531676e7`.

## Review report (Codex final message)

## 1. Summary

The state inventory and undo scheme account for the awake and sleeping tree split, migration queue, sorted CCD candidates, and sorted explosion hits in the current source. I found one contract inconsistency: callback registration changes can alter replay results, but the document both excludes them and claims only two bit-exactness exceptions.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §§2, 10, 14 | §2 names only two exceptions to bit-exact replay. §10 then excludes a mid-timeline callback registration change, even when the callbacks are pure. The registration setters in `src/physics_world.c` can change physics behavior; §14’s correction sketch restores callback-readable host values but gives no definite rule for restoring the registered functions. | Define registrations as replayed host inputs: restore the functions registered at T and replay each change at its original tick. State that rule in §10 and show it in §14. |

## 3. Checked, no change

- The four-tree split, parked leaves, migration queue, and final drain match `src/broad_phase.c`, `src/broad_phase.h`, and `src/physics_world.c`.
- Pair creation, continuous candidates, and explosion hits use the ordering the design claims in `src/broad_phase.c`, `src/solver.c`, and `src/physics_world.c`.
- The inventory covers the examined world scalars, records, solver sets, graph arrays, sensor overlaps, owned blocks, id pools, and hull refcounts. I found no additional uncovered simulation state in those paths.
- The serializer’s existing user-data behavior and omission of the migration queue match the phase-0 adaptations described in §13 (`src/world_snapshot.c`).

## 4. Proposed edits

Make §10 require the caller to restore callback registrations to their values at T and replay registration changes alongside other host inputs. Update §14’s correction example to include that step, so §2’s two named exceptions remain accurate.

## 5. Unresolved

The tree and ring rules delegated by §§5.4, 7.4, and 8 remain for their respective reviews. No additional issue is raised here.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (M) callback registrations excluded from the contract while §2 names two exceptions:
  confirmed, applied.** `b3World_SetCustomFilterCallback`, `b3World_SetPreSolveCallback` and the
  friction and restitution setters (`physics_world.c`) change what a step runs; §10 said a
  mid-timeline swap "is not a case bit-exactness covers", a third exception §2 did not name, while
  §10 already makes the host state a callback reads an input the caller sets back at T and
  replays. Registrations, their contexts and `world->userData` are now host inputs of that same
  kind: §2 says the caller sets host configuration back to its value at T and replays its
  changes, §10's *Inputs* bullet says `Rewind` leaves host configuration for the caller to set
  back and the caller replays everything, the *Callbacks* bullet states the rule for the function
  pointers, and §14's sketch sets registrations back and replays registration changes. No
  journal entry and no engine mechanism: `Rewind` still leaves host configuration alone.

Raw log: the reviewer read no round record or changelog.

## Status

One Medium finding in the confirmation round, on §2, §10 and §14. Fix folded; a fresh clean
round and its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed; registrations become replayed host inputs, a
caller rule, not an engine structure (U-T1). §10 last produced a finding in round 45 (the fork);
#1 is not a consequence of any round's fix (U-T2); the reviewer read no round record. Doc length
1169 -> 1169 lines; #1 added 0 (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 49: 1 findings (0H-1M-0L), 1 applied, 0 declined
Series total: 110 findings (20H-56M-34L) across 49 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L)

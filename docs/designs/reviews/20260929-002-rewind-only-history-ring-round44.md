---
title: Codex review - rewind-only history ring, round 44
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round43.md
mode: full pass, same prompt and scope as round 40, on revision 43
---

# Round 44

## Exact request sent to Codex

Identical to round 43 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T19:37:12+10:00 to 2026-10-15T19:46:03+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13eec-3f91-7751-9335-4516fec85de3`.

## Review report (Codex final message)

## 1. Summary

The state inventory and restore sequence account for the current awake and sleeping tree split, migration queue, and sorted CCD and explosion candidates. I found one gap in the proposed compile-time journaling boundary and one inconsistency in the callback exception to bit-exact replay.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §5.2, §7.3 | Opaque journaled arrays alone do not close the cold-sim write path. `b3GetBodySim` and `b3GetJointSim`/`b3GetJointSimCheckType` return writable pointers across files; `b3SyncBodyFlags` writes non-awake body sims, and joint setters write sleeping joint sims through them (`src/body.c`, `src/body.h`, `src/joint.c`, `src/joint.h`). Those writes would still compile without a journal entry. | Require these getters to expose const data outside their owner, and route every mutable cold-sim write through a journaled set operation. Make that signature change part of the compile-time boundary. |
| 2 | L | §2, §10 | §2 limits the order-dependent callback exception to a layout-changing restore. §10 also allows worker interleaving of `preSolve` and custom-filter calls to differ after restore, including when tree layout is unchanged. | State the exception for callbacks with order-dependent side effects regardless of whether restore changed tree layout. |

## 3. Checked, no change

- The source uses separate awake and sleeping dynamic trees and a migration queue; the document accounts for their retained state (`src/broad_phase.c`, `src/broad_phase.h`).
- Pair discovery sorts pair keys, CCD evaluates candidates in sweep-key and shape-id order, and explosion applies hits in shape-id order (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The proposed image and journal cover the reviewed awake records, sleeping sets, sensor overlaps, world scalars, pool state, and owned-block transitions.
- The phase-0 caveats match the serializer: it omits the migration queue, clears body/shape/joint `userData`, and can retain a matching shape’s renderer handle (`src/world_snapshot.c`).

## 4. Proposed edits

Add the mutable sim getters to §5.2’s required API migration and revise §7.3’s compiler-enforcement claim accordingly. Broaden §2’s callback exception to match §10.

## 5. Unresolved

None within this document. The tree rules delegated by §5.4 and §7.4 and the ring rules delegated by §8 remain for their own reviews.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) writable sim getters defeat the compile-time boundary: confirmed, applied.**
  `b3GetBodySim` and `b3GetJointSim` are declared in `body.h` and `joint.h` returning writable
  pointers into a set's arrays, and `b3SyncBodyFlags` writes a sim's flags through one on any
  body (`body.c`), as the joint setters do on a sleeping joint; §5.2 named only
  `b3GetShapeMaterials` as a getter to make const-returning and §7.3 claimed the compiler then
  rejects every direct mutator. §5.2 now names the three getters as const-returning, adds
  `b3SyncBodyFlags` to the in-place sim writers, and §7.3 counts the getter change in the one-time
  work.
- **#2 (L) the §2 exception narrower than §10: confirmed, applied.** §10 names worker interleaving
  of `preSolve` and custom-filter calls as able to differ after a restore whatever the layout; §2
  now states the exception the same way.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High and one Low. §5.2 has produced findings in rounds 42, 43 and 44, three
rounds running: the owner's stop condition and U-S4. Fixes folded; the series is stopped for the
owner. All three rounds' §5.2 findings are of one kind: a caller or route missing from the
migration worklist, a table the document itself says is a starting point the compiler completes.
The U-S4 cut on offer is to delete that table's caller column and keep the routes; the owner
decides. Not converged.

Tripwire (U-T): no mechanism added or removed; three getters made const-returning (U-T1). §5.2
three rounds running, each a different row or sentence, none a consequence of the previous fix
(U-T2); the reviewer read no round record. Doc length 1163 -> 1166 lines; added lines per fix: #1
four, #2 zero (U-T3). Over the split check (U-T6): unchanged.

Round 44: 2 findings (1H-0M-1L), 2 applied, 0 declined
Series total: 106 findings (20H-53M-33L) across 44 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L)

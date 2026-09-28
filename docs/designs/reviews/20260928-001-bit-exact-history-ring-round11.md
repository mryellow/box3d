---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 11), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 11

## Review report (Codex final message)

## Summary

The whole-world rewind approach is sound, but the journal specification is incomplete for dense cold arrays. The state-hash inventory also needs to state explicitly whether it covers `shape->aabb`.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1, §9 | The inventory lists cold solver sets and sensors, but the entry scheme does not specify how to undo their dense-array append and swap-removal operations. Destroying a static or disabled body swap-removes a `bodySims` row; transferring bodies and joints changes cold array counts and positions; destroying a sensor swap-removes another sensor. Restoring only id-keyed records and set creation or destruction entries cannot reconstruct those array contents and order. See `src/body.c`, `src/solver_set.c`, `src/joint.c`, `src/sensor.c`. | Add reversible append, swap-remove, and resize entries for each affected cold array, including counts, displaced elements, and owned storage. Enumerate the body, joint, contact, and sensor call sites that emit them. |
| 2 | Low | §12 | The proposed full-state-hash list names `fatAABBs` under shape bounds but does not explicitly name `shape->aabb`. That bound is read by sensor queries and CCD, so the stated oracle could miss a restore error in simulation-affecting state. See `src/sensor.c`, `src/solver.c`, `src/shape.h`. | Explicitly include both `shape->aabb` and `fatAABBs` in the hash specification and restore-exactness tests. |

## Checked, no change

- The existing recording player restores a serialized image into a live world, and the current hash covers transforms and velocities at the granularity the design claims. See `src/recording_replay.c`, `src/world_snapshot.c`, `src/recording.c`.
- Sensor overlap results are sorted and deduplicated before event comparison; the sensor task signals changed overlaps. See `src/sensor.c`.
- Broad-phase candidate pair keys are sorted before contact creation. See `src/broad_phase.c`.
- CCD currently passes the running fraction to candidate TOI tests and caps sensor hits during traversal, as §7.4 states. See `src/solver.c`.

## Proposed edits

Specify the cold-array journal operations in §7.1 and apply them consistently to every array listed in §5.2. In §9, describe restoring array counts and displaced rows before dependent records are used. Amend §12 to name `shape->aabb` explicitly.

## Unresolved / disagreements

The word "shapes" in §12 may be intended to mean every field of `b3Shape`; the separate bound list makes that coverage unclear. I found no reason to revisit the declined findings.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:33:22+10:00, end 20:37:08+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 4 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–7 (nine items) was included in the prompt; Codex confirmed it
found no reason to revisit any of them.

Both findings independently re-verified against source:

- **#2 (`shape->aabb`, the tight bound, is missing from §12's explicit hash inventory alongside
  `fatAABBs`), CONFIRMED, applied.** Confirmed directly: `sensor.c`'s sensor query uses
  `sensorShape->aabb` as its query bounds, and `solver.c` derives `fatAABBs` from `shape->aabb`
  via `b3AABB_Inflate`. Both facts make `shape->aabb` genuinely simulation-affecting, matching its
  existing listing as hot/imaged state in §5.1 — the hash's omission was a documentation gap, not
  a mechanism gap (the field was already correctly imaged/journaled per §5.1/§5.2, just not named
  in §12's hash enumeration). Fixed by naming `shape->aabb` alongside `fatAABBs` in the hash list.

- **#1 (the journal has no entry kind for a non-awake solver-set array's or `sensors[]`'s own
  append/swap-remove, only for the moved element's own record), CONFIRMED, applied — the most
  significant finding of this round.** Traced every `localIndex` write site (`solver_set.c`):
  they all live inside functions §5.2's table already names (`b3DestroySolverSet`,
  `b3WakeSolverSet`, `b3TrySleepIsland`, `b3MergeSolverSets`, `b3TransferBody`,
  `b3TransferJoint`), so the write-site *audit* was complete — no missing journal hook. The actual
  gap was one level up: `b3SolverSet` (`solver_set.h`) holds its own dense arrays (`bodySims`,
  `bodyStates`, `jointSims`, `contactIndices`, `islandSims`), structurally identical in kind to
  `b3Island`'s own link arrays (`island.h`), which already have a dedicated entry kind (`island
  arrays`, §7.1: "island id, ownership handle, or (old length, popped element bytes) | truncate
  and restore the popped element / reattach"). `b3TransferBody`/`b3TransferJoint`/
  `b3MergeSolverSets` append to a target solver-set array and swap-remove from the source via
  `b3Array_RemoveSwap` — the exact append/swap-remove pattern island arrays already got a
  dedicated kind for — but solver-set arrays and `sensors[]` had no equivalent, only §5.2's terse
  "ordinary journaled record writes; nothing special is needed" claim, which addresses the moved
  element's own *content* but never specifies how the array's *count* is tracked, nor how a
  content-only record write locates the correct *slot* in an array whose length is changing
  underneath it. §9's restore section corroborates the gap from the other side: its journal-walk
  step asserts "sleeping sets, islands, non-awake records and sims are now exactly as at the end
  of step T" as a bare claim, true for islands (which have a proper entry kind) but not, before
  this fix, for solver-set arrays or `sensors[]` (which didn't). This is a restore-*algorithm* gap,
  not a missed-hook gap — §12.4's cold-hash guard, which only catches a write that bypassed the
  journal, would not have caught it. Fixed by generalizing the existing `island arrays` entry kind
  (already proven correct across ten prior rounds) into a `dense cold arrays` kind covering island
  link arrays, non-awake solver-set arrays, and `sensors[]` uniformly, and correcting §5.2's
  swap-compaction paragraph to state which arrays need nothing (hot, re-imaged every tick) versus
  which need the entry kind (cold, never imaged). §9's journal-walk step needed no wording change:
  it already applies journal entries generically by kind, so it becomes correct for these arrays
  automatically once the entry kind itself covers them.

## Status

Round 11 of an ongoing series (rounds 1–10 committed). Both findings genuine and applied. Finding
#1 is the most significant defect found since round 6: not a pointer-lifetime bug in an otherwise-
sound mechanism (rounds 5, 8, 10's pattern), but a case where the design's own stated reasoning
("nothing special is needed because the journal stores whole records") was simply wrong for a
class of state — cold dense arrays outside the one case (islands) that happened to get a correct
mechanism from round 1 onward. That the island-array mechanism was right all along, and reusing it
verbatim closes the gap for every other affected array, suggests the fix is now complete rather
than partial — but the same reasoning ("this pattern already works elsewhere, so it must be fine
here too") was exactly what let this gap stand through ten rounds, so that should not be taken on
faith. Finding rate (8, 8, 8, 7, 8, 3, 1, 2, 1, 1, 2 applied) stayed low in count but this round's
findings were higher-consequence than the last several rounds'. Another full-scope round is
warranted, specifically re-checking every other place the doc asserts a swap-compaction or
array-shape claim is "covered by ordinary record writes" without an explicit array-shape-tracking
mechanism behind it.

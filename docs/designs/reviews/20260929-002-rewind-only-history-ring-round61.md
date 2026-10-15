---
title: Codex review - rewind-only history ring, round 61
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round60.md
mode: full pass, same prompt and scope as round 40, on revision 58
---

# Round 61

## Exact request sent to Codex

Identical to round 60 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T23:14:17+10:00 to 2026-10-15T23:22:25+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13fb2-fd58-7d51-a915-3d3721a952b0`.

## Review report (Codex final message)

## 1. Summary

The hot image and undo journal cover the reviewed state transitions, including the current awake and sleeping tree split. One write-route instruction conflicts with the live joint layout and would misdirect setters for awake joints.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §5.2 | The setter recipe sends a joint sim write to its solver set’s `jointSims` array at `localIndex`. An awake joint’s sim is instead in its graph colour’s `jointSims` array; the awake solver set has no such element (`src/joint.c`, `src/constraint_graph.c`). Following the recipe would access the wrong array. | Specify a sim write accessor that dispatches by `setIndex`: use `colorIndex` and `localIndex` for awake joints, and the solver set’s `jointSims` for non-awake joints. |

## 3. Checked, no change

- The live broad phase has separate awake and sleeping dynamic trees and a migration queue; the document accounts for them (`src/broad_phase.c`, `src/broad_phase.h`).
- Continuous candidates and explosion hits are sorted before evaluation or application (`src/solver.c`, `src/physics_world.c`).
- The reviewed pool, sensor, contact, island, and solver-set transitions have a corresponding image or proposed undo route.
- The recording hash covers transforms and velocities only, as the document states (`src/recording.c`, `test/test_recording.c`).

## 4. Proposed edits

Correct the joint-sim setter route in §5.2 and its write-route table to name the graph colour array for awake joints.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) the sim write route named a solver set's array for every joint: confirmed, applied.**
  `b3GetJointSim` (`joint.c`) returns an awake joint's sim from
  `constraintGraph.colors[colorIndex].jointSims` at `localIndex` and only a non-awake joint's from
  its solver set's `jointSims`; a body's sim is in its set's array in every state (`b3GetBodySim`,
  `body.c`). §5.2 said a sim is an element of a solver set's array and that a setter writes it
  "through its set's array `set` at the record's `localIndex`". Both sentences now name the graph
  colour's `jointSims` array for an awake joint, the array the record's `colorIndex` names; the
  colour arrays are imaged and the write makes no entry, as §5.2 already states for awake arrays.
  No table row changes: the row for set array contents is already scoped to a non-awake owner.

Raw log: the reviewer read no round record or changelog.

## Status

One finding, High, a wrong array named for one case in §5.2, a section that last produced a
finding in round 56. Fix folded as revision 59. Not a consequence of round 60's fixes (§13, §14).
A fresh clean round and its confirmation are required on revision 59 (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). No section recurs; the finding is not a
consequence of the previous round's fix (U-T2); the reviewer read no round record or changelog.
Doc length 1193 -> 1195 lines; added lines for the fix: two (U-T3). Over the split check (U-T6):
unchanged, the tree and ring decisions already delegated.

Round 61: 1 findings (1H-0M-0L), 1 applied, 0 declined
Series total: 122 findings (25H-59M-38L) across 61 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L) -> 1 (1H-0M-0L) -> 1 (0H-0M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 2 (0H-1M-1L) -> 1 (1H-0M-0L)

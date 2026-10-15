---
title: Codex review - rewind-only history ring, round 42
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round41.md
mode: full pass, same prompt and scope as round 40, on revision 41 (round 34's fixes re-applied)
---

# Round 42

## Exact request sent to Codex

Identical to round 41 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T19:09:22+10:00 to 2026-10-15T19:17:01+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ed2-c463-7e03-b3c7-492db482890d`.

## Review report (Codex final message)

## 1. Summary

The design accounts for the current awake/sleeping tree split and sorted collision candidates. I found one simulation-state write missing from its migration worklist, plus two inaccurate descriptions of live write paths. The proposed compile gate should catch direct writes during implementation, but the document should name these paths explicitly.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5.2 | `b3UpdateBodyMassData` and `b3Body_SetMassData` clear `b3_relativeTransformValid` on attached contacts in `src/body.c`. A sleeping contact can be affected, but the cold-contact caller list names only `b3RefreshBodyContactIndices` from that file. The flag controls contact recycling in `src/physics_world.c`. | Add both paths to the migration worklist and route each flag write through `b3Contact_Write`, journaling it when the contact is cold. |
| 2 | M | §5.2 | The proposed single `shape.c` writer for `userShape` omits debug-draw creation: `src/physics_world.c` currently assigns the handle directly. That path needs a route through the proposed boundary even though the handle itself is not journaled. | Specify a `shape.c` handle setter used by debug draw, and include that caller in the migration. |
| 3 | L | §5.2 | The examples say `b3Body_SetLinearVelocity` and explosion write an awake body *record*. In `src/body.c` and `src/physics_world.c`, their already-awake paths write `bodyStates`; waking can separately change the record. | Correct the examples and identify the awake `bodyStates` array mutator as their write route. |

## 3. Checked, no change

- The live broad phase has separate awake and sleeping dynamic trees and a migration queue; pair keys are sorted. Continuous candidates and explosion hits are also ordered as the design states.
- The cited struct sizes and 24 graph colours match the source.
- Sensor hits are cleared by the sensor pass, while `overlaps2` carries the overlap state needed at a step boundary.
- The current recording hash covers transforms and velocities only, and the serializer zeroes body, shape and joint `userData` on restore, as described.

## 4. Proposed edits

Update §5.2’s caller table and write-path examples for findings 1–3. Add the debug-draw handle setter to the phase-1 migration description.

## 5. Unresolved

The tree reconstruction and ring eviction rules belong to their cited companion designs; this review makes no finding on those rules.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) the mass-data paths clear a contact flag: confirmed, applied.** `b3UpdateBodyMassData`
  and `b3Body_SetMassData` clear `b3_relativeTransformValid` on every contact attached to the body
  (`body.c`), and contact recycling reads it (`physics_world.c`); an attached contact can be
  sleeping. Both join the `contacts[id]` worklist row.
- **#2 (M) the debug draw assigns `userShape` directly: confirmed, applied.** Six assignments in
  `physics_world.c`'s debug draw; §5.2 names them as callers that migrate to the one `shape.c`
  writer.
- **#3 (L) velocity setters write the record: confirmed, applied.** `b3Body_SetLinearVelocity`
  wakes the body and writes `state->linearVelocity` in the awake set's `bodyStates` (`body.c`);
  the explosion's impulse likewise. §5.2's example now routes them through the awake set's array,
  imaged and without an entry, with the wake as the journaled structural part.

Raw log: the reviewer opened no round record or changelog; five changelog filenames appear in one
directory listing of `docs/designs/`, none read.

## Status

Three findings, two Medium and one Low, all in §5.2's worklist and examples, on the first round
after round 34's fixes were re-applied. Categories: two migration worklist omissions (#1, #2) and a
wrong write route in an example (#3). §5.2 last drew findings in rounds 31 to 33 and 36. Not
converged. Next: round 43, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Not a cluster against the previous round
(41 had none); not a consequence of a previous fix (U-T2); the reviewer read no round record. Doc
length 1159 -> 1163 lines; added lines per fix: #1 zero, #2 one, #3 three (U-T3). Over the split
check (U-T6): what remains is one mechanism; the two splits are out.

Round 42: 3 findings (0H-2M-1L), 3 applied, 0 declined
Series total: 103 findings (19H-53M-31L) across 42 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L)

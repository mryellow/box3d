---
title: Codex review — rewind-only history ring, round 9
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Attempt 1 killed by the shell timeout, resumed once (see Post-review verification).
session: 01a0ecee-f11a-7c73-9627-ca1385a9eae1
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 9

## Review report (Codex final message)

## Summary

The rewind design is broadly consistent with the inspected step, sleep/wake, ownership, and broad-phase paths. Two specifications need changes: the full-state hash can miss a geometry difference, and bitset growth is absent from the undo rules.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12.1 | The proposed hash substitutes a hull’s stored `hash` for its contents. Hull lookup compares full bytes after hashing, and the source has a test for distinct geometry with the same hash. Two otherwise identical worlds using different, same-hash hulls could receive the same “full state” hash while later contacts differ. | Hash canonical geometry contents for the state oracle, including nested compound geometry; keep stored hashes as lookup aids only. |
| 2 | Low | §§5.2, 7.1, 12.1 | The bitset journal specifies set/clear entries but no growth undo. Adding a constraint for a sufficiently high body id calls `b3SetBitGrow`, which increases `blockCount`. Rewinding clears the bit but leaves the colour bitset longer than at T, contrary to the stated restoration and bitset hash inventory. | Specify growth and its undo, or explicitly treat trailing zero blocks as capacity and normalize them in the state hash. |

## Checked, no change

- The inspected sleep and wake paths support the distinction between pre-wake journal entries and the hot image.
- Shape, contact, sensor, island, solver-set, and hull paths contain the owned or shared allocations the document identifies.
- World proxy creation and destruction go through the broad phase; compound child trees are separate borrowed geometry.
- The sensor pass clears CCD hits and uses `overlaps2` as the persistent overlap list.
- The existing recording hash covers body poses and velocities, so the document correctly calls for a broader oracle.

## Proposed edits

Specify canonical geometry hashing in §12.1. Add a bitset growth rule to §§5.2 and 7.1 and state how §12.1 hashes its logical length.

## Unresolved / disagreements

The quoted performance baseline comes from a review file excluded by the requested constraints, so I did not independently verify those numbers.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as rounds 6 to 8. Start 2026-09-29T21:31:17+10:00, end 21:40:17, exit code 124. Attempt 2, resume of session 01a0ecee-f11a-7c73-9627-ca1385a9eae1 with `-m gpt-6-sol -c model_reasoning_effort="high"` before `resume` and a finalize-only prompt, start 21:40:28, end 21:41:07, exit code 0. The working tree differs before and after only by a new untracked file, `docs/designs/20260929-003-box2d-snapshot-comparison.md`, created at 21:31:41 (24 s into attempt 1) and not by Codex: it is 24 KB of authored analysis, Codex ran read-only, and it is not the target doc or a review file. The target doc had a diff (rounds 7 and 8's fixes, uncommitted) that was read and was not mangled. Codex's markdown file links were reduced to their link text when copied here (Codex's link targets carried line numbers, which this project does not cite).

- **#1 (Medium), declined; rationale added to the doc.** The oracle hashes a hull's, mesh's or height field's stored `hash` (`b3HashHullData` returns `hull->hash`), and `b3CompareHullData` and the recording registry compare bytes after a hash match because a lookup must be exact. `GeometryHashCollision` in `test_recording.c` forces the collision by handing the same hash to distinct blobs; no real geometry collides. The state hash is itself a 64-bit digest, so a same-hash pair of distinct geometries is no more likely to slip past it than the state hash is to collide, and hashing geometry contents on every `b3World_ComputeStateHash` call would cost O(geometry bytes), against the oracle's purpose of running per tick. Codex did not engage either point. §12 test 1 now states both.
- **#2 (Low), applied.** `b3SetBitGrow` (`constraint_graph.c`, `bitset.c`'s `b3GrowBitSet`) raises `blockCount` and nothing shrinks it; `b3GetBit` (`bitset.h`) returns false for a bit at or beyond `blockCount`. So block count is capacity, not state: the undo of a set entry writes the old value (clear) into a bitset that stays grown, and a replay that grows it again reads identically. §7.1's bitset row now says a growing set makes no growth entry and why, and §12 test 1 hashes bitset membership, not block counts. A raw block-count hash would have differed between a rewound world and the original.

## Status

Two findings, one Medium and one Low, from a flat count (2, 2, 1, 2). #1 is a new category (the oracle's geometry digest against hash collision) and was declined, so it does not count as a defect; #2 is a new category (bitset capacity in §7.1 and §12.1). Neither was a consequence of round 8's fix, and neither reopened settled rationale, though #1 touches text no earlier round examined. Findings are now Medium and Low only in three of the last four rounds; a clean round has not occurred. Next: another fresh full-scope round 10, same brief.

`Round 9: 2 findings (0H-1M-1L), 1 applied, 1 declined`

`Series total: 24 findings (7H-13M-4L) across 9 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L)`

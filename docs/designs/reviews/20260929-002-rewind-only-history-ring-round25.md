---
title: Codex review - rewind-only history ring, round 25
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round24.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 27
---

# Round 25

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T13:14:24+10:00 to 2026-10-15T13:22:44+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13d8d-c974-78b2-83aa-467088e47ab5`.

## Review report (Codex final message)

1. **Summary**  
The phase 1 rewind design accounts for the awake/sleeping tree split, sorted candidates, and most cross-step state. Two migration-queue assumptions need correction before the design is converged.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §13 | Phase 0 calls the existing serializer sufficient, but `src/world_snapshot.c` neither writes nor restores `broadPhase.migrationQueue`. A restored marked leaf can remain parked in the awake tree after its mark is consumed because the next drain has no queued body to move. | Add queue serialization and restoration before treating phase 0 as a semantic baseline. |
| 2 | M | §§6, 7.4, 17 | The claim that the queue contains each parked body once is false at a valid tick-0 capture. Creating multiple shapes on one sleeping body before a step can enqueue its id repeatedly (`src/shape.c`, `src/broad_phase.c`). The image-size description also omits the queue’s actual length. | State that duplicates and stale ids can occur before a drain; size and copy the full stored queue, preserving its order. |

3. **Checked, no change**

- The live code has separate awake and sleeping dynamic trees; marked sleeping leaves can wait in the awake tree.
- Continuous candidates and explosion hits are sorted before evaluation.
- The sleeping tree’s proxy IDs and leaf boxes need the journal treatment described in §7.4.
- The ring’s rewind, eviction, and interval rules preserve a retained image under the stated `captureInterval ≤ tickCount` constraint.

4. **Proposed edits**  
Revise §13’s phase-0 prerequisites to include the migration queue. Revise §§6, 7.4, and 17 to use the queue’s stored length and order without a uniqueness invariant.

5. **Unresolved**  
None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) phase 0 omits the migration queue: confirmed, applied.** `world_snapshot.c` has no
  reference to the migration queue (grep for `migrat` and `queue`), so a deserialized world keeps a
  sleeping body's parked flag and marked leaf with an empty queue, and no drain places it. Fix: §13's
  phase-0 prerequisites become three; each ring slot carries the queue array beside its image and a
  restore copies it back, which needs no change to the serializer's format and keeps the placement
  order. Not a fork: phase 0 is the stand-in, and the in-place restore of phase 1 already images the
  queue. Observation, disclosed and not acted on: the recording player's own restore path shares the
  gap (a separate subsystem, §17).
- **#2 (M) queue holds each body once: confirmed, applied.** `b3BroadPhase_MigrateBody` places
  immediately outside a traversal and pushes the body again when it stays parked
  (`broad_phase.c`), so two shapes created on one sleeping body leave it queued twice, and a body
  destroyed while queued leaves its id (`b3PlaceBodies` skips a null set and drops it). The drain
  dedupes (the in-flight flag), so the statement holds after a step's drain and not for the tick-0
  image taken at `EnableHistory` after API calls. Fix: §7.4 states the image copies the stored count
  and order and names the tick-0 case; §17's bullet says "after a step's drain".

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High and one Medium, from four in round 24. Categories: a phase-0 dependency the
design did not state (#1) and an invariant true only after a drain (#2), both on the migration
queue; the queue was added to §7.4 in round 22's revision and its invariant narrowed in round 23.
Not converged. Next: round 26, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed by name; the phase-0 slot carries an array the
serializer does not (U-T1). Findings clustered on the migration-queue text in §7.4, which round 22
and round 23 also touched through the parked-flag and placement lines; round 24 did not, so not
three rounds running. #2 is a narrower reading of the sentence round 22 wrote, not a consequence of
a round 24 fix (U-T2); the reviewer read no round record. Doc length 1395 -> 1399 lines; added
lines per fix: #1 three, #2 two (U-T3). Over the split check (U-T6): still one decision; no split.

Round 25: 2 findings (1H-1M-0L), 2 applied, 0 declined
Series total: 82 findings (12H-43M-27L) across 25 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L)

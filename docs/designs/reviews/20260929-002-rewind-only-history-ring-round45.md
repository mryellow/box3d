---
title: Codex review - rewind-only history ring, round 45
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round44.md
mode: full pass, same prompt and scope as round 40, on revision 45 (the U-S4 cut of §5.2's caller column)
---

# Round 45

## Exact request sent to Codex

Identical to round 44 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T20:14:06+10:00 to 2026-10-15T20:22:04+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f0e-0b87-7d52-afb8-0c1820850ea9`.

## Review report (Codex final message)

## 1. Summary

The hot and cold state inventory is consistent with the current awake/sleeping tree split and sorted continuous and explosion candidates. Two design defects remain: the proposed compiler boundary cannot prove that every cold write is journaled, and phase 0 exposes an API with different restore semantics from the stated caller contract.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2.7, §5.2, §7.3, §13 | The design says compilation will reject every direct cold-record write left after migration. It will not reject writes inside the accessor’s own translation unit: setters in `src/body.c` and `src/shape.c`, for example, can still assign fields beside `b3Body_Write` and `b3Shape_Write`. The claimed construction proof therefore cannot establish journal completeness. | Put mutable storage behind a separate opaque module from all mutation callers, or replace the compiler-proof claim with an explicit check for file-local direct writes. |
| 2 | M | §10, §13, §14 | Phase 0 uses the public Rewind API, but its serializer clears body, shape and joint `userData` and can retain `userShape` across a geometry change (`src/world_snapshot.c`). §10 promises restored `userData` and release of live renderer handles. The phase-0 caveats acknowledge the difference without defining a separate public contract. | Keep phase 0 private, or make its restore satisfy §10 before exposing the §14 API. |

## 3. Checked, no change

- Verified the four-tree split, migration queue, and sorted continuous and explosion candidate paths against current source.
- Verified the listed single-precision structure sizes with compile-time `sizeof` checks.
- Traced contact, shape, sensor, island and sleeping-set ownership through the proposed reverse journal order; found no additional supported leak or double-free finding.
- Checked the image-after-undo restore order and the between-step pending segment against the stated caller flow.

## 4. Proposed edits

Revise §§2.7, 5.2, 7.3 and 13 to state an enforceable write boundary. Revise §13 so phase 0 either meets §10’s public semantics or is explicitly an internal prototype.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) the write boundary inside the accessor's own translation unit: confirmed as an
  inconsistency, applied.** §5.2's Containers paragraph already places every writable element
  route in the container's own file, with the record array's storage type incomplete elsewhere,
  so `body.c` cannot assign a record field beside `b3Body_Write`; but §5.2's opening sentence said
  "outside its defining file", which reads as `body.c` for a body, and §13's phase 1 said direct
  writes are removed "from every other translation unit", which reads as the accessor's keeping
  them. §5.2 now says the file that holds the storage, states that the accessor's own file is an
  outside file that builds the value in a local copy and enters the container's write function,
  and §13 says every translation unit but each container's own. The reviewer's alternative, an
  explicit check for file-local direct writes, is not needed once the boundary is stated (AC-4
  would reject it as a check standing in for construction); not a decline, since the first option
  was adopted.
- **#2 (M) phase 0 exposes the §14 API with weaker restore semantics than §10: confirmed, carried
  forward (U-S3).** `world_snapshot.c` zeroes body, shape and joint `userData` on read and restores
  a matching shape's `userShape`; §13 states both caveats and that they apply until phase 1, while
  §10 promises `userData` restored to its value at T and every live `userShape` released. Two
  viable directions with different host-visible consequences: phase 0 stays an internal prototype
  (the §14 API is not public until phase 1), or phase 0's restore meets §10 before the API is
  exposed (the reader restores `userData` and releases `userShape`, work the design assigns to
  phase 1). The owner decides; no edit made.

Raw log: the reviewer read no round record or changelog (a file listing at the start of its
research printed changelog file names; none was opened).

## Status

Two findings, both Medium. §5.2 has produced findings in rounds 42, 43, 44 and 45, the fourth
after the U-S4 cut of its caller column; U-S4 says pause and report when a section recurs after a
cut, and finding #2 forks the design (U-S3). The series is stopped for the owner on both. The
§5.2 finding is of a different kind from the three the cut answered: those were callers missing
from a worklist, this one is the boundary statement itself, in text the cut did not touch. Not
converged.

Tripwire (U-T): no mechanism added or removed (U-T1). §5.2 four rounds running; #1 is not a
consequence of the cut or of any earlier fix, #2 is in §10/§13/§14, which last produced a finding
in round 38 (U-T2); the reviewer read no round record. Doc length 1166 -> 1170 lines; added lines
per fix: #1 four, #2 zero (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 45: 2 findings (0H-2M-0L), 1 applied, 0 declined, 1 carried forward
Series total: 108 findings (20H-55M-33L) across 45 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L)

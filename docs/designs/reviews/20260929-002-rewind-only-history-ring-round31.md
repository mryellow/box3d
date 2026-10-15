---
title: Codex review - rewind-only history ring, round 31
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round30.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 31
---

# Round 31

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T14:42:59+10:00 to 2026-10-15T14:50:53+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13dde-eafc-7a20-b615-977dc54e02a8`.

## Review report (Codex final message)

## 1. Summary

The design reflects the current four-tree broad phase and the sorted continuous and explosion candidates. I found one restore-order defect: its structural journal writes can try to restore awake solver data before the awake image has recreated the array slots that hold it.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§5.2, 7.1, 9 | `b3Body_Write` and `b3Joint_Write` are specified to journal a record together with its sim or state, including structural writes. When an island sleeps, `src/solver_set.c` removes its body sims, body states and joint sims from the awake arrays before changing the records’ set indices. `Rewind(T)` then undoes those record writes **before** §9 restores the awake arrays from image T. If that was the only awake island, the old awake indices no longer exist; with other islands, they may hold different occupants. | Journal record fields separately from awake sim/state data. Restore awake arrays only from image T; give writes to non-awake sims their own journaled path. Specify the sleep transition’s logging order and update §9’s undo rule. |

## 3. Checked, no change

- The source has separate awake and sleeping dynamic trees, and the design accounts for parked marked leaves and the migration queue (`src/broad_phase.c`, `src/broad_phase.h`).
- Pair creation is sorted; continuous candidates and explosion hits are processed in deterministic candidate order (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The proposed proxy-id undo sequence matches the trees’ free-list allocation and migration order (`src/dynamic_tree.c`, `src/broad_phase.c`).
- The ring rules admit an oversized slot and retain journal segments needed to reach an imaged tick (§8).

## 4. Proposed edits

Revise §§5.2 and 7.1 to distinguish record writes from sim/state writes, then make §9 explicitly defer all awake sim/state restoration to the image copy. Walk through a sleep and rewind of a single awake island in the normative undo sequence.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) structural record writes journal a sim and state at stale coordinates: confirmed,
  applied.** `b3TrySleepIsland` removeswaps the sim and state out of the awake arrays and then
  writes the body's `setIndex` and `localIndex` (`solver_set.c`), so a structural write made there
  could only read a sim at coordinates that name the awake set after the sim has left them; the
  awake arrays are not journaled, so the undo, which runs before the image copy, would write into
  an array of the wrong length. The doc said the entry holds "the record together with its sim and
  state" without saying where the sim is read. Wake and merge write the record's coordinates while
  the source set's arrays are intact and destroy the set afterwards (`b3WakeSolverSet`,
  `b3MergeSolverSets`), so reading at the record's coordinates is valid there and undone in reverse
  after the set destroy's undo has reattached the arrays. Fix: §5.2 states that the entry holds the
  sim and state only when the record's coordinates name a non-awake set at the time of the write, and
  holds the record alone when they name the awake set. Not a fork: it is the existing reverse-order
  rule applied to the entry's payload. The reviewer's separate journal path for non-awake sims is not
  needed.

Raw log: the reviewer read no round record or changelog.

## Status

One High finding after two clean rounds. Category: an entry payload whose source was unstated (#1),
in §5.2's write accessor text, which no earlier round produced a finding in. The finding is not
a consequence of a previous fix. Not converged. Next: round 32, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed; one rule on an existing entry's payload (U-T1).
Findings did not cluster in a section that produced findings last round (U-T2); the reviewer read no
round record. Doc length 1410 -> 1415 lines; added lines per fix: #1 five (U-T3). Over the split
check (U-T6): still one decision; no split.

Round 31: 1 findings (1H-0M-0L), 1 applied, 0 declined
Series total: 89 findings (15H-45M-29L) across 31 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L)

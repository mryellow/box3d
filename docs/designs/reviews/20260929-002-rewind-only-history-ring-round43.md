---
title: Codex review - rewind-only history ring, round 43
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round42.md
mode: full pass, same prompt and scope as round 40, on revision 42
session: 01a13edf-371e-7811-aa1b-bc88c4e877f9, resumed once to finalize
---

# Round 43

## Exact request sent to Codex

Identical to round 42 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(8,302 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T19:22:58+10:00
to 19:31:58. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 19:35:41 to 19:36:00. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13edf-371e-7811-aa1b-bc88c4e877f9`.

## Review report (Codex final message)

## 1. Summary

The live source confirms the four-tree broad phase and the sorted continuous and explosion candidates assumed by the design. I found one concrete gap in section 5.2’s list of cold-state writes to migrate; the proposed type boundary would catch it at build time, but the document should name and route those writes.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §5.2 | The island migration worklist attributes island writes to `src/island.c`, but `src/body.c` directly changes an island’s `bodies` element and count, while `src/solver_set.c` changes island `setIndex` and `localIndex` during wake, sleep and merge. These are cold-state writes the named accessors and container must cover. | Add both paths to the worklist. Route body removal through a journaled island-array operation and the solver-set transitions through `b3Island_Write`. |

## 3. Checked, no change

- The awake and sleeping dynamic trees, migration queue, and static moved-proxy clearing match the live broad-phase code.
- Pair discovery, continuous candidates, and explosion candidates use the ordering described in §3.
- The examined pool, owned-block, sensor, and hull undo paths are consistent with the proposed reverse order.
- The correction timing in §14 agrees with §10’s rule that calls after step *t* are inputs to tick *t*+1.

## 4. Proposed edits

Update §5.2’s island row and migration instructions to include `src/body.c` and `src/solver_set.c`, with the write routes stated in finding 1.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (L) island writes outside `island.c`: confirmed, applied.** `b3RemoveBodyFromIsland`
  (`body.c`) removeswaps the body out of its island's `bodies` array, and the wake, sleep and
  merge transitions write the island's `setIndex` and `localIndex` (`solver_set.c`). The islands
  row named `island.c` alone. It now names both, with their routes: the array operation of the
  island's journaled link array, and `b3Island_Write`.

Raw log: the reviewer read no round record or changelog.

## Status

One Low finding, from three in round 42, again in §5.2's worklist. Category: a worklist row that
named one file where three write (#1). §5.2's table has drawn findings in rounds 42 and 43; a
third running would trip U-S4. Not converged. Next: round 44, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). §5.2 two rounds running, in different rows;
not a consequence of round 42's fixes (U-T2); the reviewer read no round record. Doc length
1163 -> 1163 lines; added lines per fix: #1 zero (U-T3). Over the split check (U-T6): unchanged.

Round 43: 1 findings (0H-0M-1L), 1 applied, 0 declined
Series total: 104 findings (19H-53M-32L) across 43 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L)

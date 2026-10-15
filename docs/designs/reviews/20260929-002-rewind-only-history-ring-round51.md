---
title: Codex review - rewind-only history ring, round 51
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round50.md
mode: full pass, same prompt and scope as round 40, on revision 50
session: 01a13f51-60b7-7882-8f87-65c92dc56b83, resumed once to finalize
---

# Round 51

## Exact request sent to Codex

Identical to round 50 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(12,268 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T21:27:40+10:00
to 21:36:40. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 21:36:51 to 21:37:21. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13f51-60b7-7882-8f87-65c92dc56b83`.

## Review report (Codex final message)

1. Summary

The current source supports the design’s main assumptions about the awake and sleeping trees, migration queue, sorted continuous candidates, and explosion order. Two changes are needed: the proposed type boundary does not enforce journaling for nested array headers, and §3 overstates which callback calls follow tree order.

2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2.7, §5.2, §7.3 | The proposed const-data array fields inside solver sets and islands leave `count`, `capacity`, and the data pointer writable. Direct header writes such as those in `src/island.c` would remain compiler-legal, so the claimed “verified by construction” journal boundary does not hold for cold arrays. | Make these array headers opaque to callers, or expose only a read-only view and route every header mutation through the journaled container. |
| 2 | L | §3 | The statement that `preSolve` or custom-filter calls still occur in tree-layout order is too broad. CCD evaluates candidates in sweep-key and shape-id order in `src/solver.c`; §10 describes this distinction correctly. | Align §3 with §10: identify pair discovery and sensor queries as tree-ordered, CCD candidates as sorted, and parallel callback interleaving separately. |

3. Checked, no change

- The four-tree split and migration queue are present in `src/broad_phase.c` and `src/broad_phase.h`.
- Pair candidates, CCD candidates, and explosion hits are sorted as the design states.
- The sensor pass swaps overlap arrays, sorts and deduplicates current overlaps, and consumes sensor hits before the step boundary.
- The listed world scalars, six id pools, awake records and arrays, and contact-owned blocks match the inspected source.
- The §14 correction sequence is consistent with §10’s rule that calls after step *t* are inputs to tick *t + 1*.

4. Proposed edits

Close the nested-array write route in §5.2 and §7.3 before claiming compiler-enforced journal completeness. Narrow the callback-order sentence in §3.

5. Unresolved

None within this review’s scope.

6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (H) the nested array headers stay assignable: confirmed, applied.** The const-data array
  type of the owned-blocks rule made the elements const but left `data`, `count` and `capacity`
  plain fields, so a local copy of an island built for `b3Island_Write` could carry a changed
  header (a reallocated `data` pointer, a bumped `count`) into a record write that journals only
  the record bytes, and `island.c` writes such headers directly today. The three fields are now
  const-qualified in the type, so neither the record in place nor a local copy can assign one, a
  record write carries the header unchanged, and the container's file alone writes a header
  through the storage type it defines.
- **#2 (L) §3's callback-order sentence too broad: confirmed, applied.** `solver.c` evaluates
  continuous candidates in sweep-key then shape-id order, as §3's own previous sentence and §10
  say; the sentence claimed every `preSolve` and custom filter call follows layout order. §3 now
  distinguishes pair discovery and sensor queries (layout order), continuous sweeps (sorted
  candidate order, workers interleaved) and public queries (layout order), matching §10.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High in §5.2, one Low in §3. §5.2 has produced findings in rounds 50 and 51,
two running; a third would be the owner's stop condition. Fixes folded; a fresh clean round and
its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed; three fields const-qualified (U-T1). §5.2 two
rounds running; #1 is on the owned-blocks sentence no round's fix wrote, #2 is not a consequence
of any fix (U-T2); the reviewer read no round record. Doc length 1172 -> 1177 lines; #1 added
four, #2 one (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 51: 2 findings (1H-0M-1L), 2 applied, 0 declined
Series total: 113 findings (22H-56M-35L) across 51 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L)

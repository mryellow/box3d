---
title: Codex review - rewind-only history ring, round 56
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round55.md
mode: full pass, same prompt and scope as round 40, on revision 55
---

# Round 56

## Exact request sent to Codex

Identical to round 55 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T22:15:31+10:00 to 2026-10-15T22:23:05+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f7d-3170-72c1-bffe-6e645e062e95`.

## Review report (Codex final message)

## 1. Summary

The state inventory and rewind sequence account for the current awake and sleeping tree split, migration queue, and sorted collision candidates. I found one inconsistency in the proposed journal’s heap ownership rules.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5.2, §7.1 | For material-block replacement, §5.2 says the journal entry **holds the old block**, while §7.1 says undo **reallocates and copies old bytes**. If both rules are followed, undo leaves the held block owned by an entry that is then discarded without a cleanup walk, leaking it. Current `src/shape.c` changes material-block size only during shape creation and destruction, so this replacement branch has no current API caller. | Remove the unused replacement branch, or specify one ownership model consistently: reattach the held block on undo and free it on eviction, or copy its bytes and free the old block at mutation time. |

## 3. Checked, no change

- The live broad phase has separate awake and sleeping dynamic trees and a migration queue (`src/broad_phase.c`, `src/broad_phase.h`); the design accounts for them in §§5.1, 6, and 7.4.
- Pair creation, continuous candidates, and explosion hits use the ordering claimed in §3 (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The proposed inventory covers the solver sets, graph arrays, sparse records, sensor overlaps, world scalars, id pools, and pair-set membership. I found no additional simulation-affecting state omitted from §5.
- The pending-segment rule and §14 correction sketch agree with §10’s contract for API calls made between steps.

## 4. Proposed edits

Resolve the material-block replacement rule in §§5.2 and 7.1, and make §8’s block charge follow the chosen ownership rule.

## 5. Unresolved

None within this document. The tree rules cited by §§5.4 and 7.4 and the ring rules cited by §8 remain matters for their respective reviews.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (M) §5.2 said the block mutator holds the old block while §7.1 copies old bytes: confirmed,
  applied.** §7.1's material block row records the old array bytes for a resize or replacement
  and its undo reallocates and copies them; the manifold block row is the same; blocks created or
  destroyed with their record ride in the pool entries. The previous revision's §5.2 sentence
  said the mutator "holds the old block", a second ownership model. §5.2 now says the mutator
  records the old bytes the entry holds (§7.1) and writes the pointer and count. The observation
  that no API caller resizes a material block today (`shape.c` allocates it at creation and frees
  it at destruction) changes no rule: the entry row already covers the branch.

Raw log: the reviewer read no round record or changelog.

## Status

One Medium finding, in §5.2, a consequence of round 55's fix wording. §5.2 has produced findings
in rounds 55 and 56, two running; a third is the owner's stop condition. Fix folded; a fresh
clean round and its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). §5.2 two rounds running; #1 is a
consequence of round 55's fix #2 (U-T2); the reviewer read no round record. Doc length 1183 ->
1181 lines; #1 added -2 (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 56: 1 findings (0H-1M-0L), 1 applied, 0 declined
Series total: 119 findings (24H-58M-37L) across 56 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L) -> 1 (1H-0M-0L) -> 1 (0H-0M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L)

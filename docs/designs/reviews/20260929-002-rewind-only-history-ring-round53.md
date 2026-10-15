---
title: Codex review - rewind-only history ring, round 53
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round52.md
mode: full pass, same prompt and scope as round 40, on revision 52
---

# Round 53

## Exact request sent to Codex

Identical to round 52 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T21:47:07+10:00 to 2026-10-15T21:55:35+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f63-324d-7b62-a836-db2f2e07d455`.

## Review report (Codex final message)

## 1. Summary

The design accounts for the current awake/sleeping tree split and the sorted continuous and explosion candidates. I found one inconsistency that would break proxy identity when phase 2 stops imaging the kinematic tree.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§4, 5.2, 7.1, 13 | The final state classification derives the kinematic tree, and phase 2 explicitly says its proxy operations gain journal entries. But the §5.2 write route and §7.1 proxy entry definitions cover only the static and sleeping trees. After phase 2 removes the raw kinematic image, a rewind across kinematic proxy creation or destruction has no defined undo for its proxy id; the restored `shape->proxyKey` can disagree with the tree. The current four-tree proxy paths are in `src/broad_phase.c` and `src/broad_phase.h`. | Include the kinematic tree in the proxy create/destroy entry definitions and cold write route, with a clear phase-1 exception while its raw image is retained. |

## 3. Checked, no change

- Verified the awake/sleeping dynamic tree split, migration queue, moved-proxy handling, and step-end drain against `src/broad_phase.c`, `src/broad_phase.h`, `src/solver.c`, and `src/physics_world.c`.
- Verified sorted continuous candidates and shape-id-ordered explosion application against the live source.
- Verified the stated single-precision structure sizes with compile-time `sizeof` checks.
- Traced the listed sensor, manifold, material, island, solver-set, and hull ownership paths; found no further definite defect in their stated undo rules.
- Checked the restore and caller-contract treatment of events, pending between-step writes, and host configuration.

## 4. Proposed edits

Revise §§4, 5.2, and 7.1 so their final-design inventory and entry table include kinematic proxy identity. Keep §13’s phase-1 raw-image exception explicit.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (H) the kinematic tree's proxy identity has no entry once phase 2 drops its raw image:
  confirmed, applied.** §13's phase 2 derives the kinematic tree "with its proxy entries made",
  §5.4 says proxy identity is journaled through §7.1's entries, and the trees doc's §7 makes the
  proxy entries for the static and sleeping trees and for the kinematic tree once it is derived;
  but §7.1's proxy create and destroy rows and §5.2's trees route row named the static and
  sleeping trees only. Both now name the kinematic tree once it is derived (§13). §4's Derived
  row already says proxy identity is cold, not derived, for all three trees.

Raw log: the reviewer read no round record or changelog.

## Status

One High finding, in §7.1 and §5.2's trees row, an omission the trees doc had already closed on
its side. §5.2 produced findings in rounds 50, 51 and 53, not three running. Fix folded; a fresh
clean round and its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed; two entry rows and one route row widened to a
third tree (U-T1). #1 is not a consequence of any round's fix (U-T2); the reviewer read no round
record. Doc length 1177 -> 1177 lines; #1 added zero (U-T3). Over the split check (U-T6):
unchanged, one decision.

Round 53: 1 findings (1H-0M-0L), 1 applied, 0 declined
Series total: 115 findings (23H-56M-36L) across 53 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L) -> 1 (1H-0M-0L)

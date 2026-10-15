---
title: Codex review - the broad-phase trees under a rewind-only world history, round 6
target: docs/designs/20261015-004-history-ring-broad-phase-trees.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-004-history-ring-broad-phase-trees-round5.md
mode: full pass, confirmation round (U-P9) of round 5, same prompt and scope, on revision 5
---

# Round 6

## Exact request sent to Codex

Identical to round 5 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T19:31:58+10:00 to 2026-10-15T19:35:23+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ee7-756d-7ad3-8254-c22f44b8576a`.

## Review report (Codex final message)

## 1. Summary

The awake-tree image, derived-tree restore pass, and proxy undo rules are consistent with the cited parent sections and the current four-tree implementation. I found one incorrect description of the source’s caller path; it does not leave a restore case uncovered.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §7 | The document says `b3ResetProxy` handles body-type changes and writes the shape’s other fields before the proxy operation. In the live source, `b3Body_SetType` destroys and recreates proxies through `b3DestroyShapeProxy` and `b3CreateShapeProxy` (`src/body.c`, `src/shape.c`). Shape setters write their fields before calling `b3ResetProxy`; the reset function updates the AABBs (`src/shape.c`). | Correct the caller description. State that both paths reach the proposed broad-phase create and destroy choke points. |

## 3. Checked, no change

- The awake and sleeping dynamic trees, parked marked leaves, migration queue, and bulk placement match §3 and §5 (`src/broad_phase.c`, `src/dynamic_tree.c`).
- The awake image includes the tree fields needed to restore proxy IDs, free-list order, leaf boxes, and moved bits; the queue image preserves placement order (§5; `src/world_snapshot.c`).
- Reverse proxy undo restores the journaled trees’ IDs and allocation order. The static and kinematic restore pass restores their leaf boxes and moved lists; sleeping leaf boxes are covered by destroy and move entries (§6–§7).
- The pass follows the parent’s §9 order. The identified writes outside `src/broad_phase.c` are accounted for by the proposed choke-point migration (§7).

## 4. Proposed edits

Revise §7’s `b3ResetProxy` sentence and its parenthetical about field writes to describe the actual setter and body-type paths. No restore-rule change is needed.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (L) the caller description for `b3ResetProxy` and body-type changes: confirmed, applied.**
  `b3Body_SetType` destroys and recreates each proxy through `b3DestroyShapeProxy` and
  `b3CreateShapeProxy` (`body.c`), not through `b3ResetProxy`; the shape setters write their fields
  and then call `b3ResetProxy`, which updates the AABBs and recreates the proxy (`shape.c`). §7
  names both paths and corrects the parenthetical round 4 wrote; both reach the broad phase's
  create and destroy choke points, which is what the argument needs.

Raw log: the reviewer read no round record or changelog.

## Status

One Low finding on the confirmation round of round 5, so the clean round is not confirmed.
Category: a caller description wrong in two details (#1), in §7, which rounds 1 and 6 found (not
running). Not converged. Next: round 7, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Not a cluster against last round (5 had
none); the parenthetical corrected was round 4's (U-T2); the reviewer read no round record. Doc
length 272 -> 272 lines; added lines per fix: #1 two (U-T3). Under the split check (U-T6).

Round 6: 1 findings (0H-0M-1L), 1 applied, 0 declined
Series total: 8 findings (0H-2M-6L) across 6 rounds
Findings: 2 (0H-1M-1L) -> 2 (0H-1M-1L) -> 2 (0H-0M-2L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L)

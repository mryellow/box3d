---
title: Codex review - the broad-phase trees under a rewind-only world history, round 8
target: docs/designs/20261015-004-history-ring-broad-phase-trees.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-004-history-ring-broad-phase-trees-round7.md
mode: full pass, confirmation round (U-P9) of round 7, same prompt and scope, on revision 6
---

# Round 8

## Exact request sent to Codex

Identical to round 7 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T19:50:51+10:00 to 2026-10-15T19:54:16+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ef8-bd7f-7762-a6bd-cc7c80b51c65`.

## Review report (Codex final message)

## 1. Summary

The proposed image, proxy entries, and restore order cover the tree state needed for the first replayed step. I found one incorrect source claim in the awake tree’s memory discussion; it does not change the restore algorithm.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §5 | `nodeEnd` does not compact only in a rebuild. When migration empties a dynamic tree, `b3SettleTreeAfterRemove` resets `nodeEnd` to 2 (`src/broad_phase.c`). | State that rebuilds compact nonempty trees and migration resets an emptied tree. |

## 3. Checked, no change

- The current source has four trees, marked sleeping leaves parked in the awake tree, a persistent migration queue, and bulk placement.
- The awake image includes its proxy free list, leaf boxes, and moved bits. The static, kinematic, and sleeping restore paths account for their live leaf boxes and moved bits.
- Proxy create and destroy calls use the identified broad-phase paths. The tree writes currently outside `src/broad_phase.c` are identified for relocation.
- The proposed tree phase follows the parent’s §9 journal-walk and image-copy order.

## 4. Proposed edits

Replace §5’s “`nodeEnd` compacts only in a rebuild” sentence with wording that also describes the empty-tree reset in `b3SettleTreeAfterRemove`.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (L) `nodeEnd` also resets when a tree empties: confirmed, applied.** `b3SettleTreeAfterRemove`
  (`broad_phase.c`) sets `nodeEnd` to 2 for a tree the migration's removes emptied, and the rebuild
  returns at once for an empty tree (`dynamic_tree.c`), so the sentence's "only in a rebuild" was
  false for that case. §5 now names both. The bound is unchanged: the high-water term is still
  `proxyCapacity`, which never shrinks.

Raw log: the reviewer read no round record or changelog.

## Status

One Low finding on the confirmation round of round 7, so the clean round is not confirmed; the
series alternates clean and one Low (rounds 5 to 8). Category: a source claim with one case
missing (#1), in §5, which no earlier round found. Not converged. Next: round 9, same prompt, same
scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Not a cluster against last round (7 had
none); not a consequence of a previous fix (U-T2); the reviewer read no round record. Doc length
274 -> 273 lines; added lines per fix: #1 one (U-T3). Under the split check (U-T6).

Round 8: 1 findings (0H-0M-1L), 1 applied, 0 declined
Series total: 9 findings (0H-2M-7L) across 8 rounds
Findings: 2 (0H-1M-1L) -> 2 (0H-1M-1L) -> 2 (0H-0M-2L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L)

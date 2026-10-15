---
title: Codex review - the broad-phase trees under a rewind-only world history, round 2
target: docs/designs/20261015-004-history-ring-broad-phase-trees.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-004-history-ring-broad-phase-trees-round1.md
mode: full pass, same prompt and scope as round 1, on revision 2
---

# Round 2

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T18:54:10+10:00 to 2026-10-15T18:58:10+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ec4-d84d-7741-a0cd-365e49c07d21`.

## Review report (Codex final message)

## 1. Summary

The awake-tree image, proxy undo entries, and derived-tree restore pass cover the leaf boxes, ids, moved bits, and migration state I traced in the live source. Two document corrections remain: section 9 conflicts with the parent’s settled phase 0, and section 5 omits a field from the serializer writer it cites.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §9 | “Static and sleeping trees are … never imaged” and require proxy entries “in every phase” contradict the parent’s §13 phase 0. Phase 0 restores serialized whole-world images; `world_snapshot.c` serializes and loads all four trees. | State that proxy entries and the derived-tree pass begin in phase 1. Describe phase 0 as restoring all four serialized trees. |
| 2 | L | §5 | The stated list of fields written by `b3SerTree` omits `version`, which `world_snapshot.c` writes before the listed scalars. The omission does not appear to affect replay, but the source claim and image specification disagree. | Include `version` in the listed image fields, or explicitly say the history image omits this constant field and why. |

## 3. Checked, no change

- The live broad phase has separate awake and sleeping dynamic trees, a migration queue, and single and bulk placement. Parked marked leaves remain in the awake tree until the pair update consumes their marks.
- Proxy create, destroy, and sleeping-tree move entries account for the relevant ids, leaf boxes, category bits, and shape ids. Reverse undo preserves the journaled trees’ LIFO proxy-id sequence, including across capacity growth.
- The restore pass’s clear-and-remark sequence covers derived-tree moved bits. Static and kinematic leaf boxes track their `fatAABBs` entries; a sleeping bullet leaf can differ, so its recorded box is needed.
- The document identifies the current world-tree writes outside `broad_phase.c` as migration work for its proposed module boundary. The compound child tree is separate from the world’s four trees.

## 4. Proposed edits

Apply the two wording changes in findings 1 and 2. No change to the tree restoration algorithm is proposed.

## 5. Unresolved

None within the review scope.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the docs.

- **#1 (M) "never imaged ... in every phase" against phase 0: confirmed, applied.** The parent's
  §13 phase 0 restores a serialized whole-world image, and `world_snapshot.c` serializes and loads
  all four trees. §9 now says never imaged by the ring, required from phase 1, and that phase 0
  restores all four trees from the serialized image.
- **#2 (L) the image field list omits `version`: confirmed, applied.** `b3SerTree` writes
  `tree->version` first (`world_snapshot.c`), a fixed compatibility constant (`types.h`). §5 says
  the image is the writer's fields less that constant. The parent's series had applied this same
  correction to the sentence in its round 34; `git log -S` on the phrase shows: . The moved
  text did not carry it, so it is re-applied here.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one Medium and one Low, from two in round 1; neither in §7, which round 1 found.
Categories: a phase statement contradicting the parent's phase 0 (#1), and an inventory omission
(#2) that the parent's round 34 had already corrected in the same sentence. Not converged. Next:
round 3, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings did not cluster in a section that
produced findings last round (§5 and §9 against §7); not a consequence of round 1's fixes (U-T2);
the reviewer read no round record. Doc length 267 -> 267 lines; added lines per fix: #1 two, #2
zero (U-T3). Under the split check (U-T6).

Round 2: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 4 findings (0H-2M-2L) across 2 rounds
Findings: 2 (0H-1M-1L) -> 2 (0H-1M-1L)

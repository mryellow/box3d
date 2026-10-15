---
title: Codex review - the broad-phase trees under a rewind-only world history, round 4
target: docs/designs/20261015-004-history-ring-broad-phase-trees.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-004-history-ring-broad-phase-trees-round3.md
mode: full pass, same prompt and scope as round 1, on revision 4
---

# Round 4

## Exact request sent to Codex

Identical to round 3 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T19:03:42+10:00 to 2026-10-15T19:08:38+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ecd-9569-7921-a98d-4a196d12a873`.

## Review report (Codex final message)

1. **Summary**

The awake-tree image, proxy journal entries, and derived-tree restore pass cover the traced proxy identities, leaf boxes, moved bits, and migration queue for replay. I found one incorrect claim about where a sleeping-tree leaf can be moved in place; the proposed move entry still covers that case.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §6 | The document says an in-place sleeping-tree leaf write happens only through `b3ResetProxy` keeping its proxy. All current `b3ResetProxy` callers recreate the proxy. A public tree-query callback can instead call `b3Body_SetTransform`: its wake migration is queued until traversal ends, so the transform can move the proxy while it remains in the sleeping tree (`src/shape.c`, `src/body.c`, `src/broad_phase.c`, `src/physics_world.c`). | Name this path in §6. Retain the sleeping-tree proxy move entry; it records the old box needed to undo it. |

3. **Checked, no change**

- The source has four trees, parked marked leaves, an ordered migration queue, and single and bulk migration placement.
- The awake-tree image includes its proxy free list, leaf boxes, and moved bits; the queue image preserves placement order.
- Reverse proxy entries restore derived-tree ids and free-list order across ordinary and bulk creates, destroys, and capacity growth.
- The restore pass uses restored shape records for static and kinematic boxes and the captured moved-proxy lists for their marks. Sleeping-tree destroy and move entries preserve boxes that differ from `fatAABBs`.
- The proposed restore sequence is consistent with the cited parent §9 order. The identified in-place move is covered by the proposed entry, so it does not establish a replay-pair gap.

4. **Proposed edits**

Replace §6’s “only inside `b3ResetProxy` keeping the proxy” claim with the query-callback transform path, and describe the sleeping-tree move entry as covering any in-place `b3BroadPhase_MoveProxy` before migration drains.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (L) the in-place sleeping-leaf path misnamed: confirmed, applied.** `b3Body_SetTransform`
  wakes the body and then moves each of its proxies (`body.c`); a wake made inside a traversal is
  queued until the traversal ends (`b3BroadPhase_MigrateBody`, `broad_phase.c`), so from a public
  query callback the move lands on a leaf still in the sleeping tree. Every `b3ResetProxy` caller
  passes its `destroyProxy` flag through from a setter that recreates the proxy (`shape.c`), so the
  sentence's "`b3ResetProxy` keeping the proxy" named a path that does not exist. §6 names the
  transform path and states the move entry covers every in-place move of a sleeping-tree leaf,
  which is what `b3BroadPhase_MoveProxy` making it already gives.

Raw log: the reviewer read no round record or changelog.

## Status

One Low finding, from two in round 3. Category: a wrong name for a real path (#1), in §6, which
round 3 did not find. Not converged. Next: round 5, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Not a cluster against last round (§9, §8,
§12); not a consequence of a previous fix (U-T2); the reviewer read no round record. Doc length
271 -> 270 lines; added lines per fix: #1 one (U-T3). Under the split check (U-T6).

Round 4: 1 findings (0H-0M-1L), 1 applied, 0 declined
Series total: 7 findings (0H-2M-5L) across 4 rounds
Findings: 2 (0H-1M-1L) -> 2 (0H-1M-1L) -> 2 (0H-0M-2L) -> 1 (0H-0M-1L)

---
title: Codex review - the broad-phase trees under a rewind-only world history, round 1
target: docs/designs/20261015-004-history-ring-broad-phase-trees.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none (round 1)
mode: full pass, round 1 of the trees doc, on revision 1 (split from the parent's sections 5.4 and 7.4)
---

# Round 1

## Exact request sent to Codex

```
Review the design document docs/designs/20261015-004-history-ring-broad-phase-trees.md as an adversarial design reviewer for the Box3D physics engine. It decides how the broad phase's four trees (static, kinematic, awake dynamic, sleeping dynamic) come back after a rewind-only, bit-exact world history restores an earlier tick: which tree is imaged, which are derived, how leaf boxes, proxy ids, moved bits and the migration queue are restored exactly, and the restore pass. The image, the undo journal, the restore order and the public API are decided by docs/designs/20260929-002-rewind-only-history-ring.md, which this document cites.

Scope: the document's own content; docs/designs/20260929-002-rewind-only-history-ring.md only where this document cites it (its sections 3, 5, 7.1, 9, 12 and 13, which are settled for this review: judge whether this document's rules are consistent with them, not whether they are right); the source it cites (src/broad_phase.c, src/broad_phase.h, src/dynamic_tree.c, src/dynamic_tree.h, src/shape.c, src/body.c, src/joint.c, src/solver.c, src/physics_world.c, src/solver_set.c, src/world_snapshot.c, src/compound.c, src/core.c, include/box3d/); and the design documents it cites only where it cites them (docs/designs/20261013-001-preflight-awake-tree-split.md, docs/designs/20261015-001-sleeping-tree-marks-and-set-transform-wake.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it: the dynamic-body tree is split into an awake tree and a sleeping tree with a migration queue and a bulk placement; check the document against the code as it is now.

Pressure-test the design. Find real defects: claims about the code that are false; a tree, proxy id, moved bit, leaf box or migration-queue case under the awake and sleeping trees that the awake-tree image, the proxy entries and the restore pass together miss, so that the first replayed step would find different pairs than the original; a write to a world tree outside broad_phase.c, or a proxy create or destroy that bypasses the choke points; a proxy entry whose undo does not restore the id, tree, box, bits or shape its mutation changed, or that leaves a free list in a different state; a restore-pass step that reads a record or tree an earlier step already overwrote, or that the parent's section 9 order contradicts; a wrong or inconsistent number; contradictions between sections, or between this document and the cited sections of the parent. Treat existing primitives (b3DynamicTree, the broad phase's migration, the serializer's tree writer) as settled tools; judge what this design adds and the question each primitive is asked. Decisions recorded in section 12 are settled unless you have fresh source evidence that their stated reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, the open question in section 11, and the parent document's own rules except as this document relies on them. Do not review or follow any other design document except where this one cites it. Do not read docs/designs/reviews/ or any changelog.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-15T18:49:18+10:00 to 2026-10-15T18:53:03+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ec0-66fc-73e3-ab94-82100d93d133`.

## Review report (Codex final message)

## 1. Summary

The awake-tree image, derived-tree pass, and proxy journal cover the tree, leaf-box, moved-bit, and migration cases I traced through the current four-tree code. Two section 7 claims need correction before its write boundary is complete and reviewable.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §7 | `world_snapshot.c` directly replaces world trees in `b3DesTree`. Section 7 names the solver and static-rebuild writes to move into `broad_phase.c`, but gives this remaining writer no route through its proposed const tree interface. | Specify a broad-phase restore/load function for the snapshot reader and include it in the write-boundary migration. |
| 2 | L | §7 | The claim that every `shapes[id]` write follows its proxy operation is false. `b3ResetProxy` updates the shape and its fat AABB before proxy destroy, create, or move (`shape.c`). The self-contained destroy payload still supports the proposed undo. | Narrow the claim to `proxyKey` assignment and retain the destroy entry’s self-contained payload. |

## 3. Checked, no change

- The four-tree split, parked marked leaves, migration queue, bulk placement, and sleeping-tree height trigger match the live source.
- The awake image covers its proxy ids, free list, leaf boxes, and moved bits. Queue count and order are needed for replay.
- Reverse proxy create/destroy entries preserve id allocation order, including after discarded capacity growth. The sleeping move entry covers the leaf box that can differ from `fatAABBs`.
- The derived-tree pass runs after the parent’s §9 journal walk and image scatter. Restoring static and kinematic boxes, then clearing and reapplying their captured marks, is consistent with that order.
- Apart from the snapshot reader identified above, the current out-of-file world-tree writes are the solver and static rebuild paths that §7 explicitly calls for moving. The compound child tree is separate.

## 4. Proposed edits

Add `world_snapshot.c`’s tree load to §7’s required broad-phase write routes. Replace “in every caller the `shapes[id]` write follows the proxy operation” with a statement about the `proxyKey` write.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) the serializer's tree load writes trees directly: confirmed, applied.** `b3DesTree`
  (`world_snapshot.c`) frees and reallocates a tree's arrays and writes its scalars in place; §7
  moved the solver's and the static rebuild's writes into `broad_phase.c` and named no route for
  this one. §7 now makes it a load function of `broad_phase.c`, the one route by which a
  deserialize writes a tree, consistent with the parent's §5.2, which routes the serializer's
  reader through restore's writer.
- **#2 (L) "every `shapes[id]` write follows the proxy operation": confirmed as wider than the
  argument needs, applied.** `b3ResetProxy` writes the shape's fields and its fat AABB before it
  destroys and recreates the proxy (`shape.c`); only the `proxyKey` assignment follows the
  operation, and that is what makes the destroy entry self-contained. §7 says exactly that.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one Medium and one Low, both in §7's write boundary, on the first round of the
split-out doc; the round took four minutes with no resume. Not converged. Next: round 2, same
prompt, same scope.

Tripwire (U-T): no mechanism added or removed; one load function named for a writer that already
exists (U-T1). First round, nothing to cluster against (U-T2); the reviewer read no round record.
Doc length 264 -> 266 lines; added lines per fix: #1 two, #2 one (U-T3). Under the split check
(U-T6).

Round 1: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 2 findings (0H-1M-1L) across 1 round
Findings: 2 (0H-1M-1L)

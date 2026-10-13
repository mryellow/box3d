---
title: Review round 1 - island-scoped rollback facts
target: 20261012-001-island-scoped-rollback-facts.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

```
Review the document docs/designs/20261012-001-island-scoped-rollback-facts.md as an adversarial design reviewer for the Box3D physics engine. It is a collation of facts and analysis written for a discussion thread; section 10 analyses an idea from that thread, splitting the broad-phase into awake and non-awake trees, for a bit-exact rewind design.

Scope: the document's own content and the source it cites (src/solver_set.c, src/broad_phase.c, src/broad_phase.h, src/dynamic_tree.c, src/dynamic_tree.h, src/solver.c, src/physics_world.c, src/shape.c, src/body.c, src/joint.c, src/sensor.c, src/constraint_graph.c, src/island.c, src/contact_solver.c, src/id_pool.c, src/core.c, include/box3d/, shared/benchmarks.c), and the design documents it cites only where it cites them (docs/designs/20260929-002-rewind-only-history-ring.md sections 2, 5.4, 7.1, 7.2, 7.4 and test 8; docs/designs/20261011-001-preflight-tree-order-independent-solver.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the analysis. Find real defects: claims about the code that are false; readers of tree layout or proxy identity the document misses; a case where the awake-tree image plus the non-awake treatment in section 10.2 would not return the world to its state at T; a write to a non-awake tree that section 10.3 misses; a consequence for the rewind-only design that section 10.4 misses or states wrongly; a wrong or inconsistent number; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the journal entry kinds of the rewind-only design) as settled tools; judge what section 10 adds and the question each primitive is asked. The churn table in section 10.5 was measured with a scratch harness; the method is described there, and a finding about it must name what the method would get wrong.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, and proposals (the document makes none; do not ask it to decide). Do not review or follow any other design document except where this one cites it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-13T17:22:52+10:00 to 17:27:34+10:00. The stream
emitted the final message twice; the last copy is below.

## Review report (Codex final message)

## 1. Summary

Section 10 identifies the tree-order problem, but the proposed split leaves several paths that currently assume three trees. Its exact-layout journal also lacks information needed to undo a rebuild or the described operation log.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 10, 10.4 | Pair discovery in `src/broad_phase.c` searches the dynamic tree against itself, static, and kinematic. After a split, an awake body moving into a sleeping dynamic body would produce no pair; a moved proxy in the non-awake tree would not trigger the current search. Sorting discovered keys does not recover missing pairs. | Account for awake–non-awake and relevant non-awake pair searches, including their moved-bit triggers, in the split analysis. |
| 2 | H | 10 | `src/sensor.c` queries exactly three trees. A sleeping proxy moved to a fourth tree would disappear from sensor overlaps. Three-tree loops in `src/physics_world.c` likewise omit it from world queries and bounds. | Inventory membership readers separately from order readers and state which paths must include the fourth tree. |
| 3 | H | 10.2 | The proposed single entry carrying the previous array cannot undo an explicit rebuild to T. `b3DynamicTree_Rebuild` also rewrites `parents`, `proxies[].node`, `nodeEnd`, `pairFreeList`, and `dfsOrdered` (`src/dynamic_tree.c`). Restoring old nodes alone leaves proxy lookups and topology inconsistent. | Specify a bulk undo entry covering every persistent array and scalar the rebuild changes. |
| 4 | M | 10.2 | The cheaper remove-operation payload omits the removed leaf’s parent index. `b3RemoveLeaf` overwrites that parent with the sibling and `b3FreePair` overwrites the leaf pair, including its parent link (`src/dynamic_tree.c`). The listed payload cannot reconstruct the old topology. | Add the lost topology information to the payload, or withdraw the stated inverse and byte estimate. |
| 5 | M | 10.3 | The claim that non-awake writes all pass through the four listed broad-phase operations misses `b3DynamicTree_ClearMoved` on the static tree in `src/broad_phase.c`. It directly changes node flags during pair update. | Include this write in the cold-tree accounting and the exact-layout journal path. |
| 6 | L | 10.2 | “No tree’s layout is simulation-affecting” is too broad with the cited order changes. The cited preflight design explicitly retains layout-dependent call order for stateful `preSolve` or custom-filter callbacks (§2, A1), consistent with the rewind design’s exception (§2). | Qualify the statement with that existing callback exception. |
| 7 | L | 10.5 | “Shapes equal bodies in every scene but `trees100`” is false: `washer` creates 40 shapes on one kinematic body, and `rain` creates two shapes on each static grid body (`shared/benchmarks.c`). Thus the sentence cannot justify one move per flipped body across those scenes. The described harness counts flipped bodies *and their shapes*; this finding does not challenge its reported counts. | Narrow the sentence to the bodies that actually flipped, if supported by the measurements. |

## 3. Checked, no change

- `src/solver_set.c` currently changes no proxies on sleep or wake; a split would require transferring their shapes.
- The proxy key has two tree bits, and `src/shape.c` currently uses the selected code when computing the fat AABB margin.
- Current pair discovery sorts emitted shape-pair keys; CCD and explosion have the cited traversal-order dependencies.
- The rewind design’s proxy-id journal (§7.4) and sleep/wake cases (§7.1 and test 8) support the document’s identity and transition analysis.
- The `sleep` and `trees100` churn means are consistent with the stated per-shape counting method.

## 4. Proposed edits

Revise sections 10–10.4 to cover fourth-tree membership readers, pair generation, complete rebuild undo state, and static moved-flag clearing. Correct the operation-log payload and qualify the layout-independence and shape-count statements.

## 5. Unresolved

The scratch harness output is unavailable here, so the measured churn values were assessed for method and internal consistency, not independently reproduced.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** `b3UpdateBroadPhasePairs` (`broad_phase.c`) seeds cross passes of the dynamic
   tree against the static and kinematic trees and a self pass of the dynamic tree, all gated by
   `b3TestPair` on either node's moved bit. A fourth tree gets no pass today. The doc's "under a
   split both query the non-awake tree" covered CCD and Explode only. Fix: state in §10 that pair
   discovery gains an awake-against-non-awake cross pass driven by moved bits on either side, and
   that a moved non-awake proxy is found against other non-awake proxies only by a non-awake self
   pass, which is the engine's choice.
2. **Adopted (H).** `sensor.c` queries `trees + 0/1/2`; `physics_world.c` loops `i <
   b3_bodyTypeCount` for overlap, cast, bounds and draw. Membership readers, none order-dependent
   beyond §5. Fix: one bullet in §10 naming them as readers that include the fourth tree.
3. **Adopted (H).** `b3DynamicTree_Rebuild` -> `b3BuildTree`, `b3BumpPair`, `b3CopySubtree`,
   `b3PlaceLeaf` write `parents`, `proxies[].node`, `nodeEnd`, and the rebuild sets `pairFreeList`
   and `dfsOrdered`. Fix: the explicit-rebuild entry carries the tree's arrays and scalars, not
   "the whole previous array".
4. **Adopted (M).** `b3RemoveLeaf` copies the sibling over the parent and `b3FreePair` overwrites
   `parents[pair]` with the free-list link, so the leaf's parent index is lost; `b3FreeProxy` keeps
   `userData`/`categoryBits` but a later `b3AllocateProxy` of the slot memsets them, and undoing
   that alloc does not restore them. Fix: the remove payload gains the parent index and the proxy
   slot; estimate raised to about 64 B; "whether the pair was freed or the node end moved" moves
   to the insert payload where it belongs.
5. **Adopted (M).** `b3DynamicTree_ClearMoved( bp->trees + b3_staticBody )` runs in every pair
   update, a node-flag write outside the four listed operations; under a split the same runs on the
   non-awake tree. Moved bits decide which pairs are searched, so they are state. Fix: §10.3 adds
   the moved-bit set and clear writes, all inside `broad_phase.c`.
6. **Adopted (L).** Preflight A1 keeps stateful `preSolve` and custom filters layout-dependent.
   Fix: qualify the sentence.
7. **Adopted (L).** Harness totals: `rain` 1,296 bodies, 1,512 shapes flipped. Fix: the sentence
   states shapes per flipped body (1 except `rain` 1.2 and `trees100` 22) and that the move counts
   are shapes.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 1: 7 findings (H3-M2-L2), 7 applied, 0 declined.
- Findings: 3/2/2 (round 1) -> pending
- Tripwire: fixes add or remove a named mechanism: no new kind; the physical node journal's
  coverage widens (moved bits, rebuild payload) and the split's pair discovery gains a cross pass
  as a stated consequence, not a rollback mechanism. Clustering: none (first round). Reviewer read
  of round records or changelog: none in the raw log. Doc length: 351 lines before; fixes are one
  bullet or one clause each except the two-bullet pair-discovery and membership additions.

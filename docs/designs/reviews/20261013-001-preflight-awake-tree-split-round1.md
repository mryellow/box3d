---
title: Review round 1 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

```
Review the design document docs/designs/20261013-001-preflight-awake-tree-split.md as an adversarial design reviewer for the Box3D physics engine. It is a preflight for a stand-alone engine change, splitting the dynamic-body broad-phase tree into awake and non-awake trees, with one section on what the split gives a bit-exact rewind design.

Scope: the document's own content and the source it cites (src/broad_phase.c, src/broad_phase.h, src/dynamic_tree.c, src/dynamic_tree.h, src/solver_set.c, src/solver.c, src/physics_world.c, src/shape.c, src/body.c, src/joint.c, src/contact.c, src/island.c, src/sensor.c, src/core.c, include/box3d/, shared/benchmarks.c), and the design documents it cites only where it cites them (docs/designs/20261011-001-preflight-tree-order-independent-solver.md; docs/designs/20260929-002-rewind-only-history-ring.md sections 5.4, 7.1, 7.2, 7.4 and test 8). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false; a reader or writer of the dynamic tree, of proxy keys or of moved bits that the changes B0 to B4 miss; a step ordering or threading hazard the split creates that section 2 does not state; a case where the awake-tree image plus the non-awake treatment in section 5.2 would not return the trees to their state at T; a wrong or inconsistent number; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the pair set, the journal entry kinds of the rewind design) as settled tools; judge what this design adds and the question each primitive is asked. The churn table in section 4 was measured with a scratch harness; the method is described there, and a finding about it must name what the method would get wrong.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, the decisions section 6 assigns to the owner, and decisions the document assigns to a later Implementation Prompt. Do not review or follow any other design document except where this one cites it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-13T17:58:08+10:00 to 18:06:32+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split misses two fixed-index readers, and its rule for creating non-awake proxies can lose pair discovery. The awake-tree image also scales with historical tree allocation after bodies sleep, contrary to the stated per-awake-proxy cost.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2.1, 2.3 | Bullet CCD queries only the dynamic tree in `src/solver.c`. After the split, a bullet cannot find sleeping dynamic bodies or their sensors in the non-awake tree. | Query both dynamic trees for bullets, including both passes required by the cited tree-order design. |
| 2 | H | 2.3 | `b3World_Explode` queries only the dynamic tree in `src/physics_world.c`. Collecting hits before waking does not make sleeping bodies visible. | Query both trees, then process the combined hit list after both traversals. |
| 3 | H | 2.2 | Sleep creates an unmarked non-awake proxy and discards a pending moved bit. For example, `b3Body_SetTransform` can mark an awake proxy, followed by `b3Body_SetAwake(false)` before pair update; the next step then misses its new overlaps. The step can also mark a proxy before sleeping its island. | Carry each proxy’s pending moved state across sleep. |
| 4 | H | 5.1 | The image is not bounded by **current** awake proxies. `src/dynamic_tree.c` retains `proxyCapacity` and `nodeEnd` after removals; rebuild returns immediately for an empty tree. A formerly large awake tree can therefore cost roughly 96 B per former proxy every capture even when none remain awake. | Account for this high-water cost and specify compaction or an exact sparse image. |
| 5 | M | 2.1, 2.2 | The blanket “non-awake create is not marked moved” rule also suppresses pairs for a shape created on a sleeping body. Today `b3BroadPhase_CreateProxy` marks every non-static create, even without `forcePairCreation`; `b3ResetProxy` also recreates proxies to find pairs. | Give sleep migration an unmarked path while marking ordinary non-awake creates and resets. |
| 6 | M | 2.0 | Code 3 still fails the current tree-code guards: `b3BroadPhase_CreateProxy` requires `< b3_bodyTypeCount`, and `b3CreateShapeProxy` asserts the same of its returned key. B0 names the margin and solver-set mappings but misses these checks. | Define a four-code proxy range and update both guards without changing the three-value body-type range. |
| 7 | M | 1, 2.1 | The unconditional O(awake) step claim fails on a sleeping proxy write. B1’s non-awake self pass uses `b3GatherMovedSiblings`, which scans `nodeEnd` for the entire non-awake tree when one leaf is moved. `src/sensor.c` also processes every sensor each step. | State the steady-state bound precisely and account for sleeping-tree writes and the sensor pass. |

## 3. Checked, no change

- The two-bit proxy key has a spare code; dynamic fat-AABB margins can be used for that code.
- The proposed awake/self and cross-tree pair combinations cover the existing pair classes, provided ordinary non-awake creates and pending moves retain their moved state.
- Contact creation can place a pair of sleeping bodies in the disabled set; the pair set culls contacts rediscovered after wake.
- The rebuild can overlap begin-touch waking today, and the explosion callback currently wakes during traversal.
- The cited rewind design journals proxy identity and provides a restore-time fat-AABB and moved-bit pass. Those remain necessary for the non-awake tree when its layout is derived.
- The approximately 100 B figure matches the tree’s array element sizes **when allocation tracks live proxies**; the finding concerns retained allocation after sleep.

## 4. Proposed edits

Revise B1 and B3 to name both fixed-index queries. Revise B2 to distinguish sleep migration from ordinary proxy creation and to preserve pending moved bits. Add the code-3 guards to B0. Revise §1 and §5.1 with the actual complexity and retained-capacity bounds.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** `b3SolveContinuous` (`solver.c`) queries the static tree and, for bullets, the
   kinematic and dynamic trees by fixed index. Fix: B3 states that the bullet pass queries both
   dynamic trees.
2. **Adopted (H).** `b3World_Explode` queries `trees + b3_dynamicBody` only. Fix: B3 states that it
   queries both dynamic trees and processes the combined hit list after both traversals.
3. **Adopted (H).** Finalize marks proxies moved (`b3BroadPhase_MarkProxyMoved`, `solver.c`) in the
   same solve whose sleep pass later moves islands, and `b3Body_SetTransform` marks a proxy
   between steps before a `b3Body_SetAwake( false )`. A destroy-and-unmarked-create at sleep drops
   that bit. Fix: B2 carries the awake leaf's moved bit into the non-awake create.
4. **Adopted (H).** `b3FreeProxy` and `b3FreePair` return slots to free lists; `proxyCapacity`
   never shrinks and `nodeEnd` compacts only in a rebuild, which runs only on a moved root or an
   unordered tree and returns at once for an empty one (`dynamic_tree.c`). Fix: 5.1 states the
   image is O(high-water awake proxies), with the compaction a rebuild gives `nodeEnd` and none for
   `proxies`, and names it as a weakening of the rewind design's requirement 2.
5. **Adopted (M).** `b3BroadPhase_CreateProxy` marks every non-static create, and `b3ResetProxy`
   recreates to force pair creation. Fix: B2 distinguishes sleep migration (unmarked, bit carried)
   from ordinary creates in the non-awake tree (marked as today).
6. **Adopted (M).** `b3BroadPhase_CreateProxy`/`DestroyProxy` assert `proxyType <
   b3_bodyTypeCount` and `b3CreateShapeProxy` asserts the same of the returned key (`shape.c`).
   Fix: B0 names a four-code proxy range and both guards.
7. **Adopted (M).** `b3GatherMovedSiblings` scans every pair below `nodeEnd` of the tree it is
   given; a non-awake self pass is O(non-awake tree) on a step with a sleeping-proxy write, and
   the sensor pass visits every sensor (`sensor.c`). Fix: §1 states the bound as O(awake) on steps
   with no sleeping-proxy write, names the cost of such a write, and leaves the sensor pass as is.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 1: 7 findings (H4-M3-L0), 7 applied, 0 declined.
- Findings: 4/3/0 (round 1) -> pending
- Tripwire: fixes add or remove a named mechanism: no; B2 gains a rule (carry the moved bit) and
  B3 two query additions. Clustering: none (first round of this doc). Reviewer read of round
  records or changelog: none in the raw log. Doc length: 256 lines before; each fix is one clause
  or one bullet.

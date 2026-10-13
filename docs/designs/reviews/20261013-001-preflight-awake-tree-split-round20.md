---
title: Review round 20 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round19.md
mode: full pass
---

# Round 20

## Exact request sent to Codex

```
Review the design document docs/designs/20261013-001-preflight-awake-tree-split.md as an adversarial design reviewer for the Box3D physics engine. It is a preflight for a stand-alone engine change, splitting the dynamic-body broad-phase tree into awake and non-awake trees. Section 5 is a pointer to a separate note and is out of scope.

Scope: the document's own content and the source it cites (src/broad_phase.c, src/broad_phase.h, src/dynamic_tree.c, src/dynamic_tree.h, src/solver_set.c, src/solver.c, src/physics_world.c, src/shape.c, src/body.c, src/joint.c, src/contact.c, src/island.c, src/sensor.c, src/core.c, include/box3d/, shared/benchmarks.c), and the design documents it cites only where it cites them (docs/designs/20261011-001-preflight-tree-order-independent-solver.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false; a reader or writer of the dynamic tree, of proxy keys or of moved bits that the changes B0 to B4 miss; a step ordering or threading hazard the split creates that section 2 does not state; a wrong or inconsistent number; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the pair set) as settled tools; judge what this design adds and the question each primitive is asked. The churn table in section 4 was measured with a scratch harness; the method is described there, and a finding about it must name what the method would get wrong.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, the decisions section 6 assigns to the owner, and decisions the document assigns to a later Implementation Prompt. Do not review or follow any other design document except where this one cites it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

The prompt changed with the U-T6 split before this round: the rewind design left the cited-doc
list, §5 is stated as a pointer out of scope, and the rewind restore clause left the defect list.
Same scope otherwise.

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:06:02+10:00 to 21:10:26+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`. The answer uses numbered bold headings.

## Review report (Codex final message)

1. **Summary**

The proposed pair passes, proxy-key handling, and migration timing match the cited source paths. I found one correctness gap in the maintenance plan for the new non-awake tree.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | 2, B4 | The non-awake tree can grow without a height bound and is never rebuilt by the step. Its queries and pair walks use fixed 512-entry stacks; when full, release builds skip branches. Measuring height in the PR cannot prevent missed overlaps under later churn, so the claim that this adds no new reliance is unsound. | Specify an overflow-safe traversal or a maintenance rule with a guaranteed stack bound for the non-awake tree. |

3. **Checked, no change**

- The four-code proxy key fits the existing two-bit field while body types remain three.
- The proposed self and cross passes cover awake, sleeping, kinematic, and static pair combinations, including static compound expansion.
- Carrying the leaf’s moved bit through migration preserves pending pair discovery; shape-keyed pair-set entries do not require proxy-ID stability.
- Joining the tree task and draining queued wakes before finalize puts sleeping proxies in the tree queried by bullet CCD.
- The cited shape, body, joint, sensor, query, cast, draw, counter, and memory-report paths were accounted for. The churn table’s stated shape counts are internally consistent with the cited benchmark scene construction.

4. **Proposed edits**

Revise B4’s stack-safety claim and add the required correctness mechanism for an unbounded non-awake tree.

5. **Unresolved**

The mechanism for preventing skipped branches remains unspecified.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

1. **Adopted in part (M), re-verified (U-V7).** Third raise of the stack reliance (rounds 15, 18).
   Fresh point: the static tree is not churned by sleep and wake and the non-awake tree is, so
   "inherits the same reliance and adds no new one" overstates the parallel. The premise of the
   decline stands: a skipped branch needs a 512-entry stack to fill, a height over 500, and
   rotation-balanced inserts do not produce that. Fix: B4 states the churn difference and the
   existing knobs: the root's `height` field is O(1) and `b3World_GetCounters` reports tree height
   (B0 includes the fourth tree), `b3World_RebuildSleepingTree` resets it, and every traversal
   asserts when its stack is full in a validation build. No overflow-safe traversal or height bound
   is added (U-P11); the reasoning is in the preflight's Provenance, where the rewind note already
   carries the rewind side of it.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 20: 1 finding (H0-M1-L0), 1 applied in part, 0 declined outright.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: the same finding three times in
  six rounds (U-T5: the rationale was not landing; the fix states the difference the reviewer
  sees). Reviewer read of round records or changelog: none in the raw log. Doc length: 420 -> 301
  (split) -> 301 lines.

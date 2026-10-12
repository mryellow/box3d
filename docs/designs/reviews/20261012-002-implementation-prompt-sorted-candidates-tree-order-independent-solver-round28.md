---
title: Review round 28 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round27.md
mode: full pass
---

# Round 28

## Exact request sent to Codex

```
Review the document docs/designs/20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md: the instructions an implementing agent follows to land that design's three changes (A0 test helper, A1 CCD candidate order, A2 Explode order) as three commits, with the test mechanism, fixtures, test-to-requirement mapping, verification commands and PR/closure requirements. The design decides what changes and why; the Implementation Prompt decides how it lands and how it is shown to work. Test mechanism, fixtures and mapping are in scope here, not out of scope as they are for a design doc.

Scope: the Implementation Prompt's own content, the design it implements (read it in full; it is binding and its section 5 Provenance decisions are settled), and the source both cite (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, src/solver_set.c, src/container.h, include/box3d/, test/, shared/, benchmark/, docs/simulation.md, CONTRIBUTING.md, CMakeLists.txt, CMakePresets.json). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the Implementation Prompt makes about it.

Pressure-test the prompt. Find real defects: claims about the code or build that are false; an instruction that contradicts the design or re-decides something the design settled; a fixture that cannot do what the prompt says (cannot be made to fail before the change, cannot pass after it, cannot observe what it asserts through the API named, or asserts something the design does not require); a witness or hash that does not measure what the prompt claims; a helper contract with a hazard the prompt does not state (world locking, recording, sleep, multi-worker, allocation, assertion builds); a command that does not work in this repository; a mapping row whose evidence does not establish its clause; a missing instruction without which the implementer must guess; contradictions between sections. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h, b3Hash, the test runner) as settled tools; judge what the prompt adds and the question each primitive is asked.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, other design documents, and the design's own decisions (cite them; do not re-open them). One such decision, stated here because it governs a fixture: the design's section 2 A1 prescribes the seeded fuzz's evidence as hashes compared over many seeds under the A0 perturbation with the count of seeds that failed before the change recorded; the Implementation Prompt accordingly witnesses the corridors inside each seed and not the field's own CCD queries, and the absence of a field witness is settled and not a finding.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0,
2026-10-12T11:25:44+10:00 to 11:29:38+10:00. Against revision 28, unchanged since round 27. The
request is the rounds 1 to 27 prompt plus one sentence in its out-of-scope paragraph stating the
settled fuzz-evidence decision (U-P13, recorded in round 27); scope is otherwise unchanged (U-P4).
U-R9: the target is untracked so there is no diff; no change since the last read.

## Review report (Codex final message)

## 1. Summary

The implementation steps and most verification mechanisms match the canonical design and live code. I found one gap that can invalidate the callback-order test and one inaccurate statement about worker thresholds.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, §4 A0–A1 | The prompt treats nonoverlapping `world->fatAABBs` as proof that the corridor sphere receives custom-filter calls from CCD alone. Pair discovery tests dynamic-tree leaf bounds, which can remain larger than the current fat AABBs (`src/broad_phase.c`, `src/dynamic_tree.c`). An unwitnessed pair-discovery call could enter the log and make the asserted order misleading. | In the before-step hook, assert that the sphere’s **tree proxy AABB** is disjoint from every other relevant proxy AABB. Use that assertion to justify the CCD-only log in §4 and §6. |
| 2 | L | §2 | “CCD on several workers only above both counts” is false. Nonbullet CCD runs in finalize blocks of at least 16 awake bodies; bullet CCD runs separately in blocks of at least 8 bullets (`src/solver.c`, `src/parallel_for.c`). The proposed 49-body, 25-bullet fixture is sufficient for four blocks in both stages, but neither threshold is required for the other stage to run on several workers. | State the thresholds independently and retain the fixture counts. |

## 3. Checked, no change

- The separate CCD tree queries, running fraction, sensor limit and event path agree with `src/solver.c` and `src/sensor.c`.
- The tree witness relies on callback order and, with an exact candidate set, follows `src/dynamic_tree.c`; the post-step witness timing accounts for the dynamic and kinematic rebuilds.
- The sleeping-body Explode fixture can observe wake order and velocities through the named APIs and internal awake set (`src/body.c`, `src/solver_set.c`, `src/physics_world.c`).
- The array lifecycle, sort macro, test filter, CMake options and benchmark command match the repository.

## 4. Proposed edits

Apply the proxy-AABB assertion to the corridor contract and callback test, then update their mapping row. Correct the worker-threshold sentence in §2.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Both adopted.

1. **M, tree leaf bounds are not `world->fatAABBs` (adopted).** Confirmed for bullets: finalize
   reassigns a regular body's leaf to the new fat AABB (`b3DynamicTree_MarkProxyMoved`,
   `src/dynamic_tree.c`), but the bullet stage re-bounds a bullet's leaf only when the leaf no longer
   contains the fat AABB (`src/solver.c`), so a bullet's leaf can stay larger than its array entry.
   The corridor fixture never reaches that case, since its sphere is a fresh proxy and its boxes rest,
   but the precondition is now asserted from the tree's leaf bounds through `shape->proxyKey` and
   `b3DynamicTree_GetAABB`, which is what pair discovery tests; §2 states the relation.
2. **L, "only above both counts" (adopted).** Confirmed: finalize blocks and bullet blocks are
   independent stages (`src/solver.c`, `src/parallel_for.c`). The thresholds are stated separately.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 28: 2 findings (H0-M1-L1), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2 -> 0/2/1 -> 0/1/0 -> 0/1/1. Series total 83 (H7-M57-L19). Not converged.

Tripwire. U-T1: no mechanism added or removed; an assertion reads the tree bound instead of the
array. U-T2: finding 1 is a consequence of the round 17 fix (the `world->fatAABBs` assertion); the
fuzz witness point did not recur under the scope sentence. Finding 2 is in a §2 fact last touched
in round 18. Each fix is a clause. U-T3: 477 lines before the fix; the after count is recorded in
the changelog. U-T6: over the split check; one decision, recorded in round 6.

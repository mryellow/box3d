---
title: Review round 1 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

```
Review the design document docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md as an adversarial design reviewer for the Box3D physics engine. The document decides three changes, A0, A1 and A2, that make continuous collision and b3World_Explode independent of broad-phase tree layout by evaluating tree candidates in ascending shape-id order.

Scope: the document's own content and the source it cites (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, include/box3d/, test/, docs/simulation.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false, orderings or results that still depend on tree layout after A1 and A2, hazards in A0, A1 and A2 that the document does not state (threading, per-worker scratch reuse, re-entrancy, callbacks that touch the world, sort ties), wrong or inconsistent statements, contradictions between sections, and consequences of the changes (behaviour, threading, determinism, cost) that the document misses. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h) as settled tools; judge what the design adds and the question each primitive is asked.

Decisions recorded in the document's section 5 (Provenance) are settled and binding unless you show fresh source evidence that their reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, other design documents, and decisions the document assigns to a later Implementation Prompt.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:09:20+10:00 to 06:12:30+10:00.

## Review report (Codex final message)

## 1. Summary

A1 and A2 would remove tree visit order from the CCD and explosion callbacks they change. The document’s broader tree-layout guarantee does not hold for thread-safe, stateful user callbacks: pair and sensor filters still run before sorting, and CCD callbacks can run concurrently.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §1, §2 A1 | Pair keys are sorted **after** the custom filter runs during pair discovery (`src/broad_phase.c`). Sensor overlaps are likewise sorted **after** filtering during tree traversal (`src/sensor.c`). A thread-safe filter whose answer depends on call order can therefore select different contacts or overlaps for different tree layouts after A1 and A2. | Either sort candidates before these filters too, or explicitly limit the guarantee to callbacks whose decisions are independent of call order. |
| 2 | M | §2 A1 | “A layout-independent call sequence” is true only within each sorted CCD query. Fast bodies are processed in parallel in `src/solver.c`, so calls to a shared, thread-safe stateful `preSolve` or custom filter can still interleave differently across workers. | State the per-query limit and the requirement for order-independent callback decisions, or provide a design for global callback ordering. |
| 3 | L | §5 | “The first eight admitted in id order” misstates the proposed sensor selection. `src/solver.c` keeps one sensor count across a fast body’s shapes and queries; it visits static, then kinematic, then dynamic trees, sorting only *within* each query. A higher-id static sensor can fill a slot before a lower-id dynamic sensor. | Describe the actual fixed order: body shape order, tree order, then ascending id within each query. Keep the settled eight-hit rule. |
| 4 | M | Opening; §2 A1 | “No added time-of-impact work” overstates the cost result. The number of TOI calls need not increase, but changing their order changes the running fraction passed to each call and can increase its work. A query can also return many candidates, making sorting and retained per-worker capacity substantial (`src/solver.c`, `src/dynamic_tree.c`). | Say “no additional TOI calls or tree traversals”; measure iteration counts, dense-query sort cost, and per-worker memory high-water alongside the planned benchmark. |

## 3. Checked, no change

- `src/solver.c` confirms the running fraction, eight-hit buffer, final strict sensor-hit filter, and per-query static/kinematic/dynamic order described in the document.
- `src/physics_world.c` confirms explosions query only the dynamic tree, wake during the callback, and hold `world->locked` through the query. Sorting unique shape ids creates no sort-tie choice.
- `src/dynamic_tree.c` confirms a full rebuild preserves proxy identities and leaf bounds while changing layout; the A0 visit-order witness is necessary because a rebuild need not produce a different visit order.
- `src/solver.c` joins the tree rebuild before CCD. The proposed per-worker scratch reuse fits the sequential queries handled by each task context; world access from filter and pre-solve callbacks is already prohibited by `include/box3d/` and `docs/simulation.md`.
- The cited pre-solve sentence in `docs/simulation.md` does conflict with the live CCD callback, so the proposed documentation correction is warranted.

## 4. Proposed edits

Qualify the opening guarantee and §1’s “everything else” claim using finding 1. Narrow A1’s callback-order claim to each query, correct §5’s sensor ordering, and replace the TOI cost claim with the measured scope in finding 4.

## 5. Unresolved

Whether the PR intends to cover thread-safe callbacks with order-dependent state determines whether finding 1 needs more implementation work or a narrower guarantee.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (H) | Confirmed. `src/broad_phase.c` (line 178 area) and `src/sensor.c` (line 135 area) call `customFilterFcn` during traversal, before the key sort and the overlap sort. A stateful filter stays layout-dependent. | Applied. The guarantee is stated for callbacks that depend only on their arguments; §1 names the custom filter as the exception. Not a fork: the same limit 20261011-001 already states in its A1 caveats, and a layout-independent filter would need sorting before filtering in two more places, which is outside this PR. |
| 2 (M) | Confirmed. `b3SolveContinuous` runs per body on parallel workers. | Applied in the A1 caveats. |
| 3 (L) | Confirmed. `sensorCount` is one buffer across a body's shapes and its three queries. | Applied in Provenance: body shape order, tree order, ascending id within a query. |
| 4 (M) | Confirmed in part. The call count is unchanged; the running fraction each call sees, and its work, can differ. | Applied: "no additional TOI calls"; measurement list extended (sort cost on a dense query, iteration counters, per-worker high-water). The "prunes no better on average" sentence was unverified and is replaced. |

Note for 20261011-001: finding 1 applies there too. Its §1 says the rest of the step is "already order-independent" without the custom-filter exception, and its A1 caveats cover only CCD. That document converged without this finding; it is not edited here (U-I7).

## Status

1 round: 1/2/1. Series total 4 (H1-M2-L1). Document 172 -> 180 lines; fixes added about 8 lines. No named mechanism added or removed. Findings are in A1 and Provenance only.

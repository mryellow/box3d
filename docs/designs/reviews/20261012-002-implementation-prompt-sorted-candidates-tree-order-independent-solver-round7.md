---
title: Review round 7 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round6.md
mode: full pass
---

# Round 7

## Exact request sent to Codex

```
Review the document docs/designs/20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md: the instructions an implementing agent follows to land that design's three changes (A0 test helper, A1 CCD candidate order, A2 Explode order) as three commits, with the test mechanism, fixtures, test-to-requirement mapping, verification commands and PR/closure requirements. The design decides what changes and why; the Implementation Prompt decides how it lands and how it is shown to work. Test mechanism, fixtures and mapping are in scope here, not out of scope as they are for a design doc.

Scope: the Implementation Prompt's own content, the design it implements (read it in full; it is binding and its section 5 Provenance decisions are settled), and the source both cite (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, src/solver_set.c, src/container.h, include/box3d/, test/, shared/, benchmark/, docs/simulation.md, CONTRIBUTING.md, CMakeLists.txt, CMakePresets.json). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the Implementation Prompt makes about it.

Pressure-test the prompt. Find real defects: claims about the code or build that are false; an instruction that contradicts the design or re-decides something the design settled; a fixture that cannot do what the prompt says (cannot be made to fail before the change, cannot pass after it, cannot observe what it asserts through the API named, or asserts something the design does not require); a witness or hash that does not measure what the prompt claims; a helper contract with a hazard the prompt does not state (world locking, recording, sleep, multi-worker, allocation, assertion builds); a command that does not work in this repository; a mapping row whose evidence does not establish its clause; a missing instruction without which the implementer must guess; contradictions between sections. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h, b3Hash, the test runner) as settled tools; judge what the prompt adds and the question each primitive is asked.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, other design documents, and the design's own decisions (cite them; do not re-open them).

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0,
2026-10-12T08:52:40+10:00 to 08:54:47+10:00. Same prompt as rounds 1 to 6 against revision 7. U-R9: a
read of the sections revision 7 changed found no mangling.

## Review report (Codex final message)

## 1. Summary

The implementation instructions largely follow the canonical design, but three parts of the proposed evidence overclaim what they measure. The benchmark’s per-step timing files mix data across runs, and two CCD tests do not establish that the specific candidate orders named in their mapping rows changed before the fix.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | Prompt §§2, 7 | The proposed per-scene, per-thread `transforms` comparison is invalid. `benchmark/main.c` initializes its profile minima once, then carries them across thread counts and scenes before writing each `-s` file. Later files can therefore contain minima from earlier scenes or thread counts. | Run each scene and worker count in a separate benchmark process for this metric, or remove the metric and use the suite CSV plus the scratch harness. |
| 2 | M | Prompt §§4 A1, 6 | `ContinuousCandidateOrderTest` witnesses the corridor’s static-tree query, but its separately asserted **field hash** depends on the field bodies’ CCD queries. A changed corridor order does not establish a changed field-query order; the prompt even permits zero pre-change corridor hash divergences. This falls short of design §2 A0’s witness check for the query relied on. | Witness an exact field-body query whose order differs, or make the corridor itself a required pre-change divergence and use its hash as the order-sensitive evidence. |
| 3 | M | Prompt §§4 A1, 6 | The callback test requires a different order for the **whole** eight-box corridor, while `preSolve` sees only boxes on the sphere’s path. The differing permutation may involve only off-path boxes. Its pre-change failure can then come entirely from the custom-filter log, leaving the mapping’s `preSolve` order claim untested. | Require a differing pre-change order among at least two boxes that reach `preSolve`, then verify that their logs become identical and ascending after A1. |

## 3. Checked, no change

- The A0 rebuild and witness APIs, lock and recording restrictions, and test target registration match the cited source.
- The A1 collection, per-query sorting, worker-array lifecycle, CCD gates, and sensor event path match the design and source.
- The sensor fixture’s eight-hit limit and the Explode fixture’s velocity and awake-set observations use reachable state.
- The build and test target names, test filtering, benchmark flags, and PR template match this repository.

## 4. Proposed edits

Revise the two A1 fixtures and their §6 mapping rows to require evidence from the candidate subsets they claim to test. Revise §7’s benchmark instructions so each reported per-step `transforms` value comes from an isolated scene and worker-count run.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All three findings checked against the live tree (U-V2). All adopted.

1. **M, per-step benchmark minima (adopted).** Confirmed: `benchmark/main.c` fills its per-step
   profile array once before the scene loop and `MinProfile` only lowers entries, so each `-s` file
   holds minima over every run, thread count and scene so far. The per-step `transforms` metric is
   removed; the suite benchmark reports the csv totals as a no-regression check, and the scratch
   harness supplies the stage columns.
2. **M, fuzz evidence attribution (adopted).** Confirmed from the fixture: the corridor's witness
   says nothing about the field bodies' queries, and the corridor's own hash was allowed to never
   diverge. The corridor hash is now the order-sensitive assertion tied to the witnessed query, with a
   required pre-change divergence in at least one seed; the field hash stays as the design's broad
   fuzz with its pre-change count recorded.
3. **M, callback test's `preSolve` evidence (adopted).** Confirmed: `preSolve` reaches only on-path
   boxes, and a witness permutation among off-path boxes would leave its order claim untested. The
   witness is now required to differ when restricted to the on-path boxes, and the pre-change
   `preSolve` logs are required to differ.

The raw log's one "changelog" hit is the request's own out-of-scope line; no read of round records or
the changelog.

## Status

Round 7: 3 findings (H0-M3-L0), 3 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0. Series total 36 (H5-M23-L8). Not
converged.

Tripwire. U-T1: the fixes remove one measurement (the per-step `transforms` metric) and add no
mechanism. U-T2: findings 2 and 3 are in §4 A1 tests, the seventh round running; both are
consequences of earlier fixes (the round 6 split hash and the round 4 callback test). Finding 1 is in
§7, which last produced a finding in round 1. Each fix is a clause or a deletion. U-T3: 401 lines
before the fixes; the after count is recorded in the changelog. U-T6: over the split check; one
decision, recorded in round 6; unchanged.

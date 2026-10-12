---
title: Review round 17 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round16.md
mode: full pass
---

# Round 17

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
2026-10-12T10:11:57+10:00 to 10:14:09+10:00. Same prompt as rounds 1 to 16 against revision 18. U-R9:
the target is untracked so there is no diff; the sections revision 18 changed were read whole and
needed no reflow.

## Review report (Codex final message)

## 1. Summary

The implementation prompt follows the canonical design’s A0→A1→A2 sequence, and most of its source and build claims check out. Two instructions need correction: the callback fixture does not reliably isolate CCD calls, and the benchmark command does not run a `main` binary.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, §4 A0–A1, §6 | The callback test says a sphere starting clear of each box’s fat AABB receives custom-filter calls from CCD alone. Pair discovery tests overlap between **both proxy AABBs** (`src/broad_phase.c`, `src/shape.c`). The sphere’s geometry can be clear while its own fat AABB overlaps a box’s, adding pair-discovery calls and invalidating the claimed exact log and fail-before evidence. | Require and check non-overlap between the sphere’s **fat AABB** and every cluster box’s fat AABB at step start. State that condition in the corridor contract and callback fixture. |
| 2 | M | §7 | The command run from `bench-main` still invokes `$PWD/build-rel/bin/benchmark`, the current checkout’s binary. Changing its working directory only changes where CSVs are written; it does not measure `main` (`benchmark/main.c`). The shown commands also never build or run a separate `main` binary. | Give commands that build `main` in a separate worktree, then run each revision’s own binary from its own results directory with identical options. |

## 3. Checked, no change

- The design’s §5 decisions are preserved: separate sorted CCD queries, first-eight sensor selection, and unchanged sensor reporting.
- The stated CCD filters, running fraction, sensor-hit path, iteration counters, worker-context lifecycle, and sort idiom match `src/solver.c`, `src/sensor.c`, `src/physics_world.c`, `src/physics_world.h`, and `src/qsort.h`.
- The tree query and rebuild APIs, post-step visit-order witness approach, and explosion lock and wake behavior match `src/dynamic_tree.c`, `src/broad_phase.c`, `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- Test registration, named-test filtering, CMake targets, benchmark CSV format, and the approved-issue requirement match `test/`, `CMakeLists.txt`, `benchmark/main.c`, and `CONTRIBUTING.md`.

## 4. Proposed edits

In §4, specify a check of `world->fatAABBs` for both shapes before the callback test steps, and use that checked condition to support the CCD-only claim in §2 and §6. In §7, replace the benchmark example with two explicitly built executable paths, one from `main` and one from the implementation branch.

## 5. Unresolved

The required differing witness orders and fail-before counts remain empirical results for the implementing agent to produce. This was a source review; no fixtures or benchmarks were executed.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Both adopted.

1. **M, "clear of every fat AABB" is not the pair-discovery condition (adopted).** Confirmed: a shape's
   proxy enters its tree with the fat AABB (`src/shape.c`, `b3CreateShapeProxy`), pair discovery
   overlaps tree AABBs (`src/broad_phase.c`), and `b3ShouldCreatePair` calls the custom filter for an
   overlapping pair with the flag on either shape. A sphere whose geometry is clear of a box's fat
   AABB can still have its own fat AABB overlap it, adding a pair-discovery call under the sphere's
   log. The corridor contract, the §2 fact and the callback test now state the condition as the
   sphere's fat AABB overlapping no other shape's fat AABB at step start, read from `world->fatAABBs`
   and asserted before the step; `b3Shape_GetAABB` returns the tight box and cannot be used for it.
2. **M, benchmark command measures only the current checkout (adopted).** Confirmed: the command built
   and ran one binary from the working tree and only changed where the csv landed. A consequence of
   the round 16 fix. §7 now builds `main` in a scratch worktree and runs each revision's own binary
   from its own results directory with the same options.

The raw log's hits on `designs/reviews` and `changelog` are one `git status` listing and the request's
own out-of-scope line; no read of round records or the changelog.

## Status

Round 17: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0. Series total 60 (H5-M42-L13). Not converged.

Tripwire. U-T1: no mechanism added or removed; a precondition stated precisely and asserted, and a
`main` worktree for the benchmark. U-T2: finding 2 is in §7, which produced findings in rounds 14, 16
and 17, and is a consequence of the round 16 fix; finding 1 is in the callback fixture, which produced
findings in rounds 14 and 15. §7 has now drawn findings in three of the last four rounds, each a
command-block correction answered by a few lines; U-S4 is not met (not three rounds running) but the
§7 block is re-read whole, not patched, before round 18. The fixes are a clause and a command block.
U-T3: 456 lines before the fix; the after count is recorded in the changelog. U-T6: over the split
check; one decision, recorded in round 6.

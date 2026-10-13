---
title: Round 5 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round4.md
mode: fresh, full pass, read-only; prompt scope wording stale (voided)
---

## Exact request sent to Codex

```
Review the design document docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md in this repository (read-only). It is a preflight design for a Box3D change: order each continuous-collision tree query's candidates nearest-first along the sweep (B1) and stop the static-candidate loop at the first candidate that cannot improve the running fraction (B2).

Scope. Review the document's own content: what it decides, the required behaviours, and the reasons given for them. Read the source it cites (src/solver.c, src/distance.c, src/aabb.h, src/shape.c, src/body.c and anything else those lead to) and check every claim the design makes about that code against the live source. Read only the section titled "Acceptance criteria: box3d" (AC-1 to AC-4) of docs/designs/WORKFLOW.md and treat it as prohibitions on what the design may add. Completeness means completeness of what this document states; decisions the document assigns to a later Implementation Prompt, and test mechanism, are outside this pass. The measurements in 20261012-006 and 20261012-004 are inputs the document cites; you may read them as evidence for the figures quoted.

Out of scope: the changelog file, any "what changed" narrative, any file under docs/designs/reviews/, finding or round numbers, and index or bookkeeping. Existing settled primitives (b3TimeOfImpact, b3RayCastAABB, the sorted-candidate PR's collection step) are tools, not review targets: judge what this design adds and which question each primitive answers for it.

Pressure-test the design: look for ways the stated bound, the no-observable-change argument for B2, the determinism/rollback statement, or the key definition can be wrong or can be violated by an input the shipped code can produce. Do not ask whether invariants have tests or whether a coverage table is complete. Citations follow the repository's format (source files by path only, docs by section number); a missing line number is not a finding.

Output exactly this shape:
1. Summary (a few lines).
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action`. Severity is H, M or L. Real defects only; write "None." if there are none.
3. Checked, no change: what you verified and found correct.
4. Proposed edits.
5. Unresolved (including findings about other documents).
6. Verdict: one line, either CONVERGED or CHANGES_PROPOSED.
```

## Review report (Codex final message)

1. **Summary**

The working-copy preflight specifies B1 sweep ordering, but explicitly excludes B2. The cited measurements distinguish B1 from B1+B2, so the document does not yet design the requested change. The B1 source claims I checked are broadly consistent with the live code.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§1, 2, 4, 6 | The document says the key is not a bound, skips no candidate, and places all key-based skipping out of scope. It therefore omits and contradicts the requested B2 static-loop stop. A simple stop cannot claim no observable change: `src/solver.c` calls the custom filter before TOI, including for candidates that cannot improve the fraction. The proposed key also cannot serve as a stop threshold without further work: `src/distance.c` may report a hit before geometric contact because of its target and tolerance, and `src/aabb.c` can report a miss for a nearly parallel segment that crosses a slab. | Specify B2’s eligible candidates, a conservative stopping bound and miss handling, and the intended custom-filter behavior. State the resulting observable-change guarantee precisely. |

3. **Checked, no change**

- `src/solver.c` uses the running fraction for TOI, evaluates static before kinematic and dynamic queries, limits sensor hits to eight, and filters those hits at the end.
- `src/shape.c`, `src/body.c`, and `src/aabb.h` support the described B1 inputs and three-query ordering. The shape-ID tie-break gives equal computed keys a deterministic order.
- The quoted B1 timing ranges are supported by 20261012-006 §§1, 3. The existing per-query candidate array adds no new structure prohibited by AC-1 to AC-4.

4. **Proposed edits**

Revise §§1–2, 4, and 6 to include B2 and its conditions. Reconcile the opening claim of one change with the two measured changes. Keep B1’s ordering key distinct from any bound used to stop evaluation.

5. **Unresolved (including findings about other documents)**

20261012-006 §6 says the preflight pads the key by two slops and a rounding term and restricts the cull to convex solids. The current preflight says it adds no such padding and performs no cull. Reconcile that evidence description when B2 is specified.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log's matches for the
reviews directory are an `rg` exclusion glob and one `git status` listing of file names; no record or
changelog content was read (U-T2).

| # | Verdict | Check |
|---|---|---|
| 1 H | Prompt scope, not a doc defect | The prompt's first paragraph still described the target as B1 plus a static-loop stop (B2). The owner cut B2 after round 4, and the document now decides one change and places key-based skipping out of scope. The finding asks for the cut change. Per U-P3 the scope wording was unclear; it is not a declined finding and is not counted. |

Unresolved item (another document): 20261012-006 §6 describes the preflight as padding the key by two slops
and a rounding term and culling convex solids. That no longer matches this preflight. Raised with the owner.

## Status

- Round 5: voided; 0 findings counted. The next round is numbered 6 and uses a prompt whose description matches the document.
- Tripwire: no fix this round. Doc length 132 lines (222 -> 132 with the cut).
- Series convergence line unchanged: 4 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0, then a voided round.

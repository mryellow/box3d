---
title: Round 1 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: none
mode: fresh, full pass, read-only
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

## 1. Summary

The proposed ordering is deterministic for the candidates it collects, but the design has two defects that prevent B2 from being result preserving: the chosen slab test can miss a valid long sweep, and skipping static sensor misses can change which later sensor hits fit in the eight hit buffer.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B1–B2 | The stated lower bound fails for a sweep the engine can produce. `b3RayCastAABB` treats a **normalized** direction component below `FLT_EPSILON` as parallel. A sweep from `(0, 0, 0)` to `(2,000,000, 0.16, 0)` has such a Y component; a small static shape at `(1,000,000, 0.08, 0)` can be hit at fraction 0.5 while the slab test reports a miss and stores key 1. B2 then skips it. The speed limit is configurable. (`src/aabb.c`, `src/solver.c`, `src/physics_world.c`) | Specify a conservative key calculation for shallow, long sweeps. A slab miss that is uncertain must not become a culling key of 1. |
| 2 | H | §2 B2 | Skipped static sensor misses can affect the shared eight hit buffer. Without B2, misses at the running fraction can fill it; a real sensor hit in a later query or a later fast shape is then refused. With B2, that hit can be retained and emitted. Thus the “nothing observable” argument considers only later candidates in the *same static loop*. (`src/solver.c`) | Define the intended sensor selection and preserve it across all queries and fast shapes, or explicitly document B2’s sensor behaviour change. |
| 3 | L | §1, §2 B1 | The cost rationale says a rejected TOI candidate takes a second distance evaluation and that a TOI call costs at least two distance queries. Convex TOI uses a separation evaluation at the interval end; mesh and compound paths can perform zero or multiple convex TOI calls. (`src/distance.c`, `src/shape.c`) | Describe the actual operations and limit the benchmark cost claim to its measured scene. |
| 4 | L | §1 Behaviour change | “Where the id order already was nearest-first, nothing changes” does not follow from the key definition. A candidate’s enlarged AABB can enter the centroid path before another shape while its actual impact is later. B1 can therefore reorder callbacks even when ID order matches actual impact order. (`src/solver.c`, `src/shape.c`) | Say that nothing changes when **key order equals ID order**. |

## 3. Checked, no change

The start box supplies a radius for rigid rotation; static shape AABBs receive speculative padding; the sweep centre interpolates linearly; shape ID resolves equal keys. The stated filters, `preSolve` acceptance rule, iteration-counter updates, and final sensor fraction filter match `src/solver.c`. The proposed per-query candidate array adds no structure prohibited by AC-1 through AC-4.

## 4. Proposed edits

Revise §2 B1’s bound and key definition to handle the slab-test failure; revise §2 B2’s sensor argument and behaviour claim. Correct the two narrower statements in §1.

## 5. Unresolved

None concerning other documents.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log shows no read
of round records or the changelog (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 H | Adopted, severity M | `b3RayCastAABB` (`src/aabb.c`) treats a normalised component below `FLT_EPSILON` as parallel and rejects when the origin is outside that slab. Lateral travel ignored is below `FLT_EPSILON` times the segment length. At a hit the centroid lies at least about three slops inside the box on every axis (box margin four slops, hit within one), so a false miss needs a sweep beyond roughly 126 km. Reviewer's 2,000 km example is beyond default world bounds but reachable with a large-world build and a raised `maximumLinearSpeed`. Fix by construction: box grown by `r + FLT_EPSILON * length`, no branch (AC-4). |
| 2 H | Adopted | `sensorHits` lives in `b3ContinuousContext`, one per fast body in `b3SolveContinuous`, shared by all its shapes and all three queries. A sensor miss is stored at the running fraction and takes a slot (`src/solver.c`). Skipping it frees the slot, so the "changes nothing observable" claim was wrong. Doc now states solid results are unchanged and sensor hits kept can differ; fork recorded as an open decision (U-S3). |
| 3 L | Adopted | `b3TimeOfImpact` (`src/distance.c`) returns separated after one distance query and one `b3FindMinSeparation` at `tMax`. Mesh, height-field and compound paths loop over convex calls. Doc scoped to convex candidates and the cost wording corrected. |
| 4 L | Adopted | "Id order already nearest-first" does not imply key order equals id order. Reworded to key order. |

Owner input during the round: the recommended options for the two open questions about one key order for all queries and argument-only filters in the callback are accepted. Both were already the body's text; they are removed from the open list.

Observation, not a finding (U-I4): the `Tests` subsections and §3 checklist carry test mechanism that U-C2 and U-C10 assign to the Implementation Prompt. Not raised by the reviewer; left for the owner.

## Status

- Round 1: 4 findings (H2-M0-L2), 4 applied, 0 declined. Series total: 4 findings.
- Findings: 2/0/2 -> pending confirmation round. Not converged (U-V9, U-S8).
- Tripwire: fixes added no named mechanism and removed one open decision; findings did not cluster (first round); no finding was a consequence of an earlier fix; doc length 190 -> 193 lines (F1 +3, F2 +9 net of the deleted decision and test text, F3 +2, F4 0).
- Next: round 2, same prompt at the same scope (U-P4).

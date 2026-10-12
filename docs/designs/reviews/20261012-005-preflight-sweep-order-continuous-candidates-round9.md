---
title: Round 9 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round8.md
mode: fresh, full pass, read-only, same prompt as round 6
---

## Exact request sent to Codex

```
Review the design document docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md in this repository (read-only). It is a preflight design for one Box3D change: order each continuous-collision tree query's candidates nearest-first along the sweep by a per-candidate key. The document deliberately decides no skipping of candidates; a static-candidate cull is out of its scope and is not a missing piece.

Scope. Review the document's own content: what it decides, the required behaviours, and the reasons given for them. Read the source it cites (src/solver.c, src/distance.c, src/aabb.h, src/shape.c, src/body.c and anything else those lead to) and check every claim the design makes about that code against the live source. Read only the section titled "Acceptance criteria: box3d" (AC-1 to AC-4) of docs/designs/WORKFLOW.md and treat it as prohibitions on what the design may add. Completeness means completeness of what this document states; decisions the document assigns to a later Implementation Prompt, and test mechanism, are outside this pass. The measurements in 20261012-006 and 20261012-004 are inputs the document cites; you may read them as evidence for the figures the document quotes. 20261012-006 also measures a cull that this document does not propose; findings about 20261012-006 itself go under Unresolved.

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

The proposed per-query ordering preserves the design’s no-skip rule and its layout-independent tie break. Two claims need qualification: the stored start box does not always give a true bounding radius, and the measurements do not show a cost reduction in every case.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §2, §6 | The farthest-corner distance is not always a bound in the default float build. At world position \(2^{24}\), a valid sphere of radius 0.1 can have a stored start AABB rounded to a point, giving `r = 0` while TOI still uses radius 0.1. This follows from `src/sphere.c`, `src/shape.c`, `src/solver.c` and `include/box3d/math_functions.h`. No candidate is lost because the design does not skip by key. | Describe `r` as derived from the stored box and qualify the geometric bound for float rounding; retain the no-skip rule. |
| 2 | L | Introduction, §1 | “The cost of the stage falls” is categorical. The B1 measurements in 20261012-006 §1 and §3 include a run slower than `main` and runs slower than ascending-id order; §6 says project benchmarks were not run. | Say the ordering *can* reduce cost in the measured dense scene, with overhead when id order is already favorable. |

## 3. Checked, no change

The source supports the stated filter split, three separate query sorts, running TOI fraction, sensor buffer behavior, candidate array sizing, and shape-id tie break. Given identical candidate AABBs and sweep state, the key calculation preserves the sorted-candidate PR’s layout-independence argument. The proposed transient array adds no structure prohibited by WORKFLOW AC-1 to AC-4. No static-candidate cull is required by this design.

## 4. Proposed edits

Qualify the radius-bound sentence in §2 and §6, and the unconditional cost claim in the introduction. Keep the explicit statement that keys only order candidates.

## 5. Unresolved

20261012-006’s **B2 cull**, which §4 excludes, is not behavior preserving if proposed later. Its skip bypasses the custom-filter callback. The float-box example above can also make its key later than a real TOI hit despite the prototype’s added slop: two static spheres at the same rounded position, with the smaller-radius, lower-id sphere evaluated first, can leave a running fraction that causes B2 to skip the larger sphere’s earlier hit. This is a finding about 20261012-006, not a missing part of this design.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log shows search output from the changelogs of 20261012-001 and 20261012-002 (other documents); nothing from this doc's records or changelog (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 L | Adopted as a stated limit | The stored start box is a fat float box (`src/shape.c`); at about 2^24 a float step exceeds the box, so it can collapse to a point and give `r = 0`. `include/box3d/constants.h` says float-mode positions beyond about 16 km already lose precision and sets the sanity limit at 100 km, so the case is outside the supported range; it is the coarse-box limit of round 7 stated generally. The farthest-corner sentence and the large-world sentence now say so. |
| 2 L | Adopted | 20261012-006 §3 has B1 rows slower than `main` and slower than ascending id in +x. Introduction says the cost falls in the measured dense scene and can rise slightly where id order is already nearest-first. |

Unresolved item (another document): the reviewer argues the cull in 20261012-006 would bypass the custom filter and, with a coarse box, could skip an earlier hit. Both concern the cull this preflight does not propose. Raised with the owner.

## Status

- Round 9: 2 findings (H0-M0-L2), 2 applied, 0 declined. Series total: 25 findings.
- Findings: 8 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> 0/0/2 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: no named mechanism added or removed. Finding 1 is in the large-world sentence that round 7 added (a consequence of an earlier fix, U-T2). Doc length 135 -> 138 lines.
- Next: round 10, same prompt at the same scope (U-P4).

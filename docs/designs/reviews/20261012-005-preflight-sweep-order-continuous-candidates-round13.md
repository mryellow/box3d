---
title: Round 13 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round12.md
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

The proposed key is correctly treated as an ordering heuristic. The design keeps every candidate that passes the existing filters in evaluation, so an inaccurate key can change order and cost without causing a key-based omission. One out-of-scope rationale overstates how TOI handles initial overlaps.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §4 | The claim that every initially overlapping solid convex candidate returns fraction 0 after one distance query is false. TOI measures core-shape distance without radii and uses a slop threshold. Two radius-0.1 spheres with centers 0.199 apart overlap physically, but their core distance exceeds the fraction-0 threshold, so TOI can continue to a separation evaluation (`src/shape.c`, `src/distance.c`). | Qualify the statement: sufficiently deep initial overlaps return fraction 0; shallow overlaps can take additional TOI work. Keep the case out of scope. |

## 3. Checked, no change

The per-query sort, shape-id tie-break, NaN normalization, filter placement, sensor limit, running fraction, and AABB sources match the live code (`src/solver.c`, `src/aabb.c`, `src/shape.c`, `src/body.c`). The rollback argument is consistent with the stated fixed-key order. The cited B1 measurements support the limited performance claim. AC-1 through AC-4 add no prohibition that this change violates.

## 4. Proposed edits

Revise the §4 initial-overlap sentence to distinguish deep overlaps from shallow physical overlaps, without changing the scope decision.

## 5. Unresolved

20261012-006 §1 and §4 measure a separate B2 cull. Its key cannot generally serve as a lower bound: an offset shape’s centroid can follow a curved path, and `b3RayCastAABB` can treat a small normalized direction component as parallel (`src/distance.c`, `src/aabb.c`). A skip based on that key needs separate justification. This is not a finding against the no-skip design in 20261012-005.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of this doc's round records or changelog in the log (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 L | Adopted by deletion | `b3TimeOfImpact` (`src/distance.c`) measures core distance without radii and returns a hit at 0 only within `target + tolerance`, where `target` is the larger of one slop and the summed radii less one slop. Two radius-0.1 spheres 0.199 apart overlap physically but sit at core distance 0.199, above 0.196, so the call continues. The sentence about what the call returns is deleted (U-F4); the bullet keeps only why the case is out of scope. |

Unresolved item (another document): the cull in 20261012-006; no action here (U-P5).

## Status

- Round 13: 1 finding (H0-M0-L1), 1 applied, 0 declined. Series total: 32 findings.
- Findings: 12 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> 0/0/2 -> 0/1/4 -> 0/0/0 -> 0/0/1 -> 0/0/1 -> pending. Not converged.
- Tripwire: no named mechanism added or removed. The sentence was one written in round 6's fix (U-T2: consequence of an earlier fix) and was deleted rather than qualified. Doc length 137 -> 137 lines.
- Next: round 14, the same prompt at the same scope (U-P4).

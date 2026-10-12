---
title: Round 11 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round10.md
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

1. **Summary**

The preflight design is consistent with the current CCD path. Its key is an ordering heuristic, and the design correctly requires every collected candidate to reach evaluation. The cited B1 measurements support the stated performance tradeoff.

2. **Findings**

None.

3. **Checked, no change**

I verified the per-query collection and sorting, filters, running TOI fraction, sensor limit, and AABB and sweep construction against `src/solver.c`, `src/distance.c`, `src/aabb.c`, `src/shape.c`, `src/body.c`, and the related headers. The radius bounds points relative to the moving shape centroid; it does not make the endpoint segment a collision bound, as §1 and §6 recognize. The proposed transient candidate element adds no structure prohibited by AC-1 to AC-4.

4. **Proposed edits**

None.

5. **Unresolved**

The **B2 cull in 20261012-006 §6 and its appendix** needs separate review. An offset shape’s centroid can arc away from its endpoint segment while its swept query box still includes a static candidate, so the key need not precede that candidate’s hit. A cull would also omit custom-filter calls that `src/solver.c` currently makes before TOI. Neither issue is a missing piece of this B1-only design.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of this doc's round records or changelog in the log (U-T2).

No findings to verify. The unresolved item concerns the cull in 20261012-006, which this preflight does not propose; no action here (U-P5).

## Status

- Round 11: 0 findings (H0-M0-L0), 0 applied, 0 declined. Series total: 30 findings.
- Findings: 10 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> 0/0/2 -> 0/1/4 -> 0/0/0. Clean round; confirmation round required (U-P9). Not yet Canonical (U-G4, U-S8).
- Tripwire: no fix this round; doc unchanged at 137 lines. The key paragraph compressed before this round drew no finding.
- Next: round 12, the same prompt at the same scope (U-P9).

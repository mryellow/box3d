---
title: Round 15 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round14.md
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

The design is internally consistent with the live continuous-collision path. Its key is a heuristic, and the explicit no-skip rule preserves candidate evaluation despite geometric or float inaccuracies.

2. **Findings**

None.

3. **Checked, no change**

Verified the per-query collection and sort, filter placement, TOI running fraction, sensor limit and end filter, AABB construction, slab-test behavior, and worker scheduling against `src/solver.c`, `src/distance.c`, `src/aabb.c`, `src/shape.c`, `src/body.c`, and related headers. The shape-id tie break gives a deterministic order for the collected candidates. The design adds none of the structures or checks prohibited by AC-1 through AC-4.

4. **Proposed edits**

None.

5. **Unresolved**

The B2 cull measured in 20261012-006 §6 is outside this design. Its no-observable-change premise does not hold for every sweep: a rotating body can carry an offset fast shape’s centroid along an arc into a static candidate while the endpoint segment misses the grown box. B2 could skip that candidate. The reviewed design explicitly identifies this case and skips no candidates.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of this doc's round records or changelog in the log (U-T2).

No findings to verify. The unresolved item concerns the cull in 20261012-006, which this preflight does not propose; no action here (U-P5).

Author gate before Canonical (U-G1, U-G4): AC-1 the only added structure is the existing transient per-query candidate array with a wider element, no new population; AC-2 no enumerated "callers must" list; AC-3 nothing to reconcile; AC-4 the NaN-to-1 rule is a clamp that keeps the sort total, not a branch that changes behaviour, and nothing is skipped.

Observation, not a finding (U-I4): the `Tests` subsection and §3 checklist carry test mechanism that U-C2 and U-C10 assign to the Implementation Prompt. No round raised it and the prompt did not ask (U-P6). Left for the owner; changing it after the confirming round would be an edit between convergence and Canonical (U-I5).

## Status

- Round 15: 0 findings (H0-M0-L0). Series total: 32 findings. Confirmation round: same prompt, same scope, no change to the document since round 14.
- Findings: 14 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> 0/0/2 -> 0/1/4 -> 0/0/0 -> 0/0/1 -> 0/0/1 -> 0/0/0 -> SETTLED 0/0/0. A voided round (5, stale prompt scope) is excluded.
- Tripwire: no fix this round; doc unchanged at 137 lines. Round 11 was also clean; round 12 then found one finding, so convergence counts from round 14.
- Result: converged. The next edit is the status word, Canonical (U-I5).

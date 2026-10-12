---
title: Round 12 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round11.md
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
The design is coherent for the proposed ordering change. The key remains a heuristic, every candidate that passes the existing filters reaches evaluation, and the shape-id tie-break gives a deterministic order. One radius claim is stronger than the floating-point code supports.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §2, §6 | The stated `r` is not a strict bound on every shape point. After a no-impact step, `src/solver.c` can retain a tight start box; the farthest-corner distance is then computed with `sqrtf`, which can round downward even when that corner is a shape vertex (`src/shape.c`, `include/box3d/math_functions.h`). This does not make B1 unsafe because the key skips nothing. | Describe `r` as a floating-point radius estimate, or specify outward rounding if a strict bound is intended. |

3. **Checked, no change**  
`src/solver.c`, `src/distance.c`, `src/aabb.c`, `src/shape.c`, and `src/body.c` support the design’s account of the three query order, running TOI fraction, moved filters, sensor limit, and key-1 rule. The cited ordering measurements match 20261012-004 §5 and the B1 columns of 20261012-006 §1. The proposal adds none of the structures or checks prohibited by `WORKFLOW.md` AC-1–AC-4.

4. **Proposed edits**  
Qualify the strict radius-bound wording in §2 and §6. Keep the explicit no-skip decision.

5. **Unresolved**  
20261012-006 §2 measures a **separate B2 cull**, outside this design. Its prototype skips candidates using a key that this design correctly calls a heuristic: a curved centroid path or `b3RayCastAABB`’s parallel-slab rule can give key 1 despite a hit (`src/aabb.c`, `src/solver.c`). The prototype also skips before the custom filter in `src/solver.c`, so B2 can remove observable callback calls. Neither concern is a missing piece of B1.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log shows a `git status` listing of file names, including round records; no record or changelog content was read (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 L | Adopted | `r` is the distance to the farthest corner of the stored start box, computed in float with `sqrtf`, so it can round below the true extent; the box itself can be the tight box left by a no-impact step. Nothing depends on a strict bound because the key skips nothing. `r` is described as a float estimate in §2, and the Provenance verb is "estimates". |

Unresolved item (another document): the cull in 20261012-006; no action here (U-P5).

## Status

- Round 12: 1 finding (H0-M0-L1), 1 applied, 0 declined. Series total: 31 findings. This was the confirmation round after the clean round 11; it returned a finding, so the clean-round count restarts (U-S8).
- Findings: 11 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> 0/0/2 -> 0/1/4 -> 0/0/0 -> 0/0/1 -> pending. Not converged.
- Tripwire: no named mechanism added or removed. Finding is in a sentence earlier rounds touched (the radius), not a consequence of the last fix. Doc length 137 -> 137 lines.
- Next: round 13, the same prompt at the same scope (U-P4).

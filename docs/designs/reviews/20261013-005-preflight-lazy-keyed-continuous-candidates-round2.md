---
title: Round 2 review of 20261013-005 preflight, lazily keyed continuous candidates
target: docs/designs/20261013-005-preflight-lazy-keyed-continuous-candidates.md
date: 2026-10-13
reviewer: codex gpt-6-sol, effort high
previous: 20261013-005-preflight-lazy-keyed-continuous-candidates-round1.md
mode: fresh, full pass, read-only, same prompt as round 1
---

## Exact request sent to Codex

```
Review the design document docs/designs/20261013-005-preflight-lazy-keyed-continuous-candidates.md in this repository (read-only). It is a preflight design for one Box3D change: defer the per-candidate sweep key, the fast shape's bounding radius and the sort in the continuous stage until after the tree query, and only when the query collected two or more candidates, so that the evaluation order and results stay bit-identical to the sweep-ordered candidate change designed in docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md. The document deliberately decides no skipping of candidates and does not change the key or the order; those are out of its scope and are not missing pieces.

Scope. Review the document's own content: what it decides, the required behaviours, and the reasons given for them. Read the source it cites (src/solver.c and anything else those lead to) and check every claim the design makes about that code against the live source. The sweep-ordered change may not yet be present in the source tree; docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md is its design and defines the order and key this document must reproduce; treat that document as context, not a review target. Read only the section titled "Acceptance criteria: box3d" (AC-1 to AC-4) of docs/designs/WORKFLOW.md and treat it as prohibitions on what the design may add. Completeness means completeness of what this document states; decisions the document assigns to a later Implementation Prompt, and test mechanism, are outside this pass. docs/designs/20261013-004-measurement-continuous-stage-paths.md and docs/designs/20261013-001-benchmark-sweep-order-continuous-candidates-implementation.md are inputs the document cites; you may read them as evidence for the figures the document quotes. Findings about those other documents go under Unresolved.

Out of scope: the changelog file, any "what changed" narrative, any file under docs/designs/reviews/, finding or round numbers, and index or bookkeeping. Existing settled primitives (the tree query, b3TimeOfImpact, the sweep-ordered candidate change's key and sort) are tools, not review targets: judge what this design adds and which question each primitive answers for it. Decisions recorded in the document's Provenance section are settled and binding unless you have fresh source evidence that the reason no longer holds.

Pressure-test the design: look for ways the bit-identical claim, the claim that the inputs to the key are unchanged between the end of the query and the start of evaluation, the per-query count threshold, the moved bounding radius, the cost statements, or the determinism/rollback statement can be wrong or can be violated by an input the shipped code can produce (including multi-worker scheduling, the three queries per bullet, sensor handling, and any early-out or per-query state). Do not ask whether invariants have tests or whether a coverage table is complete. Citations follow the repository's format (source files by path only, docs by section number); a missing line number is not a finding.

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

The proposed count threshold preserves the sweep design’s candidate order and solver results in the current code. Two statements need correction: the explanation of why key inputs stay stable across workers, and the claim that every measured query collected a candidate.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §1, Behaviour change | The stated reason for stable `shape->aabb` inputs overlooks writes by other workers. Stability follows from narrower facts: non-bullet sweeps query only static shapes; bullet sweeps start after finalization and filter out bullet candidates (`src/solver.c`, `src/parallel_for.c`). | State those scheduling and filtering facts as the reason deferred keys read the same boxes. |
| 2 | L | §1 | “Each continuous query collects one candidate” overstates the cited measurement. Its query counter includes only queries with at least one candidate (20261013-003 §2–3); the tree query can also return zero (`src/dynamic_tree.c`). | Say “each non-empty query” and retain the explicit zero-candidate path in §2. |

3. **Checked, no change**

The callback and evaluation sequence supports the threshold of two *collected* candidates. Each query evaluates and clears its own array; a bullet’s static, kinematic, and dynamic queries remain separate. Moving the radius calculation still uses the held start box and centroid segment. Singleton evaluation preserves order, including sensor handling and the running fraction. No candidate skip, key change, sort change, or AC-1–AC-4 conflict is introduced (`src/solver.c`, 20261012-005 §2, WORKFLOW §“Acceptance criteria: box3d”).

4. **Proposed edits**

Clarify the two §1 statements identified above. No change to the proposed algorithm is needed.

5. **Unresolved**

The same “one candidate per query” shorthand appears in the supporting documents (20261013-003 §1; 20261013-004 §3). Their wording is outside this review.

6. **Verdict: CHANGES_PROPOSED**

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of round records or the changelog in the log (U-T2).

Finding 1 (L, adopted): confirmed in `src/solver.c`. A non-bullet fast shape queries only the static tree; a bullet queries kinematic and dynamic trees as well, runs in a later stage, and bullets write their own `fastShape->aabb` concurrently, so "the stage mutates only the fast shape's box" was not by itself a reason a candidate's box is stable. It is stable because bullet candidates are dropped in the callback (20261012-005 §2). The §1 sentence now states those two facts.

Finding 2 (L, adopted): the figures in 20261013-003 §3 count only queries that collected at least one candidate. "Each continuous query" became "each non-empty continuous query" in §1 and in Provenance; the zero-candidate path stays in §2.

Unresolved item (other documents): "one candidate per query" shorthand in 20261013-003 §1 and 20261013-004 §3. Not this doc's to fix (U-P5); raised with the owner together with the round 1 item.

## Status

- Round 2: 2 findings (H0-M0-L2), 2 applied, 0 declined. Series total: 3 findings.
- Findings: 2 rounds: 0/1/0 -> 0/0/2.
- Tripwire: fixes removed and added no named mechanism (U-T1). Both findings landed in §1, as did round 1's; neither is a consequence of the round 1 fix, which was a different sentence (U-T2). Doc length 94 -> 95 lines; the fixes added 1 line net, no paragraph (U-T3).
- Result: not converged; round 3 is the same prompt at the same scope after the revision.

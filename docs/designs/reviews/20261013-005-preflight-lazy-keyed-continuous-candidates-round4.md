---
title: Round 4 review of 20261013-005 preflight, lazily keyed continuous candidates
target: docs/designs/20261013-005-preflight-lazy-keyed-continuous-candidates.md
date: 2026-10-13
reviewer: codex gpt-6-sol, effort high
previous: 20261013-005-preflight-lazy-keyed-continuous-candidates-round3.md
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

Reviewed the design from commit `62467bf`; the requested document is absent from the current checkout. The proposed count threshold preserves the sweep-ordered evaluation path, but its claim of zero keying and sorting on every project scenario conflicts with the cited measurements.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §1, §3 | Junkyard collected 503 candidates across 463 queries with candidates (20261013-003 §3). At least one query therefore had two or more candidates and would compute keys and sort. The “one candidate per query” rationale and the §3 zero-keys, zero-sorts criterion are false for that scenario. | State the saving for queries with zero or one candidate, and revise the criterion to allow keying and sorting for junkyard’s multi-candidate queries. |

3. **Checked, no change**

The per-query threshold preserves static, kinematic, then dynamic query order; singleton evaluation needs no sort. In `src/solver.c`, the start box is retained, evaluation changes the running fraction and sensor buffer, and candidate boxes do not change between collection and evaluation on the eligible source paths. Finalization precedes bullet queries, while bullets are filtered as candidates. The moved radius and extra shape load have the stated per-path costs. The design adds none of the structures or checks prohibited by AC-1 through AC-4.

4. **Proposed edits**

Revise §1’s project-scenario claim and §3’s counting criterion as described in Finding 1. Keep the two-candidate threshold and the key and order unchanged.

5. **Unresolved**

The supporting documents also describe the regime as one candidate per query (20261013-003 §1; 20261013-004 §1 and §3), despite the junkyard counts in 20261013-003 §3. Those documents are outside this review.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

Voided. Same wrong working directory as round 3 (header `workdir: /home/user/box3d`); the reviewer reviewed the committed first draft from git history ("the requested document is absent from the current checkout"). Its finding 1 is the round 1 finding already adopted in the revision under review; no verification of it is needed against the revised document. Exit 0, header shows `gpt-6-sol` / `high`; raw log saved before reading (U-V1).

## Status

- Round 4: voided (wrong working directory, stale target). Excluded from the findings line and the series total (U-R8). Series total stays 3 findings.
- Tripwire: no fix this round; doc unchanged at 95 lines.
- Result: no result. The next round is round 5, run from `/home/user/box3d-solver`, counted as the first valid round after the round 2 revision.

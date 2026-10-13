---
title: Round 1 review of 20261013-005 preflight, lazily keyed continuous candidates
target: docs/designs/20261013-005-preflight-lazy-keyed-continuous-candidates.md
date: 2026-10-13
reviewer: codex gpt-6-sol, effort high
previous: none
mode: fresh, full pass, read-only
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

The lazy keying rule preserves the sweep-ordered design’s per-query evaluation order. One cost premise is false: the cited sweep-order measurement records multi-candidate queries in `junkyard`, so the design’s “zero keys and zero sorts on every project scenario” criterion cannot hold.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §1, §3 | §1 says every project query collects one candidate, and §3 requires zero keys and sorts on every project scenario. The sweep-order baseline records 463 nonempty `junkyard` queries and 503 candidates (20261013-003 §2–§3). At least one query therefore has two or more candidates and must be keyed and sorted under this design. | Correct the project-scenario claim. State the zero-work requirement per query with fewer than two candidates, and retain keying and sorting for the measured multi-candidate queries. |

3. **Checked, no change**

The count threshold, independent handling of a bullet’s three queries, sensor selection, and running fraction preserve evaluation order as specified. In `src/solver.c`, non-bullets query static shapes; bullets run after body finalization, and bullet candidates are filtered. Eligible candidate AABBs therefore have no identified stage mutation between collection and keying. The design adds no structure or validation prohibited by AC-1 to AC-4.

4. **Proposed edits**

Revise §1’s “every query” rationale to acknowledge `junkyard`, and revise §3’s counting criterion to require zero keys and sorts **for each query collecting fewer than two candidates**.

5. **Unresolved**

The cited input 20261013-004 §1 and §3 repeats the one-candidate-per-query claim, while 20261013-003 §3 reports `junkyard` at 1.09 candidates per nonempty query. That discrepancy belongs to those documents. The sweep-ordered implementation is absent from the live source tree, so its exact implementation remains unverified here; its design supplied the comparison key and order.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The only mention of the changelog or round records in the log is the prompt's own out-of-scope sentence; no read of them (U-T2).

Finding 1 (M, adopted): confirmed against 20261013-003 §3 (junkyard 463 queries, 1.09 candidates per query) (the prototype differs only through the declared order change). The doc's "every project scenario" premise and the zero-work checklist item were false for junkyard. Fixed by scoping the claim to queries with fewer than two candidates and by adding the multi-candidate queries to the order check. The citation moved from 20261013-004 §1 to 20261013-003 §3, where the figure lives.

Unresolved item (other document): 20261013-004 states one candidate per query everywhere in its intro; 20261013-003 §3 shows junkyard at 1.09. Not this doc's to fix (U-P5); raised with the owner.

## Status

- Round 1: 1 finding (H0-M1-L0), 1 applied, 0 declined. Series total: 1 finding.
- Findings: 1 round: 0/1/0.
- Tripwire: the fix removed no mechanism and added none (U-T1). First round, no clustering (U-T2). Doc length 92 -> 94 lines; the fix added 2 lines (U-T3). Under the 400-line split-check.
- Result: not converged; round 2 is the same prompt at the same scope after the revision.

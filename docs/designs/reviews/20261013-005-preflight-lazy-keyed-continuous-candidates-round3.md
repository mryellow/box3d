---
title: Round 3 review of 20261013-005 preflight, lazily keyed continuous candidates
target: docs/designs/20261013-005-preflight-lazy-keyed-continuous-candidates.md
date: 2026-10-13
reviewer: codex gpt-6-sol, effort high
previous: 20261013-005-preflight-lazy-keyed-continuous-candidates-round2.md
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
The design is internally consistent with the sweep-order design’s key and per-query order. I found no defect in its bit-identical claim or its stated cost changes.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| — | — | — | None. | — |

3. **Checked, no change**  
In `src/solver.c`, candidates are evaluated after each query; bullets run three separate queries; and sensor selection and the running hit fraction depend on evaluation order. The proposed threshold preserves that order. The worker schedule in `src/parallel_for.c` and the candidate filtering in the sweep-order design (§2) support the claim that a collected candidate’s box remains unchanged before keying. The stated shape-load and bounding-radius costs also match those paths. AC-1 through AC-4 add no conflict.

4. **Proposed edits**  
None.

5. **Unresolved**  
None.

6. **Verdict:** CONVERGED
2026-10-13T03:51:01.718549Z ERROR codex_core::session: failed to record rollout items: thread 01a13360-352f-7563-ba3b-71e67704048c not found

## Post-review verification (Claude)

Voided. The command ran with the working directory `/home/user/box3d` instead of `/home/user/box3d-solver` (header `workdir`); the target path did not exist there and the reviewer read the committed first draft of the target from git history, not the revised document. The verdict below says nothing about the revised document. Exit 0, header shows `gpt-6-sol` / `high`; raw log saved before reading (U-V1).

## Status

- Round 3: voided (wrong working directory, stale target). Excluded from the findings line and the series total (U-R8). Series total stays 3 findings.
- Tripwire: no fix this round; doc unchanged at 95 lines.
- Result: no result. Same prompt re-sent from the repository root `/home/user/box3d-solver`.

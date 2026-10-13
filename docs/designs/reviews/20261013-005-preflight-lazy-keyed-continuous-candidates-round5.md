---
title: Round 5 review of 20261013-005 preflight, lazily keyed continuous candidates
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

## 1. Summary

The design is internally consistent with the sweep-ordered design and the live continuous-stage code. Deferring key computation preserves candidate order and results under the phase ordering in `src/solver.c`.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| | | | None. | |

## 3. Checked, no change

- `src/solver.c` evaluates and clears candidates after each query; a bullet’s three queries therefore have separate counts.
- `src/solver.c` and `src/parallel_for.c` show that non-bullet finalization finishes before bullet queries. Non-bullet queries see static candidates; bullet callbacks exclude bullet candidates. Collected candidate AABBs remain stable between query and evaluation.
- The saved start box and centroid segment remain available after the query. Keying every candidate when the count is at least two reproduces the sweep-ordered design’s key and sort; one candidate needs neither.
- Sensor selection, the running fraction, and callback order retain the same evaluation sequence. The design adds no mechanism prohibited by AC-1 through AC-4.

## 4. Proposed edits

None.

## 5. Unresolved

`20261013-004 §3` says “one candidate per query,” while `20261013-003 §3` reports 1.09 for junkyard. This is an inconsistency in the supporting documents; this design states the junkyard exception correctly.

## 6. Verdict

CONVERGED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`, `workdir: /home/user/box3d-solver`. Raw log saved before reading (U-V1). No read of round records or the changelog in the log (U-T2).

No findings to verify. The unresolved item concerns 20261013-004 §3 and 20261013-003 §1, not this document (U-P5).

## Status

- Round 5: 0 findings (H0-M0-L0). Series total: 3 findings. Confirmation round (U-P9) follows: same prompt, same scope, no change to the document since round 2's revision.
- Findings: 3 valid rounds (rounds 3 and 4 voided, wrong working directory): 0/1/0 -> 0/0/2 -> 0/0/0.
- Tripwire: no fix this round; doc unchanged at 95 lines (U-T1, U-T3). No clustering to record (U-T2).
- Result: clean round; not yet converged (U-S8).

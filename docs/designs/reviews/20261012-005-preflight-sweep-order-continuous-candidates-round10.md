---
title: Round 10 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round9.md
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

The design preserves the current candidate set and changes its per-query evaluation order. That is consistent with the solver and the stated no-skip scope. Several claims about what the key represents and why it should improve performance need tightening.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §1–2 | The key is not always the segment’s box-entry fraction. `b3RayCastAABB` in `src/aabb.c` treats a normalized axis component below `FLT_EPSILON` as parallel. A valid sweep can start just outside a grown box, cross it with a small transverse motion, and receive key 1. This can defeat the promised nearest-first order even without rotation. | Describe the helper’s near-parallel behavior and qualify “nearest-first,” or specify a segment test that computes the intended entry fraction. Keep the no-skip rule. |
| 2 | L | §1 | Key 0 does not mean the shapes are “already within the speculative margin.” Growth by the fast shape’s farthest-corner radius can put its centroid inside the grown box while the shapes remain separated by much more than that margin. | Describe key 0 as “the start centroid lies inside the grown candidate box.” |
| 3 | L | §1 | The claim that box coarseness starts farther away in a large-world build is unsupported. `shape->aabb` remains a float `b3AABB` in absolute coordinates; `src/shape.c` and `include/box3d/math_functions.h` round those bounds outward but do not give them double precision. The distance at which boxes cease to resolve candidates also depends on shape size. | Remove the fixed distance threshold and the large-world comparison; state that key resolution depends on world position and box size. |
| 4 | L | §1, §4 | The TOI cost explanation is categorical where the code is conditional. In `src/distance.c`, being geometrically separated at the upper fraction does not guarantee that the selected separation axis proves separation in one evaluation. Also, `src/shape.c` can query multiple mesh triangles or compound children, so an initially overlapping solid candidate need not take one distance query. | Present the cheap rejection as a common convex-case outcome, and limit the one-query overlap statement to the direct convex path. |
| 5 | L | §1–2 | The measured B1 build in 20261012-006 §6 uses `r` plus slop and rounding growth; this design specifies `r` alone. Its timings are evidence for a close prototype, not a direct measurement of the exact proposed key. | Qualify the performance claim as prototype evidence and name the growth difference. |

## 3. Checked, no change

The source supports the stated three-query order, running TOI fraction, candidate filters, sensor cap and end filter, `preSolve` ordering, and candidate AABB timing (`src/solver.c`, `src/shape.c`, `src/body.c`). Sorting by key then shape ID, with NaNs mapped to 1, gives a deterministic order for equal state. Moving the listed built-in filters into collection does not add a user callback. The ephemeral per-query array adds no prohibited shadow population, reconciliation, or validation gate under WORKFLOW §Acceptance criteria: box3d, AC-1–AC-4.

## 4. Proposed edits

Tighten the key’s definition and key-0 interpretation, correct the float-box precision claim, qualify the TOI cost explanation, and identify the benchmark prototype difference. Retain evaluation of every collected candidate.

## 5. Unresolved

20261012-006 also measures a static-candidate cull. Its bound and observable effects were not assessed here because §4 excludes that cull; its B2 results do not support this design’s performance claim.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of this doc's round records or changelog in the log (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 M | Adopted | `b3RayCastAABB` (`src/aabb.c`) treats a normalised component below `FLT_EPSILON` as parallel, as found in round 1; the key was then a bound, now only an order, so a false key 1 costs time. Stated beside the rotation caveat. |
| 2 L | Adopted | The grown box is wider than the speculative margin, so key 0 means the start centroid is inside the grown box. The "already within the speculative margin" phrase removed. |
| 3 L | Adopted | `b3OffsetAABB` (`include/box3d/math_functions.h`) returns a float `b3AABB` in absolute coordinates in both builds; large-world mode does not refine it. Round 9's "farther in a large-world build" was wrong and is removed, with the fixed 16 km threshold. |
| 4 L | Adopted | `b3TimeOfImpact` returns early only when the selected separation clearly exceeds target plus tolerance; mesh and compound paths run inner calls. Cheap rejection stated as the usual convex outcome; the start-overlap bullet limited to convex solid candidates. |
| 5 L | Adopted | 20261012-006 §6 says its prototype's growth is `r` plus two slops and a rounding term. Costs names the difference. |

Unresolved item (another document): 20261012-006's cull results do not support this design's claim; the preflight cites only §1 and the B1 columns. No action here.

## Status

- Round 10: 5 findings (H0-M1-L4), 5 applied, 0 declined. Series total: 30 findings.
- Findings: 9 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> 0/0/2 -> 0/1/4 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: no named mechanism added or removed. Finding 3 is a consequence of round 9's own fix (the "farther in a large-world build" clause), and finding 4 re-opens the sentence round 8 edited (U-T2). Findings cluster in §1's single key paragraph, which each round has extended; it grew from 5 to 12 lines (U-T3). Doc length 138 -> 140 lines. The caveats were then compressed into one statement of three limits before round 11 (U-S4, U-T3), taking the doc to 137 lines.
- Next: round 11, same prompt at the same scope (U-P4).

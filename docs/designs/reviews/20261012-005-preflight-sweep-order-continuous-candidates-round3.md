---
title: Round 3 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round2.md
mode: fresh, full pass, read-only, same prompt as round 1
---

## Exact request sent to Codex

```
Review the design document docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md in this repository (read-only). It is a preflight design for a Box3D change: order each continuous-collision tree query's candidates nearest-first along the sweep (B1) and stop the static-candidate loop at the first candidate that cannot improve the running fraction (B2).

Scope. Review the document's own content: what it decides, the required behaviours, and the reasons given for them. Read the source it cites (src/solver.c, src/distance.c, src/aabb.h, src/shape.c, src/body.c and anything else those lead to) and check every claim the design makes about that code against the live source. Read only the section titled "Acceptance criteria: box3d" (AC-1 to AC-4) of docs/designs/WORKFLOW.md and treat it as prohibitions on what the design may add. Completeness means completeness of what this document states; decisions the document assigns to a later Implementation Prompt, and test mechanism, are outside this pass. The measurements in 20261012-006 and 20261012-004 are inputs the document cites; you may read them as evidence for the figures quoted.

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

B1’s ordering and B2’s static-only stop fit the current solver structure. The design needs changes before B2’s key can be treated as a lower bound for every supported configuration.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B1, B2 | The claimed four-slop AABB margin is configurable. `B3_SPECULATIVE_DISTANCE` may be set just above one linear slop, while `b3TimeOfImpact` can report a hull hit at up to 1.25 slops of separation. After a prior no-impact sweep, the fast start box can be unpadded. For a long, thin hull, its corner-derived radius adds negligible slack. The key can therefore exceed a valid hit fraction, and B2 can skip that solid candidate. `include/box3d/constants.h`, `src/shape.c`, `src/solver.c`, `src/distance.c`. | Derive growth from the TOI acceptance distance and supported configuration, then establish the lower bound before specifying the stop. |
| 2 | M | §2 B1 | The rounding argument covers sweeps shorter than “about a hundred kilometres,” but the design imposes no such limit. In the default single-precision build, `b3RoundDownFloat` and `b3RoundUpFloat` are plain casts, so they do not round the relative boxes outward. The fixed margin is not a demonstrated bound on key error over accepted sweep lengths. `include/box3d/math_functions.h`, `src/aabb.c`, `src/solver.c`. | Specify conservative arithmetic and padding that cover the supported numeric range; revise the lower-bound claim accordingly. |
| 3 | L | §2 B2 | The exception names only failure at the distance-iteration cap. `b3TimeOfImpact` also returns a failed fraction when the initial separation of the witness points falls below its tolerance; the solver can accept that fraction too. `src/distance.c`, `src/solver.c`. | Describe both failed-return paths under the existing solid-result exception. |
| 4 | L | §2 B1 | A dynamic candidate’s `shape->aabb` is not always its end-of-step box. CCD can stop a fast non-bullet body at an intermediate fraction and rewrite its AABB there before a bullet queries the dynamic tree. `src/solver.c`. | Describe it as the candidate’s current stored AABB; retain the conclusion that it is used for ordering only. |

## 3. Checked, no change

The source supports the per-query collection and sort, static–kinematic–dynamic query order, callback filter sequence, strict solid acceptance, accepted-candidate iteration maxima, and eight-entry sensor buffer with its final fraction filter. B2’s stated custom-filter and sensor differences follow from that flow. The design adds no structure or control-flow check prohibited by WORKFLOW §Acceptance criteria: box3d (AC-1–AC-4).

## 4. Proposed edits

Revise §2 B1’s bound and numeric rationale to cover the configurable margin and accepted sweep range. Then revise §2 B2’s unchanged-solid-results claim to depend on that established bound. Correct the failed-return and dynamic-AABB descriptions as noted above.

## 5. Unresolved

The owner’s sensor-cull choice in §5 remains open. No findings about other documents.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of round records
or the changelog in the log (U-T2).

| # | Verdict | Check against live source |
|---|---|---|
| 1 H | Confirmed in principle, severity M | `B3_SPECULATIVE_DISTANCE` is `#ifndef`-overridable (`include/box3d/constants.h`); the default is four slops but "at least four" is not guaranteed. `b3TimeOfImpact` accepts a hit at `target + tolerance`, 1.25 slops. The default configuration keeps the bound; a build that overrides the constant may not. |
| 2 M | Confirmed | Without `BOX3D_DOUBLE_PRECISION`, `b3RoundDownFloat` and `b3RoundUpFloat` are plain `(float)` casts (`include/box3d/math_functions.h`), so "rounded outward" is false in the default build. The slop margin still dominates for realistic sweeps; the claim is mis-worded, and the hundred-kilometre limit is an unenforced precondition. |
| 3 L | Confirmed | A second failed return exists: `s1 < target - tolerance` sets `b3_toiStateFailed` with fraction `t1` (`src/distance.c`). |
| 4 L | Confirmed | `src/solver.c` rewrites `shape->aabb` at the interpolated transform when a non-bullet fast body hits, before bullets query the dynamic tree. |

### Resolution

The round paused under U-S1 and U-S4 and was reported to the owner with three options: keep B2 and
fold the findings as exceptions, cut B2 to its own later doc, or restrict B2 so the bound holds by
construction. The owner chose the third (U-S3).

| # | Resolution |
|---|---|
| 1 | Applied. The key growth carries a two-slop term of its own, covering the 1.25-slop distance at which `b3TimeOfImpact` accepts a hit; the bound no longer cites `B3_SPECULATIVE_DISTANCE`. |
| 2 | Applied. "Rounded outward" and the rounding helpers leave the bound; the growth gains `8 * FLT_EPSILON * M`, with `M` the largest coordinate the key touches, which bounds every rounding for any sweep length. The hundred-kilometre precondition is gone. |
| 3 | Applied. The exception describes a failed return generically (the fraction reached, whatever stopped the call) instead of naming one cause. |
| 4 | Applied. A kinematic or dynamic candidate's box is described as whichever box it holds when the query runs. |
| §5 | Resolved with the same decision: the skip applies only to candidates the callback flags as convex solids, so sensors, mesh, height-field and compound candidates are always evaluated. B2's sensor difference, its sensor test and the open decision are deleted. |


## Status

- Round 3: 4 findings (H1-M1-L2), 4 applied, 0 declined. Series total: 12 findings.
- Findings: 3 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: findings clustered in §2 B1/B2's bound for the third round running, and round 3 F3 is a sibling of round 2 F3 (U-G6). Paused under U-S1 and U-S4. The fold replaced the bound's premises rather than adding to them: it removed the dependence on the fat-box margin, on the outward-rounding helpers, on a sweep-length precondition and on an enumeration of failed-return causes, and deleted the B2 sensor test and the open decision. It added one named mechanism, the `convexSolid` flag on the candidate element; re-checked against Part B: a property of the candidate recorded at collection, compared with the key in the skip, no shadow state and no check on control flow beyond the bound itself.
- Doc length 201 -> 223 lines (F1 +3, F2 +4, F3 -2, F4 +1; the convex restriction and sensor exemption +16 across B2, §4 and §6; §5 -1).
- Next: round 4, same prompt at the same scope (U-P4).

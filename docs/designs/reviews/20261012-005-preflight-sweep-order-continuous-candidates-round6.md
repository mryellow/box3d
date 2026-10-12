---
title: Round 6 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round5.md
mode: fresh, full pass, read-only; prompt description corrected to the one-change document
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

The proposed radius bounds the fast shape, and using the key only for ordering preserves evaluation of every collected candidate. The design needs to qualify its nearest-first claim and its benchmark comparison. The review was read-only.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §1, §2, §6 | The key does not guarantee nearest-first order, even for static candidates. `src/distance.c` rotates the shape through the sweep; an off-center shape centroid can follow a curved path while the proposed key casts only the segment between its endpoints. That segment can miss the grown box and assign key 1 to a candidate hit during the rotation. No collision is lost because the candidate is still evaluated, but the stated ordering and cheap-rejection rationale are too strong. | Describe nearest-first as a heuristic. State that rotation can assign key 1 to a real hit and that the measured gain is scene-dependent. |
| 2 | L | §2, Costs | The “2 to 3 percent of the stage” cost is not supported by the cited rows. In `20261012-006 §3`, that range comes from non-bullet **total** times; the bullet continuous-stage rows show a larger cost against the ascending-id PR. The measured prototype also computed keys before the filters that this design moves ahead of key computation (`20261012-006 §2, §6`). | Label the figures as prototype measurements, distinguish total time from continuous-stage time, and avoid attributing the exact deltas to the proposed implementation. |

3. **Checked, no change**

- `src/solver.c` collects, sorts, and evaluates each tree query separately in static, kinematic, dynamic order. Its running fraction, filters, `preSolve`, sensor cap, and final sensor filter support the stated observable-order consequences.
- `src/shape.c`, `src/body.c`, and `src/aabb.h` support the start-box radius construction. The radius bounds the rigid shape about its centroid; `b3RayCastAABB` supplies a segment entry fraction.
- The proposed key sanitation and shape-id tie-break give an ordered sort for valid collected candidates. The design adds no candidate skip or shadow engine population prohibited by `WORKFLOW §Acceptance criteria: box3d` AC-1 to AC-4.

4. **Proposed edits**

Qualify “nearest-first” and the later-candidate cost claim in §1, then correct the measurement wording in §2. No repository files were changed.

5. **Unresolved**

`20261012-006 §1, §2, §6` still describes its static cull as a change proposed by this preflight and says the preflight adds slop padding. Those statements conflict with this document’s explicit no-cull, radius-only design; they are issues in `20261012-006`, not missing work here.

6. **Verdict: CHANGES_PROPOSED**

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). No read of round records or
the changelog in the log (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 M | Adopted | `b3GetSweepTransform` (`src/distance.c`) places the body at `lerp(c1, c2) - R(nlerp q) * localCenter`, so a shape centroid that differs from the centre of mass follows a curve; the key casts the endpoint segment. A hit during rotation can get key 1. The candidate is still evaluated. Doc states the key is a heuristic and the consequence. |
| 2 L | Adopted | `20261012-006` §2: non-bullet rows are total step time, bullet rows the continuous stage; the prototype computes keys before the filters. Costs figures labelled accordingly and the bullet result added. The same ambiguity in "10 to 20 percent of the stage" (20261012-004 §5 data are totals) is removed from §1 (U-G6). |

Unresolved item (another document): 20261012-006 §1, §2 and §6 describe the cull as part of this preflight and the preflight's
padding as two slops and a rounding term. That no longer matches this preflight. Raised with the owner.

## Status

- Round 6: 2 findings (H0-M1-L1), 2 applied, 0 declined. Series total: 16 findings (the voided round excluded).
- Findings: 5 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: fixes added no named mechanism and removed none. Findings no longer cluster in one section after the cut (§1, §2 Costs). Neither finding is a consequence of the previous round's fix; both concern statements that predate the cut. Doc length 132 -> 137 lines (F1 +3, F2 +2).
- U-S1 does not apply: this round's problems are local wording, not structural.
- Next: round 7, same prompt as round 6 at the same scope (U-P4).

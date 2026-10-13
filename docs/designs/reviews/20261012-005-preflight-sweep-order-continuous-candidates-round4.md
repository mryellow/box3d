---
title: Round 4 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round3.md
mode: fresh, full pass, read-only, same prompt as round 1; target revised by another agent after round 3 (convex-solid-only cull, derived padding)
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

B1’s ordering and the stated sensor behavior match the live solver. B2’s claim that it changes solid results only for failed TOI returns is not established: a convex TOI can report `Hit` without confirming surface proximity at the returned fraction. The numerical argument for the key also needs correction before that key controls a skip.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B1–B2 | The bound assumes every convex `Hit` occurs within 1.25 linear slops. In `src/distance.c`, TOI can return `Hit` from a separation-axis value at an advanced fraction without rechecking actual shape distance there. The source notes a known edge/face case where that separation can disagree with the distance query. Thus B2 can skip a candidate whose `Hit` fraction B1 would accept; this is not limited to `Failed` returns. | Prove the bound for every TOI return the solver accepts, or revise B2 so this return path remains evaluated. Update the results claim accordingly. |
| 2 | M | §2 B1 | The rounding proof says every value used by the key has magnitude at most `M`. A static candidate’s box can extend far beyond `M`, and `b3RayCastAABB` in `src/aabb.c` creates normalized directions and slab-division intermediates. The stated half-epsilon sum therefore does not justify `8 * FLT_EPSILON * M` as a conservative allowance. | Derive a bound for the active slab faces and actual operations, or specify a conservative key calculation before using it for B2. |

## 3. Checked, no change

The per-query sort and shape-id tie-break, argument-only filters, strict solid acceptance, inclusive sensor admission, shared eight-hit buffer, accepted-candidate iteration maxima, and static/kinematic/dynamic query order match `src/solver.c`. Static boxes are fixed during the step; bullet candidates whose boxes may change during parallel bullet work are filtered before key calculation. The quoted performance figures match the cited measurements. The design adds none of the structures or checks prohibited by AC-1 through AC-4.

## 4. Proposed edits

Revise §2’s lower-bound proof and B2 results statement to cover the TOI separation-axis path and the actual float calculations. If either bound cannot be established, remove the corresponding B2 skip.

## 5. Unresolved

Whether B2 can retain its proposed skip depends on those proofs. No findings about the other documents.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log's only matches for
the reviews directory are `rg` exclusion globs; no round record or changelog was read (U-T2). Before the round, one
stray line break in §1 was rewrapped (U-R9).

| # | Verdict | Check against live source |
|---|---|---|
| 1 H | Confirmed | `b3TimeOfImpact` (`src/distance.c`) returns `Hit` at `t1` when `b3EvaluateSeparation` at the witness indices is at most `target + tolerance`, without rechecking the distance at `t1`. The separation function and the GJK distance can disagree: the source's own `todo` notes diagonal edge contact on faces can give a separation that differs from the distance. A hit can therefore sit where the surfaces are farther apart than 1.25 slops, the fast centroid can be outside the key box at that fraction, and the key can exceed it. The doc's premise (§2 B1 lower-bound paragraph) and its B2 Results claim (only failed returns differ) do not hold for that path. |
| 2 M | Confirmed as wording | The proof states every value has magnitude at most `M`, but a far static box has faces beyond `M`. The faces that decide the entry lie within about `M`, since the entry point is on the segment; the far faces only push the other axes' entry beyond the segment. The bound is plausible, the stated reason is not. |

Not folded: finding 1 breaks the premise the owner's restriction (B2 holds by construction) was built on. Whether B2 may change a result wherever time of impact reports a fraction below the key, or is cut, is the owner's decision (U-S3).

## Status

- Round 4: 2 findings (H1-M1-L0), 0 applied, 0 declined, pending owner. Series total: 14 findings.
- Findings: 4 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> not converged.
- Tripwire: findings in §2 B1/B2's bound for the fourth round running. Finding 1 is a consequence of the previous revision's own fix: it attacks the new claim about the distance at which `b3TimeOfImpact` reports a hit, written without confirming it against the separation path (U-G7). Doc length 223 -> 222 lines.
- U-S1 and U-S4 apply again. Paused and reported to the owner.

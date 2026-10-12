---
title: Round 2 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round1.md
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

B1’s ordering is consistent with the cited query and sort path. B2 needs changes before the design can support its claims that solid results are unchanged and that sensor selection differs only when the buffer fills.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B2 | Stopping the static loop also skips custom-filter calls. The API permits a thread-safe filter with mutable user context; a skipped call can change a later filter decision and therefore a solid result. The “no solid result changes” argument does not cover this. (`src/solver.c`, `include/box3d/types.h`) | Preserve the calls that B1 would make, or explicitly revise the behaviour guarantee and callback contract. |
| 2 | M | §2 B2 | “A miss returns the interval’s upper end” is false for mesh and height-field candidates. Their no-hit path can return a zero-initialized output with fraction 0. A static mesh sensor beyond the running fraction can therefore be retained before B2 and skipped after B2 even with an empty sensor buffer. (`src/shape.c`, `src/solver.c`) | Account for these TOI paths in the sensor claim and settle §5 accordingly. |
| 3 | M | §2 B1–B2 | The lower-bound argument covers geometric impacts, but B2 relies on a bound for every fraction the solver accepts. `b3TimeOfImpact` can return a positive `b3_toiStateFailed` fraction while still separated, and the solver accepts positive fractions without checking the state. The document does not establish that the key bounds those returns. (`src/distance.c`, `src/solver.c`) | Establish the bound for accepted failure outputs, or revise the cull so it cannot discard them. |
| 4 | L | §1, §6 | The claim that a candidate touching at sweep start has key 0 applies to a fixed static box. A moving candidate’s `shape->aabb` is its end-of-step box, so its start overlap need not yield key 0. (`src/solver.c`) | Qualify the claim as applying to static candidates. |

## 3. Checked, no change

The source supports per-query collection and sorting, the static–kinematic–dynamic query order, the shared eight-entry sensor buffer, strict solid acceptance, maxima over accepted-candidate iteration counts, and static AABB construction with speculative padding. The quoted B1 cost and B2 call reduction match the cited measurements. B1/B2 add none of the structures or checks prohibited by AC-1 through AC-4.

## 4. Proposed edits

Revise §2’s B2 behaviour and proof around custom filters, mesh and height-field sensor returns, and failed TOI outputs. Qualify the key-zero statements in §§1 and 6.

## 5. Unresolved

The owner’s §5 choice about applying B2 to sensors remains open. No findings about the measurement documents.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The only log match for
round records or the changelog is the prompt's own out-of-scope sentence (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 H | Adopted | The custom filter is called in the evaluation loop before the time-of-impact call (`src/solver.c`), so the stop skips its calls. `b3CustomFilterFcn` (`include/box3d/types.h`) is documented thread-safe, not side-effect free. Doc states the skipped calls and that a filter that is a function of its arguments alone is unaffected. B1 already changes the call order. |
| 2 M | Adopted | `b3ShapeTimeOfImpact` (`src/shape.c`) for mesh and height field starts from a zero-initialised output and updates it only on improvement, so a miss returns fraction 0; the solver admits `0 <= running fraction`, the end filter keeps `0 < final fraction`, and the hit reaches `sensor->hits`. Round 1's "a miss returns the interval's upper end" holds for convex candidates only. Doc rewritten; compound not claimed (only partly read). The open decision on sensors stays, and as written B2 now changes mesh and height-field sensor hits regardless of buffer fullness. |
| 3 M | Adopted | `b3TimeOfImpact` (`src/distance.c`) returns `b3_toiStateFailed` with fraction `t1` at the 25-iteration cap with the shapes still separated; the solver accepts any fraction in `(0, running)` without reading the state. `t1` can be below the key. Doc states B2 skips such a candidate. Rare corner; not worth a mechanism (AC-4). |
| 4 L | Adopted | The key-0 claim relies on a fixed box; a kinematic or dynamic `shape->aabb` is end-of-step. Qualified to static candidates in §1 and §6. |

Owner decision still pending: whether the static stop applies to sensors (§5).

## Status

- Round 2: 4 findings (H1-M2-L1), 4 applied, 0 declined. Series total: 8 findings.
- Findings: 2 rounds: 2/0/2 -> 1/2/1 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: fixes added no named mechanism. Findings clustered in §2 B2, which also produced findings in round 1 (two rounds running; U-S4 pauses at three). Round 2's findings 2 and 3 are in claims round 1 reworded or left standing in B2, but none was caused by a round 1 fix. B2's "nothing observable" argument has now needed three exceptions (custom filter, sensors, failed returns); U-T4 applied: B2 re-checked against Part B, it is a bound that hoists existing pruning and adds no structure or check, so it stands.
- Doc length 193 -> 201 lines (F1 +2, F2 +3, F3 +4, F4 0 net of rewrap); the failed-return text is longer than its finding warrants and is the first candidate to cut if round 3 lands on it (U-T3).
- Next: round 3, same prompt at the same scope (U-P4).

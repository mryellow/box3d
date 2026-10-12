---
title: Round 8 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round7.md
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

The proposed key is a heuristic, and the design keeps every candidate that passes the existing filters. Its ordering and rollback reasoning is consistent with the live solver. Three low-severity statements need correction.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §1 | The claim that a convex candidate at or beyond the running fraction returns after the upper-end separation check is too absolute. When separation is near the target, `b3TimeOfImpact` can advance to that fraction and run another distance query (`src/distance.c`). | Describe the cheap return as the case where the upper end remains separated. |
| 2 | L | §2 | The reported candidate-array bytes follow its **capacity**, not the count it currently holds (`src/container.h`, `src/physics_world.c`). | Replace “count it holds” with “allocated capacity.” |
| 3 | L | §4 | Start-overlapping candidates are not all rejected. A sensor’s fraction-0 result can enter the sensor buffer and survive its end filter (`src/solver.c`, `src/distance.c`). | Limit the statement to solid candidates and describe the sensor exception. |

## 3. Checked, no change

The start-box radius bounds the rigid shape about its centroid; a rotating centroid can still leave the endpoint segment, as the design says (`src/solver.c`, `src/shape.c`, `src/body.h`). The ray-cast key is therefore an ordering estimate, and assigning misses key 1 remains safe because no candidate is skipped (`src/aabb.c`, `src/aabb.h`).

The filters moved to collection are engine-state checks; custom filtering and `preSolve` remain in sorted evaluation order. Shape-ID ties give a total order within each query. The static, kinematic, and dynamic query order and the sensor end filter match the design (`src/solver.c`, `src/body.c`). The proposed transient candidate array adds no shadow population or reconciliation prohibited by AC-1–AC-4.

## 4. Proposed edits

Apply the three wording corrections in §1, §2, and §4 above. No change to the proposed ordering or no-skip decision is needed.

## 5. Unresolved

The separate B2 cull measured in 20261012-006 §6 has an unsound “cannot improve” premise. An offset fast shape can rotate through a static convex candidate while its centroid’s endpoint segment misses the grown candidate box; the shipped rotation limit still permits such an arc (`src/solver.c`, `include/box3d/constants.h`). If another solid has already reduced the running fraction, B2 can skip that hit. This is a finding about 20261012-006, not a missing part of this design.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log's only match for this doc's files is a `git status` listing of file names; no record or changelog content was read (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 L | Adopted | `b3TimeOfImpact` (`src/distance.c`) returns separated only when the separation at the interval's upper end exceeds target plus tolerance; otherwise it advances to that fraction and runs another distance query. Sentence limited to a candidate still separated at the upper end. |
| 2 L | Adopted | `b3Array_ByteCount` (`src/container.h`) is capacity times element size. "Count it holds" replaced by "allocated capacity". |
| 3 L | Adopted | A sensor at fraction 0 is admitted (`0 <= running fraction`) and survives the end filter, as found in round 2. Out-of-scope bullet limited to solid candidates. |

Unresolved item (another document): the reviewer argues 20261012-006's separate cull has an unsound "cannot improve" premise, since a rotating offset shape can hit a candidate its centroid segment misses. This matches the heuristic caveat already in this preflight; it concerns the cull this document does not propose. Raised with the owner.

## Status

- Round 8: 3 findings (H0-M0-L3), 3 applied, 0 declined. Series total: 23 findings.
- Findings: 7 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> 0/0/3 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: no named mechanism added or removed. Findings in §1, §2 and §4, none in a section that produced findings last round except §2 (round 7 memory sentence, which finding 2 corrects: a consequence of the previous fix, U-T2). Doc length 135 -> 135 lines.
- Next: round 9, same prompt at the same scope (U-P4).

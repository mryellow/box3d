---
title: Round 7 review of 20261012-005 preflight, sweep-ordered continuous candidates
target: docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md
date: 2026-10-12
reviewer: codex gpt-6-sol, effort high
previous: 20261012-005-preflight-sweep-order-continuous-candidates-round6.md
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

The proposed key changes evaluation order without using it to skip candidates. The main design defect is a large-world case where float AABBs make distinct nearby candidates tie at key 0, defeating the intended ordering. Three smaller claims need correction.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, §6 | In double-precision mode, `shape->aabb` remains an outward-rounded float box. Around x = 1e8, a small shape’s start box can span roughly 16 m. Its derived radius, combined with similarly rounded candidate boxes, can give several distinct candidates key 0 and restore id order. The claimed nearest-first behavior then fails in a supported large-world scene. `src/solver.c`, `src/shape.c`, `include/box3d/math_functions.h` | Define how the key preserves local precision in large-world mode, or explicitly limit the performance claim and account for this case. |
| 2 | L | §1 | “Every call” first doing one distance query and later candidates being rejected cheaply describes convex TOI calls too broadly. Mesh, height-field, and compound candidates can perform zero or multiple inner TOI calls; sensor hits do not narrow the running fraction. `src/shape.c`, `src/solver.c` | Restrict this cost argument to the applicable convex candidates after an accepted solid hit. |
| 3 | L | §2 | “Memory statistics are unchanged” conflicts with the doubled element size: the reserved candidate array contributes its byte count to `b3World_DumpMemoryStats`. Moving filters also can change its peak capacity. `src/physics_world.c` | Say the accounting code is unchanged, while reported memory can change. |
| 4 | L | §2 | The quoted prototype ranges do not match all rows: non-bullet B1 costs about 1.3–2.0% over ascending-id order in +x, and gains about 12.5–15.3% against farthest-first order. `20261012-006 §3` | Correct the ranges or label them as approximate. |

## 3. Checked, no change

The start box gives a conservative radius for supported fast shapes; a missed or late key cannot suppress evaluation. The shape-id tie-break gives a deterministic order for equal finite keys, and the proposed NaN rule prevents an unordered sort key. Moving the listed engine filters before key computation preserves their decisions; the custom filter remains in evaluation order. The proposal adds no structure or check prohibited by AC-1 through AC-4.

## 4. Proposed edits

Resolve finding 1 in the key definition. Narrow the §1 cost explanation, correct the memory-statistics sentence, and align the §2 figures with the cited rows.

## 5. Unresolved

`20261012-006 §1, §6` still describes a preflight that proposes a static cull and extra key padding. This design proposes neither. That is a finding about **20261012-006**, not a missing part of this design.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Exit 0, header shows `gpt-6-sol` / `high`. Raw log saved before reading (U-V1). The log shows a search that
printed a line of another document's changelog (20261012-002) and a read of WORKFLOW.md beyond Part B; neither
is this doc's record or changelog (U-T2).

| # | Verdict | Check against live source and resolution |
|---|---|---|
| 1 M | Adopted as a stated limit | In double-precision builds `b3OffsetAABB` (`include/box3d/math_functions.h`) rounds a world box outward to float; far from the origin a float step is metres. The start box's farthest-corner `r` and the candidate boxes then span several candidates, which tie at key 0 and sort by id. Not a determinism issue; the doc states where the gain applies. |
| 2 L | Adopted | The cheap-rejection sentence is true for convex candidates after a solid hit; mesh, height-field and compound paths run inner calls, and sensors do not lower the running fraction. Sentence restricted. |
| 3 L | Adopted | `src/physics_world.c` adds `b3Array_ByteCount` of `continuousCandidates` to the task context bytes, so the reported bytes change with element size. Sentence corrected. |
| 4 L | Adopted | Recomputed from 20261012-006 §3: B1 against the PR in +x is +2.0, -1.2 and +1.8 percent; against the PR in -x -12.5, -13.9 and -15.3 percent; against `main` -2.8, -7.2, -3.7 (+x) and -2.4, -3.9, -4.0 (-x). The doc's "2 to 3" and "14 to 15" were wrong; corrected, and the drift stated. |

Unresolved item (another document): 20261012-006 §1 says B1 is "2 to 3 percent behind" ascending id in +x, which its own §3 table does not show (-1.2 to +2.0); §1, §2 and §6 also describe the cull as part of this preflight. Raised with the owner.

## Status

- Round 7: 4 findings (H0-M1-L3), 4 applied, 0 declined. Series total: 20 findings.
- Findings: 6 rounds: 2/0/2 -> 1/2/1 -> 1/1/2 -> 1/1/0 -> 0/1/1 -> 0/1/3 -> pending confirmation. Not converged (U-V9, U-S8).
- Tripwire: no named mechanism added or removed. Findings did not cluster in a section two rounds running (round 6 touched §1 and Costs; round 7 touches §1, §2 and Costs, which round 6 edited: Costs is a repeat, and finding 4 is a consequence of round 6's relabelled figures being checked against the table for the first time). Doc length 137 -> 142 lines (F1 +3, F2 +0, F3 +2, F4 +3 net of rewrap). The Costs paragraph grew past its job, so it was cut back to the work added with a pointer to 20261012-006 §1 before round 8 (U-T3); that cut takes the doc to 135 lines.
- Next: round 8, same prompt at the same scope (U-P4).

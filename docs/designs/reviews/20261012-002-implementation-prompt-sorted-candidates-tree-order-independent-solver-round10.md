---
title: Review round 10 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round9.md
mode: full pass
---

# Round 10

## Exact request sent to Codex

```
Review the document docs/designs/20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md: the instructions an implementing agent follows to land that design's three changes (A0 test helper, A1 CCD candidate order, A2 Explode order) as three commits, with the test mechanism, fixtures, test-to-requirement mapping, verification commands and PR/closure requirements. The design decides what changes and why; the Implementation Prompt decides how it lands and how it is shown to work. Test mechanism, fixtures and mapping are in scope here, not out of scope as they are for a design doc.

Scope: the Implementation Prompt's own content, the design it implements (read it in full; it is binding and its section 5 Provenance decisions are settled), and the source both cite (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, src/solver_set.c, src/container.h, include/box3d/, test/, shared/, benchmark/, docs/simulation.md, CONTRIBUTING.md, CMakeLists.txt, CMakePresets.json). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the Implementation Prompt makes about it.

Pressure-test the prompt. Find real defects: claims about the code or build that are false; an instruction that contradicts the design or re-decides something the design settled; a fixture that cannot do what the prompt says (cannot be made to fail before the change, cannot pass after it, cannot observe what it asserts through the API named, or asserts something the design does not require); a witness or hash that does not measure what the prompt claims; a helper contract with a hazard the prompt does not state (world locking, recording, sleep, multi-worker, allocation, assertion builds); a command that does not work in this repository; a mapping row whose evidence does not establish its clause; a missing instruction without which the implementer must guess; contradictions between sections. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h, b3Hash, the test runner) as settled tools; judge what the prompt adds and the question each primitive is asked.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, other design documents, and the design's own decisions (cite them; do not re-open them).

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0,
2026-10-12T09:13:40+10:00 to 09:16:20+10:00. Same prompt as rounds 1 to 9 against revision 10. U-R9: a
read of the section revision 10 changed found no mangling.

## Review report (Codex final message)

## 1. Summary

The prompt follows the canonical design’s A0–A2 sequence and describes the main source paths accurately. Four test instructions need correction: one witness cannot be captured through the specified runner, and three fixture claims overstate what their setup establishes.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A0, §4 A1 | `RunScenePair` offers only a pre-step hook, but the fuzz test requires first-step **post-step** witnesses for the kinematic and dynamic trees. The next pre-step hook occurs after another perturbation, so it cannot witness the tree CCD queried in the preceding step (`src/broad_phase.c`, `src/solver.c`). | Add a post-step hook, invoked before the next perturbation, and specify that these two witnesses use it. |
| 2 | M | §4 A1, §6 | The four-worker fixture has at most 22 bullets: 20 field bullets and two corridor bullets. With a bullet block size of eight, `b3ParallelFor` dispatches at most three bullet tasks, contrary to the claim that bullet CCD dispatches to four workers (`src/solver.c`, `src/parallel_for.c`). | Use at least 25 bullets, or narrow the claim to several tasks in a four-worker world. |
| 3 | M | §4 A1, §6 | A corridor in which the sphere hits only one box need not produce any pre-change hash difference. Moving that sole hit away from the lowest shape id does not itself make the final TOI depend on visit order; the stated reason that query order “reaches the hash” is therefore insufficient (`src/solver.c`). | Require and demonstrate a pre-change divergence for each corridor; give placement guidance involving multiple relevant hits or another measured order-sensitive outcome. |
| 4 | M | §4 A2 | The explosion witness queries with `B3_DEFAULT_MASK_BITS`, while `b3World_Explode` uses `explosionDef->maskBits`. The fixture never specifies that field; a zero-initialised definition queries nothing. Its falloff-branch claim also needs the **shape distance** to exceed `radius`, rather than merely placing a body centre in the band (`src/physics_world.c`, `src/joint.c`). | Require `b3DefaultExplosionDef()` or an explicit matching mask, and specify the falloff sphere’s closest-surface distance. |

## 3. Checked, no change

- The direct full rebuild, query traversal, tree rebuild timing, and recording and lock cautions match `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/physics_world.c`.
- The CCD filter, sensor, pre-solve, and per-worker array instructions match `src/solver.c`, `src/sensor.c`, and `src/physics_world.h`.
- The explosion wake and impulse path, test entry-name filtering, CMake target names, and benchmark output descriptions match the cited source and build files.
- The prompt preserves the settled choices in canonical design §5.

## 4. Proposed edits

Revise the A0 runner contract, the A1 worker count and corridor calibration instructions, and the A2 explosion definition and distance fixture. Update the corresponding §6 evidence rows to reflect those corrected tests.

## 5. Unresolved

The proposed tests do not yet exist, so their fail-before and pass-after results cannot be verified in this review.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All four findings checked against the live tree (U-V2). All adopted.

1. **M, no post-step hook (adopted).** Confirmed from the document: revision 5 removed the after-step
   hook, revision 9 required post-step witnesses for the kinematic and dynamic corridors, and the next
   pre-step hook comes after another perturbation. The pair runner regains an after-step hook, called
   before the next perturbation, and the two witnesses name it.
2. **M, four-worker bullet count (adopted).** Confirmed: twenty field bullets plus two corridor bullets
   is 22; with a minimum range of 8, `b3ParallelFor` forms three blocks and enqueues three tasks
   (`src/parallel_for.c`). The rerun now uses forty-eight fast bodies, half bullets, so bullets number
   at least 26 and awake bodies well over 64, four blocks in each stage.
3. **M, single-hit corridor cannot diverge (adopted).** Confirmed from `src/solver.c`: a separated
   candidate returns the running fraction and leaves it unchanged, so with one on-path box the final
   fraction is the same in any order; divergence needs a second on-path box whose fraction bounds the
   first's time-of-impact call. Every corridor now has at least two on-path boxes, and the placement
   guidance is corrected.
4. **M, explosion mask and falloff distance (adopted).** Confirmed: `b3World_Explode` queries with
   `explosionDef->maskBits` and only `b3DefaultExplosionDef` sets it (`src/physics_world.c`,
   `src/joint.c`); the falloff scale applies when the shape distance, not the centre distance,
   exceeds `radius`. The fixture starts from the default definition, the witness uses the
   definition's mask, and the band body's closest surface is placed beyond `radius`.

The raw log's hits on `designs/reviews` and `changelog` are a `git status` listing and the request's
own out-of-scope line; no read of round records or the changelog.

## Status

Round 10: 4 findings (H0-M4-L0), 4 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0. Series
total 44 (H5-M30-L9). Not converged.

Tripwire. U-T1: one hook restored, nothing removed. U-T2: findings 1 to 3 are in §4 A1 tests; 1 and
2 are consequences of earlier fixes (revision 5's hook removal against revision 9's post-step
witness; the round 6 workload against the round 8 corridor bullets); 3 corrects the round 7
placement guidance; 4 is in §4 A2. Each fix is a clause. U-T3: 422 lines before the fixes; the after
count is recorded in the changelog. U-T6: over the split check; one decision, recorded in round 6.

Criteria review threshold reached (ten rounds without converging, B8): reported to the owner with
this record. Of the 44 findings, none named a shape Part B forbids in the shipped code (AC-1 to AC-4);
all were in test evidence, commands and measurements, and 15 of them were consequences of earlier
fixes to the same fixtures. The owner decides whether to reconsider Part B or the prompt's level of
fixture detail.

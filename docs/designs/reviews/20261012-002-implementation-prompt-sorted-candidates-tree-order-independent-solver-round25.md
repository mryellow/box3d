---
title: Review round 25 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round24.md
mode: full pass
---

# Round 25

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
2026-10-12T11:12:44+10:00 to 11:15:22+10:00. Same prompt as rounds 1 to 24 against revision 26. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The prompt largely tracks the canonical design and the live CCD, sensor, and explosion paths. I found two implementation instructions that need correction, plus two smaller inaccuracies in its source and PR guidance.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1 and A2 | Both increments instruct the implementer to use `QSORT`, but neither names the required `#include "qsort.h"`. Neither `src/solver.c` nor `src/physics_world.c` currently includes it; `src/broad_phase.c` does. Following the stated mechanics literally will not compile. | Explicitly require the include in both files. |
| 2 | M | §4 A1 | Moving the callback body “from the ‘skip same shape’ test” omits the existing assertion immediately before that test in `src/solver.c` that the fast shape is not a sensor. That conflicts with design §2 A1’s instruction to keep the callback body unchanged. | Specify where the assertion stays in the split callback. |
| 3 | L | §2 | “A proxy enters its tree marked moved” is too broad. `src/broad_phase.c` marks dynamic and kinematic insertions, but a static insertion is marked only when pair creation is forced. | Qualify the statement; the corridor’s dynamic and kinematic layout argument remains valid. |
| 4 | L | §8 | The PR template’s checkbox says “run all samples,” while the prompt permits ticking it after merely running the samples app. Launching the app does not establish that all samples ran. | Require running all samples before ticking the box, or leave it unticked and explain what was run. |

## 3. Checked, no change

- The per-query CCD sort, eight-hit sensor behavior, and ordered explosion processing match design §§2 and 5.
- The proposed sensor events and awake-set inspection paths exist in the cited source.
- The post-step witness timing for dynamic and kinematic trees is supported by the rebuild, refit, and enlargement paths.
- The test runner filters by entry name. The CMake targets and executable paths in §7 match repository configuration. These commands were checked against files, not executed.

## 4. Proposed edits

Apply the four corrections in the findings table. No design decision needs revisiting.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All four findings checked against the live tree (U-V2). All adopted.

1. **M, `qsort.h` include not named (adopted).** Confirmed: only `src/broad_phase.c` includes it. The
   A1 and A2 code paragraphs now name the include for `src/solver.c` and `src/physics_world.c`.
2. **M, the assertion before the split point (adopted).** Confirmed: `b3ContinuousQueryCallback`
   asserts the fast shape is not a sensor immediately before the "skip same shape" test
   (`src/solver.c`). The moved range now starts at that assertion.
3. **L, "a proxy enters marked" too broad (adopted).** Confirmed: `b3BroadPhase_CreateProxy` marks a
   non-static insertion, and a static one only when pair creation is forced (`src/broad_phase.c`).
   The §2 fact is qualified.
4. **L, samples box (adopted).** Confirmed: the template line is "run all samples"
   (`.github/pull_request_template.md`). §8 now ticks it only after running every sample, and
   otherwise says what was run.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 25: 4 findings (H0-M2-L2), 4 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2. Series total 77 (H7-M53-L17). Not converged.

Tripwire. U-T1: no mechanism added or removed; two include lines, a range boundary, two
qualifications. U-T2: finding 3 is a consequence of the round 22 fix (the §2 rebuild fact);
findings 1, 2 and 4 are in sections (A1 and A2 code paragraphs, §8) that had not produced findings
since round 13 or earlier. Each fix is a clause. U-T3: 472 lines before the fix; the after count is
recorded in the changelog. U-T6: over the split check; one decision, recorded in round 6.

---
title: Review round 19 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round18.md
mode: full pass
---

# Round 19

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
2026-10-12T10:22:58+10:00 to 10:26:36+10:00. Same prompt as rounds 1 to 18 against revision 20. U-R9:
the target is untracked so there is no diff; the sections revision 20 changed were read whole and the
§7 block was parsed with `bash -n`; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The implementation path and most source claims match the canonical design. Two verification defects prevent the prompt from reliably showing that A1 and A2 work: several required layout witnesses cannot differ in the prescribed scenes, and the fail-before command accepts a fixture failure as evidence of a behavior change.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 A0, A1, A2; §6 | The required first-step kinematic and dynamic CCD witnesses, and the post-sleep Explode witness, can be vacuous. Newly created nonstatic proxies are marked moved; the step rebuilds their trees before CCD. A prior full rebuild preserves those marks, so both runs reach the same rebuilt tree. In the zero-gravity Explode scene, the initial step likewise rebuilds the dynamic tree, and subsequent stationary sleep steps leave it stable (`src/broad_phase.c`, `src/dynamic_tree.c`, `src/solver.c`). Changing box placement alone does not establish a different visit order at those calls. | Specify scene preparation that leaves the unperturbed tree in a layout changed by the full rebuild at the **actual** CCD or Explode call. For Explode, initially sleeping bodies can avoid the canonicalizing step. Require the resulting witnesses before claiming coverage in §6. |
| 2 | M | §3, §7 | `fail_before` treats any test exit of 1 as the expected failure. The runner also returns 1 when a witness, setup condition, or other fixture assertion fails first (`test/main.c`, `test/test_macros.h`). Thus the recorded exits can satisfy §7 without demonstrating the claimed pre-change divergence. | Require and record the specific failing assertion for each entry test, after its witness and setup checks pass. Reject fail-before runs that fail for another reason. |

## 3. Checked, no change

- The A1 sort and per-worker array instructions match the CCD query order, callback gates, array lifecycle, and sort idiom (`src/solver.c`, `src/physics_world.c`, `src/physics_world.h`, `src/broad_phase.c`, `src/qsort.h`).
- The sensor event path and A2 wake, impulse, and lock instructions match the cited code (`src/sensor.c`, `src/body.c`, `src/solver_set.c`, `src/physics_world.c`).
- The test target, entry-name filter, benchmark options and CSV output, and contributor issue requirement match the repository files. The commands were inspected, not executed.

## 4. Proposed edits

Revise §4’s CCD and Explode scene preparation and §6’s witness mapping to require differing visit sequences at the calls being tested. Revise §7 to capture and check each test’s diagnostic failure, not just its exit code.

## 5. Unresolved

The revised fixtures need a demonstrated visit-order difference and fail-before run; this document review did not execute them.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Neither is folded: the series pauses (U-S4,
U-S3) and both are carried forward to the owner.

1. **H, first-step kinematic and dynamic corridor witnesses and the post-sleep Explode witness can be
   vacuous (confirmed; carried forward).** A non-static proxy is inserted with the moved mark on its
   path (`src/broad_phase.c`, `src/dynamic_tree.c`). The step's rebuild of the dynamic and kinematic
   trees is `b3DynamicTree_Rebuild( tree, false )` (`src/broad_phase.c`): it descends every marked
   internal node, so on a tree whose leaves were all just inserted it gathers every leaf and builds
   from the same leaf array, in the same gather order, that a full rebuild of that tree gathers. The
   build (`b3BuildTree`) sets no moved mark on internal nodes, so after the helper's full rebuild the
   root is unmarked and the tree is ordered, and the step's rebuild returns before touching it. Run 1
   therefore reaches at the first step's CCD the tree that run 2's perturbation built, and run 2's
   step leaves it as it is: both runs query one layout, whatever the placement. The sanity test's
   "change the placement rule until it differs" cannot succeed for a kinematic or dynamic corridor
   stepped first, and the §9 stop condition is reached by construction. For Explode, the sleep steps
   leave the dynamic tree as the first step built it; the perturbation then rebuilds a tree that was
   itself built by the same partition from the same leaves, and any difference comes from the gather
   order inside the Hoare partition (`b3PartitionMid`, `src/dynamic_tree.c`), not from placement.
   The static corridor and the sensor test are unaffected: the step does not rebuild the static tree
   (`src/physics_world.c`). Feasibility of the reviewer's Explode remedy confirmed: a body created
   with `isAwake` false and sleep enabled goes into its own new sleeping solver set at creation and
   still gets its proxies (`src/body.c`, `src/shape.c`), so no step is needed before the explosion
   and the distinct-set precondition holds by construction. A remedy for the corridors needs the
   unperturbed tree non-canonical at the corridor step, which the prescribed create-then-step scene
   never gives; a shape inserted after a step would give it, which changes the corridor contract,
   the pair runner's hooks, the fuzz corridors, the callback test and the §6 witness rows. Two
   directions are viable, with different fixture shapes (U-S3): scene preparation inside this
   document, or a different perturbation primitive, which is a design decision (design §2 A0). The
   owner decides.
2. **M, an exit of 1 does not show which check failed (confirmed; carried forward).** `ENSURE` prints
   `condition false:` and returns 1 for any failed check, fixture or divergence alike
   (`test/test_macros.h`). §3 already has the closure doc quote both runs of each test, so the quoted
   line is the evidence and the exit code never was; the document does not say which line the
   fail-before run must show. This is the fourth consecutive §7 finding (rounds 16 to 19), after the
   round 18 cut to one token: U-S4 pause. The cut to apply before any further §7 patch is deletion
   (U-F4): the shell function is machinery that has drawn five findings in four rounds, and the
   requirement is two sentences in §3 (build the new tests against the previous commit in a scratch
   worktree; each must fail there on its divergence check and pass in the working tree; quote both).

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 19: 2 findings (H1-M1-L0), 0 applied, 0 declined, 2 carried forward. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0. Series total 65 (H6-M46-L13).
Not converged. Paused under U-S4 (§7, four rounds running) and U-S3 (finding 1 forks the fixture
design); the document stays at revision 20.

Tripwire. U-T1: no change to the doc this round. U-T2: finding 2 is in §7 for the fourth round
running and is a consequence of the round 18 fix (exit 1 as the expected result); finding 1 is in
the corridor contract of §4 A0, which produced findings in rounds 14, 15 and 17, and is the first
finding against the first-step scene itself. U-T3: 470 lines, unchanged. U-T6: over the split check;
one decision, recorded in round 6. B8: the series passed the criteria review threshold at round 10;
the §7 pattern (a command block absorbing corrections) is input to reconsidering Part B.

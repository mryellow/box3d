---

title: Codex review — bit-exact history ring, round 11
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 11

## Review report (Codex final message)

## Summary

The whole-world rewind recommendation is plausible, and the document identifies the main source-level dependencies. I found two correctness gaps in the proposed journal rules and two places where the capture and verification contracts need tightening. I read the design and checked it against `src/`, `test/`, `docs/faq.md`, `docs/simulation.md`, and `CMakeLists.txt`. I did not read `docs/designs/reviews/` or modify files.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1–7.3 | The “verified by construction” rule has no protected write path for a shape’s heap material array. `b3Shape_SetSurfaceMaterial` and `b3Shape_SetMeshMaterial` write through `b3GetShapeMaterials` in `src/shape.c`; an awake shape’s image copies its record but excludes that pointee. The design defines a material-block journal entry, yet its stated accessor and container hooks do not force these writes through it. | Define a material write accessor that captures old and new element or block values, migrate both setters, and make direct writable material access unavailable to callers. Add awake and sleeping material-setter cases to restore and replay tests. |
| 2 | Medium | §6, §8 | The branch is truncated on the first **journal entry or capture**, but §7.1 deliberately does not journal between-step setters on awake owners. After `Rewind(T)`, such a setter changes the live world while later slots remain available for forward scrub. That conflicts with the rule that no retained tick belongs to another branch. | Invalidate future slots on the first world mutation after rewind, including unjournaled awake setters; state explicitly whether a subsequent forward scrub is allowed to discard that mutation. |
| 3 | Medium | §6 | Slot reservation is described as sized from awake counts and staging bytes. The image also has variable-sized manifold and mesh caches, island link arrays, shapes per awake body, moved proxies, and sensor overlaps. Awake body counts alone cannot determine its size. | Specify an exact size prepass or a bounded writer that accounts for every image component before reserving the slot. |
| 4 | Medium | §10, §12 | The proposed hash is called an oracle for all simulation-affecting state, but callback-visible `userData` is restored while the hash is described as engine-semantic and excludes host state. A wrong restored `userData` pointer could leave restore hashes equal and change replay through a callback. Callback registration is also expressly outside rewind. | Narrow the hash claim and separately test restoration of `userData` with callback-driven replay; document that hash equality alone cannot establish the caller contract. |

## Checked, no change

- `ScrubBackward` in `test/test_recording.c` compares the existing `b3HashWorldState` after seeks; that hash covers body transforms and velocities, matching the design’s qualification.
- The FAQ’s rollback statement, the documented 64-bit determinism claim, and the compiler flags in `CMakeLists.txt` are represented fairly.
- Sensor overlaps are recomputed each step even for sleeping owners; `overlaps2` is the cross-step overlap array. The CCD sensor cap and running-fraction dependency described in §7.4 are present in `src/solver.c`.
- The three trees have independent proxy free lists, and broad-phase create and destroy functions are the relevant proxy operation boundary. The document correctly distinguishes proxy identity from tree layout.
- Sleeping solver sets, contact manifolds, mesh caches, and shared hull refcounts have the separate ownership concerns the journal inventory identifies.
- The phase-0 serializer clears body, joint, and shape `userData`, as the document warns.

## Proposed edits

Resolve findings 1–3 in the mechanism and API contract, then add the targeted material, branch-invalidation, and callback-data cases to §12. Qualify the full-state hash claim as described in finding 4.

## Unresolved / disagreements

The document already names the order-dependent `preSolve` exception. Solver-owner acceptance of the proposed CCD, sensor-hit, and explosion order changes remains an open decision; this read-only review does not settle their numerical impact. I treated §11.1’s figures as supplied context.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:20:01+10:00, ended 12:24:33+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` before and after the call showed the same set (the target doc and
`WORKFLOW.md` modified, the untracked round-6 to round-10 files), so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 6 to 10, no declined-findings list (round 10
declined nothing). Before the round the round-10 edits were checked intact and against AC-1 through
AC-4 with no failure.

**Findings verified against source.**

1. **Applied.** `shape.c` writes materials in place: `b3Shape_SetSurfaceMaterial`,
   `b3Shape_SetMeshMaterial`, and the friction and restitution setters all assign through
   `b3GetShapeMaterials( shape )[i]`. The doc's material block entry existed but no accessor forced
   these writes through it, so completeness rested on the setters remembering to call it (AC-2).
   §5.2's const-access rule now covers each shape's `materials` block, and the material block entry
   is written by one `b3Shape_WriteMaterial` accessor: an element write carries the index and old
   and new element bytes (not the whole array, which for a mesh can be large), a resize or
   replacement carries the whole array. Test 2's churn already includes setters on sleeping bodies;
   the awake case is covered by the same churn with no new clause.
2. **Applied.** Between-step setters on an awake owner make no journal entry (round 10's clarification
   of §7.1), so a truncation trigger of "first journal entry" left a mutated live world with later
   slots still forward-scrubbable, and a forward scrub would silently discard the mutation while a
   journaled mutation would instead make the later slots unavailable. §8 now truncates on the first
   write through any write accessor or journaled container, journaled or not, and on the first
   capture. Truncation at the accessor is a state transition, not a validity check that changes what
   the write does. After a rewind the pending segment is empty, so no entry is lost by truncating
   at the first write.
3. **Applied.** §6 and §8 sized the slot from "the awake counts", which cannot give the size of
   manifold blocks, mesh caches, island link arrays, shapes per body, moved leaves or sensor
   overlaps (`b3Contact.manifoldCount`, `b3Island` arrays, per-body shape lists). §6 step 1 now
   defines the image count as a parallel reduction over those quantities, with the moved leaves
   counted by the enumeration round 10 added; §8 refers to it.
4. **Applied, as a narrowing.** `userData` is a host pointer in the imaged `b3Body`, `b3Shape` and
   `b3Joint` records, so a wrong restored value would leave an engine-state hash equal while changing
   a callback's behaviour. It cannot be hashed by value for a cross-machine hash. §12 now names
   `userData` as excluded, says the hash is the oracle for engine state and not host state, test 2
   compares a distinct `userData` word per body, shape and joint against the value recorded at T
   under setter churn, and test 3 runs with `preSolve` and custom filter callbacks that depend on
   `userData`. §10's caller contract already states that callbacks and host state belong to the
   caller; no change there.

4 applied, 0 declined.

## Status

Finding count held at 4 (1H-3M-0L), the same as rounds 9 and 10. Finding 1 is new ground found by
looking at setters the inventory never listed, a completeness gap of the kind AC-2 targets and now
closed by a choke point. Finding 2 is a consequence of round 10's finding 1 fix, which said awake
API writes make no entry without following through on the truncation rule's trigger. Finding 3 is
an imprecision in the §6 sizing that has been present since the first draft. Finding 4 is a claim
stronger than the hash supports. No finding reopened a stated rationale. The count is not falling
and each round still finds mechanism-level gaps at the edges, so the series is not close to
converging.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 11: 4 findings (1H-3M-0L), 4 applied, 0 declined`

`Series total: 72 findings (34H-35M-3L) across 11 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L)`

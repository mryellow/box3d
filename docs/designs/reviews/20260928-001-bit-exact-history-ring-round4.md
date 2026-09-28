---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=high
mode: broad (round 4), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 4

## Review report (Codex final message)

## Summary

The whole-world image and journal approach is plausible, but the current draft does not yet establish bit-exact rewind. I found missing journal coverage for heap data, two ways replay can diverge after tree reconstruction, and a memory-budget omission.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1 | Material setters edit `shape->materials` in place. A journal entry containing only the `b3Shape` record saves the pointer, not the old material values. Rewind can retain future friction or restitution. `shape.c`, `shape.h` | Journal the changed material element’s old and new bytes; cover single and multiple material shapes in restore tests. |
| 2 | High | §5.2, §7.1 | Destroying a sensor frees its three inner arrays before removing its record. The proposed sensor record journal would retain dangling pointers; sensor arrays are absent from the ownership-transfer rules. `sensor.c`, `sensor.h`, `shape.c` | Add explicit sensor-array ownership or value entries for destruction and swap removal, with undo, redo, and eviction rules. |
| 3 | High | §10 | Callback configuration is left at its value from P after `Rewind(T)`. Replaying a change made later cannot restore the callback needed for steps immediately after T, so filtering or solving can differ. `physics_world.c`, `physics_world.h` | Restore callback pointers and contexts at T, or require the caller to reinstall T’s configuration before any replay; test a change inside the replay interval. |
| 4 | High | §7.4, §10 | `b3World_CastRayClosest` does not break equal-fraction ties by shape id as claimed. Its callback overwrites the result on each hit, and the tree accepts equal fractions. A rebuilt tree can therefore return a different shape to a caller that uses the query as an input. `physics_world.c`, `dynamic_tree.c` | Implement a deterministic tie rule and test equal-distance hits across rewind. Specify that returned tree-visit diagnostics can change with layout. |
| 5 | Medium | §5.2, §7.1, §10 | Mesh and height-field shapes retain caller-owned geometry pointers. A caller may free that data when destroying the live shape, while an older retained tick still needs it after rewind. `shape.c`, `include/box3d/box3d.h` | Extend the documented geometry lifetime through history eviction, or retain geometry in history and account for its bytes. Include baked compounds in the ownership review. |
| 6 | Medium | §7.1, §8 | `maxBytes` counts detached arrays but omits hull allocations kept alive by journal-held references. Unique large hulls changed or destroyed during the window can exceed the reported budget without triggering interval widening. `physics_world.c`, `hull.c` | Charge retained hull bytes to history usage and apply the same budget and eviction policy. |
| 7 | Medium | §2, §6, §9 | Requirement 3 promises no rejection for any tick in the retained window, but `Rewind` rejects retained ticks without images. Those ticks require caller replay from an earlier image. | State the guarantee for **imaged** ticks, or make `Rewind` perform the replay needed for every retained tick. |
| 8 | Low | §11.1 | The timing and copy-size table is attributed to a prior review, while the allowed source and tests do not establish those measurements or a way to reproduce them. The acceptance targets therefore lack a verifiable baseline here. | Add a benchmark command, configuration, and result artifact, or label the figures as estimates pending measurement. |

## Checked, no change

- The existing serializer restores a world into a shell, and `ScrubBackward` checks replayed hashes. That hash covers transforms and velocities, as the draft qualifies. `world_snapshot.c`, `recording_replay.c`, `recording.c`, `test/test_recording.c`
- Id allocation uses a LIFO free array followed by a bump index; generations reside in object slots. `id_pool.c`, `body.c`, `shape.c`
- Broad-phase contact candidates are sorted by shape-pair key before contact creation. `broad_phase.c`
- Sensor overlap changes have a per-step event-bit signal, including changes for sensors on non-awake bodies. `sensor.c`
- The dynamic and kinematic tree rebuild puts retained subtrees into a newly ordered node array when a rebuild is needed. `dynamic_tree.c`, `broad_phase.c`
- Worker-count determinism has dedicated scenario tests; the stronger full-state rollback claim still needs the proposed tests. `test/test_determinism.c`

## Proposed edits

Specify heap payload entries for material elements and sensor arrays; settle callback configuration at the rewind boundary; make closest-ray ties deterministic; define retained geometry ownership and byte accounting. Then align requirement 3 with the API and add focused tests for each case.

## Unresolved / disagreements

The CCD change offers two algorithms. The draft should select one and define candidate ordering and ties before claiming tree layout is irrelevant. The §11.1 measurements remain unverified under this review’s source restrictions; I did not read prior review files.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`,
foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T19:17:46+10:00, end 19:22:45+10:00, `EXIT_CODE 0` — no resume needed, returned inline.
Raw log had the final message duplicated across earlier positions; one copy kept above. Codex ran
read-only; working tree was untouched by it going in (round 3's edits were already applied to the
working file). The curated declined-findings list from rounds 1–3 (six items) was included in
the prompt; Codex did not re-raise any of them.

Each finding checked against source before any doc edit:

- **#1 (in-place material-element edits not journaled), CONFIRMED, applied.** Read
  `b3GetShapeMaterials` (`shape.h`): returns `shape->materials` (a heap array) when non-NULL, else
  `&shape->material` (the shape's own inline field) as a one-element fallback. Traced
  `b3Shape_SetFriction`/`SetRestitution`/`SetSurfaceMaterial`/`SetMaterial` (`shape.c`): all write
  through `b3GetShapeMaterials(shape)[index] = ...`, i.e. in place, into whichever storage that
  returns. For a single-material shape this is the shape's own inline field — already covered by
  the existing generic shape-record journaling. For a multi-material shape it's an element of the
  heap array — a generic record write of the `b3Shape` struct only captures the (unchanged)
  `materials` pointer, not the element's new content, exactly the class of gap round 3 already
  fixed for contact manifolds and mesh triangle caches. Fixed by adding a rule (§7.1): a
  per-element material write journals that element's own old/new bytes, keyed by shape id and
  element index, distinguishing it from the already-correct single-material (inline) case.

- **#2 (sensor array ownership at destroy), CONFIRMED, applied.** Read `b3DestroySensor`
  (`sensor.c`): it frees `hits`, `overlaps1`, and `overlaps2` via `b3Array_Destroy` *before* the
  swap-remove — round 3's fix made `overlaps2` content journaled (a generic record write keyed by
  shape id, fired whenever `eventBits` is set), but never added the array itself to the
  ownership-transfer list for the *destroy* event, so a destroy occurring between a retained T and
  the current position P would leave nothing to hand back on undo — the array is already freed by
  the time any entry could reference it. Confirmed `overlaps1`/`hits` need no such treatment (both
  fully recycled/rebuilt on the next step regardless of a rewind, per round 3's own analysis, and
  a rewind happens at a step boundary before any task runs). Fixed by adding a destroyed sensor's
  `overlaps2` array to the existing ownership-transfer list (§7.1), alongside sets/islands/
  materials/triangle-caches — reusing the mechanism already there rather than inventing a new one.

- **#3 (callback configuration not restored at T), CONFIRMED, applied.** Re-read §10's own
  wording: "the caller must replay a callback-configuration change at its correct point during
  resim, exactly like any other setter" — this is not actually true of any other setter. For a
  body/shape/joint setter, the *engine* restores the T-time value automatically (image or
  journal); the caller only replays *later* changes. For a callback, §10 already states `Rewind`
  does not restore or undo it at all — meaning the caller must do strictly more than for any other
  setter (reinstall the T-time value itself, not just replay post-T changes), which "exactly like
  any other setter" denies. Fixed by removing that comparison and stating the caller's actual,
  stronger obligation directly: reinstall the T-time callback immediately after `Rewind(T)`, then
  replay later changes as usual.

- **#4 (`b3World_CastRayClosest` tie-break claim was false), CONFIRMED, applied — corrects round
  3.** This is my own round 3 error: I added "breaking ties by shape id" to §7.4 without checking
  the actual callback. Read `b3RayCastClosestFcn` (`physics_world.c`): it unconditionally
  overwrites `rayResult` on every invocation with no comparison against the previous fraction, so
  an exact-fraction tie is resolved by whichever candidate the tree visits *last*, not by shape
  id — traversal-order-dependent, exactly the property §7.4 is supposed to remove. Fixed by
  correcting the claim to describe actual current behavior, and adding the matching fix (only
  overwrite on strict improvement, or a tie with a lower shape id) as a third required engine
  change alongside the CCD and Explode order changes, updating §16 Q1 and §11.3's change list to
  match.

- **#5 (caller-owned mesh/height-field/compound geometry lifetime), CONFIRMED, applied.** Read
  `b3CreateHeightFieldShape` and `b3CreateBakedCompoundShape` (`shape.c`): both store the passed
  `const b3HeightFieldData*`/`const b3CompoundData*` pointer directly via `b3CreateShape(...)`,
  with no clone or copy — unlike a hull, which is explicitly cloned into a refcounted
  engine-owned database (`b3AddHullToDatabase`, confirmed in round 1/2's own verification). This
  means round 2's hull-reference fix cannot be reused here: the engine has no database entry to
  take an extra reference on, because it never owned a copy in the first place. Since deep-copying
  potentially large mesh/heightfield/compound data into the journal on every destroy would be
  disproportionate to this design's own "proportional to churn" philosophy, and there is no
  engine-side lifetime to extend, fixed by adding a caller-contract obligation instead (§10): the
  caller must keep such a buffer alive through the shape's destruction, for as long as history
  could still reference a tick that used it — not merely for the shape's own lifetime.

- **#6 (`maxBytes` omits hull bytes held by a journal reference), CONFIRMED, applied.** Re-read
  §8's `maxBytes` accounting sentence: it says the budget covers "the arrays ownership-transfer
  journal entries hold" — but a retained hull reference (round 2's fix) isn't an array the entry
  itself holds a copy of; it's an extra refcount on data that lives in the world's hull database,
  external to the journal arena. The existing sentence doesn't naturally extend to it. Fixed by
  adding hull-database bytes kept alive by a journal-held reference to the same accounting
  sentence, alongside detached arrays.

- **#7 (requirement 3 vs. requirement 4's now-corrected imaged/retained distinction), CONFIRMED,
  applied.** Round 3 fixed requirement 4's wording to distinguish "imaged" (directly restorable)
  from "retained" (imaged or not, reachable by replay) — but requirement 3 has the identical
  ambiguity ("restore never rejected for any tick inside the retained window") and I missed it in
  that pass. `b3World_Rewind`'s own check (§9 step 1: "Check T is imaged... else
  `b3_historyTickUnavailable`") confirms only imaged ticks are directly restorable. Fixed
  requirement 3 to match requirement 4's corrected wording.

- **#8 (§11.1 figures unverifiable under this round's own no-reviews-directory constraint),
  DECLINED — not a doc defect.** The doc already cites its source correctly
  (`docs/designs/reviews/...-perf.md`, an existing file); Codex's own review setup excluded
  reading anything under `docs/designs/reviews/`, so it couldn't check a citation that legitimately
  points there. This is an artifact of the review's scope restriction, not a gap in the target
  document. Not applied; no doc change needed.

- **Unresolved item (CCD change "offers two algorithms"), checked, no change.** Re-read §7.4's
  "Change:" paragraph: "evaluates every AABB candidate against the initial fraction and takes the
  min... (or collects candidates and evaluates in shape-id order)" describes two equivalent
  implementation strategies for the same order-independence property (a fixed reference point vs.
  an explicit sort), not two different *results*. Not a genuine ambiguity about behavior; left as
  Codex's own "unresolved" item, not promoted to a finding.

## Status

Round 4 of an ongoing series (rounds 1–3 committed or pending commit). 7 of 8 findings were
genuine and applied; 1 (#8) was an artifact of this round's own review-scope restriction, not a
document defect. One finding (#4) is a direct, source-confirmed correction of an error *I*
introduced in round 3 while fixing a different, related claim in the same sentence — the sharpest
form of the non-convergence signal this workflow tracks for, since it shows a round's own fix can
introduce a fresh, checkable-and-wrong claim in the same breath as a correct one. Two more (#2,
sensor array ownership; #7, requirement 3) are gaps in territory round 3 edited without covering
every sibling case (round 3 journaled sensor overlap *content* changes but not the array's
*destruction*; round 3 fixed requirement 4's imaged/retained wording but not requirement 3's
identical ambiguity) — exactly the "check every rule against every sibling case" failure mode the
workflow's pre-prompt step exists to catch, and in both cases it should have been caught in round
3 itself. The finding rate across four rounds (9, 8, 8, 7 applied) is not narrowing meaningfully,
and every round so far has found at least one defect traceable to the immediately preceding
round's own edits. Another full-scope round is warranted, not a narrower one.

---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 20), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 20

## Review report (Codex final message)

## Summary

The design accounts for most of the state and transitions I traced, but its journal rules have gaps that can prevent an exact restore. In particular, undoing a colour-bit clear can create a bit that was never set. The restore procedure also needs a narrower way to find affected proxies to meet its stated cost bound.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1 | The `bitset set/clear` entry says to undo a clear by setting the bit. Graph removal and sleep clear both bodies' bits, including a static body whose bit was never set. Undo would introduce a spurious colour bit, affecting later colour assignment (`src/constraint_graph.c`, `src/solver_set.c`, `src/bitset.h`). | Record the old bit value, or emit an entry only when the operation changes it. |
| 2 | Medium | §5.2, §7.1 | The inventory includes sleeping-set creation by `b3CreateBody` and destruction by `b3MergeSolverSets`, but the entry kinds and ownership rule describe set creation on sleep and destruction on wake. A merge destroys arrays that undo must recover; creating an asleep body also needs a reversible set entry (`src/body.c`, `src/solver_set.c`). | Define set create/destroy entries for every caller, including ownership transfer on merge. |
| 3 | Medium | §7.1 | The dense-cold-array entry specifies reattachment and swap-remove reversal but no append reversal. Appends occur in static and disabled solver-set arrays, sleeping-set merge targets, island links, and `sensors[]`; restoring element records alone does not restore array lengths (`src/body.c`, `src/solver_set.c`, `src/island.c`, `src/shape.c`). | Specify an append entry with its owning array, old length, and appended value, and define undo and redo. |
| 4 | Medium | §7.4 | Restore says to check every shape's owning-body set and proxy, while claiming tree repair costs `O(moved · log n)`. A scan of all shapes makes a seek in a mostly sleeping million-body world proportional to world size (`src/physics_world.h`, `src/body.c`, `src/shape.c`). | Specify how restore enumerates affected shape IDs from images and journal entries, including enable/disable transitions. |

## Checked, no change

- Awake contact collection covers both coloured touching contacts and non-touching contacts in the awake set (`src/constraint_graph.c`, `src/solver_set.h`).
- Sensor overlaps are sorted before comparison, and the task supplies a change bit when their logical contents differ (`src/sensor.c`).
- The separate proxy-category entry is justified: a filter change can leave the live proxy category unsynchronised (`src/shape.c`).
- Name-cache entries are retained rather than removed when names change, so restoring a record's `nameId` does not leave a missing name entry (`src/name_cache.c`, `src/body.c`, `src/shape.c`).

## Proposed edits

1. In §7.1, replace "clear / set" with: "Restore the bit's recorded old or new value; omit entries for operations that leave it unchanged."
2. In §7.1, rename the set entries "set create" and "set destroy," and state explicitly that they cover asleep-body creation and sleeping-set merges as well as sleep and wake.
3. Add an "append to dense cold array" case to §7.1, defining its payload and both replay directions.
4. In §7.4, replace the "for every shape" restore instructions with an affected-shape worklist and account for its construction in the restore cost.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-28T22:16:02+10:00, end 22:18:57+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 3 minutes. Raw log had the final message duplicated (a
`codex exec` streaming artifact); one copy is kept above. Codex ran read-only; the working tree was
untouched by it going in (round 19's edits were already applied to the working file, confirmed via
`git status` before the call). The same curated declined-findings list used for rounds 18–19 (ten
items) was included in the prompt, since round 19 added no new declines; Codex did not re-raise or
dispute any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, round 19's own new
§7.4 bullet (the disabled-body proxy-presence fix) was re-read once specifically for cost, since it
was the freshest, least-scrutinized addition to the restore algorithm. That re-read did not catch
what finding #4 below did (an unscoped "for every shape" reading, not caught until Codex's
independent full-scope pass traced it against §7.4's own O(moved · log n) claim) — recorded here
because it is exactly the kind of self-check miss worth being honest about rather than silently
folding into the finding's own verification.

All four findings verified directly against source:

- **#1 (the `bitset set/clear` entry's "undo a clear ⇒ set" rule doesn't distinguish a clear that
  actually changed the bit from a clear that was already a live no-op, so undo can introduce a
  colour-membership bit that was never true), CONFIRMED, applied.** Traced every `b3ClearBit(
  &color->bodySet, ...)` call site (`constraint_graph.c`'s `b3RemoveContactFromGraph`/
  `b3RemoveJointFromGraph`, `solver_set.c`'s sleep-transition body loops): every one clears
  **both** edge bodies' bits unconditionally, with the source itself carrying the comment "this
  might clear a bit for a static body, but this has no effect" at each site — confirmed by tracing
  colour assignment (`b3AssignContactColor`/`b3AssignJointColor`, same file): for a static-dynamic
  pair, only the dynamic body's bit is ever `b3SetBitGrow`'d; the static body's bit is never set in
  the first place, so its "clear" on removal is a genuine, common, unconditional no-op on live
  state. A journal entry describing this unconditional "clear ⇒ undo sets" would, on undo, install
  a colour-membership bit for a static body that was never live at any point in the recorded
  history — corrupting a value `b3AssignContactColor` itself reads (`b3GetBit`) to decide colour
  admission for the *next* contact/joint touching that body, a genuine simulation-affecting
  divergence, not merely a cosmetic one. Confirmed no equivalent problem exists on the "set" side:
  every `b3SetBitGrow` call site is preceded by a `b3GetBit` check within the same colour that
  already guarantees the bit isn't already 1. Fixed the minimal way (Codex's own first proposed
  option): the journal hook (which this design adds at each site, not the underlying
  `b3ClearBit`/`b3SetBitGrow` call) checks the bit's current value before performing the mutation
  and emits an entry only when it actually changes — eliminating the entry class this finding
  describes without needing a stored old-value payload.

- **#2 (the "set create"/"set destroy" entry rows are parenthetically labeled "(sleep)"/"(wake)"
  even though §5.2's own caller list already includes `b3CreateBody` and `b3MergeSolverSets`),
  CONFIRMED, applied — for a narrower reason than the finding's own proposed action implies.**
  Traced `b3CreateBody`'s asleep-body path (`body.c`): `b3AllocId(&world->solverSetIdPool)` then a
  zero-initialized `b3Array_Push` onto `world->solverSets` — structurally identical to
  `b3TrySleepIsland`'s own set-creation preamble, already fully covered by the existing "set
  create" entry's generic (set index, ownership handle) payload; no different mechanism is needed.
  Traced `b3MergeSolverSets` (`solver_set.c`) end to end: like `b3WakeSolverSet`, it reads each
  element out of the source set's arrays via a forward loop (`set2->bodySims.data + i`,
  `b3Array_Emplace` into the target) **without** ever swap-removing from the source array itself,
  then destroys the still-fully-populated source set in one shot via `b3DestroySolverSet` at the
  end — the exact same "whole array discarded with real content still in it" shape the existing
  "Ownership transfer instead of copying" paragraph already documents for wake, and which the
  paragraph's own prose already lists merge under ("The same applies to an island destroyed by a
  merge or split"). So the underlying mechanism was already correct and already generic for both
  callers in each row; the only actual defect was the misleadingly narrow parenthetical label.
  Fixed by dropping "(sleep)"/"(wake)" from both row names, leaving the payload/mechanism
  unchanged. Declined the "ownership transfer on merge" half of the proposed action as already
  satisfied by the existing mechanism, and declined adding a *new* reversible entry for
  `b3CreateBody` on the same basis — both would have duplicated a mechanism already correctly
  generic, the same over-fix risk avoided for `b3MergeSolverSets`' own destroy call in round 18's
  bitset finding.

- **#3 (the "dense cold arrays" entry specifies swap-remove reversal but never append reversal,
  even though `b3Array_Emplace`/`b3Array_Push` appends into these same arrays are pervasive —
  wake, sleep, merge, transfer, and sensor/island link creation all append), CONFIRMED, applied —
  the most significant finding of this round.** Grepped every `b3Array_Emplace`/`b3Array_Push`
  call against the arrays §5.2 already lists as cold (solver-set dense arrays, island link arrays,
  `sensors[]`): forty call sites across `body.c`, `solver_set.c`, `island.c`, `sensor.c`, none of
  which the "dense cold arrays" row (even after round 18's own fix to the *swap-remove* half of
  this exact row) said anything about. This is a real gap in my own immediately-preceding round's
  fix: round 18 fixed the row's swap-remove semantics without checking whether the row's *other*
  documented operation (append, implied by "owning id, ownership handle" but never given its own
  undo/redo rule) was equally complete — exactly the "check every sibling case in the same
  inventory row" failure the workflow's pre-check step exists to catch, missed by my own round 18
  self-check because that pass was scoped to a different hazard class (pointer lifetime) than this
  one (payload completeness). Confirmed append's correct semantics are simpler than swap-remove's,
  not symmetric to it: since `b3Array_Emplace`/`b3Array_Push` only ever grow at the true tail (no
  swap involved), undo is an unconditional length-truncate by one and needs no stored payload to
  perform; redo needs the appended element's own bytes stored explicitly (consistent with this
  design's existing practice of storing both directions' bytes explicitly, e.g. `record write`,
  `manifold block`, rather than deriving redo from live state elsewhere in the segment, which would
  require append and destroy entries within one segment to be replayed in a specific relative
  order this design doesn't otherwise depend on). Fixed by extending the "dense cold arrays" row
  with an explicit append case, payload and undo/redo, alongside the existing swap-remove case.

- **#4 (§7.4's restore bullets are scoped to "awake shapes at T, plus shapes with journaled
  records" — a candidate set bounded by the ring's own O(awake + journal) cost model — except the
  bullet round 19 itself just added, which reads "for every shape" unscoped, literally requiring a
  world-sized scan), CONFIRMED, applied — corrects round 19's own fix.** Re-read every existing
  §7.4 bullet's candidate-set wording precisely: "for every shape whose fat AABB differs..." is
  parenthetically scoped ("awake shapes at T, plus shapes with journaled records"); the
  categoryBits, body-type, and proxy-reset bullets all key off a *journaled* fact (§5.2), each
  implicitly ranging over the same bounded population. Round 19's own bullet had no such scoping
  clause — literally "for every shape whose owning body's restored `setIndex` is..." — which, read
  as written, requires iterating every shape in the world to evaluate the condition, directly
  contradicting §7.4's own stated `O(moved · log n)` bound and requirement 2's `large_world`
  (1M bodies, 10 awake) cost target the whole design is built around. Confirmed the fix doesn't
  need a new journal entry (as finding #2 might have primed me to reach for): `b3Body_Disable`/
  `b3Body_Enable` already call `b3TransferBody` into/out of `b3_disabledSet`, an already-structural,
  already-unconditionally-journaled function (§7.1's general rule) — so "every body with a
  journaled transfer into or out of `b3_disabledSet` in the walked range" is already a directly
  available, bounded (O(journal events)) candidate set, with no new bookkeeping required. Fixed by
  replacing round 19's unscoped bullet with this properly-scoped version: enumerate candidate
  *bodies* from their own already-journaled disabled-transfer entries, then reconcile each such
  body's owned shapes' proxy presence from its already-restored `setIndex` — same correctness,
  same O(moved · log n)-consistent cost, only the enumeration source changed.

## Status

Round 20 of an ongoing series (rounds 1–19 committed or pending commit). All four findings genuine
and applied, none declined — matching round 18's pattern (2/2 applied) and round 19's (1/1
applied): three consecutive full-scope rounds with a 100% apply rate and zero repeat/stale
findings, a notably different signal than the long stretch of low-count, mixed-decline rounds
earlier in the series (rounds 7–17). Two of this round's four findings (#3, #4) are direct,
source-confirmed corrections of gaps in the *immediately preceding* round's own edits — round 18's
own fix to the swap-remove half of "dense cold arrays" without checking the append half in the
same row, and round 19's own new restore bullet omitting the candidate-set scoping every sibling
bullet in the same list already carried. Both are exactly the self-correction pattern this series
has repeatedly shown (rounds 4, 5, 7, 13, 15 each corrected their immediate predecessor once) and
exactly what the workflow's from-scratch, full-scope, no-narrowing design is meant to catch: a
round reasoning forward from what it just wrote is worse-positioned to see its own gaps than a
fresh round with no memory of why the text reads the way it does. Findings #1 and #2 are, similar
to round 18, in previously-unexamined territory (bitset undo asymmetry, entry-kind caller-scope
accuracy) rather than repeats of any earlier pattern. This is the third round of the
currently-requested three-round batch (rounds 18–20); per that instruction, stopping here. Nothing
in this round's finding rate or content suggests the series is near `CONVERGED` — if anything, the
last three rounds' 100% apply rate and the density of genuine defects still being found in
well-trodden mechanisms (the journal entry table, touched by nearly every one of the prior 17
rounds) argue for continuing at the same full scope whenever review next resumes, not narrowing.

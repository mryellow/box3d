---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 28), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 28

## Review report (Codex final message)

## Summary

The hot image and cold journal split is broadly consistent with the source, including the wake and sleep bounds checkpoints and proxy reconstruction rules. I found two gaps: the proposed cross-platform state hash needs a precise representation, and the time-sliced replay example skips the order required for exact simulation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12, §14 | The public hash promises the same value across platforms, but its treatment of pointer-bearing geometry and allocations is unspecified. `b3Shape` contains pointers to materials, hulls, meshes, height fields, and compounds (`shape.h`). Hashing struct bytes would include addresses and layout; hashing geometry by value requires an explicit traversal rule. | Specify field-by-field hashing of logical values and geometry contents, excluding addresses, allocation capacity, and padding. Add a cross-platform comparison for the expanded hash. |
| 2 | Medium | §11.2 | The time-sliced example proposes replaying three old ticks "plus the live tick" each frame. `b3World_Step` advances one state at a time (`physics_world.c`); a new live tick cannot be stepped until every intervening replay tick has been processed. As written, the example could lead callers to apply inputs out of order. | Describe replaying a contiguous prefix each frame, then stepping new inputs only after the backlog is caught up. State the resulting presentation delay. |

## Checked, no change

- Sensor overlap journaling can use the sensor task's change bit: it compares the completed overlap lists and sets the bit when their logical contents differ (`sensor.c`).
- Proxy category bits can differ from a shape's current filter, and proxy resets can change its key even when bounds stay equal (`shape.c`, `dynamic_tree.c`).
- Pair keys are sorted before contact creation, supporting the document's tree-layout argument for pair order (`broad_phase.c`).
- Id allocation uses a free-list pop or bump index, and object generations increment on creation (`id_pool.c`, `body.c`, `shape.c`, `joint.c`).

## Proposed edits

1. In §12, define the canonical bytes or fields for each hashed state type, particularly external geometry, heap arrays, and pointer-bearing records. Make §14's portability claim conditional on that definition and verification.
2. Replace §11.2's "three replayed ticks per frame plus the live tick" example with contiguous replay batches; explain when normal live stepping resumes.

## Unresolved / disagreements

The document and source alone do not establish whether the expanded hash will agree across supported platforms. That needs implementation and a cross-platform test.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T07:35:39+10:00, end 07:39:02+10:00,
`EXIT_CODE 0` — no resume needed, returned inline in under 3.5 minutes. Raw log had the final
message duplicated (a `codex exec` streaming artifact); one copy is kept above. Codex ran
read-only; `git status` before the call confirmed the working tree held only this session's own
pre-prompt pre-check edit (see below) plus a pre-existing, unrelated uncommitted edit to
WORKFLOW.md from before this round started, and Codex made no file changes.

**Declined-findings list used for the prompt.** Before writing the prompt, the curated
declined-findings list carried by rounds 24–27 was audited for accuracy, since round 27's file
labelled it "8-item" while only naming 7 items. Tracing the list's lineage from round 18 (10 items)
through round 21 (drops the resolved world-`userData` item, 9 items) confirmed the 2 items missing
from rounds 24–27's list — the journal-size-known-up-front distinction and the `maxBytes`
soft-target naming — were correctly dropped, not lost: both are now answered directly in the
document's own text (§8's existing image-vs-journal sizing paragraph, and requirement 5's existing
over-budget-window exception), so per WORKFLOW.md step 2's exception a fresh reviewer no longer
needs them as curated context. The "8-item" label in rounds 24–27 was therefore a miscount in
those rounds' own prose (their actual carried list was 7 items), not evidence of a missing 8th
item; round 28's prompt carried the correct 7-item list (sleep→wake AABB bounds, proxy numeric-id
reconstruction, the allocation-free-restore half of `maxBytes`, §11.1 figures "unverifiable" under
review scope, the CCD "Change:" paragraph's two equivalent phrasings, state-hash joint/solver-array
storage order, and pool `nextIndex` hash coverage). Codex did not re-raise or dispute any of them.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Checked whether §12 point
4 (the cold-hash guard)'s own enumeration names every case §5.2's inventory lists as journaled,
since the guard's stated purpose is to hash "every cold structure §5.2 lists as journaled... not a
separately maintained subset that can drift out of sync with §5.2" — the same
one-case-against-its-siblings check this step requires, applied to §12.4 against §5.2's rows.
Traced each §5.2 row against §12.4's prose list: bodies/non-awake sims/contacts/joints map to
"non-awake records"; shape/joint fields map to "shape and joint fields journaled regardless of
owner awake state"; shape bounds, pools, pair set, colour bitsets, tree proxy `categoryBits` and
reset entries, and sensor overlap content each have an explicit term — **but `islands[id]` and
its link arrays (§5.2's own row, whose mutators — `b3CreateIsland`, `b3DestroyIsland`,
`b3MergeIslands`, `b3SplitIsland`, link/unlink — are all structural and therefore always
journaled per §7.1's Rule for record writes, exactly the kind of row the guard exists to catch)
had no matching term anywhere in §12.4's list, and neither did `sensors[]` create/destroy
(existence) as distinct from `sensors[shapeId].overlaps2` content.** Confirmed by re-reading
§12.4's exact wording before editing. Fixed by adding "islands and their link arrays" and "sensor
existence" to §12.4's enumeration, alongside the terms already covering their sibling rows.

Both of Codex's findings verified directly against source:

- **#1 (§12's state hash lists shapes' "filter, material, geometry" as hashed but never says how a
  pointer-bearing field is turned into hash input — `b3Shape`'s geometry union holds `const
  b3HullData*`, `b3Mesh` (itself pointer-bearing), `const b3HeightFieldData*`, and `const
  b3CompoundData*`, and its `materials` field is a heap array for multi-material shapes — so a
  naive implementation hashing the shape struct's raw bytes, or the pointer values themselves,
  would embed process-local addresses into a hash §14 explicitly claims is "Same on any platform,
  worker count"), CONFIRMED, applied.** Read `shape.h` in full: confirmed the geometry union's
  four pointer-bearing members and the separate `materials`/`materialCount` heap-array pair (a
  single-material shape uses the inline `material` field instead, already unambiguous). Read the
  existing `b3HashWorldState` (`recording.c`) as the pattern §12 says to extend: it hashes explicit
  field values via `B3_HASH_FLOAT` (a `memcpy` of each float's bits, not a struct-wide hash),
  confirming the design's own established methodology is value-based, per-field hashing — the gap
  is that §12 never extends this stated methodology to the pointer-bearing case, leaving open the
  tempting-but-wrong reading of hashing the union's raw bytes (which differ across platforms via
  pointer width/address and across processes via allocation address, regardless of content).
  Confirmed `b3SurfaceMaterial` (`types.h`) is a plain value struct with an explicit zero `padding`
  field and no pointers, so once the array is identified by `materialCount` its hashing is
  ordinary field-by-field work with no further traversal needed. Fixed by qualifying §12 point 1's
  "material... geometry" phrase: the `materials` array is hashed in full by `materialCount` for a
  multi-material shape, and each geometry union member is hashed by its pointed-to content, never
  by pointer value — preserving the stated cross-platform and cross-process guarantee.
- **#2 (§11.2's time-sliced replay example, "3 replayed ticks per frame plus the live tick", never
  states that these are processed in strict tick order, since `b3World_Step` only ever advances one
  state at a time and there is exactly one current tick position per world — a reader could
  misread "plus the live tick" as the live/current-time tick being available for stepping before
  the backlog is exhausted, which the engine's single-current-tick model cannot actually do),
  CONFIRMED, applied.** Confirmed `b3World_Step` (`physics_world.c`) advances the world by exactly
  one step per call with no notion of an out-of-order or parallel "live" tick coexisting with a
  still-open backlog — there is only ever one `historyTick`/current position (§8), so "the live
  tick" can only ever mean whichever tick comes next in sequence, not a separate concurrently-live
  state. The ambiguity is genuine: nothing in §11.2's existing prose rules out a reader concluding
  the live tick is processed independently of backlog order. Fixed by replacing "3 replayed ticks
  per frame plus the live tick" with an explicit strict-tick-order statement, and adding the
  resulting presentation-delay consequence (render lags real time by backlog length ÷ per-frame
  replay rate) that Codex's proposed action asked for.

## Status

Round 28 of an ongoing series (rounds 1–27 committed). Two Medium findings, both genuine and
applied, in territory no prior round in this series had examined closely: the state hash's
handling of pointer-bearing shape fields (present since round 1's §12, touched by round 21's
`userData` fix and round 25's `userData` pointer-equality-check addition, but neither round traced
the geometry union's own hashability) and the time-sliced-replay example's ordering (present since
an early round's §11 mitigations list, never previously flagged for ambiguity). Neither finding is
a consequence of the immediately preceding round's own fix (round 27 touched §5.2/§6/§10/§13, not
§11.2 or §12), and neither reopens ground the doc's own rationale already covered — both are
first-time findings in a mechanism the series had not yet scrutinized this specifically, the same
new-previously-unexamined-mechanism pattern rounds 26 and 27 each showed. This is the second round
of the currently-requested two-round batch (28–29); proceeding to round 29 next, per that
instruction, since this round's verdict was `CHANGES_PROPOSED`, not `CONVERGED`.

Round 28: 2 findings (0H-2M-0L), 2 applied, 0 declined
Series total: 103 findings (41H-47M-15L) across 28 rounds

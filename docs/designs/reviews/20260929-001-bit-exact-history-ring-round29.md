---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 29), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 29

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the engine's awake solver sets, sparse id records, and step pipeline. I found two ring-budget gaps and one tick-numbering ambiguity that should be resolved before implementation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §2, §8 | Keeping one image *somewhere* in the minimum physical window does not keep every tick in that window reachable. With a two-tick window and an image only at its newer tick, the older tick cannot be restored or replayed from a preceding image. §8 otherwise defines the retained window as starting at `oldestImageTick`. | Define the minimum window in **reachable ticks**. Preserve an image at or before its oldest tick, or state explicitly that the physical minimum can contain fewer reachable ticks. |
| 2 | Medium | §8 | `maxBytes` is said to cap the arena, but the arena grows by amortized realloc. Reserved capacity can exceed the bytes occupied by slots, so the proposed budget and `bytesUsed` cannot be implemented consistently without specifying which bytes count. | Count allocated arena capacity in the budget and reported usage, then specify how growth and eviction enforce the cap, subject to the documented minimum-window exception. |
| 3 | Low | §8, §14 | History numbering starts from `world->stepIndex`, but §8 advances `historyTick` for every `b3World_Step` call. The source increments `stepIndex` only when `b3Solve` runs; a zero-time-step call still updates pairs and sensors without calling the solver (`physics_world.c`, `solver.c`). Enabling history after such calls gives `historyTick` a value that does not count completed step calls. | Define history ticks as an independent sequence beginning at enable time, or specify a separate counter that advances on every completed step call. Clarify how callers map server ticks to it. |

## Checked, no change

- The existing backward-scrub test compares replayed transform and velocity hashes; the document correctly limits what that test demonstrates (`test_recording.c`, `recording.c`).
- Awake solver arrays are contiguous, while contacts live in the sparse contact array, supporting the proposed copy and gather split (`solver_set.h`, `contact.h`, `physics_world.h`).
- The source swaps sensor overlap buffers each step and signals changed overlap sets, consistent with journaling changed current overlaps (`sensor.c`).
- Pair candidates are sorted before contact creation, supporting the tree-layout argument for pair discovery (`broad_phase.c`).
- Pool allocation uses a free-list pop or a bump index, as the journal entry description states (`id_pool.c`).

## Proposed edits

1. In §8, replace "the minimum window always carries at least one image" with an invariant that its **oldest reachable tick is imaged**, and explain eviction when the oldest image would be lost.
2. In §8, define `bytesUsed` and `maxBytes` over allocated arena capacity and retained owned allocations; describe bounded growth under that accounting.
3. In §8 and §14, define the history tick origin and its relationship to `stepIndex`, including zero-time-step calls.

## Unresolved / disagreements

The document and source do not establish which tick numbering callers need for server reconciliation. The choice in finding 3 needs an API decision.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T07:44:06+10:00, end
2026-09-29T07:46:08+10:00, `EXIT_CODE 0`, returned inline in about 2 minutes. The raw log echoed
the prompt and the final `## Review report` block more times than usual (two full prompt
occurrences, four report occurrences, vs. the series' usual one-plus-one streaming duplicate) —
all four report copies were compared and are identical, so this is a more verbose instance of the
same known `codex exec` streaming-duplication artifact, not distinct content; one copy is kept
above. Codex ran read-only; the full diff against round 28's committed doc state contains exactly
the three edits accounted for below, confirming Codex made no file changes itself. The corrected
nine-item curated declined-findings list (reconstructed this session after finding rounds 24–27
had been carrying an undercounted seven-item version — see round 28's own Status section) was
included in the prompt; Codex did not re-raise or dispute any of the nine.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, §5.1's hot-state
inventory was re-read against §12 point 1's state-hash inventory, the same kind of one-case-not-
carried-to-siblings check that produced round 28's cold-hash-guard fix, applied here to the public
hash instead of the cold-hash guard. Traced `b3Island` (`island.c`) field by field: `setIndex`/
`localIndex` are positional bookkeeping restored by array placement itself, matching how contact
and joint `localIndex` fields are already excluded from hashing by the same reasoning elsewhere in
this section — no gap. `islandId` is covered by "island membership." But `constraintRemoveCount`
— confirmed read at `solver.c` (split-candidate selection) and `solver_set.c` (sleep-candidacy
gating), i.e. genuinely results-affecting per §11.2(4)'s own admission that split timing changes
sleep timing, which changes results — was named by neither "island membership" nor "sleep
partition," and the existing Phase 0 serializer (`world_snapshot.c`) already snapshots and
restores it explicitly, underscoring that it was already treated as real state, just not named in
the hash inventory. **Confirmed a genuine gap.** Fixed by adding `constraintRemoveCount` to §12
point 1's island-hashing clause, citing the two source files that read it, before writing the
round 29 prompt.

Both findings, and the Low finding, verified directly against source (this design has no
implementation yet, so verification is against the document's own completeness and
self-consistency, the same standard every prior round has applied to an unimplemented mechanism):

- **#1 (§8's "the minimum window always carries at least one image" doesn't say the image sits at
  or before the window's *oldest* tick, so a minimum window can be physically retained without
  every tick in it being reachable per requirement 4's own imaged-tick definition), CONFIRMED,
  applied.** Re-read requirement 5's own framing ("the minimum window is kept anyway... the one
  image it must carry") and the "graceful degradation" language in requirement 5's own summary:
  the evident purpose of a guaranteed minimum window is to guarantee some minimum *restorable*
  capability under memory pressure, not merely to guarantee some ticks are physically present in
  the arena — a window whose sole image sits at its newest tick would leave the rest of that
  "guaranteed" window unreachable, silently defeating the guarantee's own point while technically
  matching the old wording. Distinct from requirement 4's own carve-out for a tick that "predates
  the oldest surviving image" (that clause describes an *ordinary*, expected eviction-boundary
  state, not the *minimum-window* guarantee's own internal consistency). Fixed by requiring the
  minimum window's required image to sit at or before its oldest tick, explicitly tying this to
  requirement 4's reachability definition rather than leaving "at least one image" ambiguous about
  position.
- **#2 (`maxBytes`/`bytesUsed` don't specify whether they count allocated arena capacity or only
  the bytes live slots occupy, and the two can diverge under this design's own stated
  amortized-realloc growth policy), CONFIRMED, applied.** Re-read §8's existing "Capacity growth...
  is a realloc of the arena... rare after warm-up" bullet and the journal-room "amortized realloc
  growth" bullet two below it: both already establish that the arena's reserved capacity is a
  distinct quantity from its currently-occupied bytes, and neither ties `maxBytes`/`bytesUsed`
  explicitly to one or the other. Since requirement 5 (`Bounded memory`) is a promise about the
  ring's actual memory footprint, capacity is the only reading that keeps that promise meaningful
  — an implementation could otherwise report `bytesUsed` under budget by counting only occupied
  slot bytes while its actual heap footprint (reserved-but-unoccupied capacity from a shrunk-back
  burst) silently exceeded `maxBytes`. Fixed by adding one sentence to the existing `maxBytes`
  paragraph stating both quantities count allocated capacity, not occupied bytes, and pointing at
  the two growth-policy bullets that already establish why the two can diverge.
- **#3 (`stepIndex` (`solver.c`) increments only inside `b3Solve`, which a `timeStep <= 0`
  `b3World_Step` call skips, while that same call still runs the broad-phase pair update and
  sensor task (`physics_world.c`) that §6 already captures — so a world stepped with `timeStep <=
  0` advances `historyTick` (defined as incrementing on capture, i.e. on every `b3World_Step`)
  without advancing `stepIndex`, silently decoupling the two after enable time, when §8's own
  enable-time text seeds `historyTick` from `stepIndex`'s current value as if they stayed in
  correspondence), CONFIRMED, applied.** Read `b3World_Step` (`physics_world.c`) in full: the
  broad-phase pair update runs unconditionally before the `timeStep > 0.0f` branch, and
  `b3Solve` — the only site that increments `stepIndex`, per its own "Only count steps that
  advance the simulation" comment (`solver.c`) — is inside that branch, so a zero-or-negative-
  `timeStep` call genuinely skips it while still doing capturable work. Fixed by adding a
  paragraph immediately after the existing enable-time text, stating `historyTick` is its own
  counter seeded from `stepIndex` at enable but incremented once per subsequent `b3World_Step`
  call regardless of that call's `timeStep`, naming the exact divergence mechanism and warning a
  caller mapping its own tick numbering must track `historyTick` itself, not assume equality with
  `stepIndex`.

## Status

Round 29 of an ongoing series (rounds 1–28 committed or pending commit). Two Medium and one Low
finding, all three genuine and applied, plus one further gap (island `constraintRemoveCount`
missing from the public state hash) caught by this round's own pre-prompt self-check, mirroring
round 28's cold-hash-guard fix but for the public hash instead. All three of Codex's own findings
are in `§8` (the ring's memory-budget and tick-numbering mechanics), a section that has had prior
rounds' attention for other properties (eviction ordering, ownership-transfer accounting) but not
for reachability-vs-physical-retention or capacity-vs-occupied-bytes precision specifically —
consistent with this series' long-running pattern of finding genuine gaps in previously-unexamined
dimensions of already-reviewed sections, not edge cases thinning out. None of the three findings
is a consequence of round 28's own edits (round 28 touched §11.2 and §12; this round's findings
are in §8, disjoint) and none reopens ground the doc's own rationale already covered — all three
are new territory.

This is the second and final round of the currently-requested two-round batch (28–29); stopping
here per that instruction. The series has not reached `CONVERGED` — three source-confirmed gaps in
a section no recent round had focused on is the same signal every prior round in this series has
shown: a future round should continue at the same full scope, not a narrower one, whenever review
next resumes.

Round 29: 3 findings (0H-2M-1L), 3 applied, 0 declined
Series total: 106 findings (41H-49M-16L) across 29 rounds

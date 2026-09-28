---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 27), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 27

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the source tree, including the sleep and wake checkpoints and the need to restore proxy properties separately from shape filters. I found two correctness and specification gaps, plus one delivery omission. The strongest concern is an observable name-cache difference after a rewind followed by a divergent timeline.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2, §10 | Body and shape name IDs can be restored, but the name cache is append-only and is absent from the state inventory. After rewinding, a name added only on the discarded timeline remains cached. If a new timeline adds a different name with the same 32-bit hash, `b3AddName` returns the abandoned name's ID and `GetName` resolves to the abandoned string. This conflicts with §10's claim that setters are undone (`name_cache.c`, `body.c`, `shape.c`). | Specify how history handles name-cache insertions and collisions, or explicitly narrow the setter-restoration contract for names. Test a divergent branch with colliding names. |
| 2 | Low | §6, §8 | §8 says an image's size is known up front from awake counts. Awake contacts have variable `manifoldCount`, and mesh contacts have variable `triangleCache` lengths, both included in the §6 gather (`contact.h`). Counts alone therefore cannot size the image. | Specify an O(awake) sizing pass over contacts and islands before reserving the slot, then perform the parallel gather into the reserved space. |
| 3 | Low | §11.3, §13 | §11.3 lists a wind-force pointer refresh as a required engine change, but no delivery phase assigns it. `b3Shape_ApplyWind` obtains a body-sim pointer before waking a sleeping body, and wake transfers the sim (`shape.c`, `solver_set.c`). | Put the pointer refresh explicitly in Phase 1 and cover replay of wind applied to a sleeping body. |

## Checked, no change

- The wake and sleep checkpoints cover shape bounds at the hot/cold boundary (`solver_set.c`, `solver.c`, `shape.c`).
- Proxy category bits can differ from the shape filter after a filter change that leaves the proxy live, so §5.2 correctly treats them separately (`shape.c`, `broad_phase.c`).
- The CCD path currently uses a running fraction and caps sensor hits in traversal order, as §7.4 states (`solver.c`).
- Sensor overlap changes are detected independently of the owning body's awake state (`sensor.c`).
- The existing snapshot restore clears body, shape, and joint `userData`, matching the Phase 0 limitation (`world_snapshot.c`).

## Proposed edits

- **§5.2 and §10:** Add name-cache state or state an explicit exception to setter restoration, including behavior after a divergent rewind.
- **§6 and §8:** Replace "size is known up front from the awake counts" with a sizing pass that includes variable contact and island data.
- **§13:** Assign the §11.3 wind pointer refresh to Phase 1 and name its replay test.

## Unresolved / disagreements

The proposed CCD change should spell out when `preSolve` is called for solid candidates whose TOI is evaluated against the initial fraction. The current callback runs only for a candidate that improves the running fraction (`solver.c`). I could not establish from the wording alone whether the proposed selection includes every callback rejection, so I have not called this a separate finding.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T07:09:13+10:00, end 07:13:25+10:00,
`EXIT_CODE 0` — no resume needed, returned inline in under 4.5 minutes. Raw log had the final
message duplicated (a `codex exec` streaming artifact); one copy is kept above. Codex ran
read-only; `git status` before the call confirmed the working tree held only this session's own
pre-prompt pre-check edit (see below), and Codex made no file changes. The 8-item curated
declined-findings list (unchanged since round 26 — round 26's own two findings were both applied,
not declined, so nothing new was added) was included in the prompt; Codex did not re-raise or
dispute any of them.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Re-read §5.1, §5.2, §7.1
(including "Ownership transfer instead of copying"), and §7.4 for a rule stated for one
inventory case not carried to its siblings. Traced two candidates fully against source
(`src/body.c`, `src/island.c`) before writing the prompt:
- Whether §7.1's ownership-transfer wording ("an island destroyed by a merge or split") omits the
  plain-destroy path (`b3RemoveBodyFromIsland`, `body.c`, when an island empties without a
  merge/split) — checked, no change: that path only destroys an island whose `bodies`/`contacts`/
  `joints` counts are already asserted zero, so nothing live is freed there, unlike
  `b3MergeIslands` (`island.c`), where the smaller island's arrays still hold their original,
  non-empty content at the moment `b3DestroyIsland` frees them.
- Whether §5.2's `shapes[id].aabb`, `fatAABBs` (non-awake owner) row omits mutator sites its
  sibling row (tree proxy `categoryBits`) already lists — **confirmed a genuine gap**:
  `b3CreateShapeProxy` (`shape.c`) calls `b3UpdateShapeAABBs`, writing `shape->aabb`/`fatAABBs`
  from the live transform, and is called from `b3Body_Enable` and `b3Body_SetType`'s recreate pass
  (`body.c`) — both already named in the categoryBits row's mutator list, neither named in the
  aabb/fatAABBs row's. Fixed by adding `b3CreateShapeProxy` (initial shape creation, `b3Body_Enable`,
  `b3Body_SetType`'s recreate pass) to that row's mutator list, before writing the round 27 prompt.

Each of Codex's three findings was verified directly against source (this design has no
implementation yet, so verification is against the document's own completeness and
self-consistency, the same standard every prior round has applied to an unimplemented mechanism):

- **#1 (name cache is append-only, world-global, and outside the state inventory; a hash
  collision between a discarded timeline's name and a new timeline's name silently resolves to the
  abandoned string), CONFIRMED, applied.** Read `name_cache.c` in full: `b3AddName` hashes the
  string with `b3Hash32` (FNV-1a + Murmur3 finalizer) and, on a hash already present in the map,
  returns the existing entry's id unconditionally (only logging a collision when the stored string
  differs), never inserting or replacing. Confirmed `b3Body_SetName`/`b3Shape_SetName` (`body.c`,
  `shape.c`) call `b3AddName` directly, and that the cache (`world->names`, a `b3NameCache` of a
  `b3Array` plus a hash map) is destroyed only at world teardown (`physics_world.c`) — nothing in
  §9's restore procedure or anywhere else touches it. Traced the consequence: a body/shape's own
  `nameId` field is an ordinary record field, already covered by the generic body/shape journal
  rows and correctly restored by `Rewind`; what isn't restored is the cache backing `GetName`,
  since it is never cleared or rolled back, so a name string inserted only on a timeline `Rewind`
  discards remains resolvable, and a same-hash insert on the new timeline is silently absorbed into
  the old entry. Fixed by adding a new bullet to §10 (Caller contract), after the existing
  `b3World_Explode` bullet, stating the cache's append-only, unrestored nature and the exact
  collision behavior, parallel in style to that section's existing `userData`- and
  geometry-buffer-ownership bullets.
- **#2 (an image's size depends on more than the awake population counts, since awake contacts
  carry a variable `manifoldCount` and mesh contacts a variable triangle-cache length, both part of
  the §6 gather), CONFIRMED, applied.** Confirmed `manifoldCount` is a per-contact `uint16_t` field
  (`contact.h`) set by mesh-contact clustering to a variable `clusterCount` (`mesh_contact.c`), not
  a fixed per-contact constant, and that §6 step 3's gather explicitly walks "`manifoldCount`
  manifolds, and the mesh triangle cache when `b3_simMeshContact`" per awake contact — so sizing the
  image needs each awake contact's own current counts, not merely how many bodies/shapes/contacts
  are awake. Fixed by qualifying §8's "known up front from the awake counts" bullet to also name
  each awake contact's `manifoldCount` and mesh triangle-cache length as inputs to the size
  computation, keeping the existing "never allocates inside the step" conclusion (a summation pass
  over already-known counts, not an allocation).
- **#3 (§11.3 lists a wind-force pointer re-fetch as a required engine change, but §13's phase
  breakdown never assigns it to a phase), CONFIRMED, applied.** Read `b3Shape_ApplyWind` (`shape.c`)
  in full: it fetches `sim = b3GetBodySim(world, body)` before conditionally calling
  `b3WakeBodyWithLock` on a sleeping body, then dereferences `sim->transform` afterward — confirming
  the exact stale-pointer pattern §11.3 already describes (wake transfers a body's sim between
  solver-set arrays, `solver_set.c`, which can relocate the backing storage `sim` points into).
  Confirmed no phase in §13 mentions this fix. Fixed by adding it to Phase 1's bullet, alongside the
  journal hooks and cold-hash guard it ships with.

## Status

Round 27 of an ongoing series (rounds 1–26 committed). One Medium and two Low findings, all three
genuine and applied. The Medium finding (name-cache append-only state) is in a mechanism no prior
round in this series had examined — a debug-facing but world-global structure that the document's
own "every cross-step byte falls into one of three classes" framing (§4) never classified, the same
kind of previously-unexamined-mechanism gap round 26 closed twice in the enable-time image and
redo-byte-capture timing. The two Low findings are narrower: one sharpens an imprecise "known up
front" claim without changing the design's O(awake) cost model, the other is a bookkeeping gap
(assigning an already-agreed engine fix to a delivery phase) rather than a new defect. This is the
first round of a newly-requested three-round batch (27–29); per the user's message received during
this round, pausing here rather than proceeding to round 28.

Round 27: 3 findings (0H-1M-2L), 3 applied, 0 declined
Series total: 101 findings (41H-45M-15L) across 27 rounds

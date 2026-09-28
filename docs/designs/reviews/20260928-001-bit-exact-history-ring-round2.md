---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=high
mode: broad (round 2), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 2

## Review report (Codex final message)

## Summary

**Changes are needed before the design supports its bit-exact and bounded-memory claims.** The current source confirms much of the step and solver inventory, but several sibling cases are missing from the capture and journal rules. This was a read-only review of the target document and current source tree; no tests were run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §2, §7.4, §10 | The document permits `b3World_Explode` to replay in a different tree order while still promising bit-exact replay. `ExplosionCallback` applies each shape's impulse immediately to body velocity, so different traversal orders can change floating-point sums even without overflow constraints (`physics_world.c`). | Process explosion candidates in a stable shape order before applying impulses, or narrow the bit-exact contract explicitly. |
| 2 | High | §5.2, §7.4, §9 | Proxy creation and destruction are not limited to shape creation and destruction. Body type and enabled-state changes, and proxy resets, also replace proxies (`body.c`, `shape.c`). Replaying shape records and moving AABBs alone can leave a restored `proxyKey` referring to the wrong live proxy or tree. | Journal every proxy lifecycle operation, then restore a self-consistent shape-to-proxy mapping. Store moved shapes by shape ID and resolve their live proxy keys during restore. |
| 3 | High | §7.1 | A hull pointer is not safe to restore with a plain record copy. Replacing or destroying the last shape using a hull removes and frees its entry in the world's reference-counted hull database (`shape.c`, `physics_world.c`). An older journaled pointer can therefore dangle. | Retain or reconstruct hull database references for the journal's lifetime, and account for retained geometry in the budget. |
| 4 | High | §4–§6 | Sensor state does not follow the proposed awake-owner classification. Every sensor, including one on a static body, is processed each step; its `overlaps2` can change when an awake visitor moves (`sensor.c`). Imaging "per awake sensor" misses that state, while imaging every sensor makes capture proportional to static sensor count. | Give sensor overlaps their own changed-sensor delta or journal rule, including overlap storage and sensor swap-compaction. Test many static sensors with few awake bodies. |
| 5 | High | §5.2, §7.1, §9 | Mesh contacts own a heap `triangleCache` that `b3DestroyContact` frees (`contact.c`). The journal specifies ownership handling for shape materials and manifolds but not this cache. Undoing destruction with a copied contact record can restore a freed array pointer. | Add explicit ownership transfer or byte serialization for mesh-contact caches across create, destroy, and restore. |
| 6 | High | §2, §6, §8 | Doubling the capture interval cannot enforce `maxBytes` when the journal alone exceeds the budget: journal segments are still required every tick. A single image or detached array can also exceed the whole budget. Further, awake counts do not determine journal size before writes occur (`recording_replay.c` provides a keyframe policy, not a bound for this journal). | Define what happens when one required segment exceeds the budget, and specify how journal storage is reserved or grows. Qualify the hard-cap and allocation claims accordingly. |
| 7 | Medium | §5.1, §10 | Callback configuration is called "not state," but the world stores mutable custom-filter, pre-solve, friction, and restitution callback pointers and contexts; their setters can run between ticks (`physics_world.c`). A rewind across such a setter does not restore the callback used at T. Purity alone does not solve that. | Either snapshot or journal callback configuration, or require it to remain fixed throughout the retained window. |
| 8 | Medium | §12 | The proposed "full state hash" inventory omits simulation-affecting shape geometry and filters, sensor overlaps, world tuning and flags, and reconstructed proxy state. The existing hash covers only body transforms and velocities (`recording.c`); the proposed extension as written would still miss failures above. | Specify canonical hash coverage for every state class in §4, including owned data by value and derived-state invariants. |

## Checked, no change

- The recording player does restore a world shell and compare replay hashes, while its current hash has the narrower transform-and-velocity coverage stated in §1 (`world_snapshot.c`, `recording_replay.c`, `recording.c`, `test/test_recording.c`).
- Worker-count tests exist; contact-state changes are consumed in contact-ID order, pair candidates are sorted by shape-pair key, and split selection has an island-ID tie-break (`test/test_determinism.c`, `physics_world.c`, `broad_phase.c`, `solver.c`).
- The CCD traversal dependencies identified in §7.4 are present: the running TOI fraction is passed to later candidates, and sensor hits are capped during traversal (`solver.c`).
- I found no shape-bound write in `b3WakeSolverSet` (`solver_set.c`). I also found no need for identical numeric proxy IDs as such: proxy keys provide the live lookup, while candidate pairs are sorted by shape-pair key (`shape.c`, `broad_phase.c`). Finding 2 concerns restoring a **valid mapping** after additional proxy lifecycle operations.
- §9 explicitly allows manifold reallocation during restore. The design's allocation claim is about capture, so that is not a contradiction.

## Proposed edits

- **§7.4 and §10:** Replace the explosion exception with: "Collect explosion candidates and apply their impulses in ascending shape-ID order. Tree traversal must not determine wake, impulse, or constraint order on replay."
- **§7.1:** Replace the hull-pointer sentence with: "Journal entries retain a live hull database reference, or retain the hull data needed to reacquire one, until the entry is evicted."
- **§5.1–§5.2:** Add a separate sensor-overlap state class and an ownership rule for mesh-contact triangle caches.
- **§8:** State an explicit policy for a journal segment, retained allocation, or single image larger than `maxBytes`; interval widening alone is insufficient.
- **§12:** Make the hash specification mirror the complete §4 inventory and add targeted rewind tests for explosions, proxy-changing setters, hull replacement, mesh-contact destruction, and static sensors.

## Unresolved / disagreements

The document must choose whether bit-exact replay includes arbitrary supported `b3World_Explode` calls and mutable callback configuration. Its current requirements say yes; §10 leaves exceptions that violate that promise. The performance estimates and acceptance thresholds remain predictions until the proposed implementation is measured.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`,
foreground under a 540s shell-level timeout (the tool call itself was auto-backgrounded by the
harness past its own 120s default and awaited via task notification, per the workflow's
foreground/no-background rule — the shell-level `timeout` and `< /dev/null` redirection were
still in effect throughout). Start 2026-09-28T18:15:02+10:00, end 18:20:37+10:00, `EXIT_CODE 0`
— no resume needed. Raw log had the final message duplicated (streaming artifact, at two earlier
positions plus the kept copy); one copy is kept above. Codex ran read-only; working tree was
untouched by it (target doc's `git diff` against HEAD was empty going into the round). The
declined-findings list from round 1 (sleep→wake bounds, proxy-id reconstruction, the
allocation-free-restore half of `maxBytes`) was included in the prompt as curated context; Codex
did not re-raise any of the three.

Each finding checked against source before any doc edit:

- **#1 (`b3World_Explode` impulse-order dependence beyond overflow colour), CONFIRMED, applied.**
  Read `ExplosionCallback` (`src/physics_world.c`): it applies `b3MulAdd`/`b3Add` directly to
  `state->linearVelocity`/`angularVelocity` for every queried shape inside the explosion radius,
  in `b3DynamicTree_Query` traversal order. A body with multiple shapes in radius sums impulses
  in that order; float addition is non-associative, so traversal order changes the result
  independent of the overflow-colour channel §10 already documented. §7.4's own "only affects
  overflow constraint order" parenthetical was therefore wrong, not just incomplete. Fixed both
  §7.4's residual-order-dependence list and §10's `b3World_Explode` bullet to state the direct
  impulse-order channel; removed the now-false "only" qualifier.

- **#2 (proxy lifecycle beyond shape create/destroy), CONFIRMED, applied.** Traced
  `b3ResetProxy` (`src/shape.c`), called from `b3Shape_SetFilter` and every geometry setter
  (`SetSphere`/`SetCapsule`/`SetHull`/`SetMesh`), and `b3Body_SetType` (`src/body.c`): both
  destroy the live proxy and create a new one — `b3ResetProxy` in the same tree (new key, same
  `B3_PROXY_TYPE`), `b3Body_SetType` via `b3DestroyShapeProxy`/`b3CreateShapeProxy` in a
  *different* tree matching the new body type. Neither is a shape create/destroy, so §7.4's claim
  that "proxies created or destroyed after T are handled by the journaled shape create/destroy"
  was incomplete. Also confirmed `b3DynamicTree_MoveProxy` takes no category-bits argument
  (`src/dynamic_tree.c`) — it only repositions a proxy already in the right tree — so restore's
  existing "MoveProxy the live proxy" step silently left a shape's tree-stored `categoryBits`
  stale after any filter change, and could not follow a body-type change across trees at all,
  independent of round 1's declined finding #3 (which only established that proxy *numeric id*
  need not match, not that its tree or category bits could go unsynced). Fixed §7.4's restore
  bullet to also call `b3DynamicTree_SetCategoryBits`, and to destroy-and-recreate in the correct
  tree when a shape's journaled body type at T differs from its live proxy's tree.

- **#3 (hull pointer not safely copyable), CONFIRMED, applied — corrects round 1.** Round 1
  declined this concern based on `shape->hull` not being shape-owned. Re-traced
  `b3AddHullToDatabase`/`b3RemoveHullFromDatabase` (`src/physics_world.c`): the hull database is
  reference-counted (bump on add/dedupe, decrement-and-free-at-zero on remove), and
  `b3RemoveHullFromDatabase` is called from both shape destroy (`shape.c`) and `b3Shape_SetHull`
  when switching away from a hull. "Not shape-owned" was true but insufficient — round 1 checked
  ownership, not lifetime. A journaled write that drops a hull's last reference can free data an
  earlier journal entry's old bytes still point to. Fixed §7.1's ownership-transfer paragraph to
  give hull pointers the same treatment as `materials`: a journaled write that would drop the
  last reference takes one instead, releasing it on eviction.

- **#4 (sensor overlaps don't follow awake-owner classification), CONFIRMED, applied.** Read
  `b3OverlapSensors`/`b3SensorTask` (`src/sensor.c`): the parallel-for runs over
  `world->sensors.count` — every sensor, regardless of the owning body's awake/sleeping/static
  state — every step, and a per-sensor `eventBits` bit is already set exactly when `overlaps2`
  differs from the prior step's `overlaps1` (used today to decide event publication). §6's "per
  awake sensor" capture rule therefore misses a static-body sensor whose overlaps change because
  an awake visitor moved through it: that state is imaged nowhere (not awake-owned, not a
  structural journal event) and is unrecoverable on rewind. Fixed by reclassifying capture from
  "per awake sensor" to "per sensor whose `eventBits` bit is set this step" (§4, §5.1, §6) — the
  same existing per-step change signal the engine already computes, bounding cost by overlap
  churn rather than sensor count, consistent with requirement 2's "structural churn" allowance.

- **#5 (mesh-contact `triangleCache` ownership), CONFIRMED, applied.** Confirmed
  `contact->meshContact.triangleCache` is a heap `b3Array` destroyed by `b3Array_Destroy` inside
  `b3DestroyContact` (`src/contact.c`), and that §7.1's ownership-transfer paragraph listed only
  sleeping/island arrays and the shape `materials` array — not this one. Folded it into the same
  paragraph alongside the hull-pointer fix (#3), since both are heap-owned data freed by a
  structural function that a plain old/new-bytes record write can't safely reverse.

- **#6 (`maxBytes` scope vs. journal-only overflow), CONFIRMED, applied.** Re-read §6's "journal
  segments are still recorded every tick because they are needed to walk between images" against
  §8's backpressure mechanism (doubling the capture interval): confirmed interval widening only
  reduces how many retained ticks carry an *image*, never the number of ticks carrying a journal
  segment, so a budget smaller than the minimum window's total journal bytes has no described
  fallback. Separately confirmed §8's "slot sizes are known up front from the awake counts"
  conflates image sizing (genuinely known from awake counts) with journal sizing (grows with
  structural churn discovered during the step, not knowable up front). Fixed both: §8 now states
  that an over-budget minimum window is accepted over budget rather than dropping
  correctness-required state, and separates the image (upfront-sized) and journal
  (amortized-growth) sizing claims.

- **#7 (callback configuration is mutable caller state), CONFIRMED, applied.** Confirmed
  `world->frictionCallback`, `restitutionCallback`, `customFilterFcn`, `preSolveFcn` are mutable
  world-global pointers with public setters (`b3World_Set*Callback`, `src/physics_world.c`) that
  can be called at any time, and that friction/restitution callbacks feed contact solving
  directly — not merely gating or event routing. §10's "callbacks are not state" conflated
  function *purity* (a real requirement) with callback *identity* (which function is currently
  installed) being caller state the ring never journals or restores. Fixed by reframing the
  bullet: a callback-configuration change is a caller input that must be replayed at its correct
  tick like any other setter, kept separate from the still-valid purity requirement.

- **#8 (hash inventory narrower than §4's state classes), CONFIRMED, applied.** §12's proposed
  `b3World_ComputeStateHash` list omitted shapes, sensor overlaps, and world scalars/flags, all
  of which §4/§5 already classify as simulation-affecting (and two of which, #1 and #4 above,
  are exactly the kind of gap a narrower hash would fail to catch). Extended the list to match.

## Status

Round 2 of an ongoing series (round 1 committed as `ba1affc`). 8 of 8 findings were genuine and
applied; none were declined. One (#3, the hull pointer) directly corrects a conclusion round 1
reached and declined — round 1 verified "not shape-owned" but did not check the hull database's
reference-counted lifetime, which is the actual mechanism that makes the old wording unsafe. This
is exactly the scenario the workflow's re-verification rule exists for: a re-raised claim was
checked against current source with full rigor rather than assumed settled, and this time the
source read was deeper. No finding was a second-order consequence of round 1's own fixes — each
traces to a gap round 1 didn't touch (explosion impulse order, proxy lifecycle beyond
create/destroy, sensor classification, mesh-contact ownership, journal-vs-image budget scope,
callback identity, hash coverage). The finding rate did not narrow from round 1 (9 applied) to
round 2 (8 applied) despite round 2 being told about round 1's declines, and two of round 2's
findings (#2, #6) go deeper into mechanisms round 1's own fixes had just touched (proxy
create/destroy journaling, `maxBytes` scope) without round 1 having covered their edge cases.
That pattern — a fresh full-scope pass still finding as much as the first — means the document is
not yet converging; another broad round is warranted, not a narrower one.

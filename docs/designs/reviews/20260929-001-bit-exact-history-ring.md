---

title: Codex review — bit-exact history ring, round 1
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 1

## Review report (Codex final message)

## Summary

The hot image and cold journal split fits much of box3d's layout, but the specified journal does
not yet reconstruct a whole world. Several persistent arrays, owned allocations, and tree proxy
IDs fall outside its entries. The proposed tree change also leaves traversal-dependent simulation
behavior. I reviewed the document and source without reading `docs/designs/reviews/`; I did not
run the proposed mechanism.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1, §9 | The journal has no general entry for changes to existing static, disabled, or sleeping solver-set arrays. Body and joint creation/destruction and contact transfers append or swap-remove elements in those sets; recording the affected id-addressed records does not restore array contents, counts, or order. Island link arrays have the same problem outside create/destroy. `body.c`, `joint.c`, `solver_set.c`, `island.c` | Specify reversible array operations for every set and island array, including swap results and storage ownership. |
| 2 | High | §5.2, §7.1 | Whole-record bytes cannot restore owned data. Shape records point to mutable material arrays and geometry; friction/restitution setters write through the material pointer, and destruction frees it. Contacts own manifolds and mesh triangle caches. The manifold entry lists old bytes but no complete redo or cache ownership rule. `shape.c`, `shape.h`, `contact.c`, `contact.h`, `world_snapshot.c` | Journal pointee contents and allocation lifetimes, with explicit undo and redo ownership rules. |
| 3 | High | §7.1, §9 | "Alloc undo = push back" is wrong for a bump allocation. `b3AllocId` increments `nextIndex` when its free array is empty; pushing that ID on undo leaves a different pool state at T. `id_pool.c` | Record whether each allocation popped or bumped; undo a bump by restoring `nextIndex`. |
| 4 | High | §5.4, §7.4, §9 | Fat AABBs and shape `proxyKey` values do not reconstruct the trees by moving changed proxies. Each tree has its own proxy free list, proxy IDs, category bits, and live membership. A proxy destroyed after T may no longer exist for `MoveProxy`, while a proxy created after T remains in the tree. Rebuilding from all shapes would scan the world, contradicting the stated restore bound. `dynamic_tree.c`, `dynamic_tree.h`, `broad_phase.c`, `shape.c` | Journal proxy allocation/free and metadata, or state an O(world) reconstruction path and revise the requirement. |
| 5 | High | §7.4, §10 | Evaluating CCD candidates with an initial fraction does not remove all tree-order effects. CCD retains only the first eight qualifying sensor hits and filters them against the running solid fraction. `b3World_Explode` applies impulses while traversing the tree; multiple shapes on one body can change floating-point accumulation order, beyond the documented overflow-colour effect. `solver.c`, `physics_world.c` | Define deterministic candidate ordering or order-independent accumulation for these paths, and test altered tree layouts. |
| 6 | High | §4, §6, §8–§10 | Journal segments close at step end, yet API calls can mutate the world between steps. The design does not say how `Rewind(T)` undoes still-open mutations after the current completed tick, or when a correction made after rewind invalidates the old forward branch. Waiting until the next step to discard that branch permits stale redo. `body.c`, `shape.c`, `physics_world.c` | Define boundary transactions: journal post-step calls, undo them on rewind, and invalidate forward history at the first mutation on a new branch. |
| 7 | Medium | §2, §4–§6, §11 | Imaging every sensor's `overlaps2` is O(sensor count), including sensors on static or sleeping bodies. The engine updates every sensor each step. Thus capture is not generally O(awake plus structural churn), even with zero awake bodies. `sensor.c`, `physics_world.h` | Give sensor overlap state its own cost term or an incremental capture mechanism. |
| 8 | Medium | §2, §8–§9 | Widening the image interval cannot make one oversized image or one journal segment fit `maxBytes`. The promised minimum window and "no other failure" cannot both hold for an arbitrary byte cap. Restore may also allocate manifolds, and array resizing can grow storage, contrary to the allocation-free description. `physics_world.h`, `dynamic_tree.c` | Specify admission, reservation, and failure behavior for oversized slots and restore allocations. |
| 9 | Medium | §2, §5.1, §9–§10 | The treatment of boundary state is inconsistent. §2 includes events in exact replay, while §9 clears all event and end-event buffers, so querying immediately after `Rewind(T)` cannot reproduce T's events. The scalar inventory also omits simulation-relevant callback bindings that public setters can change between ticks. `physics_world.c`, `physics_world.h` | State whether exactness includes immediate event queries; capture the required buffers if it does. Include callback bindings in restore or explicitly require callers to restore them before replay. |
| 10 | Medium | §2.7, §5.2, §7.3 | The claimed compile-enforced single mutation path is not supplied by the proposed accessors. Existing callers receive mutable records through shared headers, while `b3AllocId`, `b3AddKey`, and bitset setters receive no world or journal context. A journal call cannot simply be inserted into those generic functions as described. `physics_world.h`, `body.h`, `id_pool.c`, `table.c`, `bitset.h` | Specify the ownership and API refactor: world-aware wrappers or private storage, and compiler-visible barriers to direct writes. |
| 11 | Medium | §1, §12–§13 | The existing scrub test verifies a hash of body transforms and velocities, not complete world equality. The proposed full hash still omits several identified dependencies, including shape materials, sensor overlaps, tree proxy state, and world settings. It could pass while replay differs. `test_recording.c`, `recording.c`, `world_snapshot.c` | Use a canonical hash covering every restored simulation dependency, plus targeted checks for IDs, events, and tree behavior. Narrow the claim about what current tests demonstrate. |
| 12 | Medium | §13, §15 | Phase 0 cannot put `b3SerializeWorld`/`b3DeserializeIntoShell` directly behind a standalone ring as written. Geometry is interned through a recording registry and deserialization needs its reader; the serializer also scrubs host `userData` pointers. The prototype needs registry lifetime and host-state semantics. `world_snapshot.c`, `recording_replay.c` | Specify the registry and geometry ownership used by phase 0, and how host pointers are rebound where callbacks depend on them. |

## Checked, no change

- The serializer covers six id pools, solver sets, sparse records, sensors, islands, trees, pair set, and graph colours. `world_snapshot.c`
- The scrub test performs backward seeks and compares replay hashes; the current hash's narrower coverage is addressed above. `test_recording.c`, `recording.c`
- Pair keys are sorted before contact creation, and sensor overlaps are sorted and deduplicated. `broad_phase.c`, `sensor.c`
- CCD passes a running fraction to TOI, and moved-proxy flags live in tree nodes. `solver.c`, `dynamic_tree.h`
- Sleeping solver sets hold bodies, contacts, joints, and islands outside the awake arrays. `solver_set.h`, `solver_set.c`
- The FAQ says rollback is unavailable today; the simulation docs describe deterministic runs across worker counts and supported platforms. `docs/faq.md`, `docs/simulation.md`

## Proposed edits

1. In §7.1, add reversible entries for set-array append, swap-remove, resize, and island-link edits; make §9 restore their counts and ordering.
2. In §5.2 and §7.1, distinguish record bytes from owned material, geometry, manifold, and cache storage; define both directions of every ownership transfer.
3. Replace the pool undo rule with a pop-versus-bump entry carrying the prior `nextIndex` where needed.
4. Replace §7.4's `MoveProxy` reconstruction recipe with a complete proxy-pool and metadata journal, or revise its cost and requirement claims.
5. Extend §7.4 and §10 to cover capped CCD sensor hits and explosion impulse order.
6. Add explicit between-step journal and branch-invalidation rules to §6, §8, and §9.
7. Qualify §2 and §11's capture bound by sensor count unless sensor state is captured incrementally.
8. Define what happens when a single image or journal segment exceeds `maxBytes`, and revise the allocation-free claim.
9. Align §2, §9, and §10 on events and callback bindings at a restored tick.
10. In §5.2 and §7.3, describe the required encapsulation refactor rather than claiming the existing generic functions already enforce journaling.
11. Revise §1's demonstrated-property claim and expand §12's hash and checks.
12. Add recording-registry ownership and host-pointer rebinding to phase 0.

## Unresolved / disagreements

The performance estimates and comparison with design 001 are plausible motivations, but the hot image plus old/new journal has not been measured here. The source alone cannot establish the proposed capture percentage or byte budget. Whether immediate event queries belong to the public rewind contract is a product choice; the document currently promises exact events while discarding them on restore.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only`, model `gpt-6-sol`, reasoning effort `medium`,
stdin from `/dev/null`, run in the foreground under a 540s shell timeout and a 600000ms Bash-tool
timeout. Started 2026-09-29T10:25:28+10:00, ended 10:29:23+10:00, exit code 0 — no resume needed.
Codex ran read-only throughout; the working tree was untouched by the call itself (all edits
below were made by Claude afterward). The raw transcript (tool calls, source reads) was written
to the session scratchpad and is not committed; only the final message is reproduced above, once
(the run emitted it twice, a `codex exec` streaming artifact, not two different answers).

**Findings verified against source and applied — all 12.** Before verifying, note this round ran
directly after Claude's own pre-round pass applying the design-review workflow's AC-1–AC-4
hard gates (see the target doc's `status:` history via `git log`): that pass rewrote §5.2/§7.3 to
replace an enumerated per-call-site journal-hook list and a cold-hash completeness guard with
per-structure write accessors. Finding 10 below is against that same rewrite, caught by this
round precisely because it was a fresh, from-scratch pass with no memory of what motivated it.

1. **Applied.** Confirmed: `b3SolverSet` (`solver_set.h`) holds `bodySims`/`bodyStates`/
   `jointSims`/`contactIndices`/`islandSims` as dynamic arrays; `b3TransferBody`/`b3TransferJoint`
   (`solver_set.c`) push/removeswap into them and were not in §5.2's table for the solver-sets
   row. Added an "array push/removeswap" journal entry kind (§7.1) carrying old length and, for a
   removeswap, the removed element's full old bytes (otherwise unrecoverable once the mover is
   copied over them); added the missing callers to §5.2's table; corrected the "swap-compaction...
   nothing special is needed" paragraph, which conflated the moved element's ordinary record
   write with the array's own length change (§5.2).
2. **Applied.** Confirmed: `shape->materials` is heap-allocated (`shape.c`) and setters
   (`b3Shape_SetFriction` etc.) write through `b3GetShapeMaterials(shape)`, which returns the
   heap array for a multi-material shape — a `shapes[id]` record write captures the pointer, not
   the pointee. Added a "material block" journal entry kind mirroring the existing manifold
   block, and a clarifying paragraph in §5.2. Mesh triangle cache already had an ownership-transfer
   sentence added alongside it.
3. **Applied.** Confirmed exactly in `id_pool.c`: `b3AllocId` pops the free array if non-empty,
   otherwise bumps `nextIndex`; "undo = push back" is only correct for the pop case. Rewrote the
   pool alloc/free journal entry (§7.1) to carry pop-vs-bump and the prior `nextIndex`.
4. **Applied.** Confirmed: `b3DynamicTree` has its own `proxyFreeList` (`dynamic_tree.c`),
   independent of the six id pools already journaled; `b3DynamicTree_CreateProxy`/`DestroyProxy`
   allocate/free from it. Split §7.4 into layout (genuinely derived, unaffected) and proxy
   identity (not derived — `shape->proxyKey` is part of the journaled/imaged shape record, so a
   live tree assigning a different id than T would disagree with it). Proxy alloc/free is now
   journaled at the same call sites as shape create/destroy, moved out of §5.4/§4's "Derived"
   class into "Cold". Also noted phase 1's raw-tree-image fallback already carries proxy identity
   for free, so this only becomes load-bearing at phase 2 (§13).
5. **Applied.** Confirmed both sub-claims in `solver.c`/`physics_world.c`: the CCD sensor-hit
   array is capped at `B2_MAX_CONTINUOUS_SENSOR_HITS` (8, confirmed via grep) and gated on the
   *running* fraction as candidates are visited, not the initial fraction the proposed layout fix
   uses for the solid min — so it stays traversal-order-dependent after that fix. Confirmed
   `b3World_Explode`'s callback accumulates impulses into body state once per qualifying shape
   during traversal, so a multi-shape body's result depends on visit order via non-associative
   float addition — a distinct effect from the already-documented overflow-colour ordering.
   Added both as their own paragraphs in §7.4 with fixes (two-pass sensor-hit collection;
   shape-id-ordered impulse accumulation), and updated §10's Explode bullet and §16's open
   question to cover all three now-proposed order changes.
6. **Applied.** The design's own §4 already described a tick's segment as including "API calls
   between step t−1 and step t", but §6/§8/§9 never stated how `Rewind` treats mutations made
   live after `historyTick` with no step yet run to close them into a segment — the case of two
   corrections applied back to back. Added a paragraph to §6 stating `Rewind` treats such entries
   as an unclosed pending segment and walks them first, which was already consistent with §4's
   framing but never stated for restore.
7. **Applied.** Confirmed in `sensor.c`: the sensor overlap task iterates `world->sensors.count`
   every step regardless of the owning shape's awake state. Added sensor count as an explicit
   exception to requirement 2's O(awake) claim, with the qualification that `large_world`-shaped
   scenes with few sensors are unaffected.
8. **Applied, both parts.** (a) Confirmed the interval-doubling mechanism (§8) only changes how
   often an image is taken, not the size of any single tick's own slot, so it cannot admit a slot
   that is oversized on its own; added that a single oversized slot is admitted anyway (growing
   past `maxBytes`) since requirement 3 forbids rejecting a restore, and softened requirement 5's
   "bounded memory" framing to say the bound is a target, not a hard ceiling. (b) Confirmed §9
   step 3 already said restore may "free and allocate" a manifold block — this was a real
   contradiction with §1's blanket "allocation-free" framing, not a misreading; rescoped that
   claim to capture specifically (§8 already said capture never allocates) and noted restore's
   bounded, awake-proportional reallocation is a different thing from the old serializer's
   O(world) restore allocation.
9. **Applied, both parts, with rescoped severity on the events half.** Re-reading requirement 1
   and §9 together: the document's requirement 1 already scoped its "every simulation-affecting
   byte" claim to bytes that feed back into physics, and §9 step 5 already explicitly disclaimed
   that events aren't re-delivered by `Rewind` alone — so the two were not in outright
   contradiction, but the requirement's wording invited the reading Codex reported, since it
   listed "events" among the things "re-stepping... reproduces" without saying plainly that
   `Rewind` alone does not restore query-visible event state. Sharpened requirement 1 to say so
   explicitly, pointing at §9 step 5. For callback bindings: confirmed §10 already stated
   callbacks "run live" but did not say whether that covers the registered function *pointer*
   itself; added a sentence to §10 making that explicit (the live pointer is used, not a
   restored one — a callback swap mid-timeline is caller-introduced non-determinism, not
   something bit-exactness is claiming to cover).
10. **Applied — this is a finding against Claude's own pre-round AC-gate edit, not the original
    draft.** Confirmed `b3AllocId`/`b3FreeId` (`id_pool.c`), `b3AddKey`/`b3RemoveKey` (`table.c`)
    and the bitset set/clear calls (`bitset.h`) take no `b3World*`/journal argument, and
    `b3BitSet` specifically is also used for unrelated per-step scratch bitsets in `sensor.c`,
    `solver.c` and `contact_solver.c` (confirmed by grep) — so a journal call cannot live inside
    those generic primitives without either changing their signature everywhere they're used or
    journaling unrelated state. Also confirmed (via grep of every `b3AllocId`/`b3FreeId`/
    `b3AddKey`/`b3RemoveKey` call site) that id-pool and pair-set mutation for the specific cold
    structures this design tracks already happens only inside the same structural functions
    already responsible for the record-write journal calls (e.g. `b3CreateContact` is the only
    caller of both the contact id pool's alloc and the pair set's add). Rewrote §5.2 and §7.3 to
    place these journal calls at those existing structural/record-field call sites instead of
    inside the generic primitives.
11. **Applied.** This is a consequence of applying 1, 2, 4 and 7: the existing `ScrubBackward`
    test and the proposed full hash didn't cover the newly-identified state. Rescoped §1's
    "demonstrated property" claim to the dimensions the existing hash actually covers (transforms/
    velocities), and expanded §12 item 1's hash coverage list to include shape materials, mesh
    caches, sensor overlaps, and each tree's proxy pool.
12. **Applied.** Confirmed in `world_snapshot.c`: comments and code confirm hull/mesh/
    height-field/compound geometry is interned through a recording registry, and `userData` on
    bodies/joints/shapes is explicitly zeroed by the reader ("host-owned; do not free", "userData
    is host wiring, zero it on the copy"). Added both as explicit phase-0 caveats in §13, scoped
    to phase 0 only (phase 1's in-place restore doesn't go through this serializer).

No findings were declined. All 12 were verified against actual source (not just the document's
own claims) before being applied.

## Status

This is round 1 of a fresh series against this design doc in this repository (no prior
`docs/designs/reviews/*bit-exact-history-ring*` files existed before this one); the design doc's
own status line referencing "round 1" and WORKFLOW.md's Origin note describing a prior 37-round
series are historical/illustrative context for why the acceptance-criteria hard gates exist, not
a series this file continues. Before sending this round, Claude applied AC-1 through AC-4 to the
doc as it stood and found a genuine AC-2/AC-3/AC-4 violation in the originally-drafted journal
mechanism (an enumerated per-call-site hook list plus a cold-hash completeness guard reconciling
against a shadow copy), which was rewritten to a per-structure write-accessor design before this
round ran — so this round reviewed the post-gate version, not the shape the gates exist to catch.

All 12 findings were genuine defects, verified independently against source rather than accepted
on the round's word; none were declined. One (finding 10) was against Claude's own gate-fix
rather than the original draft, which is exactly what a from-scratch round is for — it caught a
real gap (generic container primitives can't host a journal call) that a review confined to "did
the AC gates get applied" would have missed. No finding reopened ground the doc's own rationale
had already covered (there was no prior round to reopen). This is not a converged round —
`CHANGES_PROPOSED`, all findings applied — so another fresh, full-scope round is the next step,
not a narrower one.

`Round 1: 12 findings (6H-6M-0L), 12 applied, 0 declined`

`Series total: 12 findings (6H-6M-0L) across 1 round`

`Findings: 12 (6H-6M-0L)`

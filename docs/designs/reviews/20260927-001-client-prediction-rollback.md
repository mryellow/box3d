---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e019-9d18-7e90-abf8-b2f4776cf80f
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md

## Review report (Codex final message)

## Summary

The manifold lifetime hazard and the broad-phase pair sort are real, and an in-place rollback
API is a reasonable direction. The proposed capture set and restore sequence are not yet
sufficient for bit-exact resimulation: they omit non-touching contacts and persistent
shared-array state, and they treat global ID pools as though they can be rewound independently
of the rest of a live world.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5–7 | An island contains **touching** contacts only. Contacts are created non-touching and kept in a solver set until they touch (`src/contact.c:296`, `src/island.h:43`). The island-only snapshot misses existing non-touching contacts, their caches, IDs, body links, and `pairSet` entries. | Include every live contact incident to a captured body, whether touching or not, and reconcile both graph and solver-set ownership. |
| 2 | High | §§6–7 | The four ID pools are global. Rewinding them while contacts or entities outside the subset have changed can make a still-live ID available for allocation again. Restoring a pool also does **not** restore an object's generation: the generation is advanced in the object slot (`src/id_pool.c:19`, `src/contact.c:200`). | Define and enforce a global ID/topology restriction for v1, or specify a reconciliation scheme that preserves outside allocations and slot generations. Remove the claim that restoring the pools alone reproduces `(index, generation)`. |
| 3 | High | §§6–7 | `colorIndex` and `localIndex` are references into persistent graph arrays, not sufficient state by themselves. `bodySet`, `jointSims`, `convexContacts`, and `contacts` persist and are serialized by the existing snapshot (`src/world_snapshot.c:442`). Removal swap-moves another entry and changes its index (`src/constraint_graph.c:185`); the awake set similarly owns body arrays, non-touching contact indices, and island slots. | Specify restoration of those persistent arrays and all affected back-references, including entries outside the captured islands that move when an entry is removed. |
| 4 | High | §4.3, §§6–7 | Sorting makes the **order of an already discovered pair set** independent of traversal; it does not make pair discovery independent of stored AABBs, moved flags, or `pairSet`. Narrow phase tests `fatAABBs` to destroy contacts (`src/physics_world.c:624`), and pair generation starts from moved tree nodes (`src/broad_phase.c:83`). The checklist omits per-shape `aabb`, `fatAABBs`, proxy-key remapping, and a complete `pairSet` rebuild. | Keep tree reconstruction, but define how these inputs and proxy references are restored and how the rebuilt tree produces the correct next-step candidates. Narrow the pair-sort claim. |
| 5 | Medium | §§4–6, §10 | "Static bodies … never move" is false as an API constraint: `b3Body_SetTransform` updates a static body's transform and shape AABBs (`src/body.c:1112`). Shape and world settings can also change between capture and restore; `world_snapshot.c` captures shapes and simulation settings (`src/world_snapshot.c:467`, `src/world_snapshot.c:1062`). Island closure at T cannot cover changes to these inputs during replay. | State which external body, shape, and world mutations the caller must replay or forbid, and validate the subset boundary before restoring. |
| 6 | Medium | §§4.2, 8–10 | The proposed sleep exemption is only a comment in the API sketch. The solver checks each body's `b3_enableSleep` flag (`src/solver.c:773`); disabling it for roots at buffer creation does not cover later captured members or overlapping buffers. A pending `splitIslandId` can also split an island on the following step (`src/solver.c:1710`). | Define registration, reference counting, flag restoration, and capture-time checks for **all** captured bodies and pending splits. |
| 7 | Medium | §9 | Preallocating `b3RollbackBuffer` does not make restore allocation-free. Recreating a manifold calls `b3AllocateManifolds`, which can grow allocator buckets or blocks (`src/physics_world.h:323`, `src/block_allocator.c:64`); graph, solver-set, and tree reconstruction can grow storage too. | Separate buffer growth from engine-storage growth, and either reserve every required engine capacity before the window or weaken the allocation guarantee. |
| 8 | Low | §§2, 6 | "Portable byte-stream image" overstates the recording format: it stores raw struct layouts and rejects a different layout hash (`docs/recording.md:170`). Also, at a completed step boundary the solver resets `b3BodyState` deltas (`src/solver.c:740`), so §6's claim that those deltas *must* be captured as-is needs qualification. | Correct both descriptions; retaining the deltas in a raw state copy is harmless. |

## Checked, no change

- Destroying a contact frees its manifold (`src/contact.c:340`); the block allocator pushes that
  element onto a free list and allocates from the free list first (`src/block_allocator.c:48`,
  `src/block_allocator.c:83`). A saved manifold pointer is unsafe after destruction.
- Worker pair keys are combined, sorted, then contacts are created in sorted order
  (`src/broad_phase.c:713`, `src/broad_phase.c:750`). The key also contains the compound child
  index (`src/table.h:47`).
- Awake dynamic and kinematic bodies have `b3BodyState`; static bodies are absent from that state
  array (`src/body.h:136`).
- `rotation0` and `center0` are previous poses used for TOI (`src/body.h:204`). Contact
  recycling reads the saved relative transform and manifold state (`src/physics_world.c:653`).
- Islands link touching contacts and joints, excluding static bodies from their body lists
  (`src/island.h:43`). `b3SplitIsland` recomputes components after constraints are removed
  (`src/island.c:388`).
- The recording documentation does disclaim rollback determinism and describe recording as a
  debugging system with an internal save-state mechanism (`docs/faq.md:134`,
  `docs/recording.md:135`).

## Proposed edits

1. **§§5–7, contact boundary:** Replace "every body, contact, and joint in that island" with:
   "Capture every body and joint in each selected island, every touching contact in those
   islands, and every non-touching contact incident to a captured body. Preserve each contact's
   owning graph or solver-set entry, body links, pair key, cache, and manifold contents. Diff
   this incident-contact set at restore."

2. **§§6–7, persistent storage and IDs:** Add a restore-state subsection: "Capture or
   reconstruct persistent graph color arrays and body sets, awake solver-set body/state order,
   non-touching contact indices, and island-sim order. Repair every index and encoded body-sim
   reference affected by removal or insertion, including swapped entries outside the subset.
   Global pool rewind is permitted only when no out-of-subset allocation or free has occurred;
   verify that condition before mutating the world. Restore object-slot generations separately."
   The document should choose an explicit response when that condition fails.

3. **§4.3, broad phase:** Replace "so tree shape doesn't affect solver results" with: "Sorting
   makes contact creation order independent of tree traversal **when the same candidate keys are
   generated**. Restore per-shape AABBs and fat AABBs; rebuild proxies with valid
   `b3Shape.proxyKey` values, the moved/candidate state needed by the next step, and `pairSet`
   from all live contacts. Verify that this reconstruction yields the same candidate set before
   claiming bit-exact replay."

4. **§§4–6, external inputs:** Add: "A captured island is closed under current touching
   constraints, not under future mutations. During a rollback window, changes to static or
   kinematic colliders, shape geometry/material/filter/sensor state, and world simulation
   settings that can affect the captured bodies must be replayed from their historical values or
   rejected. Validate that a root still belongs to the expected captured component; reject an
   unsupported merge or split before restore."

5. **§§8–10, sleep and failure behavior:** Replace the creation-time sleep comment with a defined
   per-world registration that covers every member at each capture, reference-counts overlapping
   buffers, restores prior flags when no buffer needs them, and checks pending splits. Document
   restore as a preflight followed by mutation: unsupported topology, stale handles, ID-pool
   conflicts, or inadequate reserved capacity must return failure **without partially changing
   the live world**.

6. **§9, allocation guarantee:** Replace "Steady-state capture/restore must not call `malloc`"
   with a measurable contract covering the rollback buffer **and** manifold buckets,
   graph/solver arrays, ID pools, and tree reconstruction. State the reservation limits and the
   result when a snapshot exceeds them.

## Unresolved / disagreements

Island closure is the right default API boundary, but "no island-membership changes inside the
rollback window" needs a precise meaning. Ordinary contacts can start or stop touching within an
existing body component; forbidding those would exclude much of the intended prediction use
case, while allowing them requires the fuller contact and graph reconciliation above.

## Verdict

**CHANGES_PROPOSED** — the core approach is plausible, but the current checklist and restore
algorithm cannot yet guarantee a valid or bit-exact in-place rollback of a subset.

## Post-review verification (Claude)

**Sandbox:** `codex exec -s read-only`, model `gpt-6-sol`, `model_reasoning_effort=high`.
`git status --porcelain` was clean before and after the run — Codex made no file changes (its
sandbox was read-only; it only read source and produced the report above).

Spot-checked the load-bearing citations directly rather than trusting them:

- `src/contact.c:296` — comment confirms "Contacts are created as non-touching. Later if they
  are found to be touching they will link islands and be moved into the constraint graph."
  Finding 1 confirmed.
- `src/contact.c:200-203` — generation is bumped on the `b3Contact` struct itself
  (`contact->generation = generation + 1`) at create/reuse time, not stored in `b3IdPool`
  (`src/id_pool.c:1-40` has no generation field at all). Finding 2 confirmed: the design doc's
  §6 claim that "restoring id pools" reproduces generation was wrong — generation already comes
  along for free with a captured object struct, but a rewound *pool* free-list can still hand out
  an id to something outside the subset that the pool no longer thinks is allocated.
- `src/constraint_graph.c:185-195` — swap-remove explicitly fixes up the moved entry's index
  (`movedContact->localIndex == movedIndex` asserted after the fix-up), confirming the
  persistent-array-with-back-references hazard in Finding 3.
- `src/body.c:1112` — `b3Body_SetTransform` has no body-type guard; it accepts any `b3BodyId`,
  confirming Finding 5 (static bodies are not immutable during a rollback window).
- `src/block_allocator.c:64-74` (`b3AllocateElement`) — grows `allocator->blocks` via
  `b3Array_Resize` + `b3Alloc` when the free list is empty, confirming Finding 7 (restore is not
  allocation-free in general).
- `src/solver.c:773` — sleep gating reads `body->flags & b3_enableSleep` per body, confirming
  Finding 6's point that a root-only sleep exemption misses other captured bodies.
- `src/solver.c:1710` — island split is folded into the solver step ("I'm squeezing this task in
  here because it may be expensive and this is a safe place to put it"), confirming the pending
  `splitIslandId` hazard.
- `src/solver.c:740-741` — `state->deltaPosition`/`deltaRotation` are reset to zero at the end of
  every step, confirming Finding 8's minor qualification (capturing them is harmless, not the
  load-bearing state the original wording implied).

All findings confirmed against source; none rejected. Applied edits 1-6 to
`docs/designs/20260927-001-client-prediction-rollback.md` (§§4-11), adopting Codex's proposed
replacement text where given and extending it where the design doc needed more structure (the
sleep/reference-counting subsection and the external-mutation restriction). The "Unresolved /
disagreements" note is incorporated as a refinement of the island-stability rule rather than a
disagreement: touching/non-touching transitions of existing incident contacts are now explicitly
in scope (via Finding 1's incident-contact capture), while the restriction is narrowed to forbid
only a change in which *component* (island) a captured body belongs to (a real split or merge),
not ordinary contact begin/end-touch events within a stable component.

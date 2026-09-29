---

title: Codex review — bit-exact history ring, round 2
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 2

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible, but the specified state inventory and restore procedure do not yet support bit-exact replay. Several ordinary API paths change state that neither the image nor the listed journal entries restore. The derived-tree plan also restores proxy IDs without restoring the proxies those IDs identify. I reviewed the target document and source directly; I did not run the proposed mechanism.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.1, §6, §7.1 | Awake shapes contribute only AABBs and fat AABBs to the image. Setters change their filters, geometry, density, and inline materials, while §7.1 journals ordinary record writes only for non-awake owners. Rewind can therefore retain post-T shape properties. `shape.c` | Image complete awake shape records and owned material data, or journal every setter regardless of wake state. |
| 2 | High | §5.2, §7.3 | The four proposed record accessors do not cover writes to non-awake `bodySims` and `jointSims`. For example, `b3Body_SetTransform`, `b3Body_SetMassData`, and joint tuning/frame setters write sims directly. A sleeping or static sim can retain a later value after rewind. `body.c`, `joint.c` | Add explicit sim write hooks and enumerate their callers. |
| 3 | High | §5.2, §7.1, §9 | Sensor creation and removal change a dense, swap-compacted `sensors` array and its `shapeId` mapping. The image captures overlaps, but §7.1 has no sensor create/removeswap entry. Restoring overlaps into the current array cannot recover an earlier sensor ordering or count. `shape.c`, `sensor.c` | Journal sensor array operations and ownership of their overlap buffers. |
| 4 | High | §7.4, §9 | Restoring each tree's proxy free list does not restore tree leaves, proxy membership, `categoryBits`, or `userData`. `MoveProxy` requires an existing proxy. Proxy creation and destruction also occur during filter and geometry resets, body type and enable changes, and body destruction — not just `b3CreateShapeInternal` and `b3DestroyShapeInternal`. `dynamic_tree.c`, `shape.c`, `body.c`, `broad_phase.c` | Journal every proxy lifecycle path and define fixed-ID leaf restoration, or specify a full rebuild and revise its restore cost. |
| 5 | High | §7.1, §9 | Pool undo does not undo growth of the sparse `bodies`, `shapes`, `fatAABBs`, `contacts`, `joints`, `islands`, and `solverSets` arrays. A slot first appended after T can remain present with the wrong free-slot contents; later creation chooses the reuse path and can produce a different generation or fail its free-slot assertion. `body.c`, `shape.c`, `contact.c`, `joint.c`, `island.c`, `solver_set.c` | Journal sparse-array length changes and prior slot contents, and test creation beyond T's previous high-water mark. |
| 6 | High | §5.2, §7.1 | Hull geometry is reference-counted in `world->hullDatabase`. Destroying or replacing its last shape frees the hull; undoing a shape record would restore a dangling pointer. The ownership rules cover materials and contact allocations, but not this database or its reference counts. `physics_world.c`, `shape.c` | Retain hull objects across journal entries and restore database membership and reference counts. |
| 7 | High | §5.2, §7.3 | The claimed structural choke points omit independent mutation paths. `b3LinkJoint` and `b3UnlinkJoint` change island link arrays; `b3DestroySolverSet` is called directly from body destruction; graph colour bits are changed in `constraint_graph.c` during contact and joint graph operations. Calls placed only in the functions named in §7.3 cannot cover these paths. `island.c`, `body.c`, `solver_set.c`, `constraint_graph.c` | Move hooks to every actual mutating primitive or revise the inventory and hook locations. |
| 8 | High | §2, §7.4, §10 | The document promises exact replay while expressly leaving explosion wake order dependent on rebuilt-tree traversal. Wake order can change constraint colour and overflow order, so the remaining exception contradicts requirement 1 even after impulse accumulation is sorted. `physics_world.c`, `solver_set.c`, `constraint_graph.c` | Make explosion wake processing deterministic, or narrow the stated bit-exact contract. |
| 9 | Medium | §7.1, §9 | Material and manifold entries specify old bytes only, yet forward scrub requires new values. Ownership transfer for destroyed allocations does not by itself define how both versions remain available through repeated undo and redo. `shape.c`, `contact.c` | Give these entries explicit old and new payloads or a reversible ownership protocol. |
| 10 | Medium | §6, §8, §11.1 | Journal size is unknown before structural mutations occur, while slot reservation is specified at the end of the step and capture is said never to allocate. The ring also admits capacity growth. Capture interval K reduces image frequency, but journals remain per-tick, so total ring memory does not generally fall by K. `physics_world.c` | Specify journal staging and allocation timing; qualify the allocation and memory-scaling claims. |
| 11 | Medium | §2, §5.1, §11.1 | Sensor cost is described as proportional to sensor count, but the image copies `overlaps2`, whose length can include many visitors per sensor. `sensor.c` | State the bound as sensor count plus retained overlap count and measure both. |

## Checked, no change

- The existing recording player restores serialized worlds and re-steps backward seeks; `ScrubBackward` compares forward and replayed hashes. That hash covers body transforms and velocities, as the document says. `world_snapshot.c`, `recording_replay.c`, `recording.c`, `test_recording.c`
- The FAQ says the engine lacks a rollback mechanism, and the simulation documentation describes the step stages and determinism guarantees cited in §3. `docs/faq.md`, `docs/simulation.md`
- CCD passes the running fraction into TOI queries and caps sensor hits during traversal. The proposed ordering concern is real. `solver.c`
- Broad-phase pair keys are sorted before contact creation, and explosion impulses are applied during tree traversal. `broad_phase.c`, `physics_world.c`
- The tree proxy free list is separate from the six world ID pools. `dynamic_tree.c`, `physics_world.h`

## Proposed edits

1. Replace §5–§7's inventory with a per-mutator accounting of complete awake shape records, owned materials, non-awake sims, sensors, sparse-array lengths, hull ownership, island links, solver-set destruction, and graph-bit operations.
2. Rewrite §7.4 and §9 to distinguish restoring proxy **identity** from restoring tree **membership and metadata**. Include all proxy lifecycle callers and an explicit complexity bound.
3. Make §7.1 entries reversible in both directions, including allocations and pointees. Add tests that rewind and redo across each create, destroy, replacement, and array-growth path.
4. Resolve explosion wake order before claiming requirement 1. Update §12's hash to cover the newly identified state, particularly sparse free slots, sensor mapping, hull ownership, and proxy membership.
5. Revise §8 and §11's cost language to account for journal staging, possible allocation, and overlap count.

## Unresolved / disagreements

The source supports the qualitative whole-world replay cost comparison with design 001, but the proposed capture percentages and "few KB" cases remain estimates until this mechanism is implemented and measured. The tree restore choice is a real tradeoff: full reconstruction is simpler, while incremental restoration needs substantially more proxy journaling than §7.4 describes.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only`, model `gpt-6-sol`, reasoning effort `medium`,
stdin from `/dev/null`, run in the foreground under a 540s shell timeout and a 600000ms Bash-tool
timeout. Started 2026-09-29T10:43:13+10:00, ended 10:47:21+10:00, exit code 0 — no resume needed.
Codex ran read-only throughout; the working tree was untouched by the call itself. The raw
transcript was written to the session scratchpad and is not committed; only the final message is
reproduced above, once (emitted twice in the raw log, a streaming artifact).

**Findings verified against source and applied — all 11.** This round ran fresh, with no memory
of round 1, directly against the doc as round 1 left it (all 12 of that round's findings applied,
0 declined). Findings 7 and 8 overlap textually with round 1 ground (structural choke points;
Explode) but engage it with new evidence rather than re-raising the same claim — see each
finding's note below for what was actually new.

1. **Applied.** Confirmed: `b3Shape` (`shape.h`) has `density`, `filter`, `material`/`materials`,
   the geometry union, and `flags`, none of which are in §5.1's per-tick-written list for shapes
   (only `aabb`/`fatAABBs`) or covered by a journal entry for an *awake*-owner shape — §7.1's rule
   only journals non-awake-owner record writes, and these fields are written only by setters
   (never the hot path), so an awake shape's setter writes were covered by neither mechanism. Root
   cause, once traced: unlike bodies, which already image the *whole* `b3Body` record every tick
   (so any setter-written field is covered for free), shapes were only ever gathering
   `aabb`/`fatAABBs`, a partial capture. Fixed by imaging the whole `b3Shape` record for every
   awake shape (§5.1, §6), matching how bodies are already handled, and adding a paragraph
   explaining why (§5.1).
2. **Applied.** This is a finding against round 1's own AC-gate rewrite: that pass gave joints one
   accessor scope spanning `joints[id]` and non-awake `jointSims` (§5.2's table already listed
   both under one row), but left bodies as two separate rows — `bodies[id]` and non-awake
   `bodySims`/`bodyStates` — without folding the second into `b3Body_Write`'s stated scope.
   Confirmed `b3Body_SetTransform`/`SetMassData` (`body.c`) write `bodySims`/`bodyStates` directly
   when the body is not awake, the same functions already listed as `bodies[id]` callers. Merged
   the two table rows and extended §5.2's intro to state the accessor covers a structure's sim
   data, not just its sparse record, matching the joints precedent it should have matched the
   first time.
3. **Applied.** Confirmed `world->sensors` (`shape.c`, `sensor.c`) is a dense array;
   `b3Array_RemoveSwap` on removal (`sensor.c`) updates the moved sensor's owning shape's
   `sensorIndex`, and creation pushes at `shape->sensorIndex = world->sensors.count`. This is the
   same push/removeswap shape as the solver-set arrays round 1 already added an entry kind for;
   extended that entry's scope to include `world->sensors` rather than inventing a parallel kind,
   and updated §5.2's sensor row to name the entry and note the moved sensor's `sensorIndex`
   update is an ordinary `b3Shape_Write`, same pattern as the existing swap-compaction paragraph.
4. **Applied.** Confirmed the real choke point is `b3BroadPhase_CreateProxy`/`DestroyProxy`
   (`broad_phase.c`), the only callers of the tree-level functions round 1's fix named directly —
   an even narrower point than what was journaled. Confirmed a second caller pair inside
   `b3ResetProxy` (`shape.c`) destroys and recreates a proxy, possibly in a *different* tree, on a
   filter or body-type change; round 1's fix only named
   `b3CreateShapeInternal`/`b3DestroyShapeInternal`. `b3ResetProxy` was already listed as a
   `shapes[id]` write-accessor caller in §5.2's table, so this is the same kind of gap as finding
   2: an already-correct inventory in one place, not carried over to a related mechanism. Rewrote
   the proxy-identity paragraph (§7.4) to journal at the broad-phase choke point, add
   `b3ResetProxy`, and state explicitly that recreation is a real tree insert (AABB, category
   bits, tree) driven by the paired, already-restored `shapes[id]` write, not a bare id
   reservation — which is what "restores proxy IDs without restoring the proxies" was pointing at.
5. **Applied.** Confirmed `world->bodies` (and the other five sparse arrays) grow via
   `b3Array_Push` exactly when `b3AllocId` bumps `nextIndex` (`body.c`: `if (bodyId ==
   world->bodies.count) { b3Array_Push(...) }`), a *different* mechanism from the id pool's own
   `nextIndex`/`freeArray` state that round 1's finding-3 fix addressed. Undoing a bump without
   also truncating the paired array leaves a stale slot beyond the pool's restored `nextIndex`,
   which a later creation at that same id would read as if it were the reused-slot case. Since
   the array only ever grows in lockstep with a bump (never on a pop-reuse), its length always
   equals the pool's `nextIndex` — extended the existing pool alloc/free entry's bump-undo to also
   truncate the paired sparse array to that length, rather than adding a second entry kind or new
   payload bytes.
6. **Applied.** Confirmed `world->hullDatabase` (`physics_world.c`) is a reference-counted,
   content-deduplicated `b3HullMap`; `b3RemoveHullFromDatabase` frees the hull on a decrement to
   zero. A `shapes[id]` record write restores `shape->hull` as a raw pointer value, which would
   dangle if that hull's refcount reached zero after T. Confirmed (by grep) this pattern is
   specific to hulls — no equivalent live-engine database was found for mesh/height-field/compound
   geometry, which appear to be owned per-shape rather than shared; not independently confirmed
   either way, flagged as worth checking if raised again. Added a "hull refcount" journal entry
   kind reusing the existing ownership-transfer pattern (a decrement to zero doesn't free the
   hull; the entry owns it until eviction), and a paragraph in §5.2.
7. **Applied, and this is the one finding this round that revisits round-1 ground — it engages it
   with new evidence, not a repeat.** Round 1's finding 10/fix established that id-pool/pair-set/
   bitset journal calls belong in the small set of structural functions already responsible for
   record writes, not inside the generic container primitives. This finding names three call
   paths that fix didn't enumerate: `b3LinkJoint`/`b3UnlinkJoint` (`island.c`) mutating island
   link arrays outside `b3CreateIsland`/`DestroyIsland`/`MergeIslands`/`SplitIsland`;
   `b3DestroySolverSet` called directly from body destruction (`body.c`), not only from
   `b3WakeSolverSet`; and colour-bit changes in `constraint_graph.c` during ordinary contact/joint
   graph operations, not only the four sites round 1's table cited. These are new, specific,
   verified gaps in the *call-site enumeration itself* (§5.2's table), which is exactly the
   completeness problem requirement 7 exists to solve — an incomplete enumeration under the
   accessor pattern is a bug in the doc's inventory, not evidence against the accessor pattern.
   Added the missing callers to §5.2's table and to §7.3's description of where hooks live.
8. **Applied, and also engages existing rationale rather than repeating a raised-and-declined
   claim — there was no round-1 finding on this point to repeat.** §10 already documented the
   overflow-colour wake-order exception as accepted, undocumented-as-fixed scope for v1; this
   finding's new content is that requirement 1's *wording* ("every simulation-affecting byte...
   Re-stepping... reproduces the original ticks exactly") reads as an unqualified guarantee with
   no pointer to that documented exception, so a fresh reader hits an apparent contradiction
   between §2 and §10. Added an explicit exceptions clause to requirement 1, covering both
   documented cases (Explode overflow-colour order, `preSolve` order) so the requirement no longer
   reads as contradicted by content elsewhere in the same doc.
9. **Applied.** Confirmed §9 step 2 states redo ("apply entries forward using new values")
   unconditionally for every entry kind, but the material-block and manifold-block entries as
   specified (both by the original draft and round 1's fix) carry old bytes only. Added new bytes
   to both entries' payload, symmetric with the record-write entry, which already carried both.
10. **Applied, both parts.** Confirmed journal hooks fire throughout the step, at each mutator
    (§7.3), while §6 (as round 1 left it) only reserved the ring slot at step end — with no stated
    place for in-step journal writes to land before that slot exists. Added a per-tick staging
    buffer that journal calls append to during the step, moved into the reserved slot once its
    final size is known at step end; reconciled this against the "capture never allocates" claim
    in §1 and §8 by scoping that claim to the ring slot specifically (the staging buffer amortizes
    rather than eliminates allocation, stated as such). Confirmed the "ring memory (÷K)" framing
    from round 1's carried-over text conflated image bytes (which do fall by K) with journal bytes
    (per-tick regardless of K, as the same paragraph's first sentence already said) — reworded to
    state total memory falls by less than K, not by K.
11. **Applied.** Confirmed `sensors[i].overlaps2` holds one entry per overlapping visitor (§5.1's
    own table already said "8 B per overlap"), so round 1's "O(sensor count)" fix (itself applying
    round 1's own finding 7) was an oversimplification inconsistent with §5.1's own accounting.
    Corrected requirement 2 to O(sensor count + total overlap count).

No findings were declined. All 11 were verified against actual source before being applied.

## Status

Round 1 closed 12 findings (6H-6M-0L), all applied, verdict `CHANGES_PROPOSED` — not converged,
so this round ran, fresh and full-scope as required. This round is *not* a lower-signal repeat of
round 1: 8 of 11 findings are new categories entirely (awake shape imaging completeness, sim-data
accessor scope, sensor array journaling, proxy lifecycle completeness and cross-tree recreation,
sparse-array growth undo, hull database refcounting, journal redo payload symmetry, staging/
allocation timing, sensor overlap cost precision). The two that touch prior ground (findings 7
and 8) each engaged it with genuinely new evidence — additional unenumerated call sites, and a
wording gap between §2 and §10's own documented exception — not a re-argued version of something
already declined (round 1 declined nothing, so there was nothing to re-argue). The finding rate
did not fall between rounds (12 → 11, both all-High-and-Medium, both fully applied) — round 1
fixed real defects but, being the first pass over a document that had just undergone a large
architectural rewrite (the AC-gate fix), left adjacent ground unchecked; round 2 checked it. A
still-high, still-genuine finding rate two rounds in is consistent with a series that has not yet
converged, not with a series that is failing to converge — every finding both rounds has been a
verified, distinct defect, not a reopened or manufactured one.

Next: another fresh, full-scope round, same as every prior round. No signal here calls for
narrowing scope — round 2 found defects a full-scope pass over §5.1/§6/§7 surfaced that a
narrower "just check round 1's diff" pass would have missed entirely (findings 3, 5, 6, 9, 10, 11
are in sections round 1 didn't touch).

`Round 2: 11 findings (8H-3M-0L), 11 applied, 0 declined`

`Series total: 23 findings (14H-9M-0L) across 2 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L)`

---

title: Performance review of docs/designs/20260927-001-client-prediction-rollback.md
target: docs/designs/20260927-001-client-prediction-rollback.md (round 2 draft)
date: 2026-09-27
reviewer: Claude Code (Opus 5.5)
focus: per-tick cost and restore/resim cost; is this the fastest viable approach?

---

# Performance review — client-prediction rollback (round 2)

> **Resolution (round 3):** Author direction was to assume more than 500 awake bodies and
> design for general box3d users. The design was rewritten around scoped capture, relaxed
> determinism (P2) and an island-scoped resim via an active solver set (P1). It has no sleep
> exemption (P3), moves proxies instead of rebuilding the tree (P4), and records warm starts
> compactly (P7) in a single history arena (P8). P5, P6 and P9 no longer apply.

## Summary

The round-2 draft is careful about *correctness of the restored bytes*, but it is not the most
performant approach, and two of its costs are larger than anything the doc discusses:

1. **Resimulation is not addressed, and it dominates.** After restore, `mygame` resimulates N
   ticks by calling `b3World_Step`, which steps the *entire* world (`physics_world.c:1038-1147`:
   `b3UpdateBroadPhasePairs`, `b3Collide`, `b3Solve` all run over every awake body/contact). A
   subset snapshot plus whole-world resim costs N full steps per correction, and it is also wrong:
   every non-captured awake body advances N extra ticks.
2. **The bit-exact goal forces the expensive machinery, and in a live world it will mostly
   reject.** Contact ids decide begin-touch processing order (bitset walk in contact-id order,
   `physics_world.c:945`), which decides island link order and constraint-graph colour
   assignment (`constraint_graph.c:216-274`, first free colour). So bit-exact replay does need the
   global contact id pool to match, and §6/§7 reject the restore if *any* body outside the
   captured set allocated or freed a contact id during the window. In a scene with other moving
   bodies that happens nearly every tick. The design pays for diffing, slot reconstruction, pair-set
   repair and pool restore, and then rejects most of the restores it paid for.
3. **The sleep exemption adds per-tick cost that keeps growing.** Islands are only split when a
   body in them is sleepy (`solver.c:824-835`: the split candidate is chosen only when
   `sleepTime >= B3_TIME_TO_SLEEP`). Clearing `b3_enableSleep` pins `sleepTime` at 0
   (`solver.c:773-776`), so a captured island **never splits**. Every body the player has ever
   touched stays in the player's island and stays awake. Each tick's solve and each capture get
   more expensive, and every new merge counts as a "component change" that §10 rejects.

Recommendation: decide the determinism bar first (below), add an island-scoped resim step, and
replace the sleep exemption with a wake-on-restore approach. That turns per-tick capture into
a contiguous copy of a few hundred bytes per body. Restore becomes O(captured bodies) with no
diff, and resim costs O(island) instead of O(world).

## Hot path vs cold path

The doc never separates these, and it should, because they have very different budgets:

| Operation | Frequency | Budget | Current design |
|---|---|---|---|
| Capture | every tick | ~µs, no alloc, no hashing | island walk + contact-chain walk + struct copies + refcounted exempt-set maintenance |
| Restore | per misprediction (rare-ish) | sub-ms | preflight + 3-way diff + slot reconstruction + pairSet edits + pool restore + (per §4.3) tree rebuild |
| Resim | per misprediction × N ticks | sub-ms total | **unspecified → N × full `b3World_Step`** |
| Sleep exemption side-effect | every tick, forever | 0 | whole captured component always awake, never splits |

At 60 Hz with 150 ms RTT, N ≈ 9. If a full world step takes 2 ms, one correction costs about
18 ms of resim, which is more than a whole frame. Resim is the number to design around. Capture
bytes matter much less.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| P1 | Critical | §1, §8 (missing) | No resimulation API. `b3World_Step` is whole-world, so resim costs N × world step. It also double-advances every non-captured awake body, which is a correctness bug as well as a cost. | Add an island-scoped step, e.g. `b3RollbackBuffer_Resimulate(buffer, worldId, dt, subSteps)`, that runs pair update, collide, solve and finalize over only the captured islands' bodies/contacts/joints. Non-captured bodies stay frozen at "now" and act as static for the resim (they can't touch the captured set without a merge, which is already out of scope). Cost scales with island size, not world size. The whole design depends on this; without it, subset capture saves nothing. |
| P2 | High | §1, §6, §7 | Target is bit-exact reproduction of the *client's own* earlier timeline. Client prediction doesn't need that: on a correction the predicted body's state is **replaced by the server's**, and the timeline is resimulated from the new state anyway. Server-vs-client comparison is tolerance-based because the server's world differs from the client's regardless. Bit-exactness is what forces id-pool restore, persistent-array slot reconstruction, pairSet diffing and the "no outside id churn" rejection (see summary point 2 for why that rejection fires almost every tick in a live scene). | Make v1 **consistent, not bit-exact**. Capture per-body dynamic state only, plus warm-start impulses keyed by `(shapeIdA, shapeIdB, childIndex, featureId)`. On restore, write body state back, refresh shape AABBs and mark proxies moved, and leave contact *topology* live. The next step's pair update and narrow phase fix up contacts naturally (creating/destroying through the normal paths), and warm starts are re-applied by key where they still match. This removes the §7 diff, all slot reconstruction, pool restore and the §9 engine-side reservation problem. Keep bit-exact as an explicit v2 non-goal unless `mygame` has a concrete need (e.g. lockstep). |
| P3 | High | §4.2, §8, §10 | Sleep exemption prevents island splitting (`solver.c:824` only nominates sleepy bodies), so the captured island only ever grows. The per-tick solve cost of the extra always-awake bodies, the capture cost, and the merge-rejection rate all grow over a play session. The "pending split" capture check in §8 is effectively dead code under this scheme, because captured islands can never become split candidates. | Drop the exemption. Let bodies sleep and split normally. If a captured body is asleep at restore, wake it (`b3WakeSolverSet`) on the cold path. Under P2, sleep/island state is not part of the snapshot, so this is just "wake, then write state". If bit-exact is kept, at least allow splits (e.g. a split heuristic driven by `constraintRemoveCount` rather than sleep for exempted islands), and document the growth. |
| P4 | Medium | §4.3, §6 | "Broad-phase tree is rebuilt from AABBs on restore." A whole-tree rebuild is O(world proxies) on every restore, even though only the captured shapes' proxies changed. The step already runs a partial rebuild of the dynamic/kinematic trees every tick (`broad_phase.c:622-627`, `b3UpdateTreesTask`). | On restore, call `b3BroadPhase_MoveProxy` (or set the node AABB and `b3BroadPhase_MarkProxyMoved`) for captured shapes only: O(k log n). Proxy keys stay valid because proxies are never destroyed, which removes the need to "repopulate a valid `proxyKey` after rebuild". |
| P5 | Medium | §6, §7 preflight (b) | "No outside allocation/free from any pool since capture" has no specified O(1) check. Comparing `freeArray`/`nextIndex` snapshots is O(pool free-list size) and can't tell inside churn from outside churn anyway. | If bit-exact survives P2, add a per-pool mutation counter to `b3IdPool` plus a per-capture counter of in-set allocations, then compare in O(1). Under P2 this check goes away. |
| P6 | Medium | §5, §6 capture walk | Capture walks linked contact chains (`headContactKey`) for every captured body every tick. That is pointer chasing across `world->contacts` with random access. Touching contacts are already contiguous in `b3Island.contacts` (`island.h:61-67`, added precisely to avoid touching `b3Contact`). | Take touching contacts from `island->contacts` (contiguous). Walk body chains only for non-touching contacts, or better (P2), don't capture non-touching contacts at all. They carry no warm-start state, and the next pair update re-derives them. |
| P7 | Low | §6 per contact | Whole-`b3Contact` struct copies include the SAT/simplex cache, recycling cache, pointers and flags. That's fine under bit-exact, but under P2 only the manifold impulses matter. | Under P2, capture a compact `{key, normalImpulse[], tangent/twist/rolling impulses}` record per touching contact. It's much smaller than the struct plus manifold, and copying it is a straight loop. |
| P8 | Low | §8, §9 memory layout | One heap-allocated `b3RollbackBuffer` per tick slot means N separate allocations with poor locality, and growth reallocates per slot. | Let one buffer object own an N-slot ring (`b3CreateRollbackBuffer(world, roots, count, historyLength)`) backed by a single allocation with per-slot offsets. Capture writes the next slot; restore takes a tick index. Growth reallocates once for the whole ring. This also answers §11 Q3 (box3d can report bytes-per-slot). |
| P9 | Low | §8 exempt-set maintenance | Refcounted per-body exemption with an exempt set that can grow means each capture has to compare the current component against the registered set: extra per-tick work and flag writes. | Removed by P3. |

## What the fast design looks like (for the doc revision)

- **Capture (hot, every tick):** for each captured island, copy `{bodyId, transform, center,
  center0, rotation0, linearVelocity, angularVelocity}` per body from the awake set's
  `bodySims`/`bodyStates`, plus a warm-start record per touching contact from
  `island->contacts`, plus joint warm-start impulses from `island->joints`. It's a linear
  gather into a preallocated ring slot, with no hashing, no allocation and no world mutation.
- **Restore (cold):** wake captured bodies if asleep; write state back; update shape AABBs and
  `MoveProxy` the captured shapes; re-apply warm-start impulses to contacts that still exist
  (lookup by `pairSet` key, which is already a hash set). Contacts that no longer exist are
  simply not warm-started. Contacts that exist now but didn't then keep their current impulses
  or get zeroed (pick one; zeroing is closer to "created fresh").
- **Resim (cold, N×):** island-scoped step (P1). Its cost is proportional to the player's
  island, which §5 already says `mygame` keeps small.
- **Validation:** body-id generation check on restore (a destroyed body fails the restore), and
  nothing else. The component-stability, pool-churn and capacity preflights all exist only to
  serve bit-exactness.

## Checked, no change

- `b3World_Step` has no hidden RNG/clock dependency, so determinism given identical inputs holds
  (§1). Confirmed by reading the step body.
- Manifold block reuse hazard (§7) is real (`contact.c:340`, `block_allocator.c`). It only matters
  if contacts are recreated by the rollback path, which P2 avoids.
- `b3Island` does keep contiguous `bodies`/`contacts`/`joints` link arrays (§6 per-island).
- Island split only on sleepy islands: `solver.c:824-835` plus the reduction at `solver.c:2201-2217`.

## Addendum: whole-world capture, measured

Question raised after the review: if the snapshot is the whole world instead of a subset, do
P1–P9 go away? Most of the *correctness* findings do: P2, P3, P5, P6 and P9 disappear, and
P1's double-advance bug disappears too. What remains is P1's *cost*, which is N full-world
steps per correction. Whole-world capture also adds a per-tick copy proportional to world size.
Measured to size both.

**Method.** Throwaway harness (not committed) linked against a Release `libbox3d.a`
(`-O3`, `BOX3D_VALIDATE=OFF`), run on the 13 `benchmark/` scenes. Each scene was stepped to half
its benchmark length (60 Hz, 4 substeps) so contacts/islands are in steady state. At that point
the harness enumerated every piece of world state a whole-world rollback must copy:

- `bodies`
- all solver sets' arrays
- `contacts` plus each live contact's manifold block and mesh `triangleCache`
- `joints`
- `islands` plus each island's inner arrays
- constraint-graph colours (`bodySet` bits, `jointSims`, `convexContacts`, `contacts`)
- broad-phase `pairSet`
- id-pool free arrays
- `shapes` plus `fatAABBs`
- all three trees' node/parent/proxy arrays

It then timed a gather-`memcpy` of all of it into a 10-slot preallocated ring (median of 40,
rotating slots). Step time is the mean of the next 60 steps. Machine: 12-thread WSL2 box. Treat
absolute times as relative only; the ratios are the point.

**8 workers** (step and resim are multi-threaded; the copy is a single-threaded memcpy):

| scene | bodies | contacts | max island | step ms | 9-tick resim ms | state / tick | 10-slot ring | copy ms | copy ÷ step |
|---|---|---|---|---|---|---|---|---|---|
| trees100 | 51 | 1,100 | 1 | 0.52 | 4.7 | 1.0 MB | 10 MB | 0.11 | 20% |
| trees25 | 51 | 1,100 | 1 | 1.27 | 11.5 | 2.6 MB | 26 MB | 0.30 | 24% |
| spinner | 1,502 | 7,468 | 1,501 | 2.53 | 22.8 | 5.3 MB | 53 MB | 0.64 | 25% |
| sleep | 2,101 | 5,890 | 210 | 1.59 | 14.3 | 5.0 MB | 50 MB | 0.58 | 36% |
| rain | 2,620 | 4,531 | 36 | 3.59 | 32.3 | 5.2 MB | 52 MB | 0.64 | 18% |
| convex_pile | 5,121 | 28,187 | 2,652 | 5.49 | 49.4 | 12.8 MB | 129 MB | 1.76 | 32% |
| large_pyramid | 5,051 | 14,950 | 5,050 | 5.08 | 45.7 | 11.6 MB | 116 MB | 1.75 | 34% |
| washer | 8,002 | 45,022 | 6,066 | 11.8 | 106 | 22.7 MB | 227 MB | 3.25 | 28% |
| joint_grid | 10,000 | 0 | 9,900 | 8.47 | 76.2 | 17.1 MB | 171 MB | 2.59 | 31% |
| many_pyramids | 10,781 | 28,420 | 55 | 11.8 | 106 | 23.1 MB | 231 MB | 3.34 | 28% |
| junkyard | 10,586 | 140,778 | 9,726 | 25.4 | 228 | 50.9 MB | 509 MB | 6.33 | 25% |

At 1 worker, step times are 3–4× higher and copy ÷ step is 7–12%, so the copy's share of the
tick grows as the solver parallelises and the memcpy doesn't.

`large_world` (1,000,049 bodies, 10 awake) is the pathological case. Whole-world state is
650 MB/tick, and the 10-slot ring (6.5 GB) swapped, so its copy time (18.6 s) isn't meaningful.
Even skipping sleeping sets and the static tree leaves 360 MB/tick, because the `bodies` and
`shapes` arrays are dominated by static entries. The simulated state is a few KB.

**Findings.**

1. **Resim dominates capture by ~8–10× even when capture is a full-world memcpy.** A 9-tick
   correction (60 Hz, ~150 ms RTT) fits a 16.7 ms frame only in the trees-sized scenes (≤ ~50
   bodies / ~1k contacts). By `sleep`/`rain` size (~2–2.6k bodies) it already costs 14–32 ms.
   Whole-world rollback is therefore viable only for worlds of roughly ≲ 500–1,000 awake bodies,
   extrapolating from rain/sleep at ~1–1.5 µs per awake body per step with 8 workers. Beyond
   that, an island-scoped resim (P1) is required whatever the capture strategy.
2. **Whole-world capture costs ~20–35% of a step every tick at 8 workers.** That is roughly
   ~2 KB per awake body (contacts and manifolds are >50% of it) at ~7–8 GB/s. It's cheap in
   absolute terms for small worlds, but a real per-tick tax, contrary to "anything each tick
   has to be fast". It can be reduced by:
   - flattening the 1,300–29,000 separate memcpy regions per capture (per-island arrays and
     per-contact manifold blocks are the fragmenting ones),
   - skipping sleeping sets and the static tree unless dirty,
   - splitting the copy across workers.
3. **Memory is the binding constraint for whole-world.** A 10-slot ring is 50–500 MB for the
   dense scenes and multiple GB for `large_world`-style worlds.
4. **Islands are not small in stacked scenes.** Max island size ranges from 1 to 9,900 bodies.
   A subset capture of a body resting in a pyramid or pile *is* most of the world. A subset only
   wins when the predicted body's island is small, which is true for a character capsule on
   static ground and false for anything pushing a stack. (P3's never-split behaviour makes this
   worse over time.)

**Recommendation update.** For `mygame`, the choice follows from awake-body count:

- **Small worlds (≲ 500 awake bodies): whole-world, bit-exact.** Use a flattened memcpy ring
  plus dirty-skipping of sleeping sets and the static tree, and whole-world resim. It is the
  simplest correct design, and P2/P3/P5/P6/P9 don't arise.
- **Larger worlds: subset capture plus island-scoped resim.** Use the relaxed determinism of P2.
  Whole-world resim is too expensive at this size regardless of how state is captured.

The design doc should state which regime `mygame` targets, with its expected awake-body and
contact counts, before picking one.

## Questions for the author

1. Does `mygame` have any requirement beyond "prediction converges to server state within
   tolerance" (lockstep, replays compared bit-for-bit, anti-cheat hash comparison)? If not, P2
   applies and most of §§6-7, §9 and §11 Q4 can be deleted.
2. Is an island-scoped step acceptable as new engine surface area? It's the only change here
   that touches the solver pipeline, but it's also where nearly all the time goes.

# Solver changes in the bit-exact history ring, and what they change

Summary of the solver changes proposed in `20260927-002-bit-exact-history-ring.md`
(§7.4 and §11.2 item 4), written for someone who wants to know what changes, not how.

## The short version

- **No public API signature changes.** The design only adds new functions (enable history, rewind,
  state hash and so on).
- **Simulation results change.** Three small changes in v1 make the solver independent of the
  order in which the broad-phase trees are walked. Results stay physically equivalent but are not
  bit-identical to today's in the affected cases.
- **Six larger changes are deferred.** They would make each island's result independent of the
  rest of the world, which enables incremental replay later. None is needed for v1.

## Why the solver changes at all

Rewind restores state, then re-steps. For the re-step to reproduce the original, the step must
give the same answer whatever the internal layout of the broad-phase trees is. Restore rebuilds
the trees in its own way, so any result that depends on tree walk order would differ after a rewind.
The ring avoids storing the trees, which saves memory, but only if the walk order stops mattering.

## v1: three order changes

Each one replaces "whatever order the tree gives us" with a fixed order.

### 1. Continuous collision: the solid minimum

- **Today:** a fast body's sweep tests candidate shapes one at a time as the tree hands them over.
  Each test uses the best hit found so far as its limit, so the numbers depend on visit order.
- **Change:** every candidate is tested against the same starting limit, and the earliest accepted
  hit wins.
- **Effect:** the answer no longer depends on visit order. On rare sweeps with several candidates
  the result can be slightly tighter than before. Cost is a few extra time-of-impact tests.

### 2. Continuous collision: sensor hits

- **Today:** a sweep records up to 8 sensor hits, deciding which ones qualify while the tree is
  still being walked. Which 8 fill the cap depends on visit order.
- **Change:** a first pass finds the solid minimum only. A second pass then tests the sensors and
  keeps the 8 smallest by (sensor shape id, visitor shape id) among those that hit before the solid
  fraction.
- **Effect:** the reported sensor hit set is deterministic. It can differ from today's when there
  are more than 8 candidates or when the running limit used to reject or admit them.

### 3. Explosions

- **Today:** the explosion callback wakes sleeping sets and adds impulse to bodies as the tree
  reports shapes. A body with several shapes in range accumulates impulses in tree order, and
  float addition is order-sensitive.
- **Change:** the callback only collects the hit shapes. Afterwards they are sorted by shape id,
  and waking and impulse accumulation happen in that order.
- **Effect:** explosion results are deterministic. Velocities of multi-shape bodies can differ in
  the last bits from today's, and wake order, and so constraint ordering, can change.

## Deferred: six island-independence changes

These would make an island's evolution depend only on its own state and inputs, so unchanged
islands could be copied from history instead of re-simulated. Each is described here in one line.

| Today the result depends on | Proposed change |
| --- | --- |
| Contact ids from a global free list decide begin/end processing order, and so colour assignment | Process begin/end per island in shape-pair-key order |
| The overflow colour is solved in an array that other islands' removals reshuffle | Keep overflow order island-intrinsic (sort by key, or per-island lists) |
| One island split per step, chosen by a world-wide maximum, which affects sleep timing | An island-local split trigger |
| One world-wide flag gates the restitution pass | Per-island restitution flag |
| Waking recolours constraints in stored order | Recolour in shape-pair-key order |
| Island merge survivor and link order follow merge order | Choose by lowest body id among the merged islands |

Each of these changes simulation results and needs its own review. The design treats bit-exact
scoped rollback as impossible without them.

## What changes, by audience

**Solver owner**
- Three changes to code in the CCD and explosion paths. Small and local.
- Open question 1 in the design asks whether these are acceptable, since they alter results.

**Users comparing across versions (lockstep, saved replays, regression baselines)**
- Recordings made before the change can diverge after it, in the affected cases only.
- The design does not say whether the changes are gated on history being enabled. As written they
  look unconditional. Gating would avoid the break but leave two solver behaviours to maintain.

**Users who don't compare across versions**
- Same signatures and physically equivalent results. Nothing to do.

**Engine code**
- Separate from the solver changes: record arrays become read-only outside their owning files, so
  direct field writes elsewhere stop compiling and move to accessor functions (§5.2). Internal only.

## Not changing

- The stepping pipeline, and the `b3World_Step` call that replay uses.
- Public function signatures.
- Static tree layout, which is never imaged and only needs the order changes above.

---
title: Proof tests - continuous collision depends on broad-phase tree layout
status: Canonical
date: 2026-10-12
supports: solver-tree-order-independent-pr (20261012-001 A1, 20261012-002)
---

# Proof tests: continuous collision depends on broad-phase tree layout

A request against `solver-tree-order-independent-pr`: unit tests that prove the determinism issue
the sorted-candidate change fixes, that is, that on `main` the continuous collision stage gives
different answers for the same world when the broad-phase trees are laid out differently. This
document states what such a proof has to show, records what the PR already shows and where it falls
short as proof for a reader of the PR, reports a public-API reproduction run against `main` and the
PR head, and proposes the tests to land.

## 1. The proposition, and what counts as proof

**Proposition P.** Take two worlds with identical bodies, shapes, public ids, proxy bounds, flags,
velocities and transforms, whose only difference is the internal layout of the static broad-phase
tree. Public ids of two worlds are compared with the world selector ignored: equal index and equal
generation. On `main` one `b3World_Step` can give different results, observable through the public
API as:

- **P1** the fast body's transform after the step (its time-of-impact fraction), and each of the
  three continuous iteration counters of `b3World_GetCounters` (`distanceIterations`,
  `pushBackIterations`, `rootIterations`), each its own observable; the body's velocities are not
  claimed;
- **P2** the sequence, and the set, of `b3PreSolveFcn` calls, even for a callback that depends only
  on its arguments;
- **P3** the set of sensor begin events when more than eight sensors lie on a bullet's path.

Not claimed: run-to-run nondeterminism. The same program gives the same result every run on `main`
(`DeterminismTest` passes there). The claim is about equal worlds with different construction
histories, which §3 shows a user reaches with ordinary calls. The proofs use the strict perturbation
of §3, which keeps public ids and bounds.

A proof is a test that:

1. builds the two worlds through the public API, so the perturbation is something a user does;
2. witnesses that the candidate visit order at the continuous query differs between them, otherwise
   a pass is vacuous and a failure unexplained;
3. prints both results, then asserts they are equal;
4. fails on `main` on that assertion and passes on the PR branch, with the witness still differing.

A test that passes on `main` with a differing witness proves nothing for its case; P stands on any
case that fails there. What would void a proof: a perturbation that changes anything besides layout
(public ids, bounds, state, the tree's moved marks included), a witness over a candidate set wider
than the query's, or a step that produced no impact, no call or no sensor report to compare.

## 2. Why layout reaches the result on `main`

In `src/solver.c` (`b3ContinuousQueryCallback`, `b3SolveContinuous`) and `src/distance.c`
(`b3TimeOfImpact`):

- Candidates are evaluated inside the tree callback, in the order `b3DynamicTree_Query` hands them
  back. That order is fixed by the layout alone: the traversal is depth-first over sibling pairs and
  prunes subtrees but never reorders them (`src/dynamic_tree.c`).
- Each solid candidate's `b3ShapeTimeOfImpact` runs with `maxFraction` equal to the running best
  fraction. Inside `b3TimeOfImpact` that value is the upper end of the root bracket (`t2 = tMax`).
  The root finder alternates bisection and false position and stops at the first `t` whose
  separation lies within `tolerance` (a quarter of `B3_LINEAR_SLOP`) of the target. Two brackets
  converge to two different `t` that both pass the stop test. So the same nearest shape, reached
  after different earlier candidates, returns a different fraction (P1). The structural bound on the
  difference is the tolerance band in separation, not an ulp; §5 observes a few ulps of position.
- The iteration counters are maxima over accepted candidates, and which candidates are accepted
  (fraction below the running one) depends on the order (P1, counters).
- `preSolve` is called only for a solid candidate whose fraction is below the running one, so both
  the order and the set of calls depend on visit order. A callback that returns false keeps the
  fraction open and is then called for every on-path candidate, in visit order (P2).
- A sensor candidate takes one of the eight slots when its fraction is no greater than the running
  one, first come first served. With more than eight on the path and every candidate a hit, the eight
  kept are the first eight visited (P3); a candidate that misses the sweep also takes a slot and is
  dropped when hits are emitted.

After the change the callback only collects ids and the evaluation runs in ascending shape id per
query, so none of the four depends on the layout.

## 3. How equal worlds get different layouts through the public API

A reviewer's first question is whether a user can reach two layouts at all. Two ways need no
internal header; a third is internal.

| Perturbation | Public calls | Effect on the static tree |
|---|---|---|
| Move away and back, the strict form | `b3Body_SetTransform` twice per static body | A static body's proxy is moved when its box leaves its fat box (`src/body.c`): a remove and reinsert without rotation (`b3DynamicTree_MoveProxyInternal`). The moved box is inflated by the shape margin where creation used the speculative distance (`src/shape.c`), so the first cycle changes the bounds once. The strict form gives every static body exactly one cycle in both worlds, world A in ascending and world B in descending body order: bounds, public ids and moved marks are then equal and only the reinsertion order, hence the layout, differs. The moved mark stays set until the step's pair update (`src/broad_phase.c`) clears it, before the continuous stage. One body's shapes are reinserted in shape-list order whatever the cycle, so the P3 scene puts each sensor on its own static body. |
| Destroy and recreate | `b3DestroyBody` or `b3DestroyShape`, then create again | Freed ids are reused last in, first out (`src/id_pool.c`): destroying the upper half of the shapes in descending order and recreating them in ascending order hands back the same body and shape indices, with advanced generations (`src/body.c`, `src/shape.c`). Each recreated proxy is inserted afresh with rotations (`b3InsertLeaf`, `src/dynamic_tree.c`), so the layout differs. The worlds are not equal in public ids, so this is how a user reaches a layout, not a proof perturbation (§8). |
| Full rebuild | `b3World_RebuildStaticTree` | Not public: declared in `src/physics_world.h` only. The PR's helper calls `b3DynamicTree_Rebuild` directly through internal headers. |

Game code does the first two routinely: moving a platform, editor undo, level streaming. The dynamic
and kinematic trees have a fourth way, the step's incremental rebuild that keeps settled subtrees
(the PR's corridor construction); it is not needed for the static-tree proofs.

**Witness through the public API.** `b3World_OverlapAABB` queries the three trees with
`b3DynamicTree_Query` in static, kinematic, dynamic order (`src/physics_world.c`), so the callback
order of the static shapes is the static tree's visit order, the one the continuous query sees. The
witness is exact when every shape the covering box returns is a candidate of the continuous query;
the corridor construction gives that (every box inside the sphere's one-step swept box, nothing else
in the world).

## 4. What the PR already shows, and why it is not yet a proof for its reader

The PR's tests (`test/test_tree_order.c`) and their fail-before runs, from the closure document of
20261012-002:

| Test | Proves | Fail-before on `main` |
|---|---|---|
| `ContinuousCallbackOrderTest` | P2 order, in all three trees, in every run | callback logs in visit order, different between runs, none ascending |
| `ContinuousSensorSelectionTest` | P3 | `4 6 7 8 9 10 11 12` against `4 5 7 8 9 10 11 12` |
| `ContinuousCandidateOrderTest` | world-state divergence under a full rebuild of all three trees, statistically | static corridor 23 of 40 seeds, field 1 of 40, kinematic 0, dynamic 1; four workers static 5 of 8 |
| `TreeOrderHelperTest` | the witness itself | none (no behaviour to fail on) |
| `ExplodeOrderTest` | not continuous collision | awake order and velocities differ |

Shortfalls as proof for someone reading the PR:

1. The perturbation is an internal call (`b3DynamicTree_Rebuild` on `world->broadPhase.trees`) and
   the file includes five internal headers. A reader can object that no user reaches the perturbed
   layout. §3 answers that, but the tests do not.
2. They are regression tests that land with the fix, and the PR text draft gives a reader no way to
   see them fail. The fail-before evidence is prose in the closure document, which stays outside the
   PR and reproduced it through a scratch worktree with three files copied in. A one-command recipe
   exists and works for them too (§6); the PR text does not give it.
3. The fuzz test's headline count is one seed in forty for the field. The corridor counts are
   stronger but are reported as found, not required.
4. P2 is shown as the call order of a rejecting callback. Nothing shows that the set of calls to an
   accepting callback changes, which is the user-visible form: a callback with a side effect fires a
   different number of times.
5. Nothing shows the public counters change.

## 5. Public-API reproduction against `main`

A scratch program, `proof.c`, public headers only, built against the Debug float `libbox3dd.a` of
`main` at `5d83df8` and of the PR head `f66ee7f` (GCC 14.2, one worker). Scene: the PR's static
corridor, a sphere of radius 1.0 at 300 m/s, time step 1/60 with four sub-steps, sixteen static
0.1 m boxes on sixteen static bodies scattered inside the swept cylinder, zero gravity, no sleep;
seeds 1 to 200 of the PR's placement rule. Both worlds are built the same way and given the strict
perturbation of §3, world A in ascending and world B in descending body order, then checked to hold
the same body and shape ids. The witness is `b3World_OverlapAABB` over a box covering the corridor,
recorded before the step, the sphere's own shape removed. Sensor case: the PR's twelve sensors at the
PR's spacing, each on its own static body, none overlapping another, and one bullet. The whole run
takes under half a second.

| Proof | Witness: same set / order differs | `main` | PR head |
|---|---|---|---|
| P1 transform, no callbacks | 200 / 200 of 200 | transform diverges 107; velocities 0 | 0 |
| P1 counters | same run | `distanceIterations` 67, `pushBackIterations` 67, `rootIterations` 67 | 0, 0, 0 |
| P2 order, rejecting callback | 200 / 200 of 200 | call order differs 200, call sets equal | 0 |
| P2 set, accepting callback | 200 / 200 of 200 | call set differs 188, call order differs 188 | 0, 0 |
| P3 sensor set | yes | sets differ | sets equal |

No seed diverged in any observable without a differing witness order. Excerpts from the `main` run;
shape ids are internal (`index1 - 1`), and the preSolve log is the other shape's id in call order,
so with an accepting callback its last entry is the accepted hit.

```
seed 1
  visit order A: 4 5 15 10 14 3 13 6 7 11 1 9 8 12 0 2
  visit order B: 5 4 15 14 10 3 13 7 1 11 9 6 2 0 12 8
  preSolve A (accepting): 4 5 15 13 6
  preSolve B (accepting): 5 15 13 1 6
  sphere A: p (0.664135039 0 0)
  sphere B: p (0.664135158 0 0)

seed 5
  preSolve A (accepting): 6 3 10 15
  preSolve B (accepting): 11 3 10 15
  sphere A: p (0.534326911 0 0)
  sphere B: p (0.534326911 0 0)

sensors
  visit order A: 10 11 9 7 8 5 6 3 4 2 0 1
  visit order B: 11 10 7 9 8 4 3 6 5 2 1 0
  reported A: 3 5 6 7 8 9 10 11
  reported B: 3 4 6 7 8 9 10 11
```

Seed 1 is the mechanism of §2 in one case: both runs end on box 6, but box 6's time-of-impact call
runs to box 13's fraction in A and to box 1's in B, and the two calls return fractions two ulps
apart; box 4 is called in A only and box 1 in B only. Seed 5 shows the accepting-callback set
changing (box 6 against box 11) with the transform unchanged. On the PR head the sensors reported are
`0 1 2 3 4 5 6 7` in both runs and every count in the last column is zero while the witness column
is unchanged, so the proofs are not vacuous there.

Three things the run adds to §4: the rejecting-callback order differs in every seed whose witness
differs (200 of 200), so P2 is a guaranteed proof, not a statistical one; each public counter changes
in a third of the layouts; and the perturbation is an ordinary user operation applied in a
different order.

## 6. Proposed proof tests

Land public-API tests in the PR that fail against `main`'s `src/` and pass on the branch. The test
target builds on the PR branch with `main`'s `src/` checked out over it (verified: the PR's existing
`ContinuousSensorSelectionTest` then fails with the closure document's output), so a reviewer
reproduces the fail-before on the PR branch itself:

```bash
git checkout main -- src                               # on solver-tree-order-independent-pr
cmake --build build --target test
for t in ContinuousLayoutImpactTest ContinuousLayoutPreSolveTest ContinuousLayoutSensorTest; do ./build/bin/test $t; done
git checkout solver-tree-order-independent-pr -- src
```

No scratch worktree and no file copying. The three tests live in a new `test/test_ccd_layout.c`
(§8). Each uses the strict perturbation of §3. Each requires the witness to hold the same complete
candidate set in both worlds, some seed's order to differ, and every seed whose results differ to
have a differing order. Each prints the divergence counts and, for the first diverging seed of each
observable, both values and both witness orders, before its first assertion.

- **`ContinuousLayoutImpactTest`** (P1). The corridor at seeds 1 to 200. Per seed: require the
  sixteen boxes in the witness, equal contact counts before the step and an impact in both worlds;
  count separately the seeds whose order differs, whose sphere transform differs bitwise, and whose
  `distanceIterations`, `pushBackIterations` and `rootIterations` differ. Require the four
  divergence counts zero. Fail-before: 107 and 67, 67, 67 of 200.
- **`ContinuousLayoutPreSolveTest`** (P2). The same corridor with `enablePreSolveEvents` on every
  shape and a callback that logs the other shape's id. Two passes: a rejecting callback, requiring
  equal, non-empty call sequences (fail-before 200 of 200); an accepting one, counting and requiring
  separately equal, non-empty call sets and equal call sequences (fail-before 188 and 188 of 200).
- **`ContinuousLayoutSensorTest`** (P3). The twelve-sensor scene of §5. Require the reported sets to
  be equal and to hold eight. Fail-before: the sets in §5.

The existing tests stay. They cover what the public proofs cannot: the kinematic and dynamic trees
(bullets), four workers, the ascending-order property itself, and Explode. The public proofs add
user-reachability, the accepting-callback set and the counters.

## 7. Checks before the proofs are final

- Run the three tests in the `BOX3D_DOUBLE_PRECISION` configuration, fail-before and pass; the
  closure document ran both configurations for the existing tests.
- Run them from a Release build once and record the counts; the divergence rate may differ from the
  Debug build's and the order-differs count must stay above zero.
- Quote the `git checkout main -- src` recipe and its output for the three tests in the PR text.
- Keep 200 seeds; the whole program runs in under half a second.
- The four-worker configuration is not covered by these tests (single-worker worlds); the PR's fuzz
  test covers it.
- 20261011-001 (superseded) calls `b3World_RebuildStaticTree` "the public equivalent"; it is not
  public. One-line correction when that document is next touched.

## 8. Decisions

- **Where the proofs live.** A new `test/test_ccd_layout.c`, public API only and readable on its
  own; its freedom from internal headers is the point. `test/test_tree_order.c` is not changed.
- **Which evidence leads the PR text.** The three public proofs, with the `git checkout main -- src`
  recipe and the §5 counts; the internal tests are presented as coverage of the other trees and of
  parallel workers.
- **One perturbation, the strict move-away-and-back form.** Destroy and recreate was to be a second
  proof perturbation because it hands back the same indices; it advances the id generations, so a
  proof under it is a proof for worlds that are not equal in public ids, and P would have to be
  weakened to equal indices. The strict move form proves every observable with equal public ids and
  bounds, at a higher rate in each, so nothing is lost. Destroy and recreate stays in §3 as a way a
  user reaches a different layout.

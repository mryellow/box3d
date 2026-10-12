---
title: Proof tests - continuous collision depends on broad-phase tree layout
status: Draft
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

**Proposition P.** Take two worlds with identical bodies, shapes, ids, flags, velocities and
transforms, whose only difference is the internal layout of the static broad-phase tree. On `main`
one `b3World_Step` can give different results, observable through the public API as:

- **P1** the fast body's transform after the step (its time-of-impact fraction), and the continuous
  iteration counters of `b3World_GetCounters`;
- **P2** the sequence, and the set, of `b3PreSolveFcn` calls, even for a callback that depends only
  on its arguments;
- **P3** the set of sensor begin events when more than eight sensors lie on a bullet's path.

Not claimed: run-to-run nondeterminism. The same program gives the same result every run on `main`
(`DeterminismTest` passes there). The claim is about equal worlds with different construction
histories, which §3 shows a user reaches with ordinary calls.

A proof is a test that:

1. builds the two worlds through the public API, so the perturbation is something a user does;
2. witnesses that the candidate visit order at the continuous query differs between them, otherwise
   a pass is vacuous and a failure unexplained;
3. prints both results, then asserts they are equal;
4. fails on `main` on that assertion and passes on the PR branch, with the witness still differing.

What would falsify P: the test passing on `main` with a differing witness. What would void a proof: a
perturbation that changes anything besides layout (ids, bounds, state), or a witness over a candidate
set wider than the query's.

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
  one, first come first served. With more than eight on the path, the eight kept are the first eight
  visited (P3).

After the change the callback only collects ids and the evaluation runs in ascending shape id per
query, so none of the four depends on the layout.

## 3. How equal worlds get different layouts through the public API

A reviewer's first question is whether a user can reach two layouts at all. Two ways need no
internal header; a third is internal.

| Perturbation | Public calls | Effect on the static tree |
|---|---|---|
| Destroy and recreate | `b3DestroyBody` or `b3DestroyShape`, then create again | Freed ids are reused last in, first out (`src/id_pool.c`): destroying the upper half of the shapes in descending order and recreating them in ascending order hands back the same body and shape ids. Each recreated proxy is inserted afresh with rotations (`b3InsertLeaf`, `src/dynamic_tree.c`), so the layout differs from one insertion pass. |
| Move away and back | `b3Body_SetTransform` twice | A static body's proxy is moved when its box leaves its fat box (`src/body.c`): a remove and reinsert without rotation (`b3DynamicTree_MoveProxyInternal`). After the second move the bounds and ids are what they were. |
| Full rebuild | `b3World_RebuildStaticTree` | Not public: declared in `src/physics_world.h` only. The PR's helper calls `b3DynamicTree_Rebuild` directly through internal headers. |

Game code does the first two routinely: level streaming, editor undo, moving a platform. The dynamic
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
| `ContinuousCandidateOrderTest` | P1 statistically | static corridor 23 of 40 seeds, field 1 of 40, kinematic 0, dynamic 1; four workers static 5 of 8 |
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
0.1 m boxes scattered inside the swept cylinder, zero gravity, no sleep; seeds 1 to 200 of the PR's
placement rule. World A is built once; world B is built the same way, then perturbed as in §3, then
checked to hold the same body and shape ids. The witness is `b3World_OverlapAABB` over a box
covering the corridor, recorded before the step, the sphere's own shape removed. Sensor case: the
PR's twelve sensors on one static body and one bullet. The whole run takes under half a second.

| Proof | Perturbation | Witness: same set, order differs | `main` | PR head |
|---|---|---|---|---|
| P1 transform, no callbacks | destroy and recreate | 200 / 106 of 200 | transform diverges 19, counters differ 25 | 0, 0 |
| P1 transform, no callbacks | move away and back | 200 / 186 of 200 | transform diverges 60, counters differ 42 | 0, 0 |
| P2 order, rejecting callback | destroy and recreate | 200 / 106 of 200 | call order differs 106, sets equal | 0 |
| P2 set, accepting callback | destroy and recreate | 200 / 106 of 200 | call set differs 70 | 0 |
| P3 sensor set | destroy and recreate | yes | sets differ | sets equal |
| P3 sensor set | move away and back | yes | sets differ | sets equal |

Excerpts from the `main` run. Shape ids are internal (`index1 - 1`); the preSolve log is the other
shape's id in call order with an accepting callback, so its last entry is the accepted hit.

```
seed 26, destroy and recreate
  visit order A: 6 8 13 0 14 5 11 1 2 4 9 7 10 15 3 12
  visit order B: 4 9 0 6 5 11 1 2 14 15 3 12 7 13 8 10
  preSolve A: 6
  preSolve B: 4 9 0 6
  sphere A: p (0.576496363 0 0)
  sphere B: p (0.576496124 0 0)

seed 32, destroy and recreate
  preSolve A: 15 8 6 13 1 11
  preSolve B: 15 8 6 13 11
  sphere A: p (0.499357581 0 0)
  sphere B: p (0.499357492 0 0)

sensors, destroy and recreate
  visit order A: 9 10 11 6 7 8 5 3 4 2 0 1
  visit order B: 5 3 4 2 0 1 9 10 11 6 7 8
  reported A: 3 5 6 7 8 9 10 11
  reported B: 0 1 2 3 4 5 9 10
```

Seed 26 is the mechanism of §2 in one case: both runs end on box 6, but in A box 6 is visited first
and its time-of-impact call runs to the full sweep, while in B it runs to box 0's fraction, and the
two calls return fractions four ulps apart. Seed 32 ends on box 11 in both runs with box 1 called in
A only. On the PR head the sensors reported are `0 1 2 3 4 5 6 7` in every run and the counts in the
last column are all zero while the witness columns are unchanged, so the proofs are not vacuous there.

Three things the run adds to §4: the rejecting-callback order differs in exactly the seeds whose
witness differs (106 of 106), so P2 is a guaranteed proof, not a statistical one; the public counters
change more often than the transform does; and the perturbations are two ordinary user operations.

## 6. Proposed proof tests

Land public-API tests in the PR that fail against `main`'s `src/` and pass on the branch. The test
target builds on the PR branch with `main`'s `src/` checked out over it (verified: the PR's existing
`ContinuousSensorSelectionTest` then fails with the closure document's output), so a reviewer
reproduces the fail-before on the PR branch itself:

```bash
git checkout main -- src                               # on solver-tree-order-independent-pr
cmake --build build --target test && ./build/bin/test ContinuousLayoutImpactTest
git checkout solver-tree-order-independent-pr -- src
```

No scratch worktree and no file copying. The three tests live in a new `test/test_ccd_layout.c`
(§8), one entry function each, registered in `test/main.c` like the PR's others, each printing its
evidence before its first assertion:

- **`ContinuousLayoutImpactTest`** (P1). The corridor at seeds 1 to 200 under both perturbations of
  §3. Per seed: require the witness to hold the same set; count the seeds whose order differs, the
  seeds whose sphere transform and velocities differ bitwise, and the seeds whose counters differ.
  Print the counts and the first diverging seed's visit orders and positions. Require the
  order-differs count above zero and both divergence counts zero. Fail-before: 19 and 60 of 200.
- **`ContinuousLayoutPreSolveTest`** (P2). The same corridor with `enablePreSolveEvents` on every
  shape and a callback that logs the other shape's id. Two passes: a rejecting callback, requiring
  equal call sequences (fail-before 106 of 200), and an accepting one, requiring equal call sequences,
  which fixes the set too (fail-before 70 of 200).
- **`ContinuousLayoutSensorTest`** (P3). The twelve-sensor scene under both perturbations. Require the
  witness to differ and the reported sets to be equal. Fail-before: the sets in §5.

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
- **Both perturbations.** Move-away-and-back has the higher divergence rate (60 of 200) and is the
  more everyday operation; destroy-and-recreate shows the id pools hand back the same ids, which
  pre-empts the objection that the two worlds are not equal.

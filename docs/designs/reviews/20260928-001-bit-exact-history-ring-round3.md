---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=high
mode: broad (round 3), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 3

## Review report (Codex final message)

## Summary

The whole-world approach is a sound direction, and the existing serializer is a useful prototype. The proposed hot image and cold journal do not yet establish the document’s bit-exact guarantee. Two state transitions lose data needed for rewind, and the design knowingly leaves an ordinary physics API call dependent on tree traversal order.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.1, §6, §9 | A sensor is imaged only when its overlaps change *at that tick*. If it is unchanged at T and changes later, rewinding from that later tick cannot recover T’s `overlaps2`; there is neither a T image nor an overlap journal entry. `src/sensor.c` | Journal old and new overlap sets on every change, or provide another way to recover each sensor’s value at every restorable image tick. Test a quiet T followed by a changed overlap. |
| 2 | High | §5.2, §7.1, §9 | A sleeping contact at T retains its manifold block. Waking moves the contact into the graph, and later collision updates can overwrite that block without changing its count. Journaling the contact record or taking ownership only on free/reallocation cannot recover T’s manifold bytes. The same transition needs scrutiny for mesh triangle-cache contents. `src/solver_set.c`, `src/contact.c`, `src/mesh_contact.c` | Preserve the cold manifold and cache contents when a contact first becomes hot, or journal their subsequent writes. Add a sleep → wake → update → rewind test. |
| 3 | High | §2, §7.4, §10 | The document admits that replaying `b3World_Explode` can produce different velocities after tree reconstruction. That is an ordinary recorded API input, so the stated bit-exact replay guarantee has an exception even with unchanged inputs. `src/physics_world.c` | Apply explosion hits in a stable shape order before waking bodies and accumulating impulses, then include explosions in replay tests. |
| 4 | Medium | §7.4, §13 | Phase 1 images only the dynamic and kinematic trees as its temporary exactness fallback. CCD queries the static tree first; static proxy create, destroy, and movement can change its traversal layout before the Phase 2 CCD change. Phase 1 therefore has no stated means to preserve that order. `src/solver.c`, `src/shape.c`, `src/broad_phase.c` | Include the static tree in the temporary fallback, implement the CCD order change in Phase 1, or explicitly defer the bit-exact claim until Phase 2. |
| 5 | Medium | §7.4, §10 | “Ray/shape casts take a min” describes only some query use. The general cast APIs invoke caller callbacks during tree traversal and use their returned fractions to prune later hits. A rebuilt tree can change query results that game code uses to choose subsequent inputs; equal-distance closest hits also need a stable tie rule. `src/physics_world.c`, `include/box3d/box3d.h` | Define query replay semantics and either canonicalize relevant hit order and ties or state a precise caller restriction. Test queries used to generate gameplay inputs. |
| 6 | Medium | §12 | The proposed “full state hash” does not explicitly cover `fatAABBs`, logical moved-proxy state, or the ordering of solver and graph arrays. Bounds and moved state affect pair discovery; array order affects solver processing. Hashing objects in id order can miss an incorrect restore with identical membership. `src/physics_world.h`, `src/broad_phase.c`, `src/solver.c`, `src/constraint_graph.h` | Specify a canonical hash of every result-affecting value, including bounds, moved state, and order-sensitive array positions. Show that each omission is derived before calling the hash a full-state oracle. |
| 7 | Medium | §2, §8, §14 | `maxBytes` is called a bound, but the ring explicitly exceeds it when required journal storage or one image is too large. Increasing the capture interval cannot reduce that required storage, and arena capacity also needs accounting. The API currently presents a budget that is advisory in important cases. | State a hard-bound policy with a defined unavailable-tick outcome, or rename and document `maxBytes` as a soft target, including physical capacity and owned arrays in `bytesUsed`. |
| 8 | Medium | §1, §8 | The “allocation-free” description is broader than the mechanism. Journal entries grow during structural writes inside `b3World_Step`, and §8 proposes amortized arena reallocation for that growth. `src/physics_world.c`, `src/solver_set.c`, `src/container.h` | Limit the claim to image writing, or reserve and enforce sufficient journal capacity before the step. Measure and report journal allocations separately. |
| 9 | Medium | §10, §13 | Phase 0 cannot validate the stated “every setter” contract as written: deserialization clears body, shape, and joint `userData`, while the full design does not define how world `userData` is rewound. These host values can be read by caller code and callbacks during replay. `src/world_snapshot.c`, `src/body.c`, `src/shape.c`, `src/joint.c`, `src/physics_world.c` | Define host-data ownership and replay rules explicitly. Narrow Phase 0’s claimed test coverage or preserve the required host wiring around deserialization. |
| 10 | Low | §2, §6, §8, §9 | “Any retained tick” is promised as restorable, but with `captureInterval > 1` every tick has a journal segment while only imaged ticks can be passed to `Rewind`. | Define the retained window as a range containing *restorable image ticks*, and state plainly that intervening ticks require replay from `GetRestorableTick`. |

## Checked, no change

- The serializer restores a whole-world image, and `ScrubBackward` checks re-stepped hashes. The document correctly limits that existing hash to transforms and velocities. `src/world_snapshot.c`, `src/recording_replay.c`, `src/recording.c`, `test/test_recording.c`
- Broad-phase candidate pairs are sorted before contact creation; id pools use a free list and bump index. `src/broad_phase.c`, `src/id_pool.c`
- Worker-count determinism has dedicated tests, and the cited floating-point contraction flags are present. `test/test_determinism.c`, `CMakeLists.txt`
- The sensor task already computes a change bitset, and its query results are sorted and deduplicated before event publication. That supports an O(changed-sensors) delta mechanism, though it does not by itself make changed-only images restorable. `src/sensor.c`
- The document correctly identifies the traversal-dependent CCD fraction and eight-sensor cap. `src/solver.c`

## Proposed edits

Resolve findings 1–3 before describing the mechanism as bit-exact. Make the Phase 1 guarantee conditional on the static-tree solution, expand the state-hash specification into an explicit inventory, and align the API wording with the actual memory and capture-interval behavior. Add targeted restore and replay cases for the transitions in findings 1, 2, 3, and 5.

## Unresolved / disagreements

The performance figures attributed to a prior review were not independently checked here because the requested review excludes `docs/designs/reviews/`. They should remain labeled as prior measurements rather than current-source verification. No prior review files were read.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`,
foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T18:46:16+10:00, end 18:52:43+10:00, `EXIT_CODE 0` — no resume needed. The tool call
returned inline (not auto-backgrounded). Raw log had the final message duplicated across three
earlier positions plus the kept copy (streaming artifact); one copy is kept above. Codex ran
read-only; working tree was untouched by it (target doc's `git diff` against HEAD reflected only
round 1 + round 2's already-committed/applied edits going in, and no other paths changed during
the run). The curated declined-findings list from rounds 1–2 (sleep→wake AABB bounds, proxy
numeric-id reconstruction, restore-allocation-free half of `maxBytes`) was included in the prompt;
Codex did not re-raise any of the three.

Each finding checked against source before any doc edit:

- **#1 (sensor overlaps imaged only on the tick they change), CONFIRMED, applied.** Traced
  `b3SensorTask` (`src/sensor.c`): every sensor's `overlaps1`/`overlaps2` are swapped every step
  regardless of change, but the *content* only differs from the prior step's on a tick where
  `eventBits` is set; on every other tick `overlaps1` and `overlaps2` hold identical content.
  §6's "image the changed sensors" rule (round 2's own fix) copies a value into image T's slot
  only on a changed tick — but image T is supposed to be a *complete* snapshot restorable on its
  own, and a sensor unchanged at T (last changed at some T' < T) has no entry in image T and no
  cold-journal fallback either, so nothing reconstructs its value at T once the live overlaps
  have moved on past it. This is exactly the completeness property every other "hot" class member
  gets for free by being memcpy'd/gathered *unconditionally* every tick (awake bodies, contacts,
  etc.) — sensors were the only class imaged *conditionally*, which breaks that property. Fixed
  by reclassifying sensor overlap changes from Hot/Image to Cold/Journal (§4, §5.1, §5.2, §6):
  the existing `eventBits`-gated write now fires an ordinary journal entry (structure tag =
  sensor overlaps, id = the owning shape's stable `shapeId`, per `src/sensor.h`, rather than the
  sensor's own array index, which swap-compaction changes), so the standard journal-walk (§9 step
  2) reconstructs any T's value regardless of how many ticks separate it from the sensor's last
  actual change — the same mechanism every other sparsely-written cold field already relies on.
  Confirmed `overlaps1` needs no such treatment: it is fully recycled scratch storage (cleared and
  refilled every step, `src/sensor.c`), already correctly listed under §5.3 Scratch, unaffected
  by this reclassification. Also fixed §9 step 3, which (before this pass) still listed "sensor
  overlaps" as part of the hot image-copy step — a second-order inconsistency this same fix
  introduced when I moved the classification but hadn't yet swept §9; caught and corrected in the
  same edit.

- **#2 (contact manifold / mesh triangle-cache across sleep→wake), CONFIRMED, applied.** Read
  `contact.h`: `manifolds` is a heap pointer (`b3Manifold*`), not inline, allocated from
  `world->manifoldAllocators` (block allocator, `physics_world.c`). Traced the wake path
  (`solver_set.c`, "Transfer touching contacts from sleeping set to constraint graph" →
  `b3AddContactToGraph`, `constraint_graph.c`): it asserts `manifoldCount > 0` and only manages
  graph-colour bookkeeping (colour index, bitsets) — it never touches `contact->manifolds` or its
  content. So the wake transition's own write is just `contact->setIndex = b3_awakeSet` (plus
  colour/local-index bookkeeping); a generic "record write" journal entry for this write would
  snapshot the *pointer* value, not the pointed-to manifold bytes. Once awake, ordinary narrow-
  phase updates (unjournaled, image-covered) can then overwrite that same heap block in place on
  a later tick — at which point the wake entry's stale "old bytes" (really just an old pointer)
  would, if ever walked back to, dereference memory holding *post*-wake content, not the T-time
  manifold the design needs. The existing "manifold block" entry kind (§7.1) already exists and
  already deep-copies manifold bytes (not just a pointer) — the gap was that the doc never said
  *which* writes use it. Confirmed the identical problem for a mesh contact's `triangleCache`
  (`contact->meshContact.triangleCache`, a heap `b3Array`, `src/mesh_contact.c`/`contact.c`): the
  wake path doesn't touch it either, so it has the same latent-overwrite exposure. Fixed by adding
  an explicit rule (§7.1, after the ownership-transfer paragraph): any structural write to a
  touching contact (create, destroy, sleep, wake, merge, split, transfer) fires the manifold-block
  entry for its manifold and, for a mesh contact, its triangle cache — deep-copying both — instead
  of the generic record write those fields would otherwise fall under. Extended the manifold-block
  table row's payload to name the triangle-cache bytes explicitly.

- **#3 (`b3World_Explode` traversal-order dependence), CONFIRMED, applied.** Read
  `b3World_Explode`/`ExplosionCallback` (`src/physics_world.c`): it queries the dynamic tree with
  `b3DynamicTree_Query` and, inside the callback itself, calls `b3WakeBody` and then directly
  applies `state->linearVelocity = b3MulAdd(...)` / `angularVelocity` — waking and accumulating
  impulses inline, per-candidate, in whatever order the tree traversal visits them. §7.4/§10
  already (correctly, per round 2's own fix) documented this as a genuine order-dependence, but
  left it as an accepted, unfixed v1 exception — which directly contradicts requirement 1's
  unqualified bit-exact promise for "every simulation-affecting byte" under "the same inputs," of
  which an explosion call is an ordinary example, not an edge case. Rather than re-document the
  exception more carefully (as rounds 1–2 did), applied the actual fix, matching the CCD fix's own
  pattern already accepted in the same section: collect every candidate shape from the query
  first, sort by shape id, then wake and apply impulses in that order. This is a plain reordering
  of an existing loop, not a new solver participant kind, consistent with requirement 6. Updated
  §7.4's "Change:" paragraph, its residual-dependence count (now one instead of two, having
  dropped Explode but — see #5 below — gained a different residual item), §10's Explode bullet,
  §16 open question 1 (now "one of two" behavior-altering changes), and §11.3's engine-change list.

- **#4 (Phase 1 fallback omits the static tree), CONFIRMED, applied.** Grepped `solver.c` and
  confirmed CCD queries `world->broadPhase.trees + b3_staticBody` before the kinematic and dynamic
  trees. §7.4's Phase-1 fallback paragraph named only "the kinematic and dynamic trees" for raw
  imaging — omitting the static tree despite CCD's traversal-order dependence applying to it
  equally. Fixed by adding the static tree to the fallback, but not at O(proxies)-per-tick cost
  like the other two: since a static proxy never moves, its tree's DFS layout is stable except at
  a structural event (static shape create/destroy, or `b3World_RebuildStaticTree`), so it only
  needs re-imaging at those events — O(events), not O(static proxies) per tick, which keeps the
  fallback from reintroducing exactly the `large_world`-scale cost requirement 2 exists to
  prevent.

- **#5 (general cast/overlap callbacks are traversal-order-visible), CONFIRMED, applied.** Read
  `include/box3d/box3d.h`: `b3World_CastRay`, `b3World_CastShape`, `b3World_OverlapShape`,
  `b3World_OverlapAABB` all take a caller-supplied result callback (`b3CastResultFcn`/
  `b3OverlapResultFcn`) whose returned fraction can prune later candidates during traversal —
  genuinely different from `b3World_CastRayClosest`, a callback-free convenience that takes a
  plain min and was the only cast API §7.4's "ray/shape casts take a min" claim was actually true
  of. Fixed by narrowing that claim to the closest-hit API (adding a shape-id tie-break, matching
  the project's existing shape-id-as-canonical-order convention used elsewhere), and folding the
  general callback-based APIs' traversal-order visibility into the same residual-dependence
  bucket as `preSolve` (§7.4, §10) rather than proposing an engine fix — unlike Explode, a pruning
  callback's entire point is to *avoid* visiting every candidate, so "collect then sort" isn't
  applicable the same way.

- **#6 (state-hash coverage gaps: `fatAABBs`, moved-proxy state, array order), CONFIRMED, applied
  — partially.** §12's hash list (already extended by round 2 to cover shapes/sensors/scalars)
  still omitted `fatAABBs` and moved-proxy membership, both of which §5.1/§5.2 already classify
  as simulation-affecting (gating broad-phase pair discovery) and are exactly the kind of gap
  findings #1/#2 above show a narrower hash would fail to catch. Added both. Declined the
  "ordering of solver and graph arrays" half: within a single graph colour, every constraint
  touches a disjoint body pair by construction (that's what a colour *is*), so no shared
  accumulation order exists within a colour to hash; cross-colour order doesn't feed a shared
  float sum either. Found no evidence of a result-affecting array-position dependency beyond what
  round 1 already checked (`test/test_determinism.c` worker-count invariance implies solving
  order within a colour isn't itself order-sensitive to results). Not applied; no edit made for
  that half.

- **#7 (`maxBytes` presented as a hard bound it doesn't enforce), CONFIRMED, applied — partially.**
  Requirement 5's summary line ("a byte budget widens the capture interval instead of failing")
  doesn't mention the over-budget exception §8 already states (round 2's own fix) — a genuine
  requirement-vs-mechanism inconsistency introduced by round 2 correcting §8 without updating §2
  to match. Fixed by adding the exception to requirement 5's own wording. Declined the rest:
  "awake counts do not determine journal size before writes occur" is already correctly stated by
  §8 post-round-2 (which explicitly separates image sizing, known upfront, from journal sizing,
  which grows via amortized realloc) — Codex's finding here reads as reviewing text that predates
  that fix, or restating it; either way, no gap remains. "Rename `maxBytes` as a soft target" not
  adopted — the corrected requirement-5 wording already documents the one exception precisely
  without renaming the field.

- **#8 ("allocation-free" broader than the mechanism), CONFIRMED, applied.** §1 point 2 describes
  the missing capability as "in-place, allocation-free, O(awake)," unqualified — but §8 (already
  correct post-round-2) documents that journal segments grow via amortized arena reallocation
  during the step, which is not allocation-free. Scoped §1's claim to image writes specifically,
  matching what §8 already, correctly, only claims for images.

- **#9 (Phase 0 / userData rewind), DECLINED — verified not a defect.** `userData` fields are
  caller-opaque: the engine stores and returns them but never reads them internally, so they are
  outside "simulation-affecting" by requirement 1's own definition and outside §12's hash oracle
  by construction — Phase 0's serializer clearing them on deserialize cannot affect whether tests
  2/3/5 (all hash-based) pass, contra the finding's premise. Checked `world->userData`
  specifically (`physics_world.c`, `b3CreateWorld`): set once from `b3WorldDef` at creation, with
  no setter found — a world is never destroyed and recreated by a rewind, so there is nothing for
  `Rewind` to restore here in the first place. Not applied; no doc change needed.

- **#10 (`captureInterval > 1` retained-but-not-restorable ticks vs. requirement 4's wording),
  CONFIRMED, applied.** Requirement 4 promised "any retained tick can be restored... without
  re-stepping," but §6 (an existing, correct rule, not new to this round) only allows direct
  restore of *imaged* ticks, requiring replay to reach an intervening retained-but-unimaged tick.
  Reworded requirement 4 to state the imaged/retained distinction directly, pointing at §6.

## Status

Round 3 of an ongoing series (round 1 committed as `ba1affc`; round 2 pending commit alongside
this round). 8 of 10 findings were genuine and applied (3 fully, 3 partially — each partial
decline backed by a specific source or in-doc check, not a blanket dismissal); 1 (#9) was
verified false after tracing `userData`'s actual read sites and the hash oracle's actual scope.
One finding (#1, sensor overlaps) is a direct consequence of round 2's own fix (#4 in that round):
narrowing sensor capture to "changed this tick" removed the O(sensor count) cost problem but
silently broke the image-completeness invariant every other hot class relies on — a genuine
second-order defect from a prior round's fix, exactly the pattern the workflow's tracking exists
to catch. Two more (#4, static tree; #7, requirement-5 wording) are also latent gaps in territory
round 1 or round 2 had just edited (the Phase-1 fallback list; the `maxBytes` requirement wording)
without covering these specific edges. Applying #1's fix surfaced one more second-order
inconsistency within this same round, caught before it left the round: §9's image-copy step still
listed "sensor overlaps" after the reclassification moved that state to the journal; fixed in the
same pass. The finding rate has not narrowed across three rounds (9, 8, 8 applied) and two of
three rounds have found a defect introduced by the *immediately preceding* round's own fix, which
is a stronger non-convergence signal than a steady rate alone: it suggests each round's edits are
still large enough, and still touching close-enough-together mechanism, to create fresh gaps a
from-scratch pass then finds. Another full-scope round is warranted, not a narrower one.

---

title: Comparison of the three rollback designs (scoped rollback, history ring, rewind-only history ring) and how they combine
status: Analysis, not a design. Compares docs/designs/20260927-001-client-prediction-rollback.md
  ("001"), docs/designs/20260927-002-bit-exact-history-ring.md ("002") and
  docs/designs/20260929-002-rewind-only-history-ring.md ("002R"). No prototype exists for any of
  them; every cost figure below is an estimate taken from those docs, not a measurement.
date: 2026-09-29

---

# Rollback designs: comparison and combination

Sources: 001 (scoped, consistent, non-bit-exact), 002 (whole-world, bit-exact, hot image + cold
journal that replays both ways) and 002R (002 with forward scrub removed: an undo-only journal).
The diagrams (`20260927-002-bit-exact-history-ring.html`, `20260929-002-rewind-only-history-ring.html`)
only illustrate their `.md` files, so this comparison uses the `.md` files.

002R is derived from 002 and shares its hot/cold split, image, order changes and phase plan. The
sections below therefore treat "002 family" as both, and call out where 002R differs.

## 1. Side-by-side

| Dimension | 001: Scoped rollback | 002: History ring (both-ways journal) | 002R: Rewind-only history ring (undo journal) |
|---|---|---|---|
| **Core idea** | Record only the predicted islands. On restore, resimulate only that scope and freeze everything else. | Rewind the whole world (awake state imaged, sleeping/static state journaled) and re-step the whole world. | Same as 002, but the journal is an undo log only: the caller can go back, never forward without stepping. |
| **Determinism** | Consistent, not bit-exact. Restore destroys and recreates contacts, and contact-id order changes solver results. | Bit-exact, verified by an extended full-state hash. | Same as 002, with two named exceptions v1 does not fix: `preSolve` and custom-filter callback order after a layout-changing restore (only matters for order-dependent side effects), and the name string for two names whose hashes collide. |
| **Capture cost** | O(scope): ~100 B/body, ~84 B/manifold. Kilobytes for a character. | O(awake) plus sensors plus journal events: ~470 B/body, ~490 B/touching contact. 15% or less of a step on all-awake scenes, a few KB on `large_world`. | Image identical to 002 and bounded by the same serializer copy figures (20-34% of a step, expected lower with a parallel gather). Journal entries carry only the old value, so segments are roughly half the size; the estimate is under 1 MB/tick for junkyard-scale churn. Sensor overlap state is O(sensors + overlaps) every tick, even for sleeping or static owners. |
| **Resim cost** | O(scope + boundary) for collide and solve. Broad-phase and sensors stay O(world) (001 §9.3). | N whole-world steps. Fits a frame only at about 1-2k awake bodies; 10k awake takes 100-228 ms. | Same as 002. |
| **Restore cost** | O(scope), but destroys and recreates every scope contact. | O(awake + sensors + journal since T), plus tree fixes. Works in either direction. | Same terms as 002, backward only. |
| **Restore failure modes** | Tick unavailable or scope overflow. | Tick unavailable only. | Tick not restorable only: not imaged (a byte budget widens the capture interval), evicted, or later than the current tick. No slot can go stale, so no staleness check exists. |
| **Approximations** | Non-scope bodies frozen and treated as immovable. Mesh contacts restart cold. Joint settings and static geometry not restored. | None in the engine. The caller must replay every API call. | None in the engine beyond the two exceptions above. The caller must replay every API call and set back whatever host state its callbacks read; callback registrations and `world->userData` are not restored. |
| **Caller-visible artifacts** | Spurious end-touch/begin-touch events, stale contact handles, frozen-neighbour error. | None, but step T's own events are unavailable after rewind. Callbacks must be pure. | Same as 002. Also: rewind is one-way (forward scrub and peek-and-return are explicit non-goals), so the caller records its own poses to look at the past without giving up the present. Recording and history are mutually exclusive. |
| **Engine changes** | Zero-mass override in every joint prepare, scoped iteration lists, pass-local parallel-solver buffers, `validSinceTick` on every body, pending-split slot, scoped `b3Collide`. | Write accessors and journaled containers (a missed direct write fails to compile), three solver order changes (CCD min, CCD sensor hits, explode), a state hash. Plus: a hook in every write path to discard stale slots, two-way heap-block ownership, journal entries for leaving the awake set. | Same accessors, same three order changes, same hash. No new-value half of any entry, no element bytes in a push, one-way block ownership, no discard hook in write paths, no journal entries for leaving the awake set. |
| **Changes existing behavior?** | No, except in resim mode. | Yes. The three order changes slightly alter results outside rollback. | Yes, the same three. |
| **Scrubbing** | Backward only, one restore at a time. | Backward and forward, from any position. | Backward only. Later ticks exist again only by stepping. |
| **Huge scope (5k-body pile)** | Inherent cost; the API exposes scope size so the caller can fall back. | Same inherent cost; the fallback is time-sliced replay. | Same as 002. |
| **Review state** | Nine rounds on §9.2 alone. | 43 rounds; a clean last round is not proof of correctness. | 21 rounds, converged with a fresh full-scope round returning no findings. Convergence of 002 does not carry over, since the journal payloads and ownership rules changed. |
| **Leftover work** | Prototype the joint-type/contact-solver audit and two Low items. | Phase 0 (serializer-backed) through phase 2. Phase 1 fails req. 2 for many sleeping dynamic proxies until phase 2. | Same phases 0-2 and same req. 2 gap in phase 1. Phase 3 (contiguous contact storage, incremental replay) optional. |

## 2. Which fits which scenario

| Scenario | Better fit | Why |
|---|---|---|
| Small or medium world, most bodies sleep, client-side correction only | 002R | Exact replay, simple contract, full-world resim is cheap, and the correction flow uses nothing 002's forward replay adds. |
| Server lag compensation by rewinding the physics world, or a debugging timeline that returns to the present | 002 | Only design that can go back and return without re-stepping. |
| Large always-awake world, one predicted character | 001 | Only option where resim cost tracks the scope. |
| Lockstep or GGPO-style, desync detection | 002R (or 002) | Bit-exactness and a public state hash; rewind-only is enough because the replay re-steps. |
| Corrections must land inside one frame at 10k awake | 001 | 002 and 002R need time-sliced replay or future incremental replay. |
| Long-term maintenance risk | 002R | Same compile-time module boundaries as 002, with less to prove: no redo half, no two-way ownership, no stale-slot hook. |
| Minimum blast radius on existing solver behavior | 001 | Normal stepping is untouched. 002 and 002R change CCD and explode results. |

## 3. Evaluation

**001 strengths**

- The only design that delivers O(predicted) resim, and it is upfront about the broad-phase and
  sensor O(world) gap (§9.3).
- Its API exposes scope size before restore, so a caller can fall back.

**001 weaknesses**

- Carries the most invasive machinery: per-joint mass overrides, a parallel-solver rewrite,
  per-body stamps. Nine review rounds went to §9.2 alone.
- The "consistent" contract has real caller-visible costs (contact events and handles).
- The frozen-boundary approximation can make a predicted body behave differently from the
  server's truth near other awake bodies.

**002 strengths**

- Simple bit-exact contract with one failure mode.
- Reuses the demonstrated `ScrubBackward` property.
- Completeness argued structurally (module boundaries), not by audit.
- Supports forward scrub, and is cheap in large `sleep`-heavy worlds.
- Correctly declines to attempt scoped resim.

**002 weaknesses**

- Migration risk: the accessor and journal refactor touches every writer of every cold
  structure. Joint setters span every joint file.
- Journaling hazards: heavy ownership-transfer rules for blocks, hulls and sensors. 43 review
  rounds, and a clean last round is not proof of correctness (its own status line says so).
- Three solver order changes alter results outside rollback (002 open question 1).
- Code references "have not been checked by execution".
- Resim is a hard N-full-steps floor.

**002R strengths**

- Everything 002 has except forward scrub, with a strictly smaller proof burden: it removes the
  redo half of every entry, the element bytes in pushes, two-way heap-block ownership, the
  journal entries for leaving the awake set, and the hook that discards stale slots (its §11.3).
- The current tick is always the newest retained tick, so there is no position inside the ring
  and nothing to invalidate when the caller writes after a rewind.
- Smaller journal payloads. The image dominates the ring in every scene with meaningful awake
  state, so the saving is in what must be built and proven, not in total bytes (its §11.1).
- Keeps the door open to incremental replay (its §11.2 (4)), and to rebuilding the recording
  player's backward seeks on the ring (its open question 5).

**002R weaknesses**

- Cannot return to the present after a rewind without re-stepping. A caller that needs that
  (lag-compensated hit checks by rewinding the world, a debugging timeline) must record poses as
  ticks happen or use 002. The choice must be made before phase 1, because it decides every
  journal entry's payload and the ownership rules (002R open question 4).
- Inherits 002's migration risk, three order changes, unexecuted code references and N-full-steps
  resim floor.
- Its review series (21 rounds) is shorter than 002's, and the derivation from 002 means the
  removed-forward-scrub changes were reviewed fresh. A converged round is not an endorsement.
- Adds caller-contract obligations of its own: recording and history are mutually exclusive,
  borrowed geometry must stay valid for the whole retained window, and callback function
  pointers and `world->userData` are not restored.

## 4. Can they be combined?

Yes, and 002 already anticipates it (§11.2(4), §15); 002R restates it (§11.2(4), §15). There are
three ways. Where the substrate is written "002 family", 002R is the smaller-scope choice and 002
is the choice if forward scrub is needed.

| Option | What it is | Bit-exact? | Cost | Verdict |
|---|---|---|---|---|
| **A. Two modes, one API** | Ship the 002 family as the substrate. Keep 001's approximate scoped resim as an opt-in mode for worlds that can't sleep. | Ring mode yes, 001 mode no | Both codebases, two contracts | Possible, but carries both maintenance burdens. |
| **B. 002 family substrate + exact incremental replay** | Rewind whole world. Re-step only islands touched by the correction. Copy untouched islands' results from the ring. | Yes | Needs the six solver order changes (§11.2(4)) and an image every tick. 002R's `Rewind` would also have to hand the removed slots' images to the replay instead of dropping them. | Best end state; a real solver project. |
| **C. Ring first, scoping later** | 002R (or 002) phases 0/1/2 now. Add scoped resim as a later phase only if profiling demands it. | Yes for the ring, then depends | Deferred | Pragmatic path. |

### What transfers, and what conflicts

| Piece | From | Fits in a combined design? |
|---|---|---|
| Scope-size query and caller fallback (`GetRollbackInfo`, `maxScopeBodies`) | 001 | Yes. Neither 002 nor 002R has an equivalent, and it is cheap to add. |
| Scope id set and compact per-step iteration lists | 001 | Yes; this is the mechanism option B needs to step only some islands. |
| Frozen boundary via zero-mass override | 001 | Only as an approximation. Option B replaces it with exact promotion of neighbours when a contact reaches them. |
| Capture only the scope | 001 | No. The ring images the whole awake set; capturing less would break exact rewind. |
| Destroy and recreate contacts on restore | 001 | No. Breaks bit-exactness (new ids, spurious events). The ring restores contacts exactly. |
| `validSinceTick`, pending-split slot, side table | 001 | Mostly unnecessary. The ring restores sleeping sets, splits and pools exactly. |
| Whole-world journal and image | 002, 002R | Yes. It is the foundation. |
| Forward-replaying journal (redo half, two-way ownership, stale-slot hook) | 002 only | Only if a caller needs return-to-present. Option B does not need it, since it copies old-timeline images rather than redoing journal entries. |

### What combining buys

- Much of 001's complexity exists because live topology after restore is wrong. With the ring it
  is exact, so scoped replay starts from a correct world.
- The frozen-neighbour approximation goes away: in B, neighbours are re-stepped, not frozen.
- Resim cost drops to O(touched islands), 001's headline requirement, on an exact base.

### What blocks it

- **Solver order changes.** B needs all six so an island's result is independent of global
  order. Each is a separate solver change with its own review.
- **Ring cost.** An image every tick (`captureInterval` 1) is required so untouched islands'
  results can be copied.
- **Old-timeline images.** In 002R the rewind drops the later slots; B needs them kept until the
  replay passes their ticks.
- **Events.** The ring stores none, so replayed events for copied ticks must be kept or
  reproduced.
- **Unmeasured.** None of the designs has been prototyped, and B's neighbour-promotion logic has
  no design yet.
- **Workflow gates.** `WORKFLOW.md`'s acceptance criteria (e.g. AC-1, no shadow structure over a
  population the design doesn't own) apply to any combined design. 001's scope id set and compact
  lists would need to be checked against them before being reused for option B.

## 5. Recommendation

Go with **C**, using **002R** as the ring unless a caller needs return-to-present:

1. Build 002R, starting with phase 0 on the existing serializer, so the correction path can be
   tested end to end before any engine change.
2. Add 001's scope-size query API to it now. It is cheap and useful either way.
3. Treat 001's approximate scoped resim as a fallback only, and revisit option B once
   measurements show worlds that can't sleep.

Why 002R over 002: the correction flow (rewind, apply, re-step) uses none of what forward replay
adds, and forward replay costs the redo half of every entry, two-way block ownership, entries for
leaving the awake set, and a discard hook in every write path. Keep 002 only if the answer to the
first open decision below is yes.

Open decisions before committing:

- Does any caller need to return to the present without re-stepping (server lag compensation by
  rewinding the world, a debugging timeline)? If yes, 002; if no, 002R. This must be settled
  before phase 1 (002R open question 4).
- Does the solver's owner accept the three order changes? Both ring designs need them.
- Should `b3World_ComputeStateHash` be public, and should journaling be always-on or enabled with
  history (002R open questions 2 and 3)?
- Should the recording system's keyframe ring be rebuilt on this ring once phase 1 lands (002R
  open question 5)?

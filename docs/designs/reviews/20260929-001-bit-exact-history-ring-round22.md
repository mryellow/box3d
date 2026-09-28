---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 22), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 22

## Review report (Codex final message)

## Summary

The design's hot image and cold journal approach is broadly consistent with the current engine, but §7.4 does not yet reconstruct every target proxy correctly. I found two restore failures involving proxy category bits and bounds, plus a closest-ray tie case that remains dependent on tree traversal. The static-tree rebuild statement also needs correction.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.4 | **Proxy category bits can be wrong after restore.** `b3Shape_SetFilter(..., false)` changes the shape filter without changing its live proxy. If that shape is later disabled, changes body type, or has its proxy reset, §7.4 recreates the target proxy using the restored filter. At T, the proxy may instead have held the *earlier* category bits. The inventory also omits the old category value when a proxy is destroyed. `src/shape.c`, `src/body.c`, `src/dynamic_tree.c` | Preserve the target proxy's category bits across every lifecycle path, including destruction, and use that value whenever restore recreates a proxy. |
| 2 | High | §7.4, §9 | **Presence can overwrite T's saved bounds.** Rewinding from a disabled body to a tick where it had a proxy calls for proxy creation from T's transform. The live creation helper recomputes both `shape->aabb` and `fatAABBs[id]`; an existing fat AABB can retain an older, larger box while the body moves inside it. The recomputed box therefore need not equal T's saved box. `src/shape.c`, `src/solver.c`, `src/body.c` | Create restored proxies directly from T's restored fat AABB, without recomputing or overwriting either saved bound. |
| 3 | Medium | §7.4 | **The closest-ray shape-id tie break can miss a tied mesh.** After one shape reports fraction *f*, subsequent shape casts receive *f* as their limit. The mesh ray cast accepts a hit only when its fraction is strictly less than that limit. A mesh at exactly *f* can therefore report no hit, so the proposed callback tie break never sees its lower shape id. `src/physics_world.c`, `src/dynamic_tree.c`, `src/mesh.c` | Evaluate closest-ray candidates with the original fraction limit before comparing `(fraction, shape id)`, or make every shape cast reliably report hits at the current limit. Add a tied mesh/convex-shape test with both traversal orders. |
| 4 | Low | §7.4 | The statement that the next step's rebuild puts **the tree** into DFS order is too broad. Automatic step rebuilding covers dynamic and kinematic trees; the static tree is rebuilt explicitly by `b3World_RebuildStaticTree`. `src/broad_phase.c`, `src/physics_world.c` | Limit the statement to dynamic and kinematic trees and state how the restored static tree is handled. |

## Checked, no change

- Proxy creation, body enable, body type change, and reset creation take category bits from `shape->filter`; `SetFilter(..., false)` can leave an existing proxy with different bits. `src/shape.c`, `src/body.c`
- The live `SetFilter(..., true)` path resets the proxy, including when the filter's category bits did not change. `src/shape.c`
- Broad-phase candidate pairs are sorted by shape-pair key before contact creation, and sensor overlaps are sorted and deduplicated. `src/broad_phase.c`, `src/sensor.c`
- The existing backward-scrub test checks the recording hash, whose current coverage is narrower than the proposed full-state hash. `test/test_recording.c`, `src/recording.c`
- Wake transfers do not themselves update shape AABBs. `src/solver_set.c`

## Proposed edits

1. In §5.2 and §7.4, define how the old and new **actual proxy category bits** survive creation, destruction, type changes, resets, and filter changes with `invokeContacts=false`. Apply the target value after Presence and Properties, including when both operations affect one shape.
2. In §7.4 and §9, specify a restore-only proxy insertion using T's saved `fatAABBs[id]`. Preserve T's saved `shape->aabb`.
3. In §7.4, revise the closest-ray algorithm so equal-fraction candidates reach the shape-id comparison even when a shape's own cast uses a strict fraction limit.
4. In §7.4, correct the DFS rebuild sentence for the static tree.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-29T06:01:55+10:00, end 06:07:02+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 6 minutes. Codex ran read-only; the working tree was
untouched by it going in (round 21's edits were already applied to the working file, confirmed via
`git status` before the call). The curated declined-findings list used for rounds 18–21 (nine items
— the ten carried into round 21, minus the world-`userData` item round 21 itself overturned) was
included in the prompt; Codex did not re-raise any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, round 21's own new
Presence/Properties split was re-read once specifically for whether it introduced a fresh gap the
way round 19's fix did for round 20 to find (round 21's own Status section flagged this as the
thing to watch). That re-read caught one genuine issue directly: the Properties bullet's body-type
and proxy-reset clauses still said "with T's journaled category bits" while the Presence bullet
(as round 21 first wrote it) said to use `shape->filter` directly — an internal inconsistency,
fixed before this round's prompt was even written by reverting the Presence bullet back to
`shape->filter`... **which then turned out to be the wrong direction** once Codex's own finding #1
below was verified: the self-check caught the *inconsistency* but picked the wrong side of it to
standardize on. Recorded here in full rather than silently folded into the finding's own
verification, since it's exactly the kind of near-miss worth being honest about.

All four findings verified directly against source:

- **#1 (a live proxy's category bits can diverge from `shape->filter` and persist that divergence
  across a boundary this restore must cross, so a recreated proxy must not simply read
  `shape->filter` at T), CONFIRMED, applied — and it overturns a change this round's own pre-check
  made minutes earlier.** Re-traced every `b3CreateShapeProxy` call site (`shape.c`, `body.c`
  twice) and confirmed, again, that all four pass `shape->filter.categoryBits` directly — this part
  of the pre-check's reasoning was correct. The error was in what that fact implies: it's true
  *only at the instant of a real, live recreation*, because at that instant no divergence can be
  outstanding (a fresh proxy's bits trivially equal the filter that just created it). It does
  **not** imply that "T's `shape->filter`" is the right source when *restoring to* a past tick T,
  because a divergence can have been established *before* T by an earlier `invokeContacts=false`
  call and never resynced by any creation event *before* T — in which case the live proxy that
  actually existed at T (in the original run) had the *stale, pre-divergence* bits, while
  `shape->filter` already reflects the *post-divergence* value at T. Constructed the exact
  timeline: shape created at T0 (bits A); `SetFilter(B, false)` at T1 (`shape->filter` → B, live
  proxy stays at A, journaled `categoryBits` entry — correctly, per its own trigger definition —
  stays at A, since `invokeContacts=false` isn't one of its trigger events); restoring to any T2
  between T1 and a later real recreation must produce bits A, but `shape->filter` restored to its
  T2 value is already B. This is precisely what the journaled `categoryBits` entry is *for*, and
  reverted the Presence bullet and the Properties bullet's two recreate clauses back to "T's
  journaled category bits" uniformly — restoring the pre-round-21-edit behavior for this specific
  value, but keeping every other round-21 structural change (the Presence/Properties split itself,
  which findings #1–#3 below confirm is still the right shape). Made the journal entry's own
  trigger definition explicit in §5.2 (previously just "proxy creation", ambiguous about whether it
  covers `b3Body_Enable`/`b3Body_SetType`'s recreate calls, not just initial creation) to name all
  four call sites, since the whole fix depends on the hook firing at every one of them, not just the
  literal creation-time one. The finding's second half ("the inventory also omits the old category
  value when a proxy is destroyed") needed no separate fix: the journaled entry already persists
  unchanged across a destroy (it's keyed by shape id, not proxy lifetime), ready to be read by
  whichever bullet's recreate clause runs next — the destroy side was never the missing piece.

- **#2 (the Presence bullet's proxy-creation path recomputes fat AABB from the live transform via
  the ordinary creation helper, which can produce a narrower box than T's actual saved fat AABB,
  since awake shapes coast on an existing, wider fat AABB until movement escapes it), CONFIRMED,
  applied.** Traced `b3UpdateShapeAABBs` (`shape.c`, called from `b3CreateShapeProxy`): computes a
  *tight* AABB plus a fixed margin from the current transform alone — no dependence on any prior
  fat AABB. Traced the hot per-step update (`solver.c`) that normally maintains `fatAABBs[id]` for
  an awake shape: it only replaces the fat AABB when the fresh tight AABB is *not* already
  contained within the existing one (`b3AABB_Contains`), i.e. the fat AABB deliberately lags behind
  and absorbs small motion without a tree update. `shape->aabb`/`fatAABBs[id]` are already correctly
  restored to their T value by the time §7.4 runs regardless of awake status at T — for an awake
  shape, via the image copy (§9 step 3, "World scalars, fat AABBs", which already runs before
  §7.4); for a non-awake shape, via the journal walk (§5.2's existing non-awake-owner bounds row,
  with the wake/sleep checkpoints round 6 added) — so the Presence bullet's creation branch never
  needed to *compute* these values at all; it only needed to *use* what an earlier restore step had
  already gotten right. Fixed by passing T's already-restored `shape->aabb`/`fatAABBs[id]` directly
  into the tree-proxy-creation call instead of invoking the path that recomputes them.

- **#3 (a shape type whose own internal closest-candidate search uses a strict `<` against the
  incoming fraction limit — rather than `<=` — silently reports no hit at all when its true nearest
  intersection exactly ties the current best, so it can never reach the proposed callback's
  shape-id tie-break), CONFIRMED as a real, if narrow, residual gap; documented rather than
  algorithmically closed.** Traced `b3RayCastMesh` (`mesh.c`) precisely: `bestOutput.fraction`
  initializes to `input->maxFraction` (the query's current limit) and the per-triangle test is
  `if (alpha < bestOutput.fraction)` — strict. A mesh whose nearest triangle intersection lands
  exactly on the current best fraction from another shape therefore never sets `bestOutput.hit`,
  never reaches `b3RayCastClosestFcn`, and so can never win the proposed tie-break regardless of
  its shape id. Checked the other multi-primitive cast paths for the same shape of bug: capsule,
  sphere, and hull are single-candidate convex primitives with no internal best-of-many loop to
  have this problem; compound and height-field dispatch through their own tree/cell traversal and
  were not fully re-derived against the same hazard in the time available for this round. Declined
  Codex's "make every shape cast reliably report hits at the current limit" as the fix to attempt
  here — that's a source change to `b3RayCastMesh` (and potentially compound/height-field) outside
  what a single round should redesign and re-verify blind. Documented it instead as a third residual
  order dependence, in the same place and register as the two §10 already carries, so the
  caller-contract's honesty about traversal-order edge cases stays complete; a future round or an
  engine-side pass should re-derive whether compound/height-field share the exact same hazard shape
  before either gets its own fix.

- **#4 (the claim that "the next step's rebuild puts the tree back into DFS order" is true for the
  dynamic and kinematic trees but not the static one, which the engine only ever rebuilds via an
  explicit `b3World_RebuildStaticTree` call), CONFIRMED, applied.** Confirmed in `broad_phase.c`
  that the automatic per-step rebuild call names only `b3_dynamicBody` and `b3_kinematicBody`;
  confirmed `b3World_RebuildStaticTree` (`physics_world.c`) is the only other call site touching the
  static tree's layout. Fixed by scoping the claim to the two trees it's actually true for and
  stating explicitly that the static tree keeps whatever layout restore left it in until the caller
  rebuilds it — a documentation-accuracy fix with no cost or correctness implication (the static
  tree remains query-correct either way; DFS order is a cache-locality optimization, not a
  correctness requirement, so this was never a live bug, only a wrong claim about it).

## Status

Round 22 of an ongoing series (rounds 1–21 committed or pending commit). All four findings genuine
and applied — the seventh consecutive full-scope round with zero declines. This round is the most
direct evidence yet of the risk round 20's own Status section named: a round's fix, written from
inside the reasoning that produced it, is worse-positioned to see its own gaps than a fresh round
with no memory of why the text reads the way it does — round 21 touched the tree-proxy-restore
mechanism for the fourth time in four consecutive rounds and *still* left two more genuine defects
(findings #1 and #2) in the same paragraph it had just rewritten, one of which (#1) directly
reversed a choice this round's own pre-prompt self-check had just made minutes earlier for
plausible-sounding but incomplete reasoning — caught only because Codex's independent, from-scratch
pass re-derived the source facts without inheriting that reasoning at all, which is the whole
argument for never narrowing scope or skipping a from-scratch round even deep into a series.
Findings #3 and #4 are in different territory (ray-cast completeness, a documentation-accuracy
claim) than the categoryBits/bounds cluster, suggesting the *proxy-lifecycle* mechanism specifically
— now touched by six of the last seven rounds — may finally be approaching its actual complexity
floor now that Presence/Properties correctly separates "does a proxy exist" from "what are its
properties," while nearby but distinct mechanisms (ray-cast tie-breaking, tree-rebuild scope) still
turn up fresh, unrelated gaps at the same rate. This is the second round of the currently-requested
three-round batch (rounds 21–23); one more full-scope round follows before stopping per that
instruction, unless it lands on `CONVERGED` first. Given #3 was documented rather than fixed, round
23 should re-derive whether compound and height-field share the same strict-`<` hazard mesh does,
and whether the Presence bullet's now-corrected categoryBits/bounds sourcing holds up against a
fresh, independent trace.

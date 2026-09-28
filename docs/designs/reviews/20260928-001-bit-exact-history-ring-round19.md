---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 19), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 19

## Review report (Codex final message)

## Summary

The hot image and cold journal approach accounts for the main awake, sleeping, contact, and sensor state transitions I checked against the source tree. I found one restore gap: enabling or disabling a body changes whether its shapes have tree proxies, but the proposed proxy journal and tree repair rules do not cover that transition.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.4, §9, §12 | `b3Body_Disable` destroys every shape proxy and `b3Body_Enable` creates them again without creating or destroying the shapes (`src/body.c`, `src/shape.c`). The design excludes `proxyKey` from shape records and repairs proxy presence for shape creation and destruction, type changes, and resets, but omits enable and disable. Rewinding from a disabled body to an enabled tick can leave its shapes without proxies; the reverse can leave proxies for a disabled body. | Journal proxy existence changes by shape id. During restore, reconcile whether each affected shape should have a proxy before repairing its bounds, tree, category bits, and moved state. Include proxy presence in the state hash and restore tests. |

## Checked, no change

- The existing recording test checks backward keyframe restore and replay using the current, narrower state hash (`src/recording.c`, `src/recording_replay.c`, `test/test_recording.c`).
- The hot inventory's awake solver arrays, graph arrays, contact records, and mesh cache have corresponding step writes (`src/solver.c`, `src/constraint_graph.c`, `src/physics_world.c`, `src/contact.c`, `src/mesh_contact.c`).
- The cold inventory's dense array swap removals and the associated moved-record index updates occur as described (`src/solver_set.c`, `src/island.c`, `src/sensor.c`, `src/contact.c`).
- The pool uses a bump index and a LIFO free array; pair discovery sorts shape-pair keys (`src/id_pool.c`, `src/broad_phase.c`).
- The proposed CCD and explosion ordering changes address traversal-dependent behavior present in the source (`src/solver.c`, `src/physics_world.c`).

## Proposed edits

1. In §5.2, add body enable and disable to the proxy lifecycle inventory, including the bounds recomputation performed when enabling a body.
2. In §7.1 and §7.4, define a proxy presence entry and restore rule for both directions of that transition. Resolve a restored shape's proxy through its current `proxyKey` only after its required presence has been reconciled.
3. In §12, hash proxy presence and test backward and forward scrubs across enable and disable, including static bodies.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-28T22:09:50+10:00, end 22:13:25+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 4 minutes. Raw log had the final message duplicated (a
`codex exec` streaming artifact); one copy is kept above. Codex ran read-only; the working tree was
untouched by it going in (round 18's edits were already applied to the working file, confirmed via
`git status` before the call). The same curated declined-findings list used for round 18 (ten
items) was included in the prompt, since round 18 added no new declines; Codex did not re-raise or
dispute any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, the pair-set hash
table's add/remove/grow mechanics (`table.c`) were audited against the class of gap round 18 found
in the bitset entry (growth changing an internal capacity field that a naive hash could expose):
confirmed `b3AddKey`/`b3RemoveKey`'s open-addressing table already only exposes *membership*
(`b3ContainsKey`), never raw slot layout or capacity, and §12's hash already only claims to cover
"pair set membership" (not raw table bytes) — no analogous gap, no doc edit needed. This check
surfaced no new finding for Codex to independently re-derive.

The finding verified directly against source:

- **(body enable/disable destroys and recreates every owned shape's tree proxy independently of
  shape lifetime, body-type change, or a `b3ResetProxy` call — a fourth distinct way a shape's
  proxy can come into or go out of existence that §7.4's restore algorithm doesn't reconcile),
  CONFIRMED, applied.** Read `b3Body_Disable`/`b3Body_Enable` (`body.c`) in full: disable calls
  `b3DestroyShapeProxy` for every shape on the body's chain (setting `shape->proxyKey =
  B3_NULL_INDEX`, confirmed in `shape.c`) without touching the shape records themselves, and
  transfers the body to `b3_disabledSet` via `b3TransferBody` — already a structural function per
  §7.1's unconditional-journaling rule. Enable does the reverse: `b3TransferBody` out of
  `b3_disabledSet`, then `b3CreateShapeProxy` for every shape, assigning a fresh `proxyKey`.
  Confirmed this applies to static bodies too (an explicit comment in `b3Body_Disable`: "necessary
  even for static bodies"), so the gap isn't scoped to dynamic/kinematic bodies only. Traced
  whether the existing four §7.4 restore bullets (fat-AABB move, category-bits sync, body-type
  cross-tree recreate, proxy-reset same-tree recreate) cover this: all four presuppose a live
  proxy already exists to move, resync, or recreate *in place* — none creates a proxy from nothing
  for a shape that currently has none, and none destroys a live proxy outright for a shape that
  should have none at T. A direct `Rewind(T)` landing on a tick where a body's disabled/enabled
  state differs from its live state would therefore leave a stale live proxy referencing a
  disabled-at-T body (queryable, wrongly), or leave a shape that should be queryable at T with no
  proxy at all (silently invisible to every broad-phase query) — a real, high-severity restore
  defect, independent of whether the caller ever replays the disable/enable call itself (this is
  about a direct jump-restore, not forward replay, which already works correctly since the live
  `b3Body_Disable`/`Enable` functions handle their own proxies when actually called).

  Found a smaller fix than Codex's proposed "journal proxy existence changes by shape id" (a new
  journaled entry kind): confirmed `b3TransferBody`'s call inside both `b3Body_Disable` and
  `b3Body_Enable` means `body->setIndex` is *already* exactly correct for T by the time §7.4 runs,
  through the ordinary mechanisms already in place — the journal walk (§9 step 2) restores it for
  a non-awake body, and the image copy (§9 step 3, which runs before step 4's tree repair) restores
  it for an awake one, both already unconditionally covering `b3Body`'s full record. No new
  journal entry or hash field is needed to know, during tree restore, whether a given shape's
  owning body is disabled at T — that fact is already reconstructed by the time §7.4 executes.
  Declined the "journal proxy existence... include proxy presence in the state hash" half of
  Codex's proposed action on that basis: it would add a redundant, separately-maintained fact
  (proxy presence) that is already fully implied by an already-hashed, already-restored one (body
  `setIndex`), reintroducing exactly the kind of "two mechanisms tracking the same fact,
  divergeable by a missed update" risk round 14's cold-hash-guard fix was written to eliminate.
  Fixed §7.4 by adding a new restore bullet, ordered first among the tree-repair bullets (since the
  others all assume a live proxy already exists to operate on): for every shape whose owning
  body's already-restored `setIndex` is `b3_disabledSet` at T, destroy any live proxy; for every
  shape whose owning body's `setIndex` is not `b3_disabledSet` at T and has no live proxy, create
  one in the tree matching T's body type with T's journaled category bits.

## Status

Round 19 of an ongoing series (rounds 1–18 committed or pending commit). One finding, genuine,
applied — a fourth distinct way tree-proxy existence/identity can desync from the restore
algorithm's assumptions (after round 2's body-type/`b3ResetProxy` lifecycle fix, round 9's
category-bits divergence fix, and round 14's same-tree proxy-reset fix), each time found by a
different round despite three prior full-scope passes having already hardened this exact
mechanism. Unlike those three, whose fixes each added a new journaled fact, this round's correct
fix needed none — the missing reconciliation was fully derivable from state the design already
journals and restores earlier in the same procedure, a reminder to check "is this already implied
by an earlier restore step" before reaching for a new entry kind, not just when a new one is
obviously unavoidable (as it was for the prior three). The pre-prompt self-check (auditing the
pair-set table against round 18's bitset-growth gap shape) found nothing further wrong. Two
consecutive full-scope rounds (18, 19) have each found genuine, previously-unnoticed, source-
confirmed defects — round 18 in journal-entry payload completeness and hash normalization, round
19 in tree-proxy lifecycle coverage — so convergence is not yet indicated. This is the second round
of the currently-requested three-round batch; one more full-scope round follows before stopping
per that instruction, unless it lands on `CONVERGED` first.

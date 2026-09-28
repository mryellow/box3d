---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 23), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 23

## Review report (Codex final message)

## Summary

The design's separate proxy category value is justified by the current engine, and §7.4 generally uses the right target values for category, fat AABB, and tree. Its restore walk can still miss a live, non-awake shape whose bounds or owning body type changed without a general shape-record entry. One sentence also contradicts the design's correct treatment of filter and proxy category as separate values.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.4 | The Presence and Properties candidate set omits shapes with only a journaled bounds change, and shapes owned by a body whose type changed. For example, `b3Body_SetTransform` can change a static shape's fat AABB; `b3Body_SetType` recreates its proxy in another tree. A shape that is non-awake at T need not have a general shape-record entry, so neither change guarantees that §7.4 examines it. The restored tree can retain P's bounds or tree choice. (`body.c`, `shape.c`) | Include shapes named by bounds entries and every shape owned by a body-type-change entry. Check their live proxies against T's restored bounds, category, and body type. |
| 2 | Low | §7.4 | Properties says T's journaled `categoryBits` is "exactly what any live creation would also have produced at T." A live creation reads `shape->filter.categoryBits`. After `b3Shape_SetFilter(..., false)`, that filter can differ from the existing proxy's category at T — the reason §5.2 journals category separately. (`shape.c`, `broad_phase.c`) | Delete that parenthetical claim. Say a proxy created *by restore* receives T's journaled category, even when T's restored filter differs. |

## Checked, no change

- Proxy creation uses the supplied body type and fat AABB, and reads category bits from `shape->filter`; restore therefore needs its own T-time values. (`shape.c`, `broad_phase.c`)
- `b3ResetProxy` recomputes both AABBs and, when recreating a proxy, takes category bits from the current filter. An `invokeContacts=false` filter change leaves a live proxy in place. (`shape.c`)
- `b3Body_Enable` creates proxies, `b3Body_Disable` destroys them, and a supported `b3Body_SetType` change recreates them in the new body-type tree. (`body.c`)
- Awake movement can retain a fat AABB after its tight AABB changes, so recomputing bounds during restore would lose T's value. (`solver.c`, `body.c`)

## Proposed edits

1. In §7.4 Presence, add journaled shape-bounds entries and owning-body type changes to the candidate sources. In Properties, apply the bounds, category, and tree checks to every surviving live proxy in that expanded set. Add restore tests for a static-body transform and a static-to-kinematic type change, including a prior `invokeContacts=false` filter change.
2. In §7.4 Properties, replace the live-creation comparison with an explicit statement that restore passes T's journaled proxy category.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-29T06:14:53+10:00, end 06:17:23+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 3 minutes. Codex ran read-only; the working tree was
untouched by it going in (round 22's edits were already applied to the working file, confirmed via
`git status` before the call). The curated declined-findings list carried into this round (nine
items, unchanged from round 22 — round 22 declined nothing) was included in the prompt, alongside
an explicit note (not framed as a decline, since these were round 22's own applied fixes, not
rejections) that §7.4's category-bits/bounds/tree sourcing had just been corrected twice in a row
and deserved the same scrutiny as everything else, not a pass on the assumption it was now settled.
Codex did not re-raise any of the nine declined items.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement and round 22's own
Status-section directive, two things were checked directly against source ahead of Codex's pass:
whether `compound`/`height-field` raycasts share mesh's strict-`<` tie-break hazard (§7.4's newly
added residual-order-dependence note from round 22), and whether the Presence/Properties
categoryBits/bounds sourcing held up under one more fresh trace. The first check found a real,
confirmed extension: `b3ShapeCastHeightField` (`height_field.c`) has the identical
`bestFraction = input->maxFraction` / `if (alpha < bestFraction)` pattern as `b3RayCastMesh`,
so height-field shares the same hazard — fixed by naming it alongside mesh in §7.4's note, with
compound shown to inherit the same gap only through whichever primitive type a child happens to
be (its own dispatch and the tree's own pruning, `b3DynamicTree_RayCast`'s `<=` comparison, are not
independently affected). The second check found nothing further wrong with categoryBits/bounds
sourcing itself, but this round's own finding #2 below shows the check wasn't thorough enough — a
leftover sentence from round 22's own edit was still factually wrong, missed by re-reading the
*mechanism* without re-reading every *sentence* describing it word for word.

Both findings verified directly against source:

- **#1 (the Presence/Properties candidate set — awake at T, has a journaled record, or owned by a
  body with a journaled disabled-transfer — does not include a shape whose only journaled change
  is its own `aabb`/`fatAABBs` entry read narrowly as distinct from "a journaled record", or a
  shape owned by a body with a journaled *type* change specifically, as opposed to a
  disabled-transfer; both leave a non-awake shape's live proxy properties or tree unreconciled at
  restore), CONFIRMED, applied.** Re-traced `b3Body_SetType` (`body.c`) precisely for the
  shape-level journaling question: its final stage explicitly destroys and recreates every owned
  shape's proxy (`b3DestroyShapeProxy` then `b3CreateShapeProxy`, in a loop over
  `body->headShapeId`), but this is pure proxy manipulation — nothing in that loop writes any
  shape-record field, so nothing there independently triggers a per-shape "record write" journal
  entry; the only journaled fact from this whole call is the body's own `type` field change (via
  the generic body-record journaling §7.1 already covers as a structural write). Confirmed the
  same reasoning holds for a static body's `aabb`/`fatAABBs` change via `b3Body_SetTransform`: this
  writes the dedicated `aabb`/`fatAABBs` journal entry (§5.2's non-awake-owner bounds row), a
  distinct entry *kind* from the generic shape "record write" my Presence bullet's prior wording
  named — a reasonable but too-narrow reading of "journaled record" could exclude it, exactly the
  ambiguity the finding points at. Fixed by extracting a shared **Candidate set** bullet ahead of
  Presence and Properties, naming every entry kind keyed to a shape's own id explicitly (general
  record, `aabb`/`fatAABBs`, proxy `categoryBits`, proxy-reset) instead of the ambiguous "journaled
  record," and adding "or a journaled body-type change" alongside the existing disabled-transfer
  clause for the owning-body case — the exact parallel structure that clause already established,
  extended to the one other body-level event (`b3Body_SetType`) that manipulates owned shapes'
  proxies without any shape-level journal trace of its own.

- **#2 (Properties' parenthetical claim that T's journaled `categoryBits` "is exactly what any live
  creation would also have produced at T" directly contradicts the design's own, correct point —
  established by this very series two rounds ago — that a live creation reads `shape->filter`
  directly, which is precisely *not* always equal to the journaled value), CONFIRMED, applied.**
  This was a mistake in round 22's own wording, introduced while fixing round 22's finding #1 and
  missed by this round's own pre-check (which re-verified the *mechanism* the parenthetical was
  attached to, not the literal sentence). No source re-verification was needed beyond what round
  22 already established — `shape->filter.categoryBits` and T's journaled proxy `categoryBits` are
  the same value only in the absence of an active `invokeContacts=false` divergence at T, which is
  exactly the case the journaled entry exists to handle differently. Fixed by deleting the false
  parenthetical and replacing it with the accurate converse: a proxy Presence creates gets T's
  journaled category *because* that can differ from what a live creation reading the current filter
  would produce, not *despite* never differing.

## Status

Round 23 of an ongoing series (rounds 1–22 committed or pending commit). Both findings genuine and
applied — the eighth consecutive full-scope round with zero declines, and the third round in a row
to find and fix a genuine defect in the §7.4 tree-proxy-restore mechanism specifically, each time
in a *different* dimension (round 21: proxy existence/lifecycle interaction; round 22: which
source a recreated proxy's category and bounds come from; round 23: which shapes the restore walk
even examines in the first place, plus a leftover factual error in round 22's own prose). This is
the closest the series has come to a clean pattern: every one of the last three rounds' fixes was
smaller and more narrowly scoped than the one before it, and this round's two findings are lower
average severity (one High narrowing an already-mostly-correct candidate set, one Low a pure prose
error with no behavioral consequence) than rounds 21 and 22's predominantly High-severity findings
in the same section — consistent with, though not yet proof of, genuine convergence on this
specific mechanism. This is the third and final round of the currently-requested three-round batch
(rounds 21–23); stopping here per that instruction. The series has not reached `CONVERGED` — a
future round should continue at the same full scope, with §7.4 the first section to re-check given
its recent history, but the last three rounds' narrowing severity and shrinking fix size is a
reasonable signal that this particular mechanism may be close to settled, to be confirmed or
refuted by whichever round runs next.

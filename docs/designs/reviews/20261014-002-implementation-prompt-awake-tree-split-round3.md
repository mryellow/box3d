---
title: Review round 3 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round2.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 3
---

# Round 3

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T08:44:19+10:00 to 08:48:42+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13770-2c33-7262-896d-0f8262002283`.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the binding design, and its tree, pair-update, snapshot, CCD, and benchmark descriptions match the live source. Four corrections are needed before it can be handed to an implementing agent: one false source claim and three test assertions or coverage gaps.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §2, Sleep, wake and the step | The prompt says `b3Body_Enable` sets `world->locked` itself. It does not; `src/body.c` checks for an unlocked world but never sets the lock in that function. | Correct the lock inventory. |
| 2 | M | §4, B2 with B3 | `AwakeTreeMigrationTest` requires `b3World_GetAwakeBodyCount` to equal the “body count,” although its scene includes a static ground body. The API counts only bodies in the awake solver set (`src/physics_world.c`), so a literal all-body count fails after a correct wake. | Specify the expected count as the pile’s dynamic bodies plus the kinematic body. |
| 3 | M | §4, B2 with B3 | `WakeFromCallbackTest` checks migration timing and placement but has no serial-run state-hash comparison. That comparison is explicitly required for the callback case by design §3; the worker-count comparison in `WakeUnderRebuildTest` covers the begin-touch case instead. | Add a matching serial wake run and compare state hashes after the query and subsequent step. |
| 4 | M | §4, B4; §6 | `SleepingTreeHeightTest` checks the moved bit only on the body whose write raised the rebuild count, then checks a contact for that same last-moved body. A rebuild that preserves that bit but clears earlier moved leaves could pass, despite design §2 B4 requiring every pending moved bit to survive. | After each rebuild, check all previously moved leaves; make at least one earlier moved leaf responsible for the post-rebuild contact check. |

### 3. Checked, no change

- Proxy-key packing, tree capacities, creation guards, and snapshot tree loops match the claims in §2.
- The pair-update order supports the stated B1 fail-before assertion, and contact-set selection supports the three B1 contact expectations.
- The solver join point, CCD candidate handling, and Explode’s collect-sort-apply path support the specified migration timing and ordering.
- The determinism hash inputs and benchmark runner behavior support the golden-hash policy and the use of a separate profile harness.

### 4. Proposed edits

Apply the four actions in the findings table. Clarify “body count” in the B2 test text and §6 mapping so the assertion has one unambiguous expected value.

### 5. Unresolved

The binding design permits callback-time body destruction and type changes while a query holds tree views. `WakeFromCallbackTest` cases 2 and 3 use a single sleeping leaf, so they do not settle safety for a larger tree; that is a design-level question, not a finding against this prompt.

### 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the design (U-V2).

1. **Adopted (L).** `b3Body_Enable` (`src/body.c`) takes the world through `b3GetUnlockedWorld`
   and never writes `world->locked`; the other four do. Fix: the §2 bullet says all but
   `b3Body_Enable`.
2. **Adopted (M).** `b3World_GetAwakeBodyCount` (`src/physics_world.c`) reads the awake set, and
   the scene's static ground is in the static set. Fix: the expected count is the pile plus the
   kinematic body.
3. **Adopted (M).** Design §3's third bullet asks the same state hash as a serial run of both
   woken bodies. Fix: the callback case runs a second world with the wake made between calls and
   compares `HashWorldState` after the wake and after one step.
4. **Adopted (M).** The leaf-bit check covered only the body whose write raised the rebuild
   count, and the contact check the same body. Fix: after each rebuild every body moved so far
   keeps its bit; the static box overlaps the first-moved body and the contact asserted is that one.

Unresolved item (callback-time destroy and type change under a held tree view): a question about
the design's B3, which states that `b3Body_SetTransform` from a callback is unguarded today and
unchanged; recorded here, not acted on (U-P5, U-P11).

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 3: 4 findings (H0-M3-L1), 4 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: findings 2, 3 and 4 are in
  §4's tests, the third round running (U-S4 watch); finding 4 is a consequence of round 2's fix 2
  (the leaf-bit check it added was too narrow); each fix is one clause or sentence, no narrative.
  Reviewer read of round records or changelog: a `git status` listing names the files; no read of
  their content in the raw log. Doc length: 705 -> 710 lines; the fixes add one line (1), one (2),
  three (3), none net (4). Above the split-check length (U-T6): still one
  decision; no split.

---

title: Codex review — bit-exact history ring, round 23
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 23

## Review report (Codex final message)

## Summary

The hot-image and cold-journal approach is a sound direction for whole-world replay, and the document identifies several state and tree-order dependencies that the current recording hash does not cover. I found two gaps that should be resolved before treating the bit-exact and memory-budget claims as settled. This was a read-only source review; I did not run the proposed implementation or tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.1–5.2, 7.3, 9 | The scalar inventory does not explicitly cover `compoundShapeCount` or its structural writers. shape.c changes it on compound creation and destruction; broad_phase.c uses it to decide whether to check compound pairs. The stated `b3World_WriteScalars` migration covers scalar *setters*, but these writes occur in shape lifecycle code. A stale count after rewind can change contact creation. | Define the scalar struct field by field, route compound lifecycle writes through its accessor, and test rewind across creation and destruction of the last compound. |
| 2 | Medium | §§7.4, 12 | The tree-order tests cover CCD and explosion, but do not exercise compound pair discovery after a layout-changing restore. broad_phase.c queries a compound’s child tree and emits child pair keys; the document treats that tree as borrowed geometry while relying on sorted pair processing for order independence. The test plan does not establish that claim for compounds. | Add a compound scene with multiple eligible children and compare pair membership, contact IDs, events, and the full hash after restore and replay. |

## Checked, no change

- The current recording hash covers transforms and velocities, as the design says; recording.c does not provide the proposed full-state oracle.
- The `ScrubBackward` test compares hashes after seeking, but its hash has that limited coverage (test_recording.c).
- Sensor overlap processing swaps buffers, rebuilds `overlaps2`, then sorts and deduplicates it; imaging the current overlap list is consistent with the next-step use (sensor.c).
- Tree proxy IDs come from a per-tree free list, and growth appends IDs in ascending order (dynamic_tree.c).
- The caller contract correctly treats query callback order as unspecified, consistent with simulation.md.

## Proposed edits

Specify the complete world-scalar inventory and every writer in §§5–7, including structural writers. Add the compound lifecycle and pair-discovery cases to §12’s restore, replay, and tree-order tests.

## Unresolved / disagreements

The document may intend “world scalars” to include `compoundShapeCount`. Even with that interpretation, its write path and verification case need to be stated to support the construction guarantee.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:54:17+10:00, ended 13:58:26+10:00, exit code 0 — no resume needed. Codex ran
read-only; the `git status --short` file list before and after was identical, so the working tree
was untouched by Codex. Same fresh full-scope prompt as rounds 18 to 22, no declined-findings list.
The final message above has its source links reduced to plain file names, per the citation rules
(the line numbers in Codex's links are dropped); nothing else is changed.

**Findings verified against source.**

1. **Applied.** `compoundShapeCount` (`physics_world.h`) is written in `b3CreateShapeInternal` and
   `b3DestroyShapeInternal` (`shape.c`) and in `b3DestroyBody` (`body.c`), and gates
   `checkCompounds` in the pair update (`broad_phase.c`). The scalar struct is imaged whole, so the
   count restores; the gap was that §5.1 did not list it and §5.2's accessor rule named only
   setters, leaving these structural writers outside the choke point. §5.1 now lists it, §5.2 routes
   every scalar write in structural code through `b3World_WriteScalars`, and §12 test 2's churn adds
   creating and destroying the world's only compound shape. Codex's "define the scalar struct field
   by field" was not taken: the struct is imaged whole, so a field list adds no restore behaviour.
2. **Declined; doc sharpened.** `b3EmitCompoundPairs` (`broad_phase.c`) pushes each child's key onto
   the same `pairKeys` list the sorted pair pass consumes, and queries the compound's own child tree,
   which restore never touches (borrowed geometry, §10). Nothing in a compound's pair path depends on
   the world trees' layout beyond what the sorted list already removes, so a dedicated test would
   repeat the existing pair-order coverage. §7.4's layout paragraph now says so at the point of use.

1 applied, 1 declined.

## Status

Finding count fell to 2 (1H-1M-0L) from 3. The High is an inventory gap (a scalar's writers) in text
that predates round 22, not a consequence of round 22's fixes; the Medium is a test request for
ground the doc's sorted-pair rationale already covers, and it did not engage that rationale, which
is now stated where the compound case would look for it. Categories are still new rather than edge
cases only, so the trend is improving but not converged (last five: 4, 2, 4, 3, 2).

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 23: 2 findings (1H-1M-0L), 1 applied, 1 declined`

`Series total: 120 findings (44H-65M-11L) across 23 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L)`

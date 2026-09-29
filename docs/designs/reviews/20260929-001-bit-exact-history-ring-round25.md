---

title: Codex review — bit-exact history ring, round 25
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 25

## Review report (Codex final message)

## Summary

The whole-world rewind recommendation is broadly consistent with the source, and the document correctly treats the existing recording hash as limited evidence. Two gaps need edits before the design converges. This was a read-only review; no files were modified or tests run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2, §7.3 | The claimed compile-time journaling boundary does not yet define a complete write path for non-awake `bodySims` and `bodyStates`. Setters such as `b3Body_SetTransform` and `b3Body_SetMassData` write those solver-set rows directly; a `b3Body_Write` accessor taking a completed body record does not, by itself, intercept them. The same boundary needs to cover non-awake `jointSims`. | Specify typed sim/state write accessors or a completed multi-record update, and require the migration to make every non-awake row write compile through that boundary. |
| 2 | Medium | §7.1, §9–10 | Forward scrub can retain a debug handle for the wrong geometry. A shape can survive with the same id and generation while `b3Shape_SetSphere`, `SetHull`, or similar setters replace its geometry and release `userShape`. Journal redo copies the new record but deliberately preserves the live handle; §10 releases handles only when a shape is removed. If the caller drew the old shape after rewinding, scrubbing forward leaves that old-geometry handle attached to the new shape. | On undo or redo of a geometry-changing shape write, release and clear a live `userShape`. Add a backward-draw-forward scrub test. |

## Checked, no change

- `ScrubBackward` does compare hashes after backward seeks; the design accurately limits what that hash proves.
- The snapshot reader clears body, shape, and joint `userData`, as the phase-0 caveat states.
- CCD uses a running fraction and caps sensor hits at eight; the proposed order changes address real tree-order dependencies.
- Sensor overlap processing sorts and deduplicates results, supporting the stated layout-independence argument for that pass.
- Tree proxy ids use a separate free list, and the broad-phase create/destroy functions provide the stated journaling choke point.

## Proposed edits

Define the non-awake sim/state accessor interfaces and their compile-time enforcement in §5.2–7.3. Extend the host-handle rule in §7.1 and §10 to geometry-changing record replay, then add the forward-scrub test to §12.

## Unresolved / disagreements

The document’s open questions about accepting the simulation-order changes and measuring capture cost remain decisions for implementation. I found no source evidence that resolves them.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:06:14+10:00, ended 14:11:03+10:00, exit code 0 — no resume needed. Codex ran
read-only; the `git status --short` file list before and after was identical, so the working tree
was untouched by Codex. Same fresh full-scope prompt as rounds 18 to 24, no declined-findings list.
The final message above has its trailing "See ..." source links removed (they carried file paths
only, no content); nothing else is changed.

**Findings verified against source.**

1. **Declined; doc sharpened.** §5.2 already says a structure's accessor covers its sim data, that
   `b3Body_Write` covers a non-awake body's `bodySims`/`bodyStates` and `b3Joint_Write` covers
   non-awake `jointSims`, and that non-awake set arrays expose const access only outside their
   defining file, so a direct write to those rows does not compile. The finding does not engage that
   text. The accessor's description said only that it takes a "completed new value"; it now says the
   value is the record together with its sim and state for a body or joint.
2. **Applied.** A shape-change setter releases `userShape` (`shape.c`) and the debug draw
   (`physics_world.c`) recreates it when null, so a live handle built after T can be for geometry
   the restored record no longer has. The old rule (keep the live handle for a shape alive at both
   ends) missed that case in undo as well as redo, and in the image scatter for an awake shape whose
   geometry changed without a journal entry. Every restore-time write of a shape record now releases
   and clears the handle unconditionally, with no comparison of geometry, since the draw recreates
   it on demand. §9 step 3 and §10 say so, and §12 test 2's churn adds a draw-before-and-after case.

1 applied, 1 declined.

## Status

Finding count fell to 2 (0H-2M-0L) from 3, with no High for the first time since round 21. Finding 2
is a gap in a rule added when the host-handle contract was written, not a consequence of round 24's
fixes; finding 1 reopened ground §5.2's stated rationale already covered without engaging it, and
the wording is sharpened. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2) are flat to slightly
down, and categories are still new, so the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 25: 2 findings (0H-2M-0L), 1 applied, 1 declined`

`Series total: 125 findings (45H-68M-12L) across 25 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L)`

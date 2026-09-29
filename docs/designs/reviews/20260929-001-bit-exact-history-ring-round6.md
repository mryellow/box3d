---

title: Codex review — bit-exact history ring, round 6
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 6

## Review report (Codex final message)

## Summary

The whole-world approach is coherent, and the document correctly limits what the existing recording test proves. Four changes are needed before the proposed journal and caller contract are complete.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.2, 7.1, 9 | The slot-pool rule accounts for one sparse array per ID pool. Shape allocation grows **both** `shapes` and `fatAABBs` (`src/shape.c`), but bump-allocation undo specifies truncating only the record array. Rewinding a newly allocated shape can leave the two arrays at different lengths. | Make the shape slot-pool operation grow and undo both arrays atomically; add a create–rewind–recreate case. |
| 2 | High | §§2.7, 5.2, 7.3 | The claimed compiler-enforced write boundary is not specified sufficiently. World arrays and writable records are exposed through shared internal headers (`src/physics_world.h`, `src/shape.h`, `src/contact.h`), and joint setters span translation units. Adding write accessors alone leaves direct writes compilable. | Specify the storage and header changes that make direct mutation inaccessible, including joint sims and owned pointees, before claiming completeness by construction. |
| 3 | Medium | §§5.2, 7.1, 9 | Material-block entries describe copying old and new bytes, but do not define ownership across shape creation and destruction. `src/shape.c` allocates material arrays during creation and frees them during destruction; record restore deliberately excludes their pointers. | Define null-to-block and block-to-null entries, transfer ownership on destroy and undo, and cover inline-to-allocated material cases in restore tests. |
| 4 | Medium | §§7.4, 10 | Rebuilt tree layout can change public query callback order. `docs/faq.md` says overlap and cast callback order is arbitrary, while §10 says replay every API call. A caller that runs the same query and uses callback order to issue simulation API calls can therefore supply a different ordered input stream. | State that callers must replay the resulting ordered API effects, or make relevant query results canonical before callbacks. |

## Checked, no change

- `test/test_recording.c` checks backward seeks against `b3HashWorldState`; `src/recording.c` hashes body transforms and velocities. The document correctly limits its existing proof to those dimensions.
- `src/sensor.c` rechecks sensors each step and sorts and deduplicates overlaps. Imaging `overlaps2` matches the state the next sensor step consumes.
- `src/solver.c` confirms both CCD dependencies identified in §7.4: the running solid fraction and the traversal-limited sensor-hit collection.
- `src/physics_world.c` confirms explosion impulses currently follow tree-query order.
- The six world ID pools, the separate tree proxy free lists, and the cross-worker determinism tests match the document’s inventory (`src/id_pool.c`, `src/dynamic_tree.c`, `test/test_determinism.c`).
- `CMakeLists.txt` supplies the stated floating-point contraction flag for GCC and Clang.

## Proposed edits

Update the shape slot-pool transaction, define material-block lifecycle entries, and give the write-boundary migration a concrete header and storage design. Add the query-order condition to the caller contract and tests for shape allocation undo, material-bearing shape destruction, and query-driven replay.

## Unresolved / disagreements

The document’s explicit performance figures and source-revision context were accepted as supplied. I did not assess the proposed CCD and explosion result changes by execution.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T11:42:12+10:00, ended 11:45:55+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` after the call showed only the target doc (edited by Claude before the
round) and `WORKFLOW.md` modified, so the working tree was untouched by Codex. The prompt was a
fresh full-scope brief: target doc and source only, no access to `docs/designs/reviews/`, file-only
citations stated as by design, and a curated context note for the one round-5 decline that was the
reviewer's misreading (the doc's revision `5643cd8` is the newest commit touching `src`; the
section 11.1 figures come from a different design's perf pass). Before the round, `git diff` of the
target was empty. The re-read of §§5.2, 7.1 and 9 found and fixed a sibling gap directly: round 5's
pointer-field rule covered the image copy and not the journal's record-write undo and redo, and
`b3Island` (`island.h`: `bodies`, `contacts`, `joints` arrays) was not in the pointer-bearing list
at all. The fix, made before the prompt, extends the rule to record-write undo and redo and to
island arrays, and states that entries free only the blocks they hold at that moment. The
round-4 additions were rechecked against AC-1 through AC-4: no shadow structure over a population
the design does not own, no caller list standing in for a choke point, no second structure being
reconciled with a first, and no check that changes control flow.

**Findings verified against source.**

1. **Applied.** `shape.c` pushes both `world->shapes` and `world->fatAABBs` under one
   `b3AllocId( &world->shapeIdPool )` and asserts equal counts; every other pool grows a single
   array (checked `body.c`, `contact.c`, `joint.c`, `island.c`, `solver_set.c`). §5.2's slot pool
   and §7.1's bump-alloc undo now truncate every array the pool grows, and name `fatAABBs`.
2. **Applied.** The finding is right that "the compiler rejects a direct write" was asserted
   without saying what makes it true: `b3World`'s arrays are plain fields visible to every
   translation unit that includes `physics_world.h`. §5.2 now specifies const-qualified element
   access outside the defining translation unit, with the writable pointer produced only by the
   write accessor's file and the hot accessor. This does not reopen the doc's stated position; it
   supplies the mechanism that position needs. The joint-setter spread was already named in §5.2 as
   a finding against the surrounding code to fix there, and stays.
3. **Applied.** `shape.c` allocates `materials` in `b3CreateShapeInternal` (compound, per-triangle
   and single-material paths) and frees it in `b3DestroyShapeAllocations`. §7.1's material block
   entry only described copying old and new bytes when the block already existed; it now has an
   absent old or new side for create and destroy, and the destroy entry owns the block like a
   destroyed contact's manifold. The shape's material block joined §7.1's create/destroy ownership
   sentence. This is a consequence of round 5's fix (finding 1 there made the entry unconditional).
4. **Applied.** `docs/simulation.md` (not `faq.md`, as Codex cited) and `box3d.h` both say query
   callbacks may receive shapes in any order, and §7.4's rebuild can change that order after a
   restore. The doc's §10 already makes API calls the replayed inputs; it now says so for the
   query-driven case: replay the recorded calls, not the queries.

4 applied, 0 declined.

## Status

Finding count fell to 4 (2H-2M-0L) from 6, the smallest so far, and the mean severity dropped:
both Highs are gaps in mechanisms the doc already has (a lockstep-array pool and an unspecified
enforcement boundary), not new architectural surface. Finding 3 is a consequence of round 5's own
fix, and finding 1 is a gap in the earlier AC-2 slot-pool rewrite. Finding 4 is a new category, the
caller contract for query order. No finding reopened a stated rationale, so none needed the
reversal-check. The remaining risk is still in the journal and restore mechanics, and the
pre-round re-read found a further gap of the same shape as round 5's finding 1, which suggests
sibling-case gaps in that area have not run out.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 6: 4 findings (2H-2M-0L), 4 applied, 0 declined`

`Series total: 50 findings (28H-21M-1L) across 6 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L)`

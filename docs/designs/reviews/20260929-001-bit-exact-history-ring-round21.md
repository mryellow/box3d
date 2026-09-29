---

title: Codex review — bit-exact history ring, round 21
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 21

## Review report (Codex final message)

## Summary

The hot image and cold journal approach fits the source tree’s awake and sleeping state split. I found two correctness gaps in the proposed journal and hash, plus an underspecified tree restore step. I read the specified design and allowed source areas only; I did not read other design files or anything under `docs/designs/reviews/`.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §7.3 | A shape’s pool allocation is journaled before its heap material block is created. In `src/shape.c`, `b3CreateShapeInternal` allocates the shape id first and allocates `materials` later. The proposed pool entry therefore cannot carry that block’s ownership handle at the point where the journaled container writes the entry. This conflicts with the claim that creation needs no separate material block entry. | Specify a completed shape creation operation that records ownership after initialization, or journal material block creation separately. Cover creation failure and undo/redo in the ownership rules. |
| 2 | High | §12.1 | The proposed compound geometry hash omits geometry embedded in the compound blob. `src/compound.c` stores hull and mesh data behind offsets in `b3CompoundData`; hashing only the instance arrays and transforms can give the same state hash for different collision geometry. That weakens the proposed full state oracle. | Hash each referenced embedded hull and mesh’s content, including mesh material index arrays, while excluding only derived tree layout and pointer values. |
| 3 | Medium | §7.4 | Restore says to set moved bits from the imaged proxy list, but does not specify marking each proxy’s ancestors. `b3DynamicTree_ClearMoved` in `src/dynamic_tree.c` stops immediately when the root is unmarked, and later tree work also relies on moved internal nodes. Setting leaf bits alone would lose pending moves. | Define a tree operation that marks the restored leaf and its ancestor path, then test a restored moved leaf through the next pair update and rebuild. |
| 4 | Low | §2.1, §4 | “Every simulation-affecting byte equals” overstates the stated restore model: §7.4 deliberately reconstructs a potentially different tree layout. The intended guarantee is equality of the state needed to reproduce results under the callback contract. | State the guarantee in terms of simulation state and replay results; keep the explicit `preSolve` order exception beside it. |

## Checked, no change

- `ScrubBackward` in `test/test_recording.c` compares hashes after backward seeks; `b3HashWorldState` in `src/recording.c` covers transforms and velocities, so the design correctly limits what that test demonstrates.
- `docs/faq.md` says rollback determinism is unavailable, while `docs/simulation.md` documents worker and cross-platform determinism.
- `src/sensor.c` reevaluates sensors each step and sorts and deduplicates overlaps. The design correctly budgets overlap count as well as sensor count.
- `src/solver.c` uses a running CCD fraction and caps continuous sensor hits at eight; `src/physics_world.c` applies explosion impulses during tree traversal. The proposed order changes address real dependencies.
- `src/id_pool.c` uses a free array followed by a bump index, as described.

## Proposed edits

Resolve findings 1–3 in the creation, hash, and tree restore algorithms, and add focused verification cases for each. Tighten the byte equality wording in §2.

## Unresolved / disagreements

I could not verify the quoted performance baseline because its cited review file was outside the permitted reading scope. The proposed capture targets remain measurements to establish during implementation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:42:27+10:00, ended 13:45:46+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 20, no declined-findings list. The final
message above has its source links reduced to plain file names, per the citation rules; nothing else
is changed.

**Findings verified against source.**

1. **Applied.** `b3CreateShapeInternal` (`shape.c`) calls `b3AllocId` on the shape pool first and
   allocates `shape->materials` afterwards, so a pool alloc entry written at allocation time could not
   hold the block's handle. The entry table now says the creating function builds the block first and
   hands it to the pool's alloc, which takes completed values like every other journaled mutator. The
   contact sibling has no such problem: `b3CreateContact` (`contact.c`) allocates the id first and
   leaves `manifolds` null, so its alloc entry carries no block and the block is bound by the later
   block entries.
2. **Applied.** In `b3CompoundData` (`compound.c`) the hull and mesh instance arrays hold offsets to
   hull and mesh data embedded in the blob, so hashing the instance arrays and transforms leaves the
   embedded collision geometry out. §12.1 now adds each embedded hull's and mesh's content hash and
   each mesh instance's material index array.
3. **Applied.** `b3DynamicTree_ClearMoved` returns at once when the root is unmarked, and the rebuild
   and pair update descend by internal nodes' moved bits, so setting leaf bits alone loses pending
   moves. `b3DynamicTree_MarkProxyMovedSerial` (`dynamic_tree.c`) marks a leaf and its ancestor path;
   §7.4's restore pass now uses it, and §12 test 2's moved-while-asleep case now requires the next
   pair update to find the proxy's pairs.
4. **Declined; doc sharpened.** §7.4 already states layout is a performance property once the order
   changes land, so tree layout bytes are not simulation-affecting, and requirement 1 already names
   the one exception. The finding restates the alternative without engaging that. Requirement 1 now
   says in place that layout is not simulation-affecting.

3 applied, 1 declined.

## Status

Finding count rose to 4 (2H-1M-1L) from 2, with two High. Both High findings are source mismatches
(an allocation order, an embedded-geometry hash) in text written before round 21; finding 3 is an
underspecified step of the §7.4 restore pass. None was a consequence of round 20's fixes (its edits
were to §12 test wording and the hull-map order, not to these passages), though finding 2 sits in the
hash text round 18 added. Finding 4 reopened stated ground without engaging its rationale, and the
rationale is now stated at the point of use. The series is not converged; the last four counts
(3, 4, 2, 4) show no clear downward trend.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 21: 4 findings (2H-1M-1L), 3 applied, 1 declined`

`Series total: 115 findings (43H-62M-10L) across 21 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L)`

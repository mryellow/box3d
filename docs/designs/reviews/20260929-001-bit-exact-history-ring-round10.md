---

title: Codex review — bit-exact history ring, round 10
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 10

## Review report (Codex final message)

## Summary

The whole-world rewind approach is plausible, and the document correctly treats the existing recording hash as limited evidence. Three parts of the proposed implementation need tightening before the bit-exact claim is supported. This was a read-only review; no files were modified.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.3 | The proposed hot accessor asserts that the world is inside a step, but public calls mutate awake state between steps. `b3Body_SetAwake` can move sims at a step boundary, and `b3World_Explode` wakes bodies and changes awake velocity state there. A step-only accessor cannot serve these paths as specified. | Define an accessor path for between-step awake mutations and specify how pending mutations are undone before an imaged tick is restored. Migrate and test those callers. |
| 2 | Medium | §7.4, §12 | The tree proxy pool is a preallocated linked free list, not the bump-index pool described for the six world id pools. Retaining capacity grown on a later timeline can leave additional free ids after rewind. That may preserve future allocation order, but it does not reproduce the proxy-pool state that §12 says the hash compares. | Specify which proxy-pool fields are semantic. Either restore the exact free list and capacity state or hash a canonical logical allocation state, then test rewind across pool growth. |
| 3 | Medium | §7.1 | The pool alloc/free entry payload lists ids and allocation history, while the material and manifold rows assign ownership of blocks created or destroyed with their owners to those same entries. The entry format and undo/redo operations do not say how those handles are carried. | Add the ownership handles, transfer operations, and eviction behavior to the pool-entry specification for shapes and contacts. |
| 4 | Medium | §5.1, §6, §7.4 | Capture says it gathers moved proxy ids while visiting awake shapes, although API proxy resets can mark static shapes moved. Conversely, the step path clears moved flags before an end-of-step capture. The stated cross-step moved-list role and capture recipe do not agree with those source paths. | State whether an end-of-step image needs a moved list at all. If another capture boundary needs one, collect moved ids from every relevant tree and cover static proxy resets in tests. |

## Checked, no change

- `ScrubBackward` (`test/test_recording.c`) does compare hashes after backward seeks; `b3HashWorldState` (`src/recording.c`) covers transforms and velocities, not the full proposed state.
- The six world id pools use LIFO reuse; the tree proxy pool has separate allocation logic.
- The serializer records world scalars, pools, sets, shapes, contacts, sensors, islands, trees, and graph colors. It clears host `userData`, as phase 0 states.
- The source supports the identified CCD sensor-cap and explosion traversal-order concerns. The proposed order changes address real dependencies.
- The FAQ’s rollback statement and the 64-bit cross-platform determinism description match the cited documentation and build flags.
- The §11.1 figures were treated as supplied context; they were not re-measured.

## Proposed edits

Resolve findings 1–4 in the accessor, journal-entry, tree-state, and capture rules. Add focused verification for between-step awake mutations, proxy-pool growth followed by rewind, block ownership across undo/redo and eviction, and static proxy resets.

## Unresolved / disagreements

The document’s open decision on whether to retain forward scrub remains consequential: removing redo would simplify entry payloads, but would change the stated scrubbing requirement. I found no source evidence that resolves that product choice.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:11:40+10:00, ended 12:15:47+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` after the call showed the same set as before it (the target doc and
`WORKFLOW.md` modified, the untracked round-6 to round-9 files), so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 6 to 9, target doc and source only, no access to
`docs/designs/reviews/`, file-only citations stated as by design, and the context note that the
doc's revision is current with `src` and that the §11.1 figures come from another design's perf
pass. No declined-findings list (round 9 declined nothing). Before the round the whole `git diff` of
the target was read and the round-6 to round-9 edits were intact, and the doc was rechecked against
AC-1 through AC-4 with no failure. The Codex final message above has its two line-numbered source
links reduced to plain paths, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Applied as a clarification; the premise was partly wrong.** `b3Body_SetLinearVelocity` and the
   explosion callback (`body.c`, `physics_world.c`) write an awake body's `b3BodyState` between
   steps, and `b3Body_SetAwake` goes through the wake and sleep functions. The doc never routed
   these through the hot accessor: wake and sleep are the journaled structural functions, and the
   explosion callback and setters are already in §5.2's `bodies[id]` caller row. The gap was that
   the doc said which accessor the step uses and not which one an API write to an awake owner uses.
   §5.2 now says the write accessor serves those writes in place, and §7.1's record-write rule says
   an API write to an awake owner between steps is not journaled because any earlier tick's image
   overwrites it. Restore of a body that was asleep at T and woken after is covered by the wake's
   own journaled entries. Codex's High is overstated for what the doc claimed.
2. **Applied to the hash, not to restore; §7.4's rationale stands.** `dynamic_tree.c` grows the
   proxy array by half its capacity and chains the new ids onto the free list, so after a rewind
   across a growth the free list is longer than at T while handing out the same id sequence. §7.4
   already says capacity is not state (round 4's decline). This round engages the stated reason
   differently: §12's hash compared "each tree's proxy pool" without saying over what, and a raw
   compare would fail a correct restore. §12 now hashes the pool as its allocation order, with the
   trailing ascending run that reaches the last capacity slot dropped, so `[3,7,8,9]` at capacity
   10 and `[3,7,8,9,10,11]` at capacity 12 both hash as prefix `[3]` continuing from 7. Test 2's churn
   now includes a step that grows a tree's proxy capacity followed by a rewind across it.
3. **Applied.** The pool alloc/free row's payload named ids and allocation history only, while the
   material and manifold rows (round 9) say blocks created or destroyed with an owner ride in that
   owner's pool entry. The row now carries the handles and sizes for a shape's `materials` block and
   a contact's manifold block and mesh triangle cache, and states the undo, redo and eviction
   behaviour. A consequence of an earlier round's fix, not new ground.
4. **Applied, but for a different defect than Codex described.** Codex's second claim is wrong:
   `b3DynamicTree_Rebuild` clears moved bits during the step, and `b3ValidateNoMoved` asserts none
   remain before finalize (`solver.c`, `physics_world.c`), but finalize then sets them
   (`b3BroadPhase_MarkProxyMoved`) and nothing clears them before the step ends, so an end-of-step
   image does need a moved list. Its first claim, static shapes, does not apply at capture:
   `b3UpdateBroadPhasePairs` clears the static tree's moved bits at the start of every step and API
   proxy resets happen after image T, where restore clears then re-sets the bits. The real defect
   is one Codex's wording pointed near: §6 gathered the moved proxy per *awake shape*, but
   island sleep at the end of the step (`solver.c`, `solver_set.c`) does not touch proxies or moved
   bits, so a body put to sleep in step T keeps a moved proxy that step T+1's pair update consumes,
   and the image would have dropped it. §6 now enumerates the moved leaves of all three trees by
   descending moved nodes, as `ClearMoved` does; the live trees are the source, no copy of the
   population is taken from anywhere else. §5.1's row says the same, and test 2's churn includes a
   body put to sleep in the imaged tick while its proxy is still marked moved.

4 applied, 0 declined.

## Status

Finding count held at 4 (1H-3M-0L), against 4 in round 9 with one High. Finding 4 is new ground: a
mismatch between where the image reads the moved list and where the engine leaves moved bits after
sleep, reached from a wrong description of the source but real on inspection. Finding 3 is a
consequence of round 9's ownership-handle fix, which added the rows without updating the entry they
point to. Finding 1 is a missing statement, not a wrong mechanism. Finding 2 revisited round 4's
declined ground and did engage the doc's rationale, by showing the hash was the part left undefined;
the rationale itself did not change. No finding was a silent re-flip. The remaining risk is still at
the edges (broad-phase state, accessor scope, entry formats) rather than the journal core.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 10: 4 findings (1H-3M-0L), 4 applied, 0 declined`

`Series total: 68 findings (33H-32M-3L) across 10 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L)`

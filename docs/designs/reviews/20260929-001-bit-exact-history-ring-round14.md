---

title: Codex review — bit-exact history ring, round 14
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 14

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible, and several key source claims check out. The draft still has gaps that could break exact restore or allow a scrub into a stale future branch. **Changes are proposed.** This was a read-only source review; no files were modified or tests run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §9 | The journal entry list has no payload for changes to a live mesh contact’s `triangleCache`. The `mesh_contact.c` resizes and writes that cache, and its prior contents affect later contact updates. The draft calls for cache capture at sleep and wake transitions, but defines only a manifold block payload. | Define cache byte and count entries, including ownership and undo/redo rules; test a mesh contact across sleep, rewind, and forward scrub. |
| 2 | High | §5.1, §8, §14 | Future history is truncated on the first write through an accessor or journaled container, but world scalar setters are not assigned such a path. For example, `physics_world.c` writes directly. After a rewind, changing gravity could leave future slots available for a forward scrub into the old timeline. | Put every tracked world scalar setter through a shared write path that triggers branch truncation, including writes that need no journal entry. |
| 3 | Medium | §8 | Eviction is described as removing the oldest slot one tick at a time. With images every K ticks, this can leave the oldest image without the intervening journal segments needed to walk to another retained image. The directory and reported window semantics do not specify how that boundary advances. | Evict complete intervals between images, or explicitly retain every segment needed to connect all advertised restorable ticks. Test eviction with K greater than 1 and backward and forward scrubs. |
| 4 | Medium | §12–13 | Phase 0 says it enables verification tests 2–4, but its `world_snapshot.c` zeroes body `userData` (and likewise shape and joint values). Test 2 requires those values to survive each rewind. The phase 0 caveat about rebinding does not satisfy that test as written. | Split phase 0 and phase 1 acceptance criteria, or add a phase 0 host-data restoration mechanism before claiming test 2 passes. |
| 5 | Medium | §5.2, §7.4, §13 | Undo of proxy destruction requires insertion at a recorded proxy ID. The current `dynamic_tree.c` only allocates from its free-list head; the proposed broad-phase journal hook alone cannot perform that operation. | Specify the required tree allocator API and free-list restoration invariant, and include it explicitly in phase 1 work and tests. |

## Checked, no change

- `test_recording.c` does seek backward and compare replayed hashes. The `recording.c` covers body transforms and velocities, as the draft qualifies.
- `sensor.c` are processed each step regardless of owner sleep state; their overlap results are sorted and deduplicated.
- The `broad_phase.c` sorts discovered pairs, while `solver.c` uses a running fraction and caps sensor hits at eight. The proposed order work addresses a real dependency.
- `physics_world.c` currently wakes bodies and accumulates impulses inside a tree query callback; the proposed shape-ID ordering is justified.
- The `physics_world.h` and the trees’ separate proxy free lists match the inventory. The documented 64-bit cross-platform and worker-count determinism claims are consistent with `faq.md` and `simulation.md`.

## Proposed edits

Specify the missing cache entry and scalar write path first, then tighten the ring eviction invariant and phase-specific test criteria. Add an explicit allocator change to the phase 1 tree work.

## Unresolved / disagreements

The document’s open decisions about solver result changes, public hash coverage, and redo remain design-owner choices. The source review does not resolve them.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:43:37+10:00, ended 12:47:51+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as round 13, no declined-findings list (round 13 declined
nothing). Before the round the full uncommitted diff (the round-13 edits) was read and found intact,
and checked against AC-1 through AC-4 with no failure; one wording slip in §7.1's sensor row ("the
removed sensor's" on a push) was corrected. The final message above has its source links reduced to
plain file names, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Applied.** `mesh_contact.c` resizes and rewrites `triangleCache` on every awake update, and the
   next update reads the old contents. §7.1's last paragraph already required the sleep and wake
   functions to journal the mesh cache's bytes, but the only entry kind named was the manifold
   block, so the cache had no payload. The manifold-block row is now a manifold-block-or-mesh-cache
   row naming which pointee, and says the transition bytes are entries of that kind.
2. **Applied.** `b3World_SetGravity` (`physics_world.c`) assigns the world scalar directly, and §8
   truncates the ring only on a write through a write accessor or journaled container, so a scalar
   setter after a rewind left future slots forward-scrubbable into the old timeline. §5.2 now writes
   the world scalar struct through one accessor that makes no journal entry (the struct is imaged)
   and does truncate; every world scalar setter goes through it.
3. **Declined; doc sharpened.** Eviction removes the oldest slot, which may be a journal-only slot,
   but a walk to an imaged tick T reads only segments after T and a forward scrub from T reads only
   segments after T, so the segments before the oldest retained imaged tick are never read and every
   segment after it is retained. Nothing can be stranded. §8 now says so and defines
   `oldestImageTick` as that imaged tick.
4. **Applied.** The phase 0 paragraph already says the serializer zeroes body, shape and joint
   `userData` (`world_snapshot.c`) while test 2 and test 3 require `userData` to survive or drive
   callbacks. §13 now says phase 0 runs tests 2 to 4 without their `userData` clauses, which apply
   from phase 1.
5. **Declined; doc sharpened.** `b3AllocateProxy` (`dynamic_tree.c`) peels the free-list head and
   `b3FreeProxy` pushes at the head. Journal entries replay in strict order (reverse for undo,
   forward for redo), so when an entry is replayed the free list is in the state it had when the
   entry was recorded and the recorded id is at its head; the tree's ordinary create and destroy
   reproduce it, and no id-directed allocator API is needed. §7.4 now states this and that the
   replay asserts the id matches.

3 applied, 2 declined.

## Status

Finding count fell to 5 (2H-3M-0L) from 7. Finding 1 is the same class as round 12's finding 1 and
round 13's finding 5 (a pointee with no entry payload, one owner over), so the ownership sweep is
still finding stragglers. Finding 2 is a new category: the truncation trigger covered accessors but
not the imaged world scalars. Finding 4 is an inconsistency between §12 and §13 that round 11's
`userData` test addition introduced. Findings 3 and 5 asked for something the doc did not need but
had not explained; neither engaged stated rationale, and both were answered by adding the rationale.
None was a consequence of round 13's fixes. The series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 14: 5 findings (2H-3M-0L), 3 applied, 2 declined`

`Series total: 88 findings (39H-45M-4L) across 14 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L)`

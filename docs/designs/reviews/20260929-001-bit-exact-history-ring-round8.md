---

title: Codex review — bit-exact history ring, round 8
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 8

## Review report (Codex final message)

## Summary

The whole-world approach is coherent, and the existing recording test supports the narrower repeatability claim the document makes about transforms and velocities. I found one restore gap that breaks forward scrubbing, plus gaps in the proposed hash and in the definition of a restorable window.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.1, 6, 7.1 | A body can update its shapes and fat AABBs while awake, then sleep later in the same step (`src/solver.c`, `src/solver_set.c`). At that tick’s end, its shapes are absent from the awake image; the sleep transition does not journal their final shape and fat-AABB values. Forward scrubbing from an earlier image can therefore restore stale values. The stated “image or journal” rule fails at this boundary. | Capture final shape and fat-AABB values for bodies leaving the awake set, and apply the same transition check to contact manifold and cache bytes updated before sleep. Add a forward-scrub test across sleep. |
| 2 | Medium | §12 | The proposed full-state hash omits the tree moved bits and the separate `fatAABBs` array, both of which affect later broad-phase work (`src/broad_phase.c`, `src/solver.c`). Hashing only “in id order” also misses permutations of ordered solver-set and colour arrays; §11.2 itself identifies array order as consequential. The proposed oracle could pass while replay state differs. | Hash fat AABBs, moved proxy identity, and simulation-affecting array contents in their stored order. Use id order only for sparse, id-addressed records. |
| 3 | Medium | §§2, 6, 9, 14 | Requirements 3–4 promise restoration of any tick in the retained window, while capture interval K retains journals for intermediate ticks but images only every Kth tick; `Rewind` explicitly rejects a tick without an image. | Define the retained *restorable* set as imaged ticks throughout the requirements and API, or provide restoration for intermediate ticks. |
| 4 | Medium | §§5.2, 7.1 | Material blocks have conflicting ownership rules: §5.2 says they use the ownership-transfer rule, §7.1 specifies reallocating and copying old/new bytes, then says create/destroy hands blocks between the world and entry. These imply different eviction and truncation behavior. | Choose one block-entry representation and state precisely which side owns each block after undo, redo, eviction, and branch truncation. |
| 5 | Medium | §§8, 12 | Verification exercises random rewind and replay, but does not exercise the ring behavior that controls availability and block ownership: interval widening, eviction, a single slot over budget, forward scrub, and truncation after a rewind. Those paths implement requirements 3–5. | Add targeted tests for each ring transition, including owned blocks and `bytesUsed` after eviction or branch truncation. |

## Checked, no change

- `test/test_recording.c`’s `ScrubBackward` compares hashes after seeks; `src/recording.c`’s current hash covers body transforms and velocities, as the document says.
- The phase-0 warning about cleared host `userData` matches `src/world_snapshot.c`.
- The CCD sensor cap and running-fraction dependence described in §7.4 match `src/solver.c`; the explosion callback’s traversal-order effects match `src/physics_world.c`.
- Contact pairs are sorted before creation in `src/broad_phase.c`. The stated worker-count and 64-bit platform determinism claims match the cited tests and documentation.

## Proposed edits

Resolve finding 1 in the state inventory and transition rules first. Then make the hash enumerate every state item used by those rules, clarify which ticks the API can restore, reconcile material ownership, and extend the verification plan to ring transitions.

## Unresolved / disagreements

The proposed CCD and explosion order changes alter engine behavior and remain a solver-owner decision, as §16 states. I found no source evidence that overturns their stated rationale.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T11:55:33+10:00, ended 11:59:09+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` after the call showed only the target doc and `WORKFLOW.md` modified plus
the untracked round-6 and round-7 files, all from before the call, so the working tree was
untouched by Codex. Same fresh full-scope prompt as rounds 6 and 7, with the same context note and
no declined-findings list (round 7 declined nothing). Before the round the whole `git diff` of the
target was read and was not mangled, and the additions were rechecked against AC-1 through AC-4
with no failure.

**Findings verified against source.**

1. **Applied.** `solver.c` writes shape `aabb` and `fatAABBs` in the finalize stage and calls
   `b3TrySleepIsland` after it in the same step; the sleep path in `solver_set.c` writes no shape.
   A body that sleeps at tick T therefore has no shape in image T and no journal entry carrying its
   final shape bytes, so a forward scrub from an earlier image lands on stale values. §7.1 gains
   the sleep-side counterpart of the wake rule: the one function moving a body out of the awake set
   and the one moving a contact out of it journal the final shape, manifold and cache bytes.
2. **Applied.** §12's hash list omitted fat AABBs and the trees' moved-proxy set, and "in id order"
   would not distinguish permutations of ordered arrays, which §11.2 shows matter. The hash now
   covers both and hashes ordered arrays in stored order.
3. **Applied.** Requirements 3 and 4 said "retained window" and "retained tick" while `Rewind`
   rejects a tick without an image. Both now say restorable tick, defined as an imaged tick inside
   the window.
4. **Applied.** The material and manifold block entries copied old and new bytes while the
   ownership paragraph moved blocks between world and entry. A block that exists on both sides of a
   write is copied by the block entry; a block created or destroyed with its owner rides in the
   owner's pool alloc/free entry as a handle. This is a consequence of round 6's finding 3 fix,
   which mixed the two.
5. **Applied.** §12 had no test of interval widening, eviction, an over-budget slot, forward
   scrub, or branch truncation. Test 6 covers them and includes a leak check on `DisableHistory`.

5 applied, 0 declined.

## Status

Finding count stayed at 5 but severity fell: 1 High, 4 Medium (1H-4M-0L) against 2H-2M-1L last
round, the lowest High count since round 1. The High is new ground the doc's transition rules never
examined (the sleep boundary), and is the mirror of a rule the doc already had for wake. Finding 4
was a consequence of round 6's own fix. Findings 2, 3 and 5 are oracle, contract and test-plan
completeness. No finding reopened a stated rationale. The findings are now mostly at the doc's
edges, but finding 1 shows a state-transition symmetry gap can still be found in the core.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 8: 5 findings (1H-4M-0L), 5 applied, 0 declined`

`Series total: 60 findings (31H-27M-2L) across 8 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L)`

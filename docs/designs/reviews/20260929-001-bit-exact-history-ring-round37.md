---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 37), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 37

## Review report (Codex final message)

## Summary

The design accounts for several difficult state boundaries, including sleeping-set transfers, proxy category bits, and sensor overlap changes. I found two correctness gaps in the proposed journal, an omitted island mutation path, and a memory-budget rule that contradicts its own accounting. The verification plan also needs explicit coverage of the journal hooks it relies on.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1 | `b3DestroyBody` destroys its shapes directly: it frees shape allocations and sets each shape’s `id` to null without calling `b3DestroyShapeInternal`. The shape-record mutation inventory omits this path. Undo could therefore lack both the old shape record and its owned allocations. (`body.c`, `shape.c`) | Add the direct body-destruction path to shape journaling, including allocation ownership; test rewind across destruction of a body with shapes. |
| 2 | High | §7.1 | A manifold-block entry specifies one `count`, yet its old and new manifold counts can differ. Contact destruction changes the count to zero, and mesh collision can replace a block with a different cluster count. The entry also needs old and new triangle-cache lengths to decode both payloads. (`contact.c`, `mesh_contact.c`) | Store separate old and new counts and byte lengths, and define allocation and copy order for undo and redo. |
| 3 | Medium | §5.2 | The island mutation inventory omits `b3CreateIslandForBody` and `b3RemoveBodyFromIsland`. These functions append to or swap-remove from `island->bodies`, including during body enable, disable, and type changes; the island row names the contact/joint link paths but does not identify these body-link writes. (`body.c`, `island.c`) | Add both functions and their callers to the island-link hook inventory; cover sleeping-island body links with dense-array entries. |
| 4 | Medium | §2, §8 | `maxBytes` counts allocated arena capacity, but §8 says that capacity never shrinks after eviction. A temporary large minimum-window capture can therefore leave `bytesUsed > maxBytes` indefinitely, even after the required retained state becomes small. That exceeds the stated minimum-window exception. | Either allow arena capacity to shrink after the spike passes, or state that the budget is a growth target rather than a continuing footprint cap. |
| 5 | Medium | §12 | The cold-hash guard detects a missed hook only when a test exercises that write. Random churn does not establish coverage of every listed mutation path, particularly the direct shape and island paths above. (`body.c`, `shape.c`, `island.c`) | Add a mutation-site coverage matrix with targeted backward, forward, and replay checks for each hook family and allocation transition. |

## Checked, no change

- `sensor.c` sets its overlap change bit after comparing the sorted current and previous overlaps, matching the proposed cold-journal trigger.
- `shape.c` and `broad_phase.c` confirm that a live proxy’s category can differ from the shape filter after a filter change that leaves the proxy in place; the separate category state is justified.
- `body.c` confirms that setting a non-awake body’s transform can change shape bounds and move its proxy.
- `id_pool.c` uses a free list and bump index, so the journal must preserve which allocation route was used.
- The restore procedure’s clearing of event buffers matches the caller contract that events for the restored tick are unavailable immediately after rewind.

## Proposed edits

1. In §5.2, name `b3DestroyBody` as a shape-record and allocation mutation site. Specify how its direct shape loop retains materials and hull references for undo.
2. In §7.1, replace the manifold entry’s single count with old/new manifold counts and old/new triangle-cache lengths; state how zero-length sides are represented.
3. In §5.2, add `b3CreateIslandForBody` and `b3RemoveBodyFromIsland`, with their body creation, destruction, enable, disable, and type-change callers.
4. In §8, reconcile reserved arena capacity with the `maxBytes` promise by specifying shrink behavior or revising the promise.
5. In §12, require targeted coverage for each inventoried hook and both directions of each journal entry kind, alongside the random tests.

## Unresolved / disagreements

The document is a design, so source alone cannot establish whether its proposed hooks will be placed at every write or whether the performance targets will be met. The findings above concern paths and formats that the design can specify before implementation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000` (set correctly this time — round 36's Post-review
verification records the earlier misconfigured attempt for this series). Start
2026-09-29T09:18:37+10:00, end 2026-09-29T09:23:40+10:00, `EXIT_CODE 0`, returned inline in about 5
minutes 3 seconds. The raw log had the final `## Review report` block duplicated once (known
`codex exec` streaming artifact, identical content both times); one copy is kept above. Codex ran
read-only; the raw log has no `apply_patch`/write/patch tool call of any kind, and the only
working-tree changes present are round 36's own edits plus this round's own post-review edits
accounted for below — confirming Codex made no file changes itself.

**Prompt.** Fresh, from-scratch, identical in structure to round 36's — no mention of round 36's
own findings or fixes, per WORKFLOW.md's rule against carrying prior-round scope into the prompt.

**Declined-findings list used for the prompt.** Unchanged from rounds 34–36: §11.1's citation into
`docs/designs/reviews/`, §7.4's CCD "Change:" paragraph's two equivalent phrasings, and §12's
unqualified "pool state" wording. Codex did not re-raise or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Checked §7.1's
"Ownership transfer instead of copying" paragraph's enumerated heap-owned-array cases (a sleeping
set's arrays, an island's arrays, a shape's `materials` array, a mesh contact's `triangleCache`,
a destroyed sensor's `hits`/`overlaps1`/`overlaps2`) against every heap-owned array actually named
in §5.2's inventory, looking for one left off the ownership-transfer list. Grepped `sensor.c`'s
`b3Array_Destroy` calls against §7.1's sensor-array list (all three present) and confirmed the
non-awake solver-set dense arrays and island link arrays are the same arrays §5.2's
"Swap-compaction on removal" paragraph already routes through the generic dense-cold-array entry
kind, not this ownership-transfer one — no separate case missed. This check did not anticipate
this round's actual findings, which were about a second call site reaching an already-covered
destructive operation (findings 1, 3) and a journal entry's field shape rather than its ownership
mechanics (finding 2).

The findings verified directly against the doc and source:

- **(Finding 1: `b3DestroyBody`'s inline shape-destroy loop bypasses the shape-record journal
  trigger `b3DestroyShapeInternal`), CONFIRMED, applied.** Read `b3DestroyBody`'s shape loop
  (`body.c:395-420`): for each owned shape it destroys the sensor, destroys the broad-phase proxy,
  calls `b3DestroyShapeAllocations`, frees the id, and sets `shape->id = B3_NULL_INDEX` directly —
  it never calls `b3DestroyShapeInternal` (`shape.c:478`), the only function §5.2's shape-record
  row names as a destroy trigger. Confirmed both paths reach the same destructive operations via
  the same shared function, `b3DestroyShapeAllocations` (`shape.c:1046`), which frees `materials`
  and calls `b3DestroyShapeAllocationForShapeChange` (`shape.c:1025`), which for a hull shape calls
  `b3RemoveHullFromDatabase` — exactly the release §7.1's hull-refcounting paragraph says a
  journaled write must take an extra reference against before it happens, so a body destroy
  reaching this release with no journal hook fired first is not just a missing-old-bytes gap but a
  potential dangling-reference bug matching the finding's "unproven" framing. Genuine second call
  site for an already-designed mechanism, not previously named anywhere in §5.2. Fixed by adding
  `b3DestroyBody`'s shape loop to the shape-record row as a second trigger, alongside
  `b3DestroyShapeInternal`.
- **(Finding 2: manifold-block entry's single `count` field cannot represent an old/new count that
  differ), CONFIRMED, applied.** Re-read §7.1's manifold-block row and the sentence just below it:
  "A structural write to a touching contact (create, destroy, sleep, wake, merge, split, transfer)
  fires a manifold-block entry... instead of the generic record write" — destroy is explicitly one
  of the triggering transitions, and a destroyed contact's new manifold count is unconditionally
  zero while its old count was whatever it held while touching; a single shared `count` field
  cannot encode both. The same problem recurs on every ordinary tick for a persisting mesh contact
  whose narrow-phase cluster count changes between old and new, which also changes its
  triangle-cache length (only the *bytes* were given old/new variants, not the count or the
  cache length driving how many bytes to read). Genuine format defect, not merely an
  under-specification: as written, undo/redo cannot be implemented without guessing how many bytes
  the shared count actually describes on each side. Fixed by giving the entry separate old/new
  counts and old/new triangle-cache lengths, and stating that either direction can be an
  allocate-for-zero case.
- **(Finding 3: island `bodies` link array's own mutation sites omitted from §5.2's islands row),
  CONFIRMED, applied.** The islands row cites only `island.c:20-337, 388-649` for "link/unlink."
  Grepped and read `b3CreateIslandForBody` (`body.c:110`, pushes to `island->bodies`, called from
  body creation, wake, and type change) and `b3RemoveBodyFromIsland` (`body.c:123`, swap-removes
  from `island->bodies`, called from body destroy, disable, and type change) — both are in
  `body.c`, entirely outside the cited `island.c` line ranges, and both write the same
  `island->bodies` array (confirmed against `island.h`'s three link arrays: `bodies`, `contacts`,
  `joints`) that the row's own citation was meant to cover. `b3RemoveBodyFromIsland` already
  appears in the `bodies[id]` row's own function list (for its effect on the body's own
  `islandId`/`islandIndex` fields), but that is a different structure than the island's own
  `bodies` array — citing it there doesn't also cite it for this row, which is what was missing.
  Genuine citation gap, not a duplicate. Fixed by adding both functions and their real callers to
  the islands row, distinguishing them from the contact/joint link functions already cited.
- **(Finding 4: requirement 5's "bounded memory" wording contradicts §8's own monotonic-capacity
  statement), CONFIRMED, applied — this is a reversal-check case (WORKFLOW.md step 5's third
  bullet), and the finding engages the existing rationale with a real textual inconsistency rather
  than just restating a prior complaint, so it passes.** Round 35's own declined-findings
  carry-forward already flagged this exact concern once before (see round 35's list, since
  superseded); round 36 substantively declined a near-identical Codex finding but added a
  clarifying sentence to §8 stating that reserved arena capacity "persists for the rest of the
  world's life" absent an explicit disable/re-enable. This round's finding cites that added
  sentence directly and points out it is in real tension with requirement 5's own framing — "the
  minimum window is kept anyway even when its total required storage... exceeds the budget" reads
  as a *conditional* exception (tied to the window's current true requirement), while "persists for
  the rest of the world's life" describes a *permanent* one that does not go away even once the
  window's true requirement drops back under budget. This is a genuine internal inconsistency
  between requirement 5 and round 36's own fix to §8, not a restatement of round 34/36's original
  complaint. Fixed by adding a clause to requirement 5 itself stating plainly that the exception,
  once triggered, is not guaranteed temporary, cross-referencing §8's mechanism and the
  disable/re-enable recovery path — reconciling the requirement's wording with the accounting
  behaviour §8 already describes, rather than re-litigating whether monotonic capacity is
  acceptable (round 36 already settled that it is).
- **(Finding 5: cold-hash guard's coverage of §5.2's inventory is only statistical, not
  guaranteed), CONFIRMED, applied.** Re-read §12 test 2's description: "1,000 random (T, P) pairs
  ... with random API churn" — genuinely randomized, with no mechanism ensuring every named
  mutating function in §5.2's now-larger inventory (including the two additional call sites this
  round's findings 1 and 3 just added) is exercised at least once. Requirement 7 claims "a missed
  write site fails a test rather than a review," which is only true for a site the test suite
  actually reaches. This is a real verification-plan gap distinct from findings 1 and 3 themselves:
  even a fully-corrected inventory only gets tested to the extent random churn happens to hit each
  entry. Fixed by adding a directed-coverage requirement to §12 test 2: one test per named
  mutating function, explicitly including the multi-site rows this round's fixes introduced, run
  through the cold-hash guard and a restore, so coverage doesn't depend on random chance.

## Status

Round 37 of an ongoing series (rounds 1–36 committed). Five findings, all genuine and all applied
— continuing round 36's pattern of substantive findings rather than round 32–34's citation nits,
and, like round 36, mostly citation/inventory gaps (findings 1, 3) plus one journal-format defect
(finding 2) plus one verification-plan gap (finding 5). Finding 4 is the one finding this round
that is a direct consequence of the *immediately preceding* round's own fix (WORKFLOW.md step 7):
round 36 added the "persists for the rest of the world's life" sentence to §8 to close finding 4's
round-36 predecessor, and that addition is what created the tension with requirement 5's wording
that this round's finding 4 found — not a new discovery about the source, but a fresh reviewer
reading round 36's own new text against a requirement round 36 didn't touch. This is the second
round in a row (after round 36) to find that a plain-destroy call site bypasses a journal trigger
named only for a different, recreate-shaped call site — round 36 found this for tree proxy
`categoryBits` (`b3DestroyBody`/`b3DestroyShapeInternal`/`b3Body_Disable` missing from that row),
this round found the same shape of gap twice more, for the shape's own record (finding 1) and for
the island body-link array (finding 3) — suggesting "does every plain-destroy call site also reach
whatever a recreate-shaped citation covers" is a productive question to keep asking of the
remaining inventory rows, not a coincidence now exhausted by three finds. No finding this round
reopened ground declined on the carried-forward list; finding 4 reopened ground round 36 itself
covered, engaged that rationale as the reversal-check requires, and found a genuine internal
inconsistency round 36's own fix had introduced.

Per the user's instruction, this batch was capped at 2 rounds (36–37) or sooner on `CONVERGED`.
Round 37's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, and the batch limit is now reached, so
this session stops here per that instruction; the series remains open, and whenever review next
resumes it should continue at the same full scope as every round so far — the last two rounds
(36, 37) each found five genuine issues after 35 prior rounds, so there is no basis for narrowing.

Round 37: 5 findings (2H-3M-0L), 5 applied, 0 declined
Series total: 126 findings (46H-62M-18L) across 37 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L) -> 5 (2H-2M-1L) -> 5 (2H-3M-0L)

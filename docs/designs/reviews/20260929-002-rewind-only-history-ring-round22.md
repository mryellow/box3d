---
title: Codex review - rewind-only history ring, round 22
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round21.md
mode: full pass at full scope on revision 23, the first round since the revision 22 reconciliation with master at 55a40d1 (U-I1); same output shape as rounds 1 to 21, prompt rewritten for the four-tree engine
---

# Round 22

## Exact request sent to Codex

```
Review the design document docs/designs/20260929-002-rewind-only-history-ring.md as an adversarial design reviewer for the Box3D physics engine. It is the design for a rewind-only, bit-exact world history: a hot image of the awake state captured per tick into a ring, an undo journal of every write to cold state, and a public Rewind API that restores the whole world to a retained tick and discards everything after it.

Scope: the document's own content and the source it cites (src/solver.c, src/broad_phase.c, src/broad_phase.h, src/physics_world.c, src/shape.c, src/sensor.c, src/body.c, src/world_snapshot.c, src/dynamic_tree.c, src/dynamic_tree.h, src/constraint_graph.c, src/solver_set.c, src/island.c, src/contact_solver.c, src/contact.c, src/joint.c, src/id_pool.c, src/core.c, src/compound.c, src/name_cache.c, src/recording.c, src/replay.c, src/recording_replay.c if it exists, include/box3d/, test/test_determinism.c, test/test_recording.c, shared/benchmarks.c), and the design documents it cites only where it cites them (docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md, docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md, docs/designs/20261013-001-preflight-awake-tree-split.md, docs/designs/20260927-002-bit-exact-history-ring.md, docs/designs/20260927-001-client-prediction-rollback.md, docs/faq.md, docs/simulation.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it. The source has changed since the document was first written: the dynamic-body broad-phase tree is now split into an awake tree and a sleeping tree with a migration queue, and the continuous and explosion candidates are sorted; check the document against the code as it is now.

Pressure-test the design. Find real defects: claims about the code that are false; a simulation-affecting byte of state that the state inventory in section 5 neither images nor journals; a write to cold state that reaches a field without passing through the accessor or journaled container type the design names; a journal entry whose undo does not restore what its mutation changed, or whose ownership rule leaks or double-frees a block; a restore order in section 9 that reads something an earlier step already overwrote; a tree, proxy id, moved bit or migration-queue case the trees section misses under the awake and sleeping trees; a ring, budget or eviction rule in section 8 that can reject a restorable tick or leave the world inconsistent; a wrong or inconsistent number; contradictions between sections, including between the API sketch in section 14 and the caller contract in section 10. Treat existing primitives (b3DynamicTree, the solver sets, the constraint graph, the id pools, the pair set, the recording serializer) as settled tools; judge what this design adds and the question each primitive is asked. Decisions recorded in section 17 are settled unless you have fresh source evidence that their stated reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, the open question in section 16, and test mechanism, fixtures and test-to-requirement mapping, which belong to a later Implementation Prompt. Do not review or follow any other design document except where this one cites it. Do not read docs/designs/reviews/ or any changelog.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-15T11:53:03+10:00 to 12:00:54+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13d43-513a-71a0-b49a-710a33137b0e`. Effort is `high` per the Parameters row; rounds 1 to 21
ran at `medium`.

## Review report (Codex final message)

## 1. Summary

The rewind design has a sound overall shape, and the current source does sort pair, continuous, and explosion candidates as claimed. I found two correctness failures in the specified restore path, one state inventory gap, and a persistent capture cost that conflicts with the awake-state requirement.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§7.4, 8–9 | Tick 0 can contain a **moved static proxy**. A static shape created with contact creation enabled marks its leaf (`src/shape.c`); pair update later consumes that mark (`src/broad_phase.c`). Enabling history before that step is allowed, but rewind clears static marks and restores only the kinematic moved list. Replaying the first step can miss pairs. | Capture and restore static moved proxy ids for any image that can contain them, including tick 0. |
| 2 | H | §§7.4, 9 | Every shape restore write enters the tree-fixup list, but undoing a later bump allocation truncates the shape slot arrays. The final tree pass can then read an id beyond the restored array after a create-and-rewind sequence. | After the journal walk, discard listed ids outside the restored shape array or whose slots are free before reading their records. |
| 3 | M | §§3, 5, 10 | `simdWidth` is absent from both the image and the stated host-configuration contract. The public SSE2 fallback setter changes this per-world field (`include/box3d/box3d.h`, `src/physics_world.c`), and the solver selects work by it (`src/solver.c`). The claim that width-4 SIMD is used everywhere is false. | Define whether SIMD width is restored engine state or a fixed caller obligation; update capture, hash, replay contract, and the cross-platform claim accordingly. |
| 4 | M | §§2, 7.4, 17 | Imaging `proxies` through `proxyCapacity` makes every later tick cost proportional to the awake tree’s **historical peak**, even after those bodies sleep. That capacity does not shrink (`src/dynamic_tree.c`, `src/broad_phase.c`); one million formerly awake proxies leave roughly 24 MB of proxy slots to copy per tick with only a few bodies awake. This conflicts with §2’s current-awake cost requirement. | Keep the awake layout image, but account for proxy allocation order without copying every free high-water slot each tick, or explicitly revise the cost requirement. |
| 5 | M | §§2, 5.2 | The proposed compile-time completeness guarantee does not cover reassignment of the world’s plain container and scalar pointers. The design explicitly leaves those pointer fields writable, so another translation unit can replace one without calling its journaled mutator. | Hide ownership pointers behind the defining module or narrow the claimed compile-time guarantee. |
| 6 | L | §§1, 8 | The cited `src/recording_replay.c` does not exist. Backward-seek and keyframe-ring code is in `src/replay.c`. | Correct those source references. |

## 3. Checked, no change

- Pair keys are sorted before contact creation; continuous candidates use sweep key then shape id; explosion candidates are applied in shape-id order (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The four-tree split, parked awake leaves, migration queue, bulk placement, and sleeping-tree height trigger match the current broad-phase code (`src/broad_phase.c`, `src/dynamic_tree.c`).
- The sensor pass supports imaging `overlaps2` while treating `hits` and `overlaps1` as step scratch (`src/sensor.c`).
- The reverse undo and block-ownership rules for the described pool, set, island, sensor, and hull transitions revealed no further defect.

## 4. Proposed edits

Resolve findings 1–4 in the capture and restore algorithms, revise §5.2’s module-boundary claim, and replace the stale source references. Add the static-mark and truncated-slot cases to the design’s restore scenarios.

## 5. Unresolved

Bit equality between the selectable SIMD widths needs an explicit guarantee or a caller constraint before the cross-platform claim can stand.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source at 55a40d1 and the doc.

- **#1 (H) static marks at tick 0: confirmed, applied.** `b3CreateShapeProxy` marks a static leaf
  when contact creation is forced (`shape.c`), `b3ResetProxy` forces it, and `b3BroadPhase_MoveProxy`
  always marks (`broad_phase.c`); the pair update reads and clears the static tree's marks
  (`broad_phase.c`), so a boundary after a step holds none, but tick 0's image is taken by
  `b3World_EnableHistory` after between-step calls and §8 forbids only queued end events. Fix: the
  moved-proxy list covers the kinematic and static trees (§5 hot row and inventory row, §6 gather,
  §7.4 moved-bit restore). No new mechanism; the gather and the mark pass already exist for the
  kinematic tree, and an unmarked static tree costs one root read.
- **#2 (H) listed ids beyond the truncated shape array: confirmed, applied.** §7.4 lists every
  restore-time shape write, a shape created after T is written through the structural accessor
  after its bump allocation, and the reverse walk lists it before the pool alloc's undo truncates
  the array (§7.1). Fix: §7.4's pass skips a listed id at or beyond the restored array's length
  without a read. The length is the restored pool's own state, not a guard.
- **#3 (M) `simdWidth`: confirmed as a false claim, applied with a different fix.** `world->simdWidth`
  is set from `b3GetSIMDWidth` at creation and by the public `b3World_EnableSSE2Fallback`
  (`physics_world.c`), and the contact solver selects width-8 paths by it (`contact_solver.c`,
  `solver.c`); "width-4 SIMD everywhere" was false. The reviewer's Unresolved asks for a bit-equality
  guarantee between widths: it exists, `SIMDWidthTest` (`test_determinism.c`) asserts identical
  hashes, sleep steps and query results between width 4 and the native width. Fix: §3 states the
  widths and the test; §10 names the per-world and process-wide width as host configuration, left
  by `Rewind` and outside the hash. Not imaged, not hashed, because the test shows the value does
  not affect results.
- **#4 (M) proxy array imaged to capacity: confirmed as an inconsistency, applied as a statement.**
  `proxyCapacity` grows by half and never shrinks (`dynamic_tree.c`). §7.4 already said requirement
  2 reads "the most that was awake at once" for this array; §2 did not. Fix: §2's requirement 2
  names the awake tree's proxy array as its second exception, 24 B per slot of the high-water mark.
  The reviewer's alternative, imaging the free list without the free slots, is O(free slots) too,
  since the free list after a mass sleep is the freed slots in LIFO order; a smaller term needs the
  engine to shrink the awake tree's proxy pool, which renumbers live proxies and is an engine change
  outside this doc. Carried to the owner as a question, not an open question in the doc.
- **#5 (M) pointer fields assignable: confirmed, applied by narrowing.** §5.2 said the world's
  container pointers are plain, non-const pointers and then claimed a pointer assignment elsewhere
  does not compile. Fix: the claim is element and count assignment; the pointer is assignable but
  has no producer outside the container's file, the type being incomplete there.
- **#6 (L) `src/recording_replay.c`: confirmed, applied.** The file does not exist; the keyframe
  ring and backward seek are in `src/replay.c`. Both citations fixed, and the same stale path in
  `20260927-002` (two) and `20260929-003` (one) fixed in the same edit (U-F7); neither doc's status
  changes (U-I7).

Raw log: the reviewer excluded `docs/designs/reviews/` from its file listing and read no round
record or changelog.

## Status

Six findings, two High and three Medium and one Low, after the clean round 21 and the revision
22 reconciliation. Categories: two restore-path correctness defects from the four-tree reconciliation
(#1, #2), one false claim about the engine (#3), one cross-section inconsistency the reconciliation
left (#4), one overclaim in the module-boundary argument (#5), one stale citation (#6). #1, #2 and
#4 landed in text revision 22 wrote or rewrote; none is a consequence of revision 23's edits. Not
converged. Next: round 23, same prompt, same scope.

Tripwire (U-T): no named mechanism added or removed (U-T1); the moved-proxy list widened to a
second tree it already had the gather and mark pass for. Findings did not cluster in a section
that produced findings in round 21, which had none (U-T2); the reviewer read no round record. Doc
length 1347 -> 1361 lines; added lines per fix: #1 six, #2 two, #3 five, #4 three, #5 three, #6
zero (U-T3). Over the 400-line split check (U-T6): the doc decides one mechanism, the hot image,
undo journal, ring and restore are one rewind, and §10 and §14 are its surface; no split.

Round 22: 6 findings (2H-3M-1L), 6 applied, 0 declined
Series total: 73 findings (10H-40M-23L) across 22 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L)

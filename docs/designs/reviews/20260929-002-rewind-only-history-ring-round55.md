---
title: Codex review - rewind-only history ring, round 55
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round54.md
mode: full pass, same prompt and scope as round 40, on revision 54
session: 01a13f72-34c1-71e2-876a-4bbb578994ec, resumed once to finalize
---

# Round 55

## Exact request sent to Codex

Identical to round 54 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(12,533 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T22:03:31+10:00
to 22:12:31. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 22:12:39 to 22:13:08. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13f72-34c1-71e2-876a-4bbb578994ec`.

## Review report (Codex final message)

## 1. Summary

The live source has the awake and sleeping dynamic trees and the sorted continuous and explosion candidates assumed by the design. The state inventory and reverse journal order are largely coherent, but two write-boundary rules need revision before the design can deliver its claimed compiler-enforced completeness.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §5.2, §7.3 | The proposed in-record array type makes `data`, `count`, and `capacity` const-qualified, yet the journaled container must change those fields. Casting to a mutable, same-layout type in the container’s file does not make writes to const-qualified subobjects valid C. This affects solver-set, island, sensor, and contact-cache headers and defeats the specified implementation of the write boundary. | Keep the actual storage mutable behind an opaque boundary. Expose read-only views through accessors, and make container mutators the only route to header changes. |
| 2 | M | §5.2, §7.1 | A whole-record write can replace an owned pointer or its count without a block entry. In particular, `b3Shape_Write` accepts a completed record containing `materials` and `materialCount`; pointer-to-const prevents writes *through* `materials`, but does not prevent replacing the pointer. The same issue applies to contact manifold ownership. Such a replacement can lose the old block or leave undo without the ownership transfer it requires. | Make owned pointers and counts immutable to general record writes. Route creation, replacement, and destruction through explicit block ownership operations, while record writes preserve those fields. |

## 3. Checked, no change

- `src/broad_phase.c`, `src/solver.c`, and `src/physics_world.c` support the document’s current awake/sleeping tree split and sorted candidate claims.
- The awake record, solver-set, graph, sensor-overlap, scalar, pool, pair-set, and hull-refcount classifications match the inspected source paths.
- The serializer scrubs object `userData` and omits the migration queue; §13 identifies both phase-0 adaptations.
- Reverse undo followed by the tick-T image is consistent for the inspected sleep/wake, slot reuse, and owned-block transitions, subject to the ownership boundary in finding 2.
- The §14 correction sketch applies a correction to the end of `serverTick`, consistently with its replay timing.

## 4. Proposed edits

Replace §5.2’s const-qualified storage-header scheme with an opaque mutable container and read-only accessors. Specify that general record writes preserve owned pointers and counts, and that only named ownership operations may change them; update §7.1’s entry descriptions to match.

## 5. Unresolved

The tree reconstruction and ring budget rules are delegated to their cited designs and were not judged as findings here.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (H) const-qualified header fields cannot be written by the container's file either:
  confirmed, applied.** An object defined with a const-qualified member cannot be modified
  through a non-const lvalue (C11 6.7.3), so the scheme the previous revision put in §5.2 left
  the container's own mutators no legal write. The const-qualified fields are gone; the
  const-data array type again makes elements const only, and the header is protected by the
  record write's construction (below).
- **#2 (M) a whole-record write can swap an owned pointer or count without a block entry:
  confirmed, applied.** `b3Shape_Write` took the completed record, `materials` and
  `materialCount` included, and the old sentence said a record write "copies the pointer". A
  record write now copies the record's other fields and leaves its owned-block pointers, counts
  and array headers as they are in storage; the only routes that change one are the §7.1 block
  entries, made by a container-file mutator addressed by (block tag, owner id) that swaps the
  pointer and count and holds the old block, `b3JournaledArray`'s mutators by (array tag, owner
  id), and the pool free entry. The same construction answers #1: a local copy's header change
  never reaches storage. The Records paragraph's "completed new value" and the pointee paragraph
  below the table say the same.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High, one Medium, both in §5.2's owned-blocks rule. #1 is a consequence of
round 51's fix, which answered the header hole with const-qualified fields that C does not let the
owner write; this round's fix removes that scheme and closes both holes by what a record write
copies. §5.2 produced findings in rounds 50, 51, 53 and 55, not three running, but clustered
(U-T4): every §5.2 fix since round 50 has narrowed a write route or removed one, none has added
a structure, so Part B is re-applied to the section and holds. Fixes folded; a fresh clean round
and its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): the const-qualified header scheme removed; no mechanism added, the §7.1 block
entries gaining the address of the mutator that makes them (U-T1). §5.2 clustered as above; #1 a
consequence of round 51's fix #1, #2 on the owned-blocks sentence no fix wrote (U-T2); the
reviewer read no round record. Doc length 1178 -> 1180 lines; #1 and #2 together added
2 (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 55: 2 findings (1H-1M-0L), 2 applied, 0 declined
Series total: 118 findings (24H-57M-37L) across 55 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L) -> 1 (1H-0M-0L) -> 1 (0H-0M-1L) -> 2 (1H-1M-0L)

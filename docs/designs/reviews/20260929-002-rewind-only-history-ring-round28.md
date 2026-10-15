---
title: Codex review - rewind-only history ring, round 28
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round27.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 30
---

# Round 28

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T13:41:10+10:00 to 2026-10-15T13:48:57+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13da6-495d-7252-b823-56728c3773d1`.

## Review report (Codex final message)

1. **Summary:** The current source supports the design’s main state split and its treatment of the awake and sleeping trees, migration queue, and sorted candidates. I found one incomplete journal undo rule and one incorrect claim about the cost of replay after a kinematic proxy move.

2. **Findings:**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §7.1 | A material element-write entry stores only the old element, but its stated undo is “reallocate and copy old.” That payload cannot reconstruct the whole block on its own; an undo that copies only the saved bytes loses the other materials. | Specify an in-place element undo using the saved index and value. Reserve whole-block reconstruction for entries that save the whole old block. |
| 2 | L | §7.4, §12 | The design says moving a kinematic proxy during restore leaves its tree unordered and makes the next step rebuild it. `b3DynamicTree_MoveProxy` in `src/dynamic_tree.c` does not always clear `dfsOrdered`; moving the sole root leaf is a counterexample. | Make the rebuild cost conditional on the resulting order or moved marks, including in the cost discussion. |

3. **Checked, no change:**

- The four-tree split, parked marked leaves, and migration-queue drain match `src/broad_phase.c` and `src/broad_phase.h`.
- Pair creation, continuous candidates, and explosion application use the ordering the design describes.
- The hot contact, sensor-overlap, proxy-id, and sleeping-set ownership paths reviewed in the cited source support the proposed image and journal split.
- The ring’s eviction and interval rules preserve the stated restorable-tick window in the cases reviewed.

4. **Proposed edits:** Split the material-block undo row into element-write and whole-block cases. Qualify the kinematic rebuild statements in §§7.4 and 12.

5. **Unresolved:** None.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) material block undo for an element write: confirmed, applied.** The row's payload for
  an element write is the index and the old element, and its undo said "reallocate and copy old",
  which only a whole-block payload supports. §7.1's row says the element write writes the old element
  back at the index; the resize or replacement reallocates and copies; both set `materialCount`.
- **#2 (L) a restore-pass kinematic move always leaves the tree unordered: confirmed, applied.**
  `b3DynamicTree_MoveProxy` clears `dfsOrdered` only when the insert breaks the order
  (`dynamic_tree.c`, `b3IsNodeOrdered`); a move of the sole root leaf does not. §7.4 says the tree
  can be left unordered or with its root marked moved, and §12 says the first replayed step can
  pay the rebuild.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one Medium and one Low, from two in round 27. Categories: an undo rule that did not
match its payload (#1) and an unconditional statement about tree order (#2). Not converged. Next:
round 29, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings did not cluster in a section that
produced findings last round (§7.1 and §7.4 against §9, §1 and §15); #2 is in the §7.4 text the
reviewer first read two rounds ago, with no fix of round 27 behind it (U-T2); the reviewer read no
round record. Doc length 1409 -> 1410 lines; added lines per fix: #1 zero, #2 one (U-T3). Over the
split check (U-T6): still one decision; no split.

Round 28: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 88 findings (14H-45M-29L) across 28 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L)

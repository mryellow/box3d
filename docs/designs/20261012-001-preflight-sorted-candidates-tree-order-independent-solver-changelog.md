# Changelog: 20261012-001 preflight sorted-candidates tree-order-independent solver

## Revision 1: first draft

- Owner: an alternative to 20261011-001 with less added processing. CCD A1 and A2 of that document are
  replaced by one change: collect candidates per tree query, sort by shape id, evaluate with today's
  logic. Explode (A3 there, A2 here) and the test helper (A0) are carried over unchanged.
- Not yet reviewed.

## Revision 2 (after round 1)

- Guarantee stated for argument-only callbacks; custom filter named as the layout-dependent exception.
- A1 caveats: per-query order, parallel interleaving; cost bullets say "no additional TOI calls" and add measurements.
- Provenance: actual sensor admission order. Out of scope: stateful callbacks.

## Revision 3 (after round 2)

- A0: trees rebuilt directly, not through `b3World_RebuildStaticTree`.
- A1: `src/physics_world.c` added to the file list; array reserve, destroy and statistics named.
- §1: both stateful callbacks named as layout-dependent.

## Revision 4 (after round 3)

- No edits. Round 3 clean; confirmation round pending.

## Revision 5 (owner edit, before round 4)

- A1 caveats: a fixed stack buffer with heap overflow named as the alternative to the per-worker
  array on `b3TaskContext`, with the reason it is not chosen. Round 4 is a full pass on the edited
  document; convergence needs a clean round and a clean confirmation round after it.

## Revision 6 (after round 4)

- §1: the impact fraction can differ with no bound (the TOI tolerance is a separation distance, and
  the failed state returns a bracket fraction); `preSolve` call set and arguments can change, custom
  filter order only.
- A1 caveats: the `preSolve` gate on the running fraction stated, with the custom filter's position
  before the TOI call.

## Revision 7 (after round 5)

- A0: the helper's state claim replaced by what a full rebuild keeps (proxy ids, leaf bounds, moved
  marks) and sets (the ordered flag), and why the flag changes only layout.
- A2: the array holds every returned candidate with a nonzero `explosionScale`.

## Revision 8 (after round 6)

- A0: the full-rebuild claim stated for a nonempty tree; an empty tree returns before any write.

## Revision 9 (after round 7)

- No edits. Round 7 clean; confirmation round pending.

## Revision 10 (after round 8)

- No edits. Round 8 confirmation clean on the same document. Converged.
- A0 paragraph reflowed to the document's line width after round 8; whitespace only, no content change.

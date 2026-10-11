# Changelog: 20261011-001 preflight tree-order-independent solver

## Revision 1: combined

- Owner: one PR, one preflight. The CCD document (A0-A2, converged after 14 rounds) and the Explode
  document (A3, one round) are merged here; their review records and changelogs are abandoned.
- A0 generalised: perturb before a named call, and a visit-order witness for the query under test
  (CCD or Explode), so A3's test needs no check of its own.
- §1 behaviour change restated as a list; Explode added to the audit and the behaviour cases.
- Provenance: A2 and A3 are separate commits that can be dropped independently.
- Not yet reviewed.

## Revision 2 (after round 1)

- No edits. Round 1 clean; confirmation round pending.

## Revision 3 (after round 2)

- No edits. Round 2 confirmation clean on the same document. Converged.

## Revision 4 (owner edit, after convergence)

- §1: the impact fraction can differ with no bound (the TOI tolerance is a separation distance, and
  the failed state returns a bracket fraction).
- A0: trees rebuilt directly, not through `b3World_RebuildStaticTree`, which writes a recording
  command; the helper's state claim replaced by what a full rebuild keeps and sets, and why the
  ordered flag changes only layout.
- A3: the array holds every returned candidate with a nonzero `explosionScale`.
- The document changed after its confirmation round, so convergence is reopened: a clean round and a
  clean confirmation round are needed before Canonical.

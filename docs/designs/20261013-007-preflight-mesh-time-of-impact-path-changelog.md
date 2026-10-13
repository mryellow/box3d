# Changelog: 20261013-007 preflight, the mesh and height-field time-of-impact path

## Revision 2 (2026-10-13)

Marked Closed by the owner on the measurement in 20261013-004: a sound per-triangle reject removes
at most 3 percent of the mesh path's time-of-impact calls on the terrain scenes, and the path's cost
sits in the query count and the fallback call, both behaviour contracts. §5 records the decision in
place of the open question; no other content change; no review round was run.

## Revision 1 (2026-10-13)

Initial draft, written after the measurement in 20261013-004 showed a sound per-triangle reject
removes at most 3 percent of the mesh path's time-of-impact calls on the terrain scenes. Records the
ceiling, where the path's cost sits (query count, fallback call), and asks the owner whether to Close.

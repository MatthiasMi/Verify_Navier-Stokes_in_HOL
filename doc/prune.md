Running

    ./holtrace-prune < oldtrace.50 > newtrace.50

converts an existing HOLTrace file
into a new HOLTrace file
that focuses on proving the same named theorems.
Traces directly from HOL Light
normally include some automatically generated proofs
that did not end up being used for the named theorems,
so pruning saves space.
Pruning also speeds up subsequent trace processing.

This tool is written in Python.
On [`rome1`](install.html),
starting with the 82MB trace of `hol.ml` from [`holtrace-save`](save.html),
`holtrace-prune` takes 50.04 seconds and 3268MB RAM,
and produces a pruned trace occupying 37.6MB (`gzip -9` reduces this to 10.4MB).

This tool is idempotent.
Given the 37.6MB pruned HOLTrace file as input,
the tool takes 33.08 seconds and 1704MB RAM,
generating an identical file as output.

This tool lets you provide a theorem name as a command-line argument.
The resulting trace then focuses on proving that theorem
(although the tool will preserve names for any lemmas relevant to that theorem).
For example, given the 37.6MB pruned trace for `hol.ml`,
`holtrace-prune ONE_OR_PRIME`
takes 16.63 seconds and 1413MB RAM,
and generates 2.4MB as output (`gzip -9` reduces this to 0.72MB).

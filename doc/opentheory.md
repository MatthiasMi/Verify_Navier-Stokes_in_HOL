[OpenTheory](https://joe.leslie-hurd.com/opentheory/) from Joe Hurd
is an existing library of traces
originally obtained from HOL Light, HOL4, and ProofPower.
One way to generate HOLTrace files is by
(1) downloading traces from the OpenTheory library and
(2) using `ot2holtrace` from the HOLTrace package
to convert traces from OpenTheory format to HOLTrace format.

The traces currently produced by `ot2holtrace`
from the OpenTheory library
will be rejected by `holtrace_restore` since they rely on different axioms and definitions
from the normal HOL Light axioms and definitions,
but will be accepted by `holtrace_restore_allownew`.

## Obtaining traces in OpenTheory format

If you install the `opentheory`
[tool](https://joe.leslie-hurd.com/software/opentheory/download.html)
and run

    opentheory install base

then `$HOME/.opentheory/packages` will contain
various subdirectories such as `base-1.221`.

Internally,
these subdirectories contain traces
split across "article" (`.art`) files.
Article dependencies are specified by "theory" (`.thy`) files.

## Converting OpenTheory traces to HOLTrace format

To use `ot2holtrace`:

    ./ot2holtrace $HOME/.opentheory/packages base > base.50

This tool is written in Python.
On [`rome1`](install.html),
running `ot2holtrace` for `base` takes 111 seconds and 421MB RAM,
and converts 13.6MB of OpenTheory article files
into a 7.3MB HOLTrace file.

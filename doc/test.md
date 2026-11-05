Download and uncompress a trace:

    wget https://holtrace.cr.yp.to/prime.50.gz
    gunzip prime.50

Start a HOL Light session
(not reusing a pre-HOLTrace session):

    hol.sh

In that session, verify the trace:

    holtrace_restore !theorems "prime.50";;

The expected outputs are many lines that start `matching`
and then the following, with more lines in place of `...`:

    types: 597
    terms: 176388
    theorems: 91115
    inferences: 91115
    axioms: 3 = matching 3 + creating 0
    typedefs: 6 = matching 6 + creating 0
    constants: 82 = matching 82 + creating 0
    named thms: 327 = matching 327 + creating 0
    tag /     0.000000 seconds          327 occurrences
    ...
    tag t     0.000000 seconds        91115 occurrences
    total     0.000000 seconds
    val it : unit = ()

You can also try `holtrace_restore_time` instead of `holtrace_restore`;
then there will be actual timings instead of `0.000000 seconds`.

Inspect the restored `ONE_OR_PRIME` theorem:

    holtrace_restore_name2thm "ONE_OR_PRIME";;

The expected output is the following:

    val it : thm =
      |- forall p.
             p = 1 \/ prime p <=> (forall n. n divides p ==> n = 1 \/ n = p)

Check that the restored theorem matches the theorem from the HOL Light standard library:

    equals_thm it ONE_OR_PRIME;;

The expected output is `val it : bool = true`.

Outside the HOL Light session,
verify the same trace from the command line:

    ./holtrace-verify-simple < prime.50
    ./holtrace-verify < prime.50

The expected output is

    verified 2399423 bytes, 359543 lines, 597 types, 176388 terms, 91115 infs, 91115 thms, 6 typedefs

from `holtrace-verify-simple`,
and the same from `holtrace-verify`.

Check that the trace is already pruned
down to what is needed for the `ONE_OR_PRIME` theorem:

    ./holtrace-prune ONE_OR_PRIME < prime.50 | cmp - prime.50

The expected output is nothing.

Convert the trace to a TeX file:

    ./holtrace2tex < prime.50 > prime.tex
    xelatex prime

The last theorem on the last page of the resulting `prime.pdf` is expected to say the following:
"Theorem (ONE_OR_PRIME). Fix _p_ ∈ ℕ. Then _p_ = 1 or _p_ is prime if and only if we have _n_ = 1 or _n_ = _p_ for each _n_ ∈ ℕ such that _n_ divides _p_."

Test some libraries used by the Python scripts:

    ./type.py
    ./term.py
    ./ptparse.py

The expected outputs are `test type` and `test term` and `test ptparse`.

Try the built-in suite of test traces:

    ./testtraces < testtraces.in | cmp - testtraces.exp

This takes 23 seconds
on [`rome1`](install.html).
The expected output is nothing.

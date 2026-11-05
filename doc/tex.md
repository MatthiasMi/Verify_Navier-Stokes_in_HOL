`holtrace2tex` is a command-line tool
that takes a trace on standard input,
and produces a LaTeX file on standard output
with statements of the named theorems.
Running the file through `xelatex` or `lualatex`
produces output such as the following:
"Theorem (ONE_OR_PRIME). Fix _p_ ∈ ℕ. Then _p_ = 1 or _p_ is prime if and only if we have _n_ = 1 or _n_ = _p_ for each _n_ ∈ ℕ such that _n_ divides _p_."

This was the original motivation for HOLTrace:
making it easier for readers of a paper to see what a theorem checked by HOL Light says.
Hopefully the tool is less error-prone than an author carrying out a similar conversion by hand.

This tool includes printouts of axioms and definitions,
but currently does not check for anything unusual in these.
Also, the tool's conversion from traced theorems to output is not necessarily invertible,
although the tool tries harder than HOL Light's default pretty-printer to avoid
[creating ambiguities](https://www.cs.ru.nl/F.Wiedijk/pubs/rap.pdf).

This tool is currently a pipeline from a tool written in C
to a tool written in Python.
The C tool is not necessarily safe to run on untrusted input;
a VM is recommended.

On [`rome1`](install.html),
`holtrace2tex` takes roughly 15 seconds (with some variability)
to convert the 37.6MB pruned trace of `hol.ml`
into a LaTeX file presenting 3520 theorem statements.
(There are 2981 named theorems in the original file;
`holtrace2tex` automatically expands multi-part theorems
into multiple theorems.)
The bottleneck is the Python tool;
the C tool is faster and will typically run on a separate core.

## Other tools

See the journal
[Formalized Mathematics](https://fm.mizar.org/)
for many examples of English proofs obtained from annotated Mizar proofs.
See
[Naproche](https://naproche.github.io/)
for a system to computer-check (some) English proofs.
See the
[bibliography](bib.html#english)
for more tools.

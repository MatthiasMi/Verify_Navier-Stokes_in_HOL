The command-line tool `holtrace2ml`
takes a trace on standard input
and prints OCaml commands on standard output.
The OCaml commands are essentially a series of HOL Light kernel calls,
and if they are provided as input to HOL Light
then they will verify proofs similarly to
[`holtrace_restore`](verify.html),
while printing out each lemma
(not just the named theorems).
These commands will also check definitions against existing definitions,
not allowing new definitions.

This tool is written in C
and is not necessarily safe to run on untrusted input.
A VM is recommended.

On [`rome1`](install.html),
`holtrace2ml` takes 1.58 seconds and 42MB RAM
to convert the 37.6MB pruned trace of `hol.ml`
into a 252MB file containing OCaml commands (`gzip -9` reduces this to 40.5MB).

Reading the commands into OCaml
is much less efficient than `holtrace_restore`.
(Also, OCaml's `.ml` parser produces `Uncaught exception: Stack overflow`
around 9MB of input.)
However, the verbosity can be useful for humans.

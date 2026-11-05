ProofTrace from Stanislas Polu
is one of the existing tools
to record HOL Light proofs, theorems, and theorem names.
One way to generate HOLTrace files is by
(1) generating traces in ProofTrace format and then
(2) using `pt2holtrace` from the HOLTrace package
to convert traces from ProofTrace format to HOLTrace format.
However,
[`holtrace-save`](save.html)
is more efficient than ProofTrace.

The HOLTrace package also provides
faster tools to convert the other way.
Converting ProofTrace files to HOLTrace files and back
will produce exactly the same ProofTrace files,
helping build confidence in both conversions,
although this is not the same as
[verifying](verify.html)
the recorded proofs and theorems.
These tools might also be useful for passing HOLTrace files
to tools that expect ProofTrace format as input.

Internally,
a ProofTrace `theorems` file has each theorem stated in full on a separate line,
with no sharing across lines.
A ProofTrace `proofs` file has each inference step on a separate line,
with previous theorems referred to by number,
but any term or type used in an inference step is spelled out in full on that line.
The HOLTrace [format](format.html)
instead allows previous types and terms to be reused as components
of new types, terms, theorems, and inferences.

## Generating traces in ProofTrace format

ProofTrace is included in the HOL Light distribution,
and documented in `ProofTrace/README`.
Running `git apply ProofTrace/fusion.ml.diff`
in the HOL Light directory modifies HOL Light to use ProofTrace.

If you are using HOL Light 3.0.0,
you will need to first [patch](pt300.diff)
the file `ProofTrace/fusion.ml.diff`.
HOL Light 3.1.0 does not need the patch.

The following timings were collected on [`rome1`](install.html)
with HOL Light 3.0.0.
Running through `hol.ml` with ProofTrace enabled
takes 198 seconds,
allocating 5787MB RAM and using 5522MB RAM.
Saving theorems with

    #use "ProofTrace/proofs.ml";;
    dump_theorems "prooftrace.theorems";;

then takes 236 minutes,
increasing RAM usage to about 22GB.
Saving proofs with

    dump_proofs "prooftrace.proofs";;

then takes 42 minutes,
similar RAM usage to before.
Saving theorem names with

    dump_names "prooftrace.names";;

then takes 77 seconds,
increasing RAM usage to about 26GB.

The resulting files are
0.1MB in `prooftrace.names`,
7643MB in `prooftrace.proofs` (`gzip -9` reduces this to 187MB),
and
41318MB in `prooftrace.theorems` (`gzip -9` reduces this to 449MB).

## Converting ProofTrace traces to HOLTrace format

To use `pt2holtrace`, and to check the conversion:

    ./pt2holtrace prooftrace.names prooftrace.proofs prooftrace.theorems > trace.50
    ./holtrace2pt-names < trace.50 | cmp - prooftrace.names
    ./holtrace2pt-proofs < trace.50 | cmp - prooftrace.proofs
    ./holtrace2pt-theorems < trace.50 | cmp - prooftrace.theorems

These tools are written in C/C++ (aside from some simple shell-script wrappers)
and are not necessarily safe to run on untrusted input.
A VM is recommended.

On [`rome1`](install.html),
running `pt2holtrace` takes 22 minutes and 2369MB RAM,
and produces a 229MB HOLTrace file (`gzip -9` reduces this to 47.9MB).
This is larger than what `holtrace-save` produces.
The 229MB includes various ProofTrace identifiers
that the `holtrace2pt` tools use to regenerate exactly the same ProofTrace files.
One effect of [pruning](prune.html) is to remove the ProofTrace identifiers.

`holtrace2pt-names`,
`holtrace2pt-proofs`,
and
`holtrace2pt-theorems`
take, respectively,
1.88 seconds,
48.49 seconds,
and 244.02 seconds
on `rome1`
to convert the aforementioned 229MB HOLTrace file
into the original `prooftrace.names`,
the original 7643MB `prooftrace.proofs`,
and the original 41318MB `prooftrace.theorems`.
Each tool uses 533MB RAM to process this file.

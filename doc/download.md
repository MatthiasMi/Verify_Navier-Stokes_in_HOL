To download and unpack the latest version of the HOLTrace package:

    wget -m https://holtrace.cr.yp.to/holtrace-latest-version.txt
    version=$(cat holtrace.cr.yp.to/holtrace-latest-version.txt)
    wget -m https://holtrace.cr.yp.to/holtrace-$version.tar.gz
    tar -xzf holtrace.cr.yp.to/holtrace-$version.tar.gz
    cd holtrace-$version

Then [install](install.html)
and [test](test.html).

### Archives (reverse chronological)

[`holtrace-20250714.tar.gz`](holtrace-20250714.tar.gz) [browse](holtrace-20250714.html)

Revamp `holtrace-verify` theorem checks
to first construct inference results
and then apply an equality test
(as in `holtrace-verify-simple` or the real HOL Light verifier).
This is roughly 1.5x slower but feels less error-prone.

Add tracing example at the top of `doc/save.md`,
and add structure to the rest of the page.

[`holtrace-20250703.tar.gz`](holtrace-20250703.tar.gz) [browse](holtrace-20250703.html)

Add easier-to-use `./install`, `./install+hashtt`, `./uninstall` scripts.

For `hashtt`, precompute hashes of theorems;
this noticeably speeds up tracing.

Revamp substitutions and alpha handling in `holtrace-verify`.
This now has no known completeness gaps,
but still needs extensive review
(and probably bug fixes) before it can be recommended.

Add a few more speedups to `holtrace-verify`.

Miscellaneous documentation improvements,
including measurements for HOL Light 3.1.0
instead of HOL Light 3.0.0
(except for ProofTrace).

[`holtrace-20250629.tar.gz`](holtrace-20250629.tar.gz) [browse](holtrace-20250629.html)

Add `ot2holtrace` tool to convert OpenTheory files to HOLTrace files.

Split `holtrace_restore_allownew` off of `holtrace_restore`.

Add `hashtt` to (optionally) patch HOL Light to precompute hashes of types and terms.

In `holtrace2ml`, reject theorem names starting with digits.

In `holtrace-verify`, allow non-tyvars to be substituted for tyvars.

Set higher recursion limits in various Python scripts.

Miscellaneous documentation improvements,
including many more citations.

[`holtrace-20250624.tar.gz`](holtrace-20250624.tar.gz) [browse](holtrace-20250624.html)

In `holtrace_restore`, allow alpha-equivalent definitions of existing constants.

Print matching+creating statistics for `holtrace_restore`.

Revamp `holtrace-verify` data structures for types and terms.
This improves code clarity and provides a slight speedup.

Print some profiling information for `holtrace-save`.

In `holtrace-save`,
actively remove `$HOLTRACE_SAVE`
to avoid potential bad interactions between a previous writer
and the new writer,
just in case the filename collides.

Add a few more test traces.

Miscellaneous documentation improvements.

[`holtrace-20250617.tar.gz`](holtrace-20250617.tar.gz) [browse](holtrace-20250617.html)

First release.

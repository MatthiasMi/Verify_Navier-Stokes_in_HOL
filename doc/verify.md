The recommended tools for verifying HOLTrace files
are `holtrace_restore` and its [variants](#variants).
There are also some [experimental](#experimental) tools.

`holtrace_restore` is a command available in a new HOL Light session
after HOLTrace is [installed](install.html).
Running

    holtrace_restore !theorems "holtrace-12345.50";;

will read and verify a trace file `holtrace-12345.50`.
Verification checks the following:

* The HOL Light kernel accepts the types, terms, and inferences in the trace.
* The resulting theorems match the theorem statements in the trace.
* If any name in the trace matches a name
  listed in `!theorems`
  then the theorem by that name in the trace matches the theorem by that name listed in `!theorems`.
* Axioms in the trace are already in the kernel's `axioms()` list.
* Constant definitions in the trace
  are already in the kernel's `definitions()` list.
* Type definitions in the trace
  are already in the `holtrace_typedef_thms()` list.

The `holtrace_typedef_thms()` list is not in the kernel;
it is a separate list maintained by HOLTrace,
whether or not traces are being
[saved](save.html) in this session.
The `!theorems` list is also outside the kernel but is built into HOL Light.

Verification remembers named theorems from the trace
(whether or not those are listed in `!theorems`),
and you can inspect those theorems
until they are erased by the next `holtrace_restore` run:

    holtrace_restore_name2thm "ONE_OR_PRIME";;

On [`rome1`](install.html),
restoring the pruned 37.6MB trace from `hol.ml`
takes 61 seconds after `./install+hashtt`
or 49 seconds after `./install`,
using about 400MB extra RAM.

## Variants {#variants}

There is a `holtrace_restore_allownew` variant
that will instead add any new axioms,
add any new constant definitions (failing if the constant name is used already),
and add any new type definitions (again failing on collisions).

With more work to add bindings for new names and any related syntax,
it should be possible to use `holtrace_restore` and `holtrace_restore_allownew`
to build a faster replacement for the normal loading of `hol.ml`
or of other HOL Light libraries.
There is also an even faster variant `holtrace_restore_skipthmchecks`
(for `hol.ml`: 28 seconds after `./install+hashtt`,
24 seconds after `./install`)
that should be suitable in this context:
this variant skips checking whether the theorems
match the theorem statements in the trace,
but still runs all of the same inferences through the HOL Light kernel.

## Experimental tools {#experimental}

There are currently two experimental command-line tools for verifying traces.
First,
`holtrace-verify-simple` _tries_ to verify traces
in essentially the same way that the HOL Light kernel does.
Beware that this tool is likely to have soundness bugs
that let it accept false theorems:
the tool has had vastly less review than the HOL Light kernel.

This tool is written in Python.
On [`rome1`](install.html),
`holtrace-verify-simple`
takes 58.38 seconds and 778MB RAM
given the 37.6MB pruned `hol.ml` trace as input.
It takes 5.98 seconds and 128MB RAM
given the 2.4MB pruned `ONE_OR_PRIME` trace as input.

Second,
`holtrace-verify` _tries_ to verify traces
and is more efficient than the HOL Light kernel.
Beware that this tool is even more likely than `holtrace-verify-simple`
to have soundness bugs
that let it accept false theorems:
the tool has had vastly less review than the HOL Light kernel
_and_ is more complicated than `holtrace-verify-simple`.
Furthermore, unlike `holtrace-verify-simple`,
this tool currently does not check whether a name refers to multiple theorems.

`holtrace-verify` is written in C
and is not necessarily safe to run on untrusted input.
A VM is recommended.

On [`rome1`](install.html),
`holtrace-verify`
takes 1.43 seconds and 259MB RAM
given the 37.6MB pruned HOLTrace file as input.
It takes 0.10 seconds and 20MB RAM
given the 2.4MB `ONE_OR_PRIME` pruned HOLTrace file as input.

The main mechanism to create new traces is `holtrace-save`,
after which the traces will typically be processed by
[`holtrace-prune`](prune.html) and further tools.
It is also possible to convert
[ProofTrace](prooftrace.html) files
(or
[OpenTheory](opentheory.html) files)
to HOLTrace format,
but generating HOLTrace format directly from HOL Light
is faster than generating ProofTrace format.

As an example of tracing,
if HOLTrace is
[installed](install.html)
and `tracedivstep.ml` contains

    needs "Divstep/make.ml";;
    needs "update_database.ml";;
    update_database();;
    holtrace_namethm !theorems;;

then the shell command

    env EDITOR=env ./holtrace-save \
    < tracedivstep.ml > tracedivstep.out 2>&1

will trace `Divstep/make.ml`.
The trace will be in a new file, separate from `tracedivstep.out`.

## Functions

`holtrace-save` is a command-line tool that starts HOL Light,
similarly to `hol.sh`:

    ./holtrace-save

The difference is that
`holtrace-save` saves the proofs and theorems in the HOL Light session.
These are saved in a file with a name of the form `holtrace-*.50`.
The `*` part includes the date and process ID,
so this name is very likely to be new,
but if it is not then the old file will be removed.

The trace file automatically accumulates proofs and theorems
as HOL Light is running.
It does not record names of theorems
until you run

    holtrace_namethm !theorems;;

using HOL Light's built-in list `!theorems` of name-theorem pairs.
The built-in list has only the HOL Light standard library by default,
but HOL Light has an `update_database()` command
that extends the list to cover further named theorems proven in this session.
There is also `holtrace_nameonethm` to handle a single name-theorem pair.

You can stop the tracing in this session:

    holtrace_stop();;

Restarting tracing within the same session is not supported.
Also, checkpointing is not supported in combination with tracing.

## Performance

Tracing slows down HOL Light,
as the following comparison illustrates:

* On [`rome1`](install.html),
  a normal `hol.sh` run takes 154 seconds to load HOL Light's standard library (`hol.ml`),
  allocating 758MB RAM and using 495MB RAM.

* A `holtrace-save` run after [`./install`](install.html)
  instead takes 2145 seconds,
  allocating 2196MB RAM and using 1930MB RAM.
  The resulting trace for `hol.ml` is
  82MB (`gzip -9` reduces this to 20.7MB).

* A `holtrace-save` run after [`./install+hashtt`](install.html)
  takes 522 seconds,
  allocating 2348MB RAM and using 2074MB RAM.

## Internals

The `holtrace-save` command is a simple shell script that picks a filename,
sets the `HOLTRACE_SAVE` environment variable to that filename,
and runs `hol.sh`.

The main work is then handled by the `Holtrace_save` module in `holtrace.ml`.
This module is a wrapper around the HOL Light kernel.
The wrapper redefines
`new_axiom`,
`new_basic_definition`,
`new_basic_type_definition`,
and the primitive inference rules such as `REFL`.
If `HOLTRACE_SAVE` is not set,
the wrapper keeps the original definitions,
except that it modifies `new_basic_type_definition`
to keep a `!holtrace_typedef_thms` list
used by the `Holtrace_restore` module for
[verification](verify.html).

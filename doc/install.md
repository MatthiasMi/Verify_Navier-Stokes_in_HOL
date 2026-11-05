Installation of HOLTrace is designed to allow normal HOL Light usage,
while adding options to HOL Light to
[save](save.html)
and
[verify](verify.html)
traces.
HOLTrace also includes separate command-line tools (written in C, C++, and Python) to handle traces;
currently these tools are usable only in the `holtrace-*` package directory.

## Prerequisites

The instructions below
have been tested on a server named `rome1` running Debian 11
with the `build-essential`, `opam`, `python3`, `pypy3`, `texlive-xetex`, and `xdot` packages installed,
in a user account that ran

    time opam init github git+https://github.com/ocaml/opam-repository.git --disable-sandboxing -a
    time opam switch create 5.2.0
    time opam install 'hol_light=3.1.0' -y
    eval `opam env`

to install HOL Light 3.1.0 with the OCaml 5.2 bytecode compiler.

The server has two AMD EPYC 7742 CPUs,
but timings reported in this documentation
use only one core (except for `holtrace2tex`).
Overclocking on the server has been (manually) disabled,
so the CPUs run at 2.245GHz.

## HOLTrace installation

Make sure you are in the `holtrace-*` package directory.

Run `make -j8` to compile the C/C++ tools.

Modify the HOL Light installation to include (1) `holtrace.ml`
and (2) a `hashtt` patch to precompute hashes of types, terms, and theorems:

    ./install+hashtt

Now [test](test.html) the installation.

## Alternative installations

Instead of `./install+hashtt`,
you can run `./install` to install just `holtrace.ml` without the `hashtt` patch.
Reasons to use `./install` rather than `./install+hashtt`:

* Compared to normal HOL Light and to `./install`,
  `./install+hashtt` increases CPU time for normal usage.
  The increase is typically between 10% and 30%.

* `./install` is less invasive than `./install+hashtt`
  and has a lower risk of bugs.

Reasons to use `./install+hashtt` rather than `./install`:

* `./install+hashtt` is much faster for saving traces than `./install` is.

You can run `./uninstall` to undo `./install+hashtt` or to undo `./install`.
If you want to switch between `./install+hashtt` and `./install`,
you don't have to run `./uninstall` in between.
Instead of switching an installation back and forth,
you can set up one account using `./install+hashtt` for saving traces,
and another account using `./install` for regular use (including replaying traces).

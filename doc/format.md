A HOLTrace file records a sequence of HOL Light proofs and theorems.
This document specifies version 1 of the file format.
This document is aimed at implementors writing tools to read or write HOLTrace files.

Files in HOLTrace format conventionally have filenames ending `.50`,
although this is not required.

This document allows multiletter italicized variable names.
Concatenation here does not mean multiplication.

## Overall structure

The file is a sequence of lines.
Each line consists of
printable ASCII characters (bytes 32 through 126)
followed by LF (byte 10).
There is always a terminating LF.
This is a binary file format: LF does not mean CR LF.

The first line is `HOLTrace 1` (followed by LF).
Each subsequent line begins with a 1-character tag
and continues with a sequence of zero or more arguments.
Arguments are encoded nonnegative integers,
except that some tags also have text as a final argument.

Integers are encoded in a way
that often appears negative or non-numeric.
Specifically:

* Each integer is encoded relative to an integer _n_ defined below by the context.

* Integer arguments _n_−1 through _n_−10 are encoded as
  the characters of `!@#$%^&*()` respectively:
  i.e., `!` means _n_−1, `@` means _n_−2, etc.

* An integer argument _i_ ≥ _n_
  is encoded as the uppercase base-32 representation of the integer _i_.
  Lowercase is not accepted as a synonym for uppercase.

* An integer argument _i_ ≤ _n_−11
  is encoded as the uppercase base-32 representation of the negative integer _i_−_n_
  (starting with a minus sign),
  or as the uppercase base-32 representation of the integer _i_
  if that has strictly fewer bytes.
  Lowercase is again not accepted as a synonym for uppercase.

For example, for encodings relative to _n_ = 10000:
argument `0` means 0;
`9` means 9;
`A` means 10;
`V` means 31;
`10` means 32;
`VV` means 1023;
`100` means 1024;
`8OG` means 8976;
`-VV` means 8977;
`-10` means 9968;
`-V` means 9969;
`-B` means 9989;
`)` means 9990;
`!` means 9999;
`9OG` means 10000;
`9OH` means 10001.

Spaces between arguments are suppressed in the file:
e.g., tag `b` with arguments `0` and `bool` is written as `b0bool`.
Exception:
if an encoded argument ends with one of the characters `0123456789ABCDEFGHIJKLMNOPQRSTUV`
and the next encoded argument begins with one of those characters (or a space)
then a space is inserted:
e.g., tag `e` with arguments `17` and `14` is written `e17 14`.

## Order

Some of the line definitions below
say that a line refers to other lines in the file.
Some references are via explicit arguments:
for example, the line `b2!!fun` specifies a type
by reference to the previously specified type.
Some references are implicit:
for example, a theorem is always proven by the most recent inference.

These references must be to lines
earlier in the file than the current line.
In other words,
circular references are prohibited,
and lines must appear in topological order.

A file writer normally deduplicates objects
(for example, there is only one `b0bool` line),
but a file reader does not rely on this.
For example,
correctly implementing the semantics of the `S` tag
relies on recognizing whether various objects are equal.

## Types

Each line with tag `a` or `b` is a "type line" specifying a type.
Each type is assigned an identifying number (a "type ID") for future reference,
counting from 0 upwards:
the first type line specifies type 0, i.e., the type having type ID 0;
the next type line specifies type 1;
the next type line after that specifies type 2;
etc.
To track these references,
the file reader mantains a type counter
that starts at 0 and that increments by 1 immediately after each type line.
(The file writer also does this.)

Specifically, type lines have the following meanings:

* `a` _text_:
  The type variable named _text_;
  HOL Light's `Tyvar`(_text_).
  For example, the line `aB` specifies a type variable named `B`;
  the line `a?328` specifies a type variable named `?328`.

* `b` _n_ _a_ ... _z_ _text_:
  The type named _text_ with arguments _a_,...,_z_;
  HOL Light's `Tyapp`(_text_,[_a_;...;_z_]).
  The first argument _n_ says the number of _a_ through _z_ arguments
  (0 or more, not necessarily 26);
  those arguments are type IDs specifying the `TyApp` arguments.
  These type IDs are encoded relative to the current type ID,
  so `!` refers to the most recent type line before this,
  and `@` refers to the most recent type line before that.
  The argument _n_ is encoded relative to the integer `-1`
  (so it never uses `!` etc.).

For example, the lines `b0bool` and `b2!!fun` and `b2@!fun`
(in that order) specify types
`bool` and `fun(bool,bool)` and `fun(bool,fun(bool,bool))`,
i.e.,
`bool` and `bool->bool` and `bool->bool->bool`.

## Terms

Each line with tag `c` or `d` or `e` or `f` is a "term line" specifying a term.
Each term is assigned an identifying number (a "term ID") for future reference,
counting from 0 upwards:
the first term line specifies term 0, i.e., the term having term ID 0;
the next term line specifies term 1;
the next term line after that specifies term 2;
etc.
To track these references,
the file reader mantains a term counter
that starts at 0 and that increments by 1 immediately after each term line.

The term counter is separate from the type counter.
For example,
if the first five lines have tags `b`, `b`, `b`, `c`, and `d` respectively,
then those lines specify type 0, type 1, type 2, term 0, and term 1 respectively.

Specifically, term lines have the following meanings:

* `c` _type_ _text_:
  The constant named _text_ whose type is ID _type_;
  HOL Light's `Const`(_text_,_type_).
  The encoding of _type_ is relative to the type counter,
  so `!` refers to the most recently specified type.
  For example,
  after the lines `b0bool` and `b2!!fun` and `b2@!fun` mentioned above,
  the line `c!=` specifies a constant named `=`
  of type `bool->bool->bool`.

* `d` _type_ _text_:
  The variable named _text_ whose type is ID _type_;
  HOL Light's `Var`(_text_,_type_).
  For example,
  after the lines `b0bool` and `b2!!fun` and `b2@!fun` and `c!=` mentioned above,
  the line `d#T` specifies a constant named `T` of type `bool`.

* `e` _type_ _fterm_ _xterm_: 
  The evaluation _fterm_(_xterm_);
  HOL Light's `Comb`(_fterm_,_xterm_).
  The _type_ argument is a type ID for the type of the value.
  The _fterm_ and _xterm_ arguments are term IDs
  for, respectively, the function being evaluated
  and the evaluation point.
  These term IDs are encoded relative to the term counter,
  so `!` refers to the most recently specified term before this line.
  For example,
  after the lines `c!=` and `d#T` mentioned above
  specifying terms `=` and `T`,
  the line `e@@!` specifies a term `Comb(=,T)`,
  i.e. =(T),
  meaning the value of the = function applied to T.
  The first `@` in this example refers to the value type, namely `bool->bool`.

* `f` _type_ _vterm_ _tterm_: 
  The function _vterm_ ↦ _tterm_;
  HOL Light's `Abs`(_vterm_,_tterm_).
  The _type_ argument is a type ID for the type of the function.
  The _vterm_ and _tterm_ arguments are term IDs
  for, respectively, the function variable and the function output.
  These term IDs are encoded relative to the term counter,
  For example,
  after `d0x` specifies a variable named `x` of type `bool`,
  the line `f$!!` specifies the term `Abs(x,x)`,
  i.e., x↦x,
  meaning the function that maps x to x.
  The `$` in this example refers to the function type, namely `bool->bool`.

## Theorems

Each line with tag `t` is a "theorem line" specifying a theorem.
Each theorem is assigned an identifying number (a "theorem ID") for future reference,
counting from 0 upwards:
the first theorem line specifies theorem 0, i.e., the theorem having theorem ID 0;
the next theorem line specifies theorem 1;
the next theorem line after that specifies theorem 2;
etc.
To track these references,
the file reader mantains a theorem counter
that starts at 0 and that increments by 1 immediately after each theorem line.
The theorem counter is separate from the term counter and the type counter.

Specifically, theorem lines have the following meaning:

* `t` _A_ ... _Z_ _conclusion_:
   The theorem saying that _conclusion_ follows
   given hypotheses _A_ through _Z_.
   The arguments (for the hypotheses and the conclusion)
   are term IDs encoded relative to the term counter.
   There must be at least one argument.

(Warning:
Often the literature on logic says "sequent" rather than "theorem",
reserving "theorem" for the case of no hypotheses,
although the conclusion can still be a term of the form `P implies Q`.
"Sequent" can also refer to a more general concept
allowing a disjunction of any number of conclusions.)

Some readers that check inferences
expect the hypotheses of each theorem
to be sorted in a particular order.
The order is outside the scope of this document
(but is connected to the reason for variables being labeled `d` instead of `v`).

As a complete example of a theorem,
here are annotations explaining
the first lines with lowercase-letter tags in a trace of `hol.ml`:

| line       | specifies | as
| ---------- | --------: | ---
| `b0bool`   |   type 0  | type bool
| `b2!!fun`  |   type 1  | type bool->bool
| `b2@!fun`  |   type 2  | type bool->bool->bool
| `c!=`      |   term 0  | constant = of type bool->bool->bool
| `d#T`      |   term 1  | variable T of type bool
| `e@@!`     |   term 2  | value =(T) of type bool
| `b2@#fun`  |   type 3  | type (bool->bool)->bool
| `b2#!fun`  |   type 4  | type (bool->bool)->(bool->bool)->bool
| `c!=`      |   term 3  | constant = of type (bool->bool)->(bool->bool)->bool
| `d%p`      |   term 4  | variable p of type bool
| `f$!!`     |   term 5  | function p↦p of type bool->bool
| `e@#!`     |   term 6  | value =(p↦p) of type (bool->bool)->bool
| `e%!@`     |   term 7  | value =(p↦p)(p↦p) of type bool
| `e%^!`     |   term 8  | value =(T)(=(p↦p)(p↦p)) of type bool
| `c%T`      |   term 9  | constant T of type bool
| `e$)!`     |   term 10 | value =(T) of type bool->bool
| `e%!$`     |   term 11 | value =(T)(=(p↦p)(p↦p)) of type bool
| `t!`       | theorem 0 | theorem =(T)(=(p↦p)(p↦p))

This theorem has no hypotheses.
Its conclusion is that the constant
T is equal to the equality of p↦p and p↦p,
i.e., that
T is true if and only if p↦p equals p↦p.

## Theorem names

Lines with tag `/` assign names to theorems.
Specifically, these lines have the following meaning:

* `/` _th_ _text_:
  Theorem _th_ has name _text_.
  Here _th_ is a theorem ID,
  encoded relative to the theorem counter.
  For example, `/!T_DEF`
  says that the most recent theorem is named `T_DEF`.
  The name _text_ is required to be nonempty.

A theorem can have multiple names,
but multiple theorems cannot have the same name.

## Inferences

Each line with an uppercase letter tag
(within the list of tags below)
is an "inference line" specifying an inference
(a way to prove a theorem).
Each inference is assigned an identifying number (an "inference ID") for future reference,
counting from 0 upwards:
the first inference line defines inference 0, i.e., the inference having inference ID 0;
the next inference line defines inference 1;
the next inference line after that defines inference 2;
etc.
To track these references,
the file reader mantains an inference counter
that starts at 0 and that increments by 1 immediately after each inference line.
The inference counter is separate from the theorem counter, the term counter, and the type counter.

For example,
in a trace of `hol.ml`,
the first lines include the lines shown above (`b0bool` etc.) with lowercase-letter tags,
and also one inference line that proves theorem 0,
namely a `C!T` line right before the `c%T` line.

Some readers of HOLTrace files ignore inferences;
inference lines can be simply removed from the file without affecting those readers.
However, file readers that check inferences expect each theorem
to be preceded in the file by at least one inference
and to be proven specifically by the last of those inferences.
(Non-inference lines can appear between that inference and the theorem.
There can also be further inferences that prove the same theorem in different ways.)

Except for the arguments listed as _text_ below,
all arguments on inference lines are nonnegative integers.
Arguments labeled _type_ are type IDs, encoded relative to the current type counter;
arguments labeled _term_ are term IDs, encoded relative to the current term counter;
arguments labeled _th_ (or _TH_) are theorem IDs, encoded relative to the current theorem counter.
For example, `!` refers to the most recently specified type or term or theorem respectively.

Ten of the inference tags are HOL Light's ten primitive inference rules:

* `B` _term_:
  HOL Light's `BETA` primitive inference rule.
  The input _term_
  is required to have the form (_x_↦_y_)(_x_),
  where _x_ is a variable and _y_ is a term.
  This rule generates the conclusion (_x_↦_y_)(_x_) = _y_ with no hypotheses.

* `E` _th_ _TH_:
  HOL Light's `MK_COMB` primitive inference rule.
  There are two input theorems _th_ and _TH_,
  which are required to have conclusions of the form _a_ = _b_ and _C_ = _D_
  respectively,
  with _a_ having suitable type to be evaluated at _C_.
  This rule generates the conclusion _a_(_C_) = _b_(_D_)
  from a merged list of hypotheses of the input theorems.

* `F` _term_ _th_:
  HOL Light's `ABS` primitive inference rule.
  The input theorem _th_
  is required to have a conclusion of the form _a_ = _b_.
  The input _term_
  is required to be a variable that does not appear free in any of the hypotheses of _th_.
  This rule generates the conclusion
  (_term_↦_a_) = (_term_↦_b_)
  from the same hypotheses as _th_.

* `P` _th_ _TH_:
  HOL Light's `EQ_MP` primitive inference rule.
  There are two input theorems _th_ and _TH_,
  which are required to have conclusions of the form _a_ = _b_ and _A_
  respectively,
  where _a_ is the same as _A_ modulo renaming of bound variables.
  This rule generates the conclusion _b_
  from a merged list of hypotheses of the input theorems.

* `R` _term_:
  HOL Light's `REFL` primitive inference rule.
  This rule generates the conclusion _term_ = _term_ with no hypotheses.

* `S` _th_ _oldterm_ _newterm_ ... _oldterm_ _newterm_:
  HOL Light's `INST` primitive inference rule
  for term substitution in the hypotheses and conclusion of theorem _th_,
  including renaming of bound variables if necessary.
  Beware that
  replacing term variable _v_ with _t_ and replacing term variable _w_ with _u_
  is written in HOLTrace format with arguments _v_ _t_ _w_ _u_,
  whereas the `INST` input is [_t_,_v_;_u_,_w_].
  There can be 0 substitutions, although this is not useful. 

* `T` _th_ _oldtype_ _newtype_ ... _oldtype_ _newtype_:
  HOL Light's `INST_TYPE` primitive inference rule
  for type substitution in the hypotheses and conclusion of theorem _th_,
  including renaming of bound variables if necessary.
  Beware that
  replacing type variable _v_ with _t_ and replacing type variable _w_ with _u_
  is written in HOLTrace format with arguments _v_ _t_ _w_ _u_,
  whereas the `INST_TYPE` input is [_t_,_v_;_u_,_w_].
  There can be 0 substitutions, although this is not useful. 

* `U` _term_:
  HOL Light's `ASSUME` primitive inference rule.
  The input _term_ is required to have type `bool`.
  This rule generates the conclusion _term_ from the hypothesis _term_.

* `V` _th_ _TH_:
  HOL Light's `TRANS` primitive inference rule.
  There are two input theorems _th_ and _TH_,
  which are required to have conclusions of the form _a_ = _b_ and _B_ = _C_
  respectively,
  where _b_ is the same as _B_ modulo renaming of bound variables.
  This rule generates the conclusion _a_ = _C_
  from a merged list of hypotheses of the input theorems.

* `Z` _th_ _TH_:
    HOL Light's `DEDUCT_ANTISYM_RULE` primitive inference rule.
    This rule generates the conclusion _p_ = _P_,
    where _p_ and _P_ are the conclusions of _th_ and _TH_ respectively,
    from a merged list of the following hypotheses:

    1. hypotheses of _th_ that are different from _P_
       modulo renaming of bound variables;
    2. hypotheses of _TH_ that are different from _p_
       modulo renaming of bound variables.

    For example,
    given theorems drawing the conclusion _p_ from the hypothesis _P_
    and drawing the conclusion _P_ from the hypothesis _p_,
    this rule generates the conclusion _p_ = _P_ with no hypotheses.

The other inference tags are as follows:

* `X` _term_:
    HOL Light's `new_axiom` (reported as `AXIOM` by ProofTrace).
    This rule generates the conclusion _term_ from no hypotheses,
    and adds _term_ to a list of axioms.

* `C` _term_ _text_:
    HOL Light's `new_basic_definition`
    (reported as `DEFINITION` by ProofTrace).
    The input _term_
    is required to have the form _c_ = _t_,
    where _c_ is a variable named _text_ of some type.
    (HOL Light's `new_basic_definition` takes a variable as input;
    beware that ProofTrace reports the output, which has a constant.)
    The term _t_ is required to have no free variables,
    and to have no type variables except for type variables appearing in _c_.
    This rule generates the conclusion _C_ = _t_ with no hypotheses,
    where _C_ is a constant named _text_.

* `A` _reptype_ _repterm_ _th_ _text_:
    HOL Light's `new_basic_type_definition`
    (reported as `TYPE_DEFINITION` by ProofTrace).
    The input theorem _th_
    is required to have a conclusion of the form _repterm_(_x_) with no hypotheses,
    where _x_ is some term of type _reptype_.
    The final argument _text_ is required to have
    the form
    `]`_abstypename_`]`_absname_`]`_repname_`]`_tyvar0_`]`_tyvar1_`]`...`]`,
    starting and ending with `]`.
    This produces a conclusion (with no hypotheses) of the form
    _absname_(_repname_(`a`)) = `a` where

    * `a` is a variable of type _reptype_,
    * _absname_ maps from _abstypename_(_tyvar0_,_tyvar1_,...) to _reptype_,
      and
    * _repname_ maps the other way.

    The names _tyvar0_, _tyvar1_, etc. must be the output of the following:
    take all type variables appearing in _repterm_,
    sort in lexicographic order,
    and remove duplicates.

    Each `A` line is assigned another counter, the "typedef ID",
    which starts at 0 and is incremented after each `A` line.

* `D` _typedef_:
    the other theorem from HOL Light's `new_basic_type_definition`
    (this theorem is reported on a second `TYPE_DEFINITION` line by ProofTrace).
    Here _typedef_ is a typedef ID of an `A` line.
    This produces a conclusion (with no hypotheses) of the form
    _repterm_(`r`) = (_repname_(_absname_(`r`)) = `r`) where `r` is a variable.

As an example of a type definition,
take type variables `A` and `B`,
and consider the set of functions
of the form (_a_ ↦ _b_ ↦ _a_=_x_ and _b_=_y_)
having type `A->B->bool`,
where _x_ and _y_ range over `A` and `B` respectively.
Types in HOL Light are nonempty,
so this set is nonempty.
Assume that the most recent theorem proves that this set is nonempty;
that the 430th term before now is this set,
with type `(A->B->bool)->bool`;
and that type 49 is the type `A->B->bool`.
Then the inference
`A1H-DE!]prod]ABS_prod]REP_prod]A]B]`
returns the theorem `ABS_prod(REP_prod(a)) = a`,
where `ABS_prod` maps `A->B->bool` to a new type `prod(A,B)`
and `REP_prod` maps the other way.

All constant names must be distinct strings,
where constant names mean the following:
`=`;
`@`;
_text_ in each `C` line;
_absname_ in each `A` line;
_repname_ in each `A` line.
All type names must be distinct strings,
where type names mean the following:
`bool`;
`fun`;
`ind`;
_abstypename_ in each `A` line.

## ProofTrace IDs

The tags `,` and `;`
are used to enable exact regeneration of a ProofTrace file
given an appropriate HOLTrace file.
Exact regeneration means a byte-for-byte match.
This often requires
multiple copies of the same theorem
and multiple proofs of the same theorem;
ProofTrace has its own set of IDs distinguishing these.
A file reader that handles ProofTrace-related tags
maintains a "ProofTrace counter"
that starts at 0 and that increments by 1 immediately after
each line with `,` or `;` as a tag.
The ProofTrace counter is separate from
the other counters mentioned above.

ProofTrace lines have the following meanings:

* `,` _newth_ _inf_ _ptth_ _ptTH_:
  The current ProofTrace ID includes inference _inf_ deriving theorem _newth_.
  Here _newth_ is a theorem ID (encoded relative to the theorem counter);
  _inf_ is an inference ID (encoded relative to the inference counter);
  _ptth_ and _ptTH_ are ProofTrace IDs (encoded relative to the ProofTrace counter)
  for theorems matching the _th_ and _TH_ inputs to the inference rule.
  The _ptTH_ argument is omitted if the inference rule does not involve _TH_,
  and the _ptth_ argument is omitted if the inference rule does not involve _th_.
  For example, the line `,!!` means that the current ProofTrace ID
  includes the most recent inference deriving the most recent theorem,
  via an inference rule not involving _TH_ or _th_.
  
* `;`:
  No effect other than increasing the ProofTrace ID counter.
  (ProofTrace IDs are mostly sequential starting from 0
  but skip numbers on certain occasions.)

Outside the context of generating ProofTrace outputs
(to check against ProofTrace or to interoperate with tools that expect ProofTrace format),
lines with tag `,` (or `;`) are not useful and can be removed,
typically saving more than half of the space in a HOLTrace file
(and speeding up further processing).
File readers not using ProofTrace tags also do not need the inference counter,
although an "inference initialized" flag
is useful for checking each theorem against the most recent inference.

A ProofTrace-to-HOLTrace converter
generates `/` before `,`
when it sees an ID that has a theorem name.
A HOLTrace-to-ProofTrace converter
treats `/` as applying to the current ProofTrace ID.

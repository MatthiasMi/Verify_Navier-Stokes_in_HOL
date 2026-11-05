This bibliography is split into the following sections:
[some proof assistants](#assistants);
computer-checked [proofs in English](#english);
more [format converters and verifiers](#format).
Each section lists references in chronological order.

## Some proof assistants {#assistants}

[1968 de Bruijn](https://research.tue.nl/en/publications/automath-a-language-for-mathematics):
Automath.

[1973 Trybulec](https://fm.mizar.org/mizar-archive/1975/Ilmenau-Heft33.pdf):
[Mizar](https://mizar.org).

1986 Constable–Allen–Bromley–Cleaveland–Cremer–Harper–Howe–Knoblock–Mendler–Panangaden–Sasaki–Smith:
[Nuprl](https://nuprl-web.cs.cornell.edu/).

[1993 Gordon–Melham](https://www.cs.ox.ac.uk/tom.melham/pub/Gordon-1993-ITH.html):
HOL88.

[1996 Harrison](https://www.cl.cam.ac.uk/~jrh13/papers/demo.html):
[HOL Light](https://hol-light.github.io/).

1997 Megill:
[Metamath](https://us.metamath.org/).

[2002 Nipkow–Paulson–Wenzel](https://isabelle.in.tum.de/dist/Isabelle/doc/tutorial.pdf):
[Isabelle](https://isabelle.in.tum.de/)/HOL.

[2004 Bertot–Castéran](https://link.springer.com/book/10.1007/978-3-662-07964-5):
Coq, subsequently renamed [Rocq](https://rocq-prover.org/).

[2008 Slind–Norrish](https://trustworthy.systems/publications/):
[HOL4](https://hol-theorem-prover.org/).

[2015 Bancerek–Bylinski–Grabowski–Kornilowicz–Matuszewski–Naumowicz–Pak–Urban](https://link.springer.com/chapter/10.1007/978-3-319-20615-8_17):
survey of the Mizar language.

[2018 Bancerek–Bylinski–Grabowski–Kornilowicz–Matuszewski–Naumowicz–Pak](https://mizar.uwb.edu.pl/people/romat/doi-10.1007-s10817-017-9440-6.pdf):
survey of the Mizar math library.

[2020 The mathlib community](https://arxiv.org/abs/1910.09336):
the Lean math library.

[2021 de Moura–Ullrich](https://lean-lang.org/papers/lean4.pdf):
the Lean 4 [language](https://lean-lang.org/).

[2023 Carneiro](https://arxiv.org/abs/2304.08391):
an open-source [reimplementation](https://github.com/digama0/mizar-rs)
of Mizar.

## Computer-checked proofs in English {#english}

[1976 Chester](https://www.sciencedirect.com/science/article/abs/pii/0004370276900072):
a program EXPOUND
to convert inference sequences into English proofs.

[1987 Felty–Miller](https://www.site.uottawa.ca/~afelty/dist/proof87.pdf):
the χ-proof system,
with extra information attached to inferences to improve presentation as English proofs.

[1990 Cohn](https://web.archive.org/web/20060101004342/http://www.cl.cam.ac.uk/%7Emjcg/papers/AvraAccountsPaper.ps.gz):
converting tactic sequences for HOL into English proofs.

[1995 Coscoy–Kahn–Thery](https://inria.hal.science/inria-00074217/file/RR-2459.pdf):
a converter (part of ctcoq)
to convert Coq inference sequences into English proofs,
including various compression mechanisms.

[1997 Coscoy](https://link.springer.com/chapter/10.1007/BFb0052156):
further compression mechanisms.

[1999 Holland-Minkley–Barzilay–Constable](https://www.cs.columbia.edu/nlp/papers/1999/holland-minkley_al_99.pdf):
converting tactic sequences for Nuprl into English proofs.

[2005 Cairns–Gow](https://www-users.york.ac.uk/paul.cairns/pubs/Maze.pdf):
a tool Maze
to convert annotated Mizar proofs into English proofs.

[2009 Cramer–Fisseni–Koepke–Kühlwein–Schröder–Veldman](https://ids-pub.bsz-bw.de/frontdoor/deliver/index/docId/8193/file/Cramer_Fisseni_Koepke_Kuehlwein_Schroeder_Veldman_The_Naproche_Project_2010.pdf):
Naproche,
a system to computer-check some English proofs.

[2013 Ganesalingam–Gowers](https://arxiv.org/pdf/1309.4501):
a tool to generate some types of proofs
and convert them into English proofs.

[2017 Bedford](https://arxiv.org/pdf/1712.03894):
a tool Coqatoo
to convert tactic sequences for Coq into English proofs.

## More format converters and verifiers {#format}

[1993 Wong](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-306.html):
a tool to record HOL88 inferences in a text format.

[1995 Gordon–Herbert–Hale–Harrison–Wong–von Wright](https://www.csl.sri.com/papers/proofchecker/):
a tool to verify inferences recorded in that text format,
and a proof that the algorithms used in the tool
match a simpler description of inference verification.

[1996 Wong](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-389.html):
a faster tool to verify inferences recorded in that text format.
See also the [journal version](https://link.springer.com/article/10.1023/A:1008643803725).

[1996 Howe](https://nuprl-web.cs.cornell.edu/documents/Howe/ImportingMathematics.pdf):
a tool to import HOL Light statements (not proofs) into Nuprl.

2000 Denney:
a prototype tool
to import some HOL Light inferences (not definitions or type instantiations) into Coq.

[2006 Obua–Skalberg](https://static.aminer.org/pdf/PDF/000/071/753/importing_hol_into_isabelle_hol.pdf):
ProofRecording to record HOL Light traces (this is maintained as part of the HOL Light distribution);
a similar tool for HOL4;
and a tool to replay ProofRecording files in Isabelle.
There are (as in Wong's format) two types of ProofRecording traces:
"BASIC" with only primitive inferences,
and "EXTENDED" with some higher-level inferences such as `SYM`.

[2006 McLaughlin](https://www.cs.cmu.edu/~seanmcl/papers/McLaughlin-IJCAR-2006.pdf):
a tool translating thousands of Isabelle proofs into HOL Light proofs.

[2010 Keller–Werner](https://inria.hal.science/inria-00520604/document):
another HOL Light trace format to ProofRecording,
and a tool to replay traces in that format inside Coq.

[2011 Hurd](https://gilith.com/papers/stdlib.pdf):
OpenTheory as a format for HOL traces.
The [OpenTheory home page](https://www.gilith.com/opentheory/)
links to tools to record inferences
from HOL Light, HOL4, and ProofPower in OpenTheory format,
and tools to replay inferences in HOL Light, HOL4, ProofPower, and Isabelle.
(The HOLTrace package now provides another way to replay OpenTheory traces in HOL Light,
since it has a [converter](opentheory.html)
from OpenTheory format to HOLTrace format.)

[2013 Kaliszyk–Krauss](http://cl-informatik.uibk.ac.at/users/cek/docs/kaliszyk-itp13.pdf):
a more efficient format for HOL Light traces,
and a tool to replay the traces in Isabelle.

[2021 Kohlhase–Rabe](https://link.springer.com/content/pdf/10.1007/s10817-021-09604-0.pdf):
survey of work on exporting libraries from Coq, HOL Light, IMPS, Isabelle, Mizar, and PVS
to a unified language, MMT.

[2022 Abrahamsson](https://core.ac.uk/download/pdf/543614031.pdf):
an OpenTheory verifier (on top of Candle, on top of CakeML)
with a proof that the machine code for the verifier
allows only valid HOL theorems.

[2024 Blanqui](https://inria.hal.science/hal-04613926/):
an export mechanism from HOL Light to Dedukti,
and an import mechanism from Dedukti to Coq.

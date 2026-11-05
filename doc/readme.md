The HOLTrace package
is a collection of tools
for processing traces of a HOL Light session.
Examples of current applications:

* Converting a HOL Light theorem statement
    into Standard Do-Not-Annoy-The-Journal-Referees style
    to include in a paper,
    rather than writing a separate theorem statement by hand.
    (See the [`holtrace2tex`](tex.html) tool.)

* Loading a theorem more quickly into a HOL Light session,
    while still having the proof checked by the real HOL Light kernel.
    (See the [`holtrace_restore`](verify.html) tool.)

* Checking a claimed HOL Light proof script from an untrusted source,
    removing the ability of the script to
    [exploit](https://jfr.unibo.it/article/view/4576)
    the general-purpose programming language surrounding HOL Light.
    ([Trace](save.html) HOL Light running the script in a temporary VM;
    copy the trace out of the VM;
    [verify](verify.html) the trace outside the VM.
    The script can take over the VM and put arbitrary data into the trace,
    but cannot fool the verification.)
    
The HOLTrace tools use
[HOLTrace format](format.html),
a new trace format that is more efficient than various earlier formats.

Latest release: [20250714](download.html).

# The HOL Light theorem prover

[HOL Light](https://github.com/jrh13/hol-light) is a computer program written
by [John Harrison](https://www.cl.cam.ac.uk/~jrh13/hol-light/) to help users
prove interesting mathematical theorems completely formally in higher order logic.
It sets a very exacting standard of correctness, but provides a
number of automated tools and pre-proved mathematical theorems (e.g. about
arithmetic, basic set theory and real analysis) to save the user work. It is
also fully programmable, so users can extend it with new theorems and inference
rules without compromising its soundness. There are a number of versions of
[HOL](http://www.cl.cam.ac.uk/Research/HVG/HOL/), going back to
[Mike Gordon](http://www.cl.cam.ac.uk/users/mjcg)'s work in the early 80s.
Compared with other HOL systems, HOL Light uses a much simpler logical core
and has little legacy code, giving the system a simple and uncluttered feel.
Despite its simplicity, it offers theorem proving power comparable to, and in
some areas greater than, other versions of HOL, and has been used for some
significant industrial-scale verification applications.


## How to install

[HOL Light](https://github.com/jrh13/hol-light) is hosted on Github so you can
get the sources from the Github repository. You can browse individual source
files online, or check out all the code using git. The following command will
copy all the code from the Github repository into a new directory hol-light
(assumed not to exist already):

```bash
git clone https://github.com/jrh13/hol-light.git
```

HOL Light is written in Objective CAML (OCaml), and it should work with any
reasonably recent version.
See the [README](https://github.com/jrh13/hol-light/blob/master/README) file
in the distribution for detailed installation instructions.


## Available documentation and resources

- [Tutorial](tutorial.pdf), which tries to teach HOL Light through examples.
- Reference Manual available as [online crosslinked HTML](references/HTML/reference.html) or
  as [one PDF file](references/reference.pdf)
- Quick Reference Guide compiled by Freek Wiedijk ([text](holchart/holchart.txt), [PDF](holchart/holchart.pdf), [Postscript](holchart/holchart.ps), [DVI](holchart/holchart.dvi), [LaTeX](holchart/holchart.teX))
- Summary of many HOL source files, written by Carl Witty ([text](summary.txt))
- [A VS Code extension for HOL Light](https://marketplace.visualstudio.com/items?itemName=monadius.hol-light-simple), written by Alexey Solovyev.
- [Notes for beginners](https://github.com/aqjune/hol-light-materials/tree/main), written by Juneyoung Lee

## Applications of HOL Light

- The [Flyspeck project](https://github.com/flyspeck/flyspeck) to machine-check [Tom Hales](https://www.mathematics.pitt.edu/people/thomas-hales)'s proof of the Kepler conjecture.
Tom has already proved the Jordan Curve Theorem and other relevant results in HOL Light.
- The [s2n-bignum project](https://github.com/awslabs/s2n-bignum/) of Amazon Web Services. This is a collection of bignum arithmetic routines
designed for cryptographic applications.
- Formalization of floating-point arithmetic, and the formal verification of several floating-point algorithms at Intel.
See [this paper](http://www.cl.cam.ac.uk/~jrh13/papers/iday.html) for a quick summary and more references,
and [this one](http://www.cl.cam.ac.uk/~jrh13/papers/sfm.html) for a more detailed presentation.
- Many other mathematical results of varying degrees of difficulty have been verified in HOL Light.
See for example the HOL Light entries on the [Formalizing 100 Theorems page](https://www.cs.ru.nl/~freek/100/).


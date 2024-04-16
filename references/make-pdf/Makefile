# Makefile for the HOL Light reference manual
# Run latex twice to get the table of contents

reference.dvi: ack.tex contents.tex entries.tex ref-macros.tex title.tex    \
                commands.tex cover_info.tex preface.tex theorems.tex \
               theorems-intro.tex entries-intro.tex reference.tex;      \
               latex reference.tex; latex reference.tex

reference.pdf: reference.dvi; pdflatex reference.tex

entries.tex: ../hol-light/Help/*.hlp; rm -f entries.tex; sh ./doc-to-tex ./doc-to-tex.sed ../hol-light/Help entries.tex

all: reference.dvi reference.pdf

clean:; rm -f *.aux *.log *.idx *.dvi *.ps *.pdf *.toc entries.tex

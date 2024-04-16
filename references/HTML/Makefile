# Makefile for the HOL Light HTML documentation.

all: entries index;
entries: ../Help/.*.hlp ../Help/*.hlp; ./make-all-html
index: ../Help/.*.hlp ../Help/*.hlp; ./make-all-index

clean:; rm -f .??*.html *.html

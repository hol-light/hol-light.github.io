#!/bin/bash

if [[ "$OSTYPE" == "darwin"* ]]; then
  echo "On Mac, building ne_text_editor will fail (at least under the configuration in this script)"
  echo "Please use Linux. Or can the build config be fixed..?"
  exit 1
fi

set -e

rm -rf hol-light
git clone https://github.com/jrh13/hol-light.git

rm -rf ne_text_editor

tar -xvf ne_text_editor.tar.gz

cd ne_text_editor
cd src

if [[ "$OSTYPE" == "darwin"* ]]; then
  sed -i "" 's/TARGET = nenewp/TARGET = nenew/g' Makefile
else
  sed -i 's/TARGET = nenewp/TARGET = nenew/g' Makefile
fi

make TARGET=nenew Linux_gcc
make

cd ../..

tar -xvf HTML-script.tar.gz
cd HTML
if [[ "$OSTYPE" == "darwin"* ]]; then
  sed -i "" 's@ne@../../ne_text_editor/src/nenew@g' make-html
  sed -i "" 's@^ne@../../ne_text_editor/src/nenew@g' make-all-index
else
  sed -i 's@ne@../../ne_text_editor/src/nenew@g' make-html
  sed -i 's@^ne@../../ne_text_editor/src/nenew@g' make-all-index
fi
cd ..
mv HTML hol-light/
cd hol-light/HTML
set +e
make
make index

cd ../..
mv hol-light/HTML .

cd make-pdf
make all
make all
mv reference.pdf ..

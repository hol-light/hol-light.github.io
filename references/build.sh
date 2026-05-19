#!/bin/bash

set -e

rm -rf hol-light
git clone https://github.com/jrh13/hol-light.git

python3 hlp2html.py hol-light/Help HTML
cp hollightref.css HTML/

cd make-pdf
if [[ "$OSTYPE" == "darwin"* ]]; then
  sed -i "" "s/For .* revision/For $(date +'%-d %B %Y') revision/" title.tex
else
  sed -i "s/For .* revision/For $(date +'%-d %B %Y') revision/" title.tex
fi
make all
make all
mv reference.pdf ..

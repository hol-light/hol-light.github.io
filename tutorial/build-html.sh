#!/bin/bash
# Build HTML from tutorial.tex using make4ht
#
# Prerequisites: make4ht (part of TeX Live)
# Output: HTML/tutorial.html

set -e

OUTPUT_DIR="HTML"
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

echo "Converting LaTeX to HTML with make4ht..."
make4ht -u -d "$OUTPUT_DIR" tutorial.tex "mathml,fn-in" "" "" "-interaction=nonstopmode"

# Remove intermediate footnote files
rm -f "$OUTPUT_DIR"/tutorial[0-9]*.html

echo ""
echo "=== Build complete ==="
echo "Output: $OUTPUT_DIR/tutorial.html"

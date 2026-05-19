#!/usr/bin/env python3
"""Convert HOL Light .hlp files to HTML."""

import os
import re
import sys
from pathlib import Path

FILENAME_MAP = {
    "=?": ".eqq",
    ">=?": ".geq",
    ">?": ".gtq",
    "++": ".joinparsers",
    "<=?": ".leq",
    "<?": ".ltq",
    "|||": ".orparser",
    ">>": ".pipeparser",
    "|=>": ".singlefun",
    "--": ".upto",
    "|->": ".valmod",
    "insert'": "insert_prime",
    "mem'": "mem_prime",
    "subtract'": "subtract_prime",
    "union'": "union_prime",
    "unions'": "unions_prime",
    "ALPHA": "ALPHA_UPPERCASE",
    "CHOOSE": "CHOOSE_UPPERCASE",
    "CONJUNCTS": "CONJUNCTS_UPPERCASE",
    "EXISTS": "EXISTS_UPPERCASE",
    "HYP": "HYP_UPPERCASE",
    "INSTANTIATE": "INSTANTIATE_UPPERCASE",
    "INST": "INST_UPPERCASE",
    "MK_BINOP": "MK_BINOP_UPPERCASE",
    "MK_COMB": "MK_COMB_UPPERCASE",
    "MK_CONJ": "MK_CONJ_UPPERCASE",
    "MK_DISJ": "MK_DISJ_UPPERCASE",
    "MK_EXISTS": "MK_EXISTS_UPPERCASE",
    "MK_FORALL": "MK_FORALL_UPPERCASE",
    "REPEAT": "REPEAT_UPPERCASE",
    "index": "index-function",
}


def name_to_html_filename(name):
    """Convert a HOL Light identifier to its HTML filename (without .html)."""
    return FILENAME_MAP.get(name, name)


def html_escape(text):
    """Escape &, <, > for HTML."""
    text = text.replace("&", "&amp;")
    text = text.replace("<", "&lt;")
    text = text.replace(">", "&gt;")
    return text


def process_inline_markup(text):
    """Process {braces} into SPAN markup, handling nested {{literal braces}}.

    No <> escaping is done here (matching the original ne editor which only
    escapes <> on the TYPE line). The & has already been escaped.
    Works on full multi-line text to handle braces spanning lines.
    """
    # First, protect double braces
    text = text.replace("{{", "\x00LBRACE\x00")
    text = text.replace("}}", "\x00RBRACE\x00")

    # Replace {content} with <SPAN CLASS=BRKT>content</SPAN>
    text = re.sub(r'\{([^}]*)\}', r'<SPAN CLASS=BRKT>\1</SPAN>', text, flags=re.DOTALL)

    # Restore literal braces
    text = text.replace("\x00LBRACE\x00", "{")
    text = text.replace("\x00RBRACE\x00", "}")
    return text


def process_em(text):
    r"""Process {\em text} into <i>text</i>.

    Called after brace→SPAN processing, so these are literal braces
    that survived (from {{ }} protection).
    """
    text = re.sub(r'\{\\em\s+([^}]*)\}', r'<i>\1</i>', text)
    return text


def process_latex_lists(text):
    """Process \\begin{itemize}/enumerate and \\item."""
    text = text.replace("\\begin{itemize}", "<ul>")
    text = text.replace("\\end{itemize}", "</ul>")
    text = text.replace("\\begin{enumerate}", "<ol>")
    text = text.replace("\\end{enumerate}", "</ol>")
    text = re.sub(r'\\item ', '<li> ', text)
    return text


def process_noindent(text):
    """Remove \\noindent."""
    return text.replace("\\noindent ", "")


def make_seealso_link(name):
    """Create an HTML link for a SEE ALSO reference."""
    name = name.strip()
    if not name:
        return ""
    html_name = name_to_html_filename(name)
    display = html_escape(name)
    return f'<A href="{html_name}.html">{display}</A>'


def check_braces(text, filename, section_name):
    """Warn about mismatched braces in non-example text."""
    # Remove double braces (they are literal/escaped)
    cleaned = text.replace("{{", "").replace("}}", "")
    # Remove example blocks (standalone { and } on their own lines)
    lines = cleaned.split('\n')
    filtered = []
    in_example = False
    for line in lines:
        if not in_example and line.rstrip() == '{':
            in_example = True
        elif in_example and line.rstrip() == '}':
            in_example = False
        elif not in_example:
            filtered.append(line)
    cleaned = '\n'.join(filtered)

    depth = 0
    for i, ch in enumerate(cleaned):
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth < 0:
                lineno = cleaned[:i].count('\n') + 1
                print(f"  WARNING: {filename}: unmatched '}}' in "
                      f"\\{section_name} (line {lineno})",
                      file=sys.stderr)
                return
    if depth > 0:
        print(f"  WARNING: {filename}: unmatched '{{' in "
              f"\\{section_name} ({depth} unclosed)",
              file=sys.stderr)


def convert_hlp_to_html(hlp_content, filename="<unknown>"):
    """Convert .hlp file content to HTML."""
    lines = hlp_content.split('\n')

    # Parse sections
    sections = []
    current_section = None
    current_lines = []

    for line in lines:
        if line.startswith('\\DOC '):
            current_section = 'DOC'
            current_lines = [line[5:]]
        elif line.startswith('\\TYPE'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'TYPE'
            current_lines = [line[6:] if len(line) > 6 else '']
        elif line.startswith('\\SYNOPSIS'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'SYNOPSIS'
            current_lines = []
        elif line.startswith('\\KEYWORDS'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'KEYWORDS'
            current_lines = []
        elif line.startswith('\\LIBRARY'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'LIBRARY'
            current_lines = []
        elif line.startswith('\\DESCRIBE'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'DESCRIBE'
            current_lines = []
        elif line.startswith('\\EXAMPLE'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'EXAMPLE'
            current_lines = []
        elif line.startswith('\\FAILURE'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'FAILURE'
            current_lines = []
        elif line.startswith('\\USES'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'USES'
            current_lines = []
        elif line.startswith('\\COMMENTS'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'COMMENTS'
            current_lines = []
        elif line.startswith('\\SEEALSO'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = 'SEEALSO'
            current_lines = []
        elif line.startswith('\\ENDDOC'):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = None
            current_lines = []
        else:
            current_lines.append(line)

    if current_section:
        sections.append((current_section, '\n'.join(current_lines)))

    # Build HTML output
    out = []
    out.append('<link rel = "stylesheet" href = "hollightref.css">')
    out.append('<A href="index.html">Go back to the list</A><BR><BR>')

    for section_name, content in sections:
        if section_name == 'DOC':
            continue
        elif section_name == 'KEYWORDS':
            continue
        elif section_name == 'LIBRARY':
            continue
        elif section_name == 'TYPE':
            # TYPE content is like {NAME : type}
            # There may be trailing body text if no \DESCRIBE follows
            parts = content.strip().split('\n\n', 1)
            type_text = parts[0].strip()
            if type_text.startswith('{') and type_text.endswith('}'):
                type_text = type_text[1:-1]
            type_text = html_escape(type_text)
            out.append(f'<DIV class=TYPE><PRE>{type_text}</PRE></DIV>')
            out.append('<DL>')
            out.append('')
            # If there's body text after the type declaration, emit it
            if len(parts) > 1 and parts[1].strip():
                trailing = parts[1]
                trailing = trailing.replace("&", "&amp;")
                trailing = process_inline_markup(trailing)
                trailing = process_noindent(trailing)
                trailing = process_latex_lists(trailing)
                trailing = process_em(trailing)
                out.append(trailing)
        elif section_name == 'SEEALSO':
            out.append('<P><DT><SPAN CLASS=FIELD-NAME>SEE ALSO</SPAN><DD>')
            # Process each line, replacing each name with its link
            # while preserving the original line structure
            for sa_line in content.split('\n'):
                if not sa_line.strip():
                    continue
                # Split line by comma, keeping structure
                # Each token is "name" possibly with trailing period/comma
                parts = re.split(r'(,\s*)', sa_line)
                result_parts = []
                for part in parts:
                    # If it's a separator, keep it
                    if re.match(r'^,\s*$', part):
                        result_parts.append(part)
                        continue
                    # Strip trailing period
                    stripped = part.strip().rstrip('.')
                    has_dot = part.strip().endswith('.')
                    if stripped:
                        link = make_seealso_link(stripped)
                        result_parts.append(link + ('.' if has_dot else ''))
                    else:
                        result_parts.append(part)
                out.append(''.join(result_parts))
            out.append('')
        else:
            section_labels = {
                'SYNOPSIS': 'SYNOPSIS',
                'DESCRIBE': 'DESCRIPTION',
                'EXAMPLE': 'EXAMPLE',
                'FAILURE': 'FAILURE CONDITIONS',
                'USES': 'USES',
                'COMMENTS': 'COMMENTS',
            }
            label = section_labels.get(section_name, section_name)
            out.append(f'<P><DT><SPAN CLASS=FIELD-NAME>{label}</SPAN><DD>')

            # Check for mismatched braces in the raw content
            check_braces(content, filename, section_name)

            # Process the content following the original .escript order:
            # 1. Escape & globally
            text = content.replace("&", "&amp;")

            # 2. Handle example blocks (standalone { on its own line)
            #    Split into segments: example vs non-example
            segments = []
            current_lines = []
            in_example = False
            for line in text.split('\n'):
                if not in_example and line.rstrip() == '{':
                    # Flush non-example lines
                    if current_lines:
                        segments.append(('text', '\n'.join(current_lines)))
                        current_lines = []
                    in_example = True
                    current_lines.append('<DIV class=XMPL><PRE>')
                elif in_example and line.rstrip() == '}':
                    current_lines.append('</PRE></DIV>')
                    segments.append(('example', '\n'.join(current_lines)))
                    current_lines = []
                    in_example = False
                elif in_example:
                    # In examples: & already escaped, {{ }} → { }
                    line = line.replace("{{", "{").replace("}}", "}")
                    current_lines.append(line)
                else:
                    current_lines.append(line)
            if current_lines:
                segments.append(('text', '\n'.join(current_lines)))

            # 3. Process non-example segments
            result_parts = []
            for seg_type, seg_text in segments:
                if seg_type == 'example':
                    result_parts.append(seg_text)
                else:
                    # Inline brace markup (handles multi-line spans)
                    seg_text = process_inline_markup(seg_text)
                    # Process \noindent
                    seg_text = process_noindent(seg_text)
                    # Process LaTeX lists (after brace restoration)
                    seg_text = process_latex_lists(seg_text)
                    # Process {\em ...} → <i>...</i>
                    seg_text = process_em(seg_text)
                    result_parts.append(seg_text)

            out.append('\n'.join(result_parts))

    out.append('')
    out.append('</DL>')
    return '\n'.join(out) + '\n'


def generate_index(help_dir, output_file):
    """Generate the index.html index file."""
    import subprocess
    # Use sort -f for locale-consistent ordering matching the original
    result = subprocess.run(
        ['bash', '-c', f'ls -a "{help_dir}" | grep ".hlp$" | sort -f'],
        capture_output=True, text=True)
    hlp_files = [f for f in result.stdout.strip().split('\n') if f]

    entries = []
    for f in hlp_files:
        if not f.endswith('.hlp'):
            continue
        hlp_path = os.path.join(help_dir, f)
        with open(hlp_path, 'r') as fh:
            for line in fh:
                if line.startswith('\\DOC '):
                    doc_name = line[5:].strip()
                    base = f[:-4]  # remove .hlp
                    html_name = name_to_html_filename(base)
                    display = html_escape(doc_name)
                    entries.append(f'<li> <a href="{html_name}.html">{display}</a>')
                    break

    with open(output_file, 'w') as fh:
        fh.write('<ul>\n')
        for entry in entries:
            fh.write(entry + '\n')
        fh.write('</ul>\n')


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <help_dir> <output_dir>")
        sys.exit(1)

    help_dir = sys.argv[1]
    output_dir = sys.argv[2]

    os.makedirs(output_dir, exist_ok=True)

    # Convert all .hlp files
    hlp_files = sorted(os.listdir(help_dir))
    for f in hlp_files:
        if not f.endswith('.hlp'):
            continue
        hlp_path = os.path.join(help_dir, f)
        base = f[:-4]
        html_name = name_to_html_filename(base)
        out_path = os.path.join(output_dir, html_name + '.html')

        with open(hlp_path, 'r') as fh:
            content = fh.read()

        html = convert_hlp_to_html(content, f)

        with open(out_path, 'w') as fh:
            fh.write(html)

        print(f"Making HTML for {f}")

    # Generate index
    print("Making HTML for the index...")
    generate_index(help_dir, os.path.join(output_dir, 'index.html'))


if __name__ == '__main__':
    main()

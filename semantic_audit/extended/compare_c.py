"""Add strict C11 compiler observations to an existing baka audit run.

Reference inputs spell aggregate type uses with struct tags because baka supplies
an implicit type name. The exact reference input is saved alongside its log.
GCC diagnostics are supporting observations, not the specification of baka.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('/tmp/baka-extended-audit'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    manifest = json.loads((root / 'manifest.json').read_text())
    observed = {x['program']: x for x in json.loads((args.output / 'results.json').read_text())}
    comparisons = []
    for case in manifest:
        source = root / 'programs' / (case['name'] + '.cpp')
        entry = {**case, 'baka': observed[source.name]}
        if case['c_reference']:
            text = source.read_text()
            tags = set(re.findall(r'\bstruct\s+(\w+)', text))
            for tag in sorted(tags):
                text = re.sub(r'(?<!struct )\b' + re.escape(tag) + r'\b', 'struct ' + tag, text)
            reference = args.output / (case['name'] + '.reference.c')
            reference.write_text(text)
            result = subprocess.run(
                ['gcc', '-std=c11', '-pedantic-errors', '-fsyntax-only', '-x', 'c', str(reference)],
                capture_output=True, text=True, timeout=5,
            )
            (args.output / (case['name'] + '.gcc.txt')).write_text(result.stdout + result.stderr)
            entry['gcc_c11'] = {'exit_code': result.returncode,
                                'diagnostics': (result.stdout + result.stderr).splitlines()}
            for key, command, input_path in (
                ('gcc_c23', ['gcc', '-std=c23', '-x', 'c'], reference),
                ('gcc_cpp23', ['gcc', '-std=c++23', '-x', 'c++'], source),
            ):
                result = subprocess.run(
                    command + ['-pedantic-errors', '-fsyntax-only', str(input_path)],
                    capture_output=True, text=True, timeout=5,
                )
                (args.output / (case['name'] + '.' + key + '.txt')).write_text(result.stdout + result.stderr)
                entry[key] = {'exit_code': result.returncode,
                              'diagnostics': (result.stdout + result.stderr).splitlines()}
        if case['group'] == 'loop_matrix' and entry['baka']['parse'].get('exit_code') == 0:
            ast = (args.output / (case['name'] + '.parse.txt')).read_text()
            lines = ast[ast.index('Parser::AST:'):].splitlines()
            start = next(i for i, line in enumerate(lines) if line.strip() == 'For(')
            indent = len(lines[start]) - len(lines[start].lstrip())
            children = []
            for line in lines[start + 1:]:
                depth = len(line) - len(line.lstrip())
                if line.strip() == ')' and depth == indent:
                    break
                if depth == indent + 4 and line.strip() != ')':
                    children.append(line.strip())
            entry['for_ast_children'] = children
            intended_label = case['name'].endswith('_label')
            entry['label_retained'] = children[3] == 'Identifier(L)' if intended_label else None
        comparisons.append(entry)
    (args.output / 'comparison.json').write_text(json.dumps(comparisons, indent=2) + '\n')
    references = [x for x in comparisons if 'gcc_c11' in x]
    print(f"GCC C11: {sum(x['gcc_c11']['exit_code'] == 0 for x in references)} accepted, "
          f"{sum(x['gcc_c11']['exit_code'] != 0 for x in references)} rejected")
    for case in comparisons:
        if case.get('label_retained') is False:
            print('Lost for label:', case['name'], case['for_ast_children'])


if __name__ == '__main__':
    main()

"""Record lexer/parser behavior; acceptance is not semantic correctness."""
import argparse
import json
from pathlib import Path
import re
import subprocess


def main():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=root.parent / 'build/baka')
    parser.add_argument('--programs', type=Path, default=root / 'programs')
    parser.add_argument('--output', type=Path, default=Path('/tmp/baka-semantic-audit-results'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    for source in sorted(args.programs.glob('*.cpp')):
        entry = {'program': source.name}
        for stage in ('lex', 'parse'):
            try:
                result = subprocess.run(
                    [str(args.compiler.resolve()), '--' + stage, '--verbose', str(source)],
                    capture_output=True, text=True, timeout=5,
                )
                output = result.stdout + result.stderr
                (args.output / f'{source.stem}.{stage}.txt').write_text(output)
                clean = re.sub(r'\x1b\[[0-9;]*m', '', output)
                entry[stage] = {
                    'exit_code': result.returncode,
                    'diagnostics': [line for line in clean.splitlines()
                                    if 'error:' in line or 'Assertion' in line
                                    or 'terminate called' in line],
                }
            except subprocess.TimeoutExpired:
                entry[stage] = {'timeout': True}
        results.append(entry)
    (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    for stage in ('lex', 'parse'):
        accepted = sum(r[stage].get('exit_code') == 0 for r in results)
        rejected = sum(r[stage].get('exit_code', 0) > 0 for r in results)
        crashed = sum(r[stage].get('exit_code', 0) < 0 for r in results)
        timed_out = sum(r[stage].get('timeout', False) for r in results)
        print(f'{stage}: {accepted} accepted, {rejected} rejected, '
              f'{crashed} crashed, {timed_out} timed out')
    print(f'Results and full token/AST logs: {args.output}')


if __name__ == '__main__':
    main()

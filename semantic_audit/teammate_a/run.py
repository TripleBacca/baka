"""Build and check A's services and parser regressions without changing shared runners."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, default=ROOT / 'build/baka')
    args = parser.parse_args()
    report = []
    with tempfile.TemporaryDirectory(prefix='baka-a-') as directory:
        for name in ('services_test', 'ast_test'):
            binary = Path(directory) / name
            semantic_sources = sorted(str(path) for path in (ROOT / 'src/semantic').glob('*.cc'))
            command = ['c++', '-std=c++23', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                       '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'src/base')]
            subprocess.run(command + [str(HERE / (name + '.cc')), *semantic_sources, '-o', str(binary)],
                           check=True, timeout=120)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
            if result.returncode:
                raise AssertionError(result.stdout + result.stderr)
            print(result.stdout.strip())
            report.append({'test': name, 'passed': True})
    # Acceptance here verifies syntax/retained metadata only, never semantic validity.
    cases = {
        'unsigned_float.cpp': False, 'unsigned_alias.cpp': False, 'wrong_type_kind.cpp': False,
        'explicit_enum_value.cpp': True, 'unspecified_array_bound.cpp': True,
        'valid_self_pointer.cpp': True, 'forward_struct_pointer.cpp': True,
        'valid_function_pointer.cpp': True, 'valid_inheritance.cpp': True,
        'valid_typedef_repeat.cpp': True,
    }
    inputs = [(ROOT / 'semantic_audit/programs' / name, accepted) for name, accepted in cases.items()]
    # C's statement dispatcher does not yet route local aggregate definitions.
    pending = {'valid_aggregate_shadow.cpp'}
    inputs += [(path, path.name.startswith('valid_')) for path in sorted((HERE / 'programs').glob('*.cpp'))
               if path.name not in pending]
    for path, accepted in inputs:
        result = subprocess.run([str(args.compiler.resolve()), '--parse', '--verbose', str(path)],
                                capture_output=True, text=True, timeout=10)
        output = result.stdout + result.stderr
        if result.returncode < 0 or 'Assertion' in output or 'terminate called' in output:
            raise AssertionError(f'{path.name}: crashed\n{output}')
        if (result.returncode == 0) != accepted:
            raise AssertionError(f'{path.name}: expected accepted={accepted}, got {result.returncode}\n{output}')
        if not accepted and 'error:' not in output:
            raise AssertionError(f'{path.name}: failed without a diagnostic\n{output}')
        report.append({'test': path.name, 'passed': True, 'parser_acceptance': accepted})
    print(f'{len(inputs)} parser regression checks passed')
    print('Known C integration gap (excluded from passing checks): ' + ', '.join(sorted(pending)))
    print(json.dumps({'checks': len(report), 'passed': len(report)}, sort_keys=True))


if __name__ == '__main__':
    main()

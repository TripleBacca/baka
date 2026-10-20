"""Verify phase contracts and semantic outcomes; crashes never count as rejection."""
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def main():
    failures = 0
    scenarios = [phase + suffix for phase in ('registration', 'resolution', 'annotation', 'checks')
                 for suffix in ('', '_error')]
    scenarios.extend(('resolution_this', 'resolution_loop_targets', 'annotation_scoped_type'))
    for scenario in scenarios:
        result = subprocess.run([str(ROOT / 'build/semantic_pass_tests'), scenario],
                                capture_output=True, text=True, timeout=10)
        ok = result.returncode == 0
        print(('PASS ' if ok else 'FAIL ') + 'phase/' + scenario)
        if not ok:
            failures += 1
            print(result.stdout + result.stderr)
    conversion_scenarios = ('numeric', 'contexts', 'calls', 'compound', 'pointers', 'metadata', 'array_address')
    for scenario in conversion_scenarios:
        result = subprocess.run([str(ROOT / 'build/semantic_conversion_tests'), scenario],
                                capture_output=True, text=True, timeout=10)
        ok = result.returncode == 0
        print(('PASS ' if ok else 'FAIL ') + 'conversions/' + scenario)
        if not ok:
            failures += 1
            print(result.stdout + result.stderr)
    member_result = subprocess.run([str(ROOT / 'build/semantic_member_call_tests')],
                                   capture_output=True, text=True, timeout=10)
    print(('PASS ' if member_result.returncode == 0 else 'FAIL ') + 'member_calls')
    if member_result.returncode:
        failures += 1
        print(member_result.stdout + member_result.stderr)
    cases = json.loads((HERE / 'cases.json').read_text())
    cases += json.loads((HERE / 'diagnostics.json').read_text())
    for case in cases:
        source = HERE / case.get('directory', 'cases') / case['file']
        try:
            result = subprocess.run([str(ROOT / 'build/baka'), '--semantic', str(source)],
                                    capture_output=True, text=True, timeout=10)
            output = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + result.stderr)
            ok = result.returncode == case['exit']
            if case['diagnostic_stage']:
                ok = ok and 'error:' in output and ('(' + case['diagnostic_stage'] + ')') in output
            if 'diagnostics' in case:
                headers = list(re.finditer(re.escape(str(source)) +
                               r':(\d+):(\d+): (error|warning): ([^\n]*)', output))
                expected_errors = sum(item.get('kind', 'error') == 'error' for item in case['diagnostics'])
                ok = ok and sum(header[3] == 'error' for header in headers) == expected_errors
                source_lines = source.read_text().splitlines()
                for expected in case['diagnostics']:
                    matching = [header for header in headers
                                if int(header[1]) == expected['line'] and int(header[2]) == expected['column']
                                and header[3] == expected.get('kind', 'error') and expected['message'] in header[4]]
                    ok = ok and bool(matching)
                    # The displayed source line must agree with the diagnostic
                    # header, not merely contain a caret somewhere in the file.
                    for header in matching:
                        following = output[header.end():].splitlines()
                        ok = ok and len(following) > 1 and following[1] == '\t' + source_lines[expected['line'] - 1]
            if not ok:
                print(f"FAIL {case['file']}: expected exit {case['exit']}, got {result.returncode}")
                print('\n'.join(line for line in output.splitlines() if 'error:' in line or 'Assertion' in line))
                failures += 1
            else:
                print('PASS ' + case['file'])
        except subprocess.TimeoutExpired:
            print('FAIL ' + case['file'] + ': timed out')
            failures += 1
    print(f'{len(cases) + len(scenarios) + len(conversion_scenarios) + 1} checks; {failures} failures')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())

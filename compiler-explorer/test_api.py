#!/usr/bin/env python3
"""Verify compiler registration, stage output, optimization and diagnostics via CE."""
import argparse
import json
from urllib.request import Request, urlopen

parser = argparse.ArgumentParser(description='Smoke-test baka through a running Compiler Explorer API')
parser.add_argument('--url', default='http://127.0.0.1:10240')
base = parser.parse_args().url.rstrip('/')
def request(path, payload=None):
    data = None if payload is None else json.dumps(payload).encode()
    req = Request(base + path, data=data, headers={'Accept': 'application/json', 'Content-Type': 'application/json'})
    with urlopen(req, timeout=60) as response:
        return json.load(response)

compilers = request('/api/compilers/c')
ids = {entry['id'] for entry in compilers}
expected = {'baka', 'baka-ir', 'baka-semantic', 'baka-parse'}
assert expected <= ids, ids
print('Registered:', ', '.join(sorted(ids)))
for compiler in sorted(expected):
    for options in ('-O0', '-O2'):
        result = request('/api/compiler/' + compiler + '/compile', {
            'source': 'int main() { int x = 2 + 3; return x * 4; }',
            'options': {'userArguments': options, 'filters': {'directives': False, 'labels': False, 'commentOnly': False, 'libraryCode': False}}})
        assert result['code'] == 0, (compiler, options, result)
        assembly = '\n'.join(line['text'] for line in result['asm'])
        marker = {'baka': 'daddiu', 'baka-ir': 'function', 'baka-semantic': 'Semantic::AST', 'baka-parse': 'Parser::AST'}[compiler]
        assert marker in assembly, (compiler, assembly)
        assert not result.get('stderr'), (compiler, result['stderr'])
        print(compiler, options, 'passed:', len(result['asm']), 'output lines')
for compiler in ('baka', 'baka-ir', 'baka-semantic'):
    result = request('/api/compiler/' + compiler + '/compile', {
        'source': 'int main() { return missing; }', 'options': {'userArguments': ''}})
    assert result['code'] != 0, (compiler, result)
    diagnostics = '\n'.join(line['text'] for line in result['stderr'])
    assert 'error:' in diagnostics and 'missing' in diagnostics and '\x1b' not in diagnostics, (compiler, result)
    print(compiler, 'diagnostics passed')
print('All Compiler Explorer API checks passed')

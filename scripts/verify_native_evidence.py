#!/usr/bin/env python3
"""Verify retained evidence bytes and repository-local ledger file references."""
import hashlib
import json
from pathlib import Path
import re


def verify(root):
    ledger = json.loads((root / 'docs/evidence/native_rule_implementation_ledger.json').read_text(encoding='utf-8'))
    manifest = json.loads((root / 'docs/evidence/native_evidence_bundle_pass125.json').read_text(encoding='utf-8'))
    errors = []
    references = set()

    def local_file(reference):
        path, _, fragment = reference.partition('#')
        if path.startswith('../reverse-tools/'):
            errors.append(f'External evidence reference: {reference}')
            return
        destination = (root / path).resolve()
        if not destination.is_relative_to(root):
            errors.append(f'Evidence outside repository: {reference}')
            return
        if not destination.is_file():
            errors.append(f'Missing evidence: {reference}')
            return
        match = re.search(r'(?:^|&)sha256=([0-9a-f]{64})(?:$|&)', fragment)
        if match and hashlib.sha256(destination.read_bytes()).hexdigest() != match[1]:
            errors.append(f'Evidence digest mismatch: {reference}')

    def visit(value):
        if isinstance(value, dict):
            for child in value.values():
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)
        elif isinstance(value, str) and value.startswith(('reverse-tools/', '../reverse-tools/')):
            references.add(value)

    visit(ledger)
    for reference in sorted(references):
        local_file(reference)
    for entry in manifest['files']:
        reference = entry['destination']
        local_file(reference + '#sha256=' + entry['sha256'])
        destination = root / reference
        if destination.is_file() and destination.stat().st_size != entry['size']:
            errors.append(f'Evidence size mismatch: {reference}')
        # Old outputs without an embedded target digest remain unverified.
        if entry['targetHeaderMatched'] and destination.is_file():
            if manifest['targetSha256'].encode() not in destination.read_bytes()[:400]:
                errors.append(f'Target header mismatch: {reference}')
    if errors:
        raise SystemExit('\n'.join(errors))
    print(f'Native evidence: {len(references)} ledger references and '
          f'{len(manifest["files"])} restored files verified; '
          'byte integrity does not establish complete algorithm equivalence.')


if __name__ == '__main__':
    verify(Path(__file__).resolve().parents[1])

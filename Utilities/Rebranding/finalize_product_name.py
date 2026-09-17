"""Retarget the initial Engine branding to WorldEngine using the original backup.

Only writes a file when its current contents exactly match the old migration.
Unrelated engine concepts named Engine are never globally substituted.
"""
import argparse
import json
from pathlib import Path
import re
import rebrand_engine as migration


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original-backup', type=Path, required=True)
    parser.add_argument('--backup', type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads((args.original_backup / 'manifest.json').read_text(encoding='utf-8'))
    root = Path(manifest['root']).resolve()
    changes, payloads, manual = [], {}, []
    for item in manifest['changes']:
        raw = (args.original_backup / 'files' / item['source']).read_bytes()
        decoded = migration.decode(raw)
        if decoded is None:
            continue
        text, encoding, bom = decoded
        if not re.search(r'(?<![A-Za-z0-9])ezengine(?![A-Za-z0-9])', text, re.I):
            continue
        migration.PRODUCT_NAME = 'Engine'
        before = bom + migration.transform(text).encode(encoding)
        migration.PRODUCT_NAME = 'WorldEngine'
        after = bom + migration.transform(text).encode(encoding)
        target = migration.rename(item['source'])
        if before == after and target == item['target']:
            continue
        current = (root / item['target']).read_bytes()
        if current != before:
            manual.append(item['target'])
            continue
        if target != item['target'] and (root / target).exists():
            raise RuntimeError('Product path collision: ' + target)
        changes.append({'source': item['target'], 'target': target,
                        'before_sha256': migration.digest(before), 'after_sha256': migration.digest(after),
                        'content_changed': before != after})
        payloads[item['target']] = after
    report = {'root': str(root), 'product': 'WorldEngine', 'changes': changes, 'blockers': [], 'manual_review': manual}
    migration.apply(root, args.backup.resolve(), report, payloads)
    print(json.dumps({'changed': len(changes), 'manual_review': manual}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Name the firmware functions (NIDs) the game imports.

A NID is the first four bytes (little-endian) of SHA-1 of the function name,
so a candidate name is either exactly right or not. Candidates live in
tools/nid_candidates.txt (plain words, original list). Prints which imports
are named, which are still unknown, and which the runtime does not provide.

  nid_names.py [--json out.json]
"""
import argparse
import glob
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def nid(name):
    return struct.unpack('<I', hashlib.sha1(name.encode()).digest()[:4])[0]


def known_names():
    words = (ROOT / 'tools' / 'nid_candidates.txt').read_text(encoding='utf-8').split()
    return {nid(w): w for w in words if not w.startswith('#')}


def provided():
    have = set()
    for f in glob.glob(str(ROOT / 'third_party' / 'psprecomp' / 'src' / 'hle' / '*.c')) + \
             glob.glob(str(ROOT / 'games' / 'dbz' / '*.c')):
        for m in re.finditer(r'0x([0-9A-Fa-f]{8})u?\s*,\s*"', Path(f).read_text(encoding='utf-8')):
            have.add(int(m.group(1), 16))
    return have


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--json', type=Path)
    a = ap.parse_args()
    names, have = known_names(), provided()
    imports = json.loads((ROOT / 'build' / 'metadata' / 'main-analysis.json').read_text())['imports']
    rows = []
    for lib in imports:
        for n in lib['nids']:
            v = int(n, 16)
            rows.append({'library': lib['library'], 'nid': n, 'name': names.get(v),
                         'provided': v in have})
    missing = [r for r in rows if not r['provided']]
    print(f"{len(rows)} imports, {sum(r['provided'] for r in rows)} provided, "
          f"{len(missing)} missing ({sum(1 for r in missing if not r['name'])} still unnamed)")
    for r in missing:
        print(f"  {r['library']:22} {r['nid']}  {r['name'] or '?'}")
    if a.json:
        a.json.write_text(json.dumps(rows, indent=1))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Copy the files of the player's disc image into build/fs/disc/ (ignored).

The runtime maps the game's `disc0:/` and `umd0:/` paths there. Reads the ISO
9660 directory tree itself (primary volume descriptor at sector 16); never
writes to the image. Existing files of the right size are kept.

  extract_disc.py [--iso baserom/game.iso] [--out build/fs/disc]
"""
import argparse
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SECTOR = 2048


def walk(f, lba, size, prefix):
    f.seek(lba * SECTOR)
    data = f.read(size)
    pos = 0
    while pos < len(data):
        n = data[pos]
        if n == 0:                      # records never straddle a sector
            pos = (pos // SECTOR + 1) * SECTOR
            continue
        rec = data[pos:pos + n]
        elba, esize = struct.unpack_from('<I', rec, 2)[0], struct.unpack_from('<I', rec, 10)[0]
        flags, nlen = rec[25], rec[32]
        name = rec[33:33 + nlen]
        pos += n
        if name in (b'\0', b'\1'):
            continue
        name = name.decode('ascii').split(';')[0]
        path = f'{prefix}/{name}'
        if flags & 2:
            yield from walk(f, elba, esize, path)
        else:
            yield path, elba, esize


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--iso', type=Path, default=ROOT / 'baserom' / 'game.iso')
    ap.add_argument('--out', type=Path, default=ROOT / 'build' / 'fs' / 'disc')
    a = ap.parse_args()
    out = a.out.resolve()
    if not out.is_relative_to((ROOT / 'build').resolve()):
        raise SystemExit('output must be below build/')
    with a.iso.open('rb') as f:
        f.seek(16 * SECTOR)
        pvd = f.read(SECTOR)
        if pvd[1:6] != b'CD001':
            raise SystemExit('not an ISO 9660 image')
        root = pvd[156:156 + 34]
        rlba, rsize = struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0]
        count = 0
        for path, lba, size in walk(f, rlba, rsize, ''):
            dest = out / path.lstrip('/')
            if dest.exists() and dest.stat().st_size == size:
                continue
            dest.parent.mkdir(parents=True, exist_ok=True)
            f.seek(lba * SECTOR)
            with dest.open('wb') as o:
                left = size
                while left:
                    chunk = f.read(min(left, 1 << 22))
                    if not chunk:
                        raise SystemExit(f'image ends early in {path}')
                    o.write(chunk)
                    left -= len(chunk)
            count += 1
            print(f'{path} ({size} bytes)')
    print(f'{count} files written to {out}')


if __name__ == '__main__':
    main()

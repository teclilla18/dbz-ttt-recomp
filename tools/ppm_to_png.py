#!/usr/bin/env python3
"""Convert the runtime's PPM screenshots (DBZ_SHOTS) to PNG, beside them.
Screenshots show game art: keep them in build/ and never commit them.

  ppm_to_png.py <folder>
"""
import struct
import sys
import zlib
from pathlib import Path


def convert(src, dst):
    data = src.read_bytes()
    magic, size, _, pixels = data.split(b'\n', 3)
    if magic != b'P6':
        raise ValueError(f'{src}: not a binary PPM')
    w, h = map(int, size.split())
    raw = b''.join(b'\0' + pixels[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
    dst.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                    + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def main():
    folder = Path(sys.argv[1] if len(sys.argv) > 1 else 'build/shots')
    for f in sorted(folder.glob('*.ppm')):
        convert(f, f.with_suffix('.png'))
        print(f.with_suffix('.png'))


if __name__ == '__main__':
    main()

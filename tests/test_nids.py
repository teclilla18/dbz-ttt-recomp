"""Every named firmware registration must use the right NID.

A NID is the first four bytes (little-endian) of SHA-1 of the function name,
so psp_hle_register(0xNID, "lib", "name", fn) can be checked exactly."""
import hashlib
import re
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REG = re.compile(r'psp_hle_register\(\s*0x([0-9A-Fa-f]{8})u?\s*,\s*"[^"]*"\s*,\s*"([^"]+)"')


def nid(name):
    return struct.unpack('<I', hashlib.sha1(name.encode()).digest()[:4])[0]


class NidTests(unittest.TestCase):
    def test_registered_names_match_their_nids(self):
        files = list((ROOT / 'third_party' / 'psprecomp' / 'src' / 'hle').glob('*.c'))
        self.assertTrue(files)
        checked = 0
        for f in files:
            for value, name in REG.findall(f.read_text(encoding='utf-8')):
                with self.subTest(file=f.name, name=name):
                    self.assertEqual(int(value, 16), nid(name))
                checked += 1
        self.assertGreater(checked, 200)

    def test_known_pair(self):
        self.assertEqual(nid('sceKernelSetCompilerVersion'), 0xF77D77CB)


if __name__ == '__main__':
    unittest.main()

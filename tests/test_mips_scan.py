"""Tests for tools/mips_scan.py using only hand-made instruction words."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import mips_scan as m  # noqa: E402

BASE = 0x08804000


def lui(rt, imm): return (0x0F << 26) | (rt << 16) | imm
def addiu(rt, rs, imm): return (0x09 << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)
def ori(rt, rs, imm): return (0x0D << 26) | (rs << 21) | (rt << 16) | imm
def lw(rt, rs, imm): return (0x23 << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)
def jal(t): return (3 << 26) | ((t >> 2) & 0x03FFFFFF)


def elf(words, va=BASE, flags=5):
    code = struct.pack(f'<{len(words)}I', *words)
    header = bytearray(52)
    header[:7] = b'\x7fELF\x01\x01\x01'
    struct.pack_into('<HHIIIIIHHHHHH', header, 16, 2, 8, 1, va, 52, 0, 0,
                     52, 32, 1, 40, 0, 0)
    ph = struct.pack('<8I', 1, 84, va, va, len(code), len(code), flags, 16)
    return bytes(header) + ph + code


class ScanTests(unittest.TestCase):
    def segs(self, words, **kw):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'sample.elf'
            p.write_bytes(elf(words, **kw))
            return m.load_segments(p)

    def test_negative_low_half_borrows_from_high_half(self):
        segs = self.segs([lui(5, 0x08A8), addiu(5, 5, -5084)])
        self.assertEqual([x[2] for x in m.formed_addresses(segs)], [0x08A7EC24])

    def test_ori_and_load_uses(self):
        segs = self.segs([lui(4, 0x1234), ori(4, 4, 0x8000),
                          lui(6, 0x08A8), lw(2, 6, 0x10)])
        self.assertEqual([x[2] for x in m.formed_addresses(segs)],
                         [0x12348000, 0x08A80010])

    def test_overwritten_register_stops_pairing(self):
        segs = self.segs([lui(5, 0x08A8), ori(5, 0, 1), addiu(7, 5, 4)])
        self.assertEqual(list(m.formed_addresses(segs)), [])

    def test_jal_target_and_decode(self):
        segs = self.segs([0, jal(0x08A572F4)])
        self.assertEqual(m.decode(BASE + 4, m.read_word(segs, BASE + 4)),
                         'jal 0x08A572F4')
        self.assertEqual(m.decode(BASE, 0), 'nop')
        self.assertEqual(m.decode(BASE, addiu(29, 29, -16)), 'addiu sp, sp, -16')

    def test_non_executable_segment_is_skipped(self):
        segs = self.segs([lui(5, 0x08A8), addiu(5, 5, 4)], flags=6)
        self.assertEqual(list(m.formed_addresses(segs)), [])

    def test_rejects_non_elf(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'x.bin'
            p.write_bytes(b'nope')
            with self.assertRaises(ValueError):
                m.load_segments(p)


if __name__ == '__main__':
    unittest.main()

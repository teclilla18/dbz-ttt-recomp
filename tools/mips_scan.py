#!/usr/bin/env python3
"""Small original MIPS (Allegrex subset) scanner for finding code facts.

Prints addresses and short instruction text for local study only. Its output
can contain game instructions: write it below build/ and never commit or paste
it. It reads ELF32 little-endian executable segments; it does not execute code.

Uses:
  refs   ADDR [ADDR...]   find lui+addiu/ori/load/store pairs forming ADDR
  range  LO HI            same, for any formed address in [LO, HI)
  calls  ADDR             find jal instructions to ADDR
  dis    ADDR COUNT       decode COUNT words starting at ADDR
"""
import argparse
import struct
import sys
from pathlib import Path

REG = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3',
       't4', 't5', 't6', 't7', 's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7',
       't8', 't9', 'k0', 'k1', 'gp', 'sp', 'fp', 'ra']
IMM_OPS = {0x08: 'addi', 0x09: 'addiu', 0x0A: 'slti', 0x0B: 'sltiu',
           0x0C: 'andi', 0x0D: 'ori', 0x0E: 'xori'}
MEM_OPS = {0x20: 'lb', 0x21: 'lh', 0x23: 'lw', 0x24: 'lbu', 0x25: 'lhu',
           0x28: 'sb', 0x29: 'sh', 0x2B: 'sw', 0x31: 'lwc1', 0x39: 'swc1',
           0x32: 'lv.s', 0x3A: 'sv.s', 0x36: 'lv.q', 0x3E: 'sv.q'}
BRANCH = {0x04: 'beq', 0x05: 'bne', 0x06: 'blez', 0x07: 'bgtz',
          0x14: 'beql', 0x15: 'bnel', 0x16: 'blezl', 0x17: 'bgtzl'}
SPECIAL = {0x00: 'sll', 0x02: 'srl', 0x03: 'sra', 0x08: 'jr', 0x09: 'jalr',
           0x0C: 'syscall', 0x0D: 'break', 0x21: 'addu', 0x23: 'subu',
           0x24: 'and', 0x25: 'or', 0x26: 'xor', 0x27: 'nor', 0x2A: 'slt',
           0x2B: 'sltu', 0x0A: 'movz', 0x0B: 'movn', 0x04: 'sllv', 0x06: 'srlv',
           0x18: 'mult', 0x19: 'multu', 0x1A: 'div', 0x1B: 'divu',
           0x10: 'mfhi', 0x12: 'mflo', 0x2C: 'max', 0x2D: 'min'}


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def load_segments(path):
    data = Path(path).read_bytes()
    if data[:4] != b'\x7fELF':
        raise ValueError('not an ELF file')
    phoff = struct.unpack_from('<I', data, 28)[0]
    phnum = struct.unpack_from('<H', data, 44)[0]
    segs = []
    for i in range(phnum):
        p = struct.unpack_from('<8I', data, phoff + 32 * i)
        if p[0] == 1 and p[4]:
            segs.append((p[2], data[p[1]:p[1] + p[4]], bool(p[6] & 1)))
    return segs


def words(segs, exec_only=True):
    for va, blob, ex in segs:
        if exec_only and not ex:
            continue
        for i in range(0, len(blob) - 3, 4):
            yield va + i, struct.unpack_from('<I', blob, i)[0]


def read_word(segs, addr):
    for va, blob, _ in segs:
        if va <= addr and addr + 4 <= va + len(blob):
            return struct.unpack_from('<I', blob, addr - va)[0]
    return None


def decode(pc, w):
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    rd, sa, fn, imm = (w >> 11) & 31, (w >> 6) & 31, w & 63, w & 0xFFFF
    if w == 0:
        return 'nop'
    if op == 0:
        name = SPECIAL.get(fn, f'special.{fn:02x}')
        if fn in (0, 2, 3):
            return f'{name} {REG[rd]}, {REG[rt]}, {sa}'
        if fn == 8:
            return f'jr {REG[rs]}'
        if fn == 9:
            return f'jalr {REG[rd]}, {REG[rs]}'
        if fn in (0x0C, 0x0D):
            return f'{name} 0x{(w >> 6) & 0xFFFFF:x}'
        return f'{name} {REG[rd]}, {REG[rs]}, {REG[rt]}'
    if op in (2, 3):
        target = (pc & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
        return f"{'j' if op == 2 else 'jal'} 0x{target:08X}"
    if op == 0x0F:
        return f'lui {REG[rt]}, 0x{imm:04X}'
    if op in IMM_OPS:
        val = f'0x{imm:04X}' if op >= 0x0C else str(s16(imm))
        return f'{IMM_OPS[op]} {REG[rt]}, {REG[rs]}, {val}'
    if op in MEM_OPS:
        return f'{MEM_OPS[op]} {REG[rt]}, {s16(imm)}({REG[rs]})'
    if op in BRANCH:
        target = (pc + 4 + (s16(imm) << 2)) & 0xFFFFFFFF
        return f'{BRANCH[op]} {REG[rs]}, {REG[rt]}, 0x{target:08X}'
    if op == 1:
        kinds = {0: 'bltz', 1: 'bgez', 2: 'bltzl', 3: 'bgezl', 0x11: 'bgezal'}
        target = (pc + 4 + (s16(imm) << 2)) & 0xFFFFFFFF
        return f"{kinds.get(rt, 'regimm')} {REG[rs]}, 0x{target:08X}"
    return f'.word 0x{w:08X}'


def writes_register(w):
    """Return the general register an instruction writes, or None (subset)."""
    op = w >> 26
    if op == 0:
        return (w >> 11) & 31 if w & 63 not in (0x08, 0x0C, 0x0D, 0x18, 0x19, 0x1A, 0x1B) else None
    if op in IMM_OPS or op == 0x0F or (op in MEM_OPS and op in (0x20, 0x21, 0x23, 0x24, 0x25)):
        return (w >> 16) & 31
    if op == 3:
        return 31
    return None


def formed_addresses(segs, window=16):
    """Yield (lui_pc, use_pc, address) for lui followed by a low-half use."""
    for va, blob, ex in segs:
        if not ex:
            continue
        n = len(blob) // 4
        ws = struct.unpack_from(f'<{n}I', blob)
        for i, w in enumerate(ws):
            if w >> 26 != 0x0F:
                continue
            reg, hi = (w >> 16) & 31, (w & 0xFFFF) << 16
            for j in range(i + 1, min(n, i + 1 + window)):
                u = ws[j]
                op, rs, rt = u >> 26, (u >> 21) & 31, (u >> 16) & 31
                if rs == reg and (op == 0x09 or op in MEM_OPS):
                    yield va + 4 * i, va + 4 * j, (hi + s16(u & 0xFFFF)) & 0xFFFFFFFF
                elif rs == reg and op == 0x0D:
                    yield va + 4 * i, va + 4 * j, hi | (u & 0xFFFF)
                if writes_register(u) == reg:
                    break  # the high half is gone after this instruction


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('elf')
    ap.add_argument('cmd', choices=['refs', 'range', 'calls', 'dis'])
    ap.add_argument('args', nargs='+')
    a = ap.parse_args()
    segs = load_segments(a.elf)
    nums = [int(x, 0) for x in a.args]
    if a.cmd == 'refs':
        want = set(nums)
        for lui, use, addr in formed_addresses(segs):
            if addr in want:
                print(f'0x{lui:08X} 0x{use:08X} -> 0x{addr:08X}  {decode(use, read_word(segs, use))}')
    elif a.cmd == 'range':
        lo, hi = nums
        for lui, use, addr in formed_addresses(segs):
            if lo <= addr < hi:
                print(f'0x{lui:08X} 0x{use:08X} -> 0x{addr:08X}  {decode(use, read_word(segs, use))}')
    elif a.cmd == 'calls':
        for pc, w in words(segs):
            if w >> 26 == 3 and ((pc & 0xF0000000) | ((w & 0x03FFFFFF) << 2)) == nums[0]:
                print(f'0x{pc:08X}')
    else:
        start, count = nums
        for k in range(count):
            pc = start + 4 * k
            w = read_word(segs, pc)
            if w is None:
                break
            print(f'0x{pc:08X}: {w:08X}  {decode(pc, w)}')
    return 0


if __name__ == '__main__':
    sys.exit(main())

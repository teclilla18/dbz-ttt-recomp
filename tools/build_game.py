#!/usr/bin/env python3
"""Recompile the game into C below build/gen/ from the player's own files.

Needs (all local, ignored): baserom/recon/EBOOT.elf (decrypted main),
baserom/pack_recon/overlay_*.elf (the 11 overlays from the PacK archive) and
the metadata copy from tools/prepare_main_elf.py. Writes only below build/.

Steps:
  1. emit main C (prefix dbz) and hook the overlay start-up/clean-up runners;
  2. for each overlay: write function-start hints, emit C, rename its symbols
     so 11 overlays sharing one address window can link into one program;
  3. write overlay_table.c mapping the header label (word 0) to its register
     function. See docs/notes/overlays.md.

  build_game.py [--recomp <allegrexrecomp.exe>]
"""
import argparse
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'build' / 'gen'
MAIN_ELF = ROOT / 'baserom' / 'recon' / 'EBOOT.elf'
MAIN_ANALYSIS = ROOT / 'build' / 'metadata' / 'main-analysis.elf'
OVERLAYS = ROOT / 'baserom' / 'pack_recon'
DEFAULT_RECOMP = ROOT / 'build' / 'psprecomp' / 'tools' / 'allegrexrecomp' / 'Release' / 'allegrexrecomp.exe'

# Guest runners from overlays.md: start-up list runner and clean-up list runner.
START_RUNNER, CLEAN_RUNNER = '08A57500', '08A57578'
SYMBOL = re.compile(r'\bpsp_(func|at|body|import)_([0-9A-F]{8})\b')


def run(cmd):
    r = subprocess.run([str(c) for c in cmd], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f'failed: {cmd[0]} ... \n{r.stdout[-2000:]}\n{r.stderr[-2000:]}')
    return r.stdout


def emit(recomp, elf, outdir, prefix, hints=None):
    if outdir.exists():
        shutil.rmtree(outdir)
    outdir.mkdir(parents=True)
    cmd = [recomp, 'emit', elf, outdir, prefix] + (['--hints', hints] if hints else [])
    log = run(cmd)
    (outdir.parent / f'{prefix}-emit.log').write_text(log, encoding='utf-8')
    return int(re.search(r'functions:\s+(\d+)', log).group(1))


def hook_main(outdir):
    """Call the host before overlay start-up and after overlay clean-up."""
    done = set()
    for f in outdir.glob('dbz_funcs_*.c'):
        s = f.read_text(encoding='utf-8')
        t = s.replace(
            f'void psp_func_{START_RUNNER}(void) {{ psp_body_{START_RUNNER}(0x{START_RUNNER}u); }}',
            f'void dbz_overlay_loaded(void);\nvoid psp_func_{START_RUNNER}(void) '
            f'{{ dbz_overlay_loaded(); psp_body_{START_RUNNER}(0x{START_RUNNER}u); }}')
        t = t.replace(
            f'void psp_func_{CLEAN_RUNNER}(void) {{ psp_body_{CLEAN_RUNNER}(0x{CLEAN_RUNNER}u); }}',
            f'void dbz_overlay_unloaded(void);\nvoid psp_func_{CLEAN_RUNNER}(void) '
            f'{{ psp_body_{CLEAN_RUNNER}(0x{CLEAN_RUNNER}u); dbz_overlay_unloaded(); }}')
        if t != s:
            done |= {n for n in (START_RUNNER, CLEAN_RUNNER) if f'dbz_overlay_' in t and n in t}
            f.write_text(t, encoding='utf-8')
    if done != {START_RUNNER, CLEAN_RUNNER}:
        sys.exit('could not hook the overlay runners in main C')


def register_imports(outdir):
    """Overlays reach main's firmware stubs by address, through the dispatch
    table, so every stub must be registered there too."""
    f = outdir / 'dbz_imports.c'
    s = f.read_text(encoding='utf-8')
    have = set(re.findall(r'^void psp_import_([0-9A-F]{8})\(void\)', s, re.M))
    lines = ['', '/* Stubs main never calls itself, which overlays do (build_game.py). */']
    stubs = all_import_stubs()
    for addr, nid in stubs:
        x = f'{addr:08X}'
        if x not in have:
            lines.append(f'void psp_import_{x}(void) {{ PSP_ENTER(0x{x}u); psp_hle_call(0x{nid:08X}u); }}')
    lines += ['', 'void dbz_register_imports(void) {']
    lines += [f'    psp_register(0x{a:08X}u, psp_import_{a:08X});' for a, _ in stubs]
    lines += ['}', '']
    f.write_text(s + '\n'.join(lines), encoding='utf-8')


def all_import_stubs():
    """(stub address, NID) for every import of the main ELF (module info at
    the section facts recorded by prepare_main_elf.py)."""
    data = MAIN_ELF.read_bytes()
    seg_va, seg_off = 0x08804040, 0x1018
    off = lambda va: seg_off + va - seg_va
    info = struct.unpack_from('<HH28sIIIII', data, 0x263C48)
    cur, end, out = info[6], info[7], []
    while cur < end:
        _, _, _, words, _, count, nids, stubs = struct.unpack_from('<IHHBBHII', data, off(cur))
        for k in range(count):
            out.append((stubs + 8 * k, struct.unpack_from('<I', data, off(nids) + 4 * k)[0]))
        cur += words * 4
    if len(out) != 228:
        sys.exit(f'expected 228 imports, found {len(out)}')
    return out


def rename_overlay(outdir, tag):
    for f in outdir.glob('*.[ch]'):
        s = f.read_text(encoding='utf-8')
        s = SYMBOL.sub(lambda m: f'{tag}_{m.group(1)}_{m.group(2)}', s)
        s = s.replace('void psp_recomp_register(void)', f'void {tag}_register(void)')
        s = re.sub(r'PSPRECOMP_(\w+_H)', lambda m: f'{tag.upper()}_{m.group(1)}', s)  # header guard
        f.write_text(s, encoding='utf-8')
    (outdir / f'{outdir.name}_imports.c').unlink(missing_ok=True)  # overlays import nothing


def overlay_labels_for(segs, starts):
    import overlay_hints
    return overlay_hints.overlay_labels(segs, starts)


def write_hints(path, starts, labels=()):
    """A hints file: [[function]] starts and [[label]] entries (build/ only)."""
    lines = ['[[function]]', 'at = ['] + [f'  0x{x:08X},' for x in starts] + [']', '']
    if labels:
        lines += ['[[label]]', 'at = ['] + [f'  0x{x:08X},' for x in labels] + [']', '']
    path.write_text('\n'.join(lines), encoding='utf-8')


MAIN_TEXT = (0x08804040, 0x08A664B0)   # .text, from prepare_main_elf.SECTIONS


def main_pointer_starts(segs):
    """Code addresses main builds with lui+addiu/ori (thread entries, callbacks)
    that sit on a function boundary: after `jr`/`j` + delay slot, or a prologue."""
    import mips_scan
    lo, hi = MAIN_TEXT
    out = []
    cands = {x for _, _, x in mips_scan.formed_addresses(segs)}
    for va, blob, _ in segs:                 # data words: vtables, callback tables
        for i in range(0, len(blob) - 3, 4):
            if not (lo <= va + i < hi):
                cands.add(struct.unpack_from('<I', blob, i)[0])
    for x in sorted(cands):
        if not (lo <= x < hi and x % 4 == 0):
            continue
        before, here = mips_scan.read_word(segs, x - 8), mips_scan.read_word(segs, x)
        if before is None or here is None:
            continue
        ends = before >> 26 == 2 or (before >> 26 == 0 and before & 63 == 8)
        if ends or ((here >> 16) == 0x27BD and here & 0x8000):
            out.append(x)
    # Functions reached only through computed pointers: a stack-frame prologue
    # right after a `jr ra` + delay slot is a function start.
    for pc, w in mips_scan.words(segs):
        if lo <= pc < hi and w and mips_scan.read_word(segs, pc - 8) == 0x03E00008:
            out.append(pc)
    return sorted(set(out))


def overlay_label(elf):
    data = elf.read_bytes()
    ph = struct.unpack_from('<I', data, 28)[0]
    off, va, _, filesz, memsz = struct.unpack_from('<5I', data, ph + 4)
    return struct.unpack_from('<I', data, off)[0], va, memsz


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--recomp', type=Path, default=DEFAULT_RECOMP)
    a = ap.parse_args()
    for p in (a.recomp, MAIN_ELF, MAIN_ANALYSIS):
        if not p.exists():
            sys.exit(f'missing {p}')
    sys.path.insert(0, str(ROOT / 'tools'))
    import overlay_hints

    import mips_scan
    main_segs = mips_scan.load_segments(MAIN_ELF)
    main_starts = main_pointer_starts(main_segs)
    main_hints = GEN / 'main.toml'
    main_hints.parent.mkdir(parents=True, exist_ok=True)
    write_hints(main_hints, main_starts)
    n = emit(a.recomp, MAIN_ANALYSIS, GEN / 'main', 'dbz', main_hints)
    hook_main(GEN / 'main')
    register_imports(GEN / 'main')
    print(f'main: {n} functions')

    rows = []
    for elf in sorted(OVERLAYS.glob('overlay_*.elf')):
        label, va, memsz = overlay_label(elf)
        tag = f'ovl{label:02d}'
        ovl_segs = mips_scan.load_segments(elf)
        starts = overlay_hints.overlay_starts(ovl_segs, main_segs)
        hints = GEN / f'{tag}.toml'
        starts = sorted(set(starts) | set(overlay_labels_for(ovl_segs, starts)))
        write_hints(hints, starts)
        n = emit(a.recomp, elf, GEN / tag, tag, hints)
        rename_overlay(GEN / tag, tag)
        rows.append((label, tag, va, memsz))
        print(f'{tag} ({elf.name}): {n} functions, {len(starts)} hinted starts')

    if sorted(r[0] for r in rows) != list(range(1, 12)):
        sys.exit('expected overlay labels 1..11')
    table = ['/* Written by tools/build_game.py -- game-derived, never commit. */',
             '#include <stdint.h>', '#include "dbz_overlays.h"']
    table += [f'void {t}_register(void);' for _, t, _, _ in rows]
    table += ['const dbz_overlay dbz_overlays[DBZ_OVERLAY_COUNT] = {']
    table += [f'    {{ {l}, 0x{v:08X}u, 0x{m:X}u, {t}_register }},' for l, t, v, m in sorted(rows)]
    table += ['};', '']
    (GEN / 'overlay_table.c').write_text('\n'.join(table), encoding='utf-8')
    print('wrote', GEN / 'overlay_table.c')


if __name__ == '__main__':
    main()

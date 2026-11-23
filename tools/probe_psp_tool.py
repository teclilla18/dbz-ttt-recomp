#!/usr/bin/env python3
"""Reproduce candidate-tool gaps with original synthetic inputs, never a game.

Use --tool to select the externally built allegrexrecomp executable.
Artifacts stay in a NEW directory below build/. A passing probe run means the
recorded limitations were reproduced, not that the translator is correct.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess


def fixture(entry, vector_immediate=False):
    """One synthetic MIPS load segment: optional vector immediate, then return."""
    base = 0x08900000
    # Hand-constructed CPU examples: vector immediate 1.0, return, delay-slot nop.
    words = ([0xDF803C00] if vector_immediate else []) + [0x03E00008, 0]
    code = struct.pack('<' + 'I' * len(words), *words)
    data = bytearray(0x100 + len(code))
    data[:16] = b'\x7fELF\x01\x01\x01' + bytes(9)
    struct.pack_into('<HHIIIIIHHHHHH', data, 16,
                     0xFFA8 if entry == 0 else 2, 8, 1, entry,
                     52, 0, 0, 52, 32, 1, 40, 0, 0)
    struct.pack_into('<IIIIIIII', data, 52,
                     1, 0x100, base, base, len(code), len(code), 5, 4)
    data[0x100:] = code
    return bytes(data)


def run(tool, output, action, source, label):
    command = [str(tool), action, str(source)]
    if action == 'emit':
        destination = output / label
        destination.mkdir()
        command.extend([str(destination), label])
    result = subprocess.run(command, capture_output=True, timeout=20)
    (output / (label + '-' + action + '.log')).write_bytes(
        result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError('External tool failed; inspect the local log')
    return result.stdout.decode('utf-8', errors='replace')


def public_symbols(folder):
    found = set()
    for path in folder.glob('*.c'):
        found.update(re.findall(r'^void\s+([A-Za-z_]\w*)\s*\([^;\n]*\)\s*\{',
                                path.read_text(), re.M))
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool', type=Path, required=True)
    parser.add_argument('--output', type=Path,
                        default=Path('build/evaluation/synthetic'))
    args = parser.parse_args()
    tool = args.tool.resolve(strict=True)
    output = args.output.resolve()
    build = Path(__file__).resolve().parents[1] / 'build'
    if not output.is_relative_to(build.resolve()) or output == build.resolve():
        parser.error('Output must be a new subdirectory of this repository build/')
    if output.exists():
        parser.error('Output already exists; choose a new directory')
    output.mkdir(parents=True)

    checks = []
    zero = output / 'zero-entry.elf'
    zero.write_bytes(fixture(0))
    report = run(tool, output, 'funcs', zero, 'zero_entry')
    count = re.search(r'^functions:\s+(\d+)', report, re.M)
    checks.append(('zero entry silently discovers no functions',
                   count is not None and int(count[1]) == 0))

    immediate = output / 'vector-immediate.elf'
    immediate.write_bytes(fixture(0x08900000, vector_immediate=True))
    report = run(tool, output, 'funcs', immediate, 'fixed_entry')
    count = re.search(r'^functions:\s+(\d+)', report, re.M)
    checks.append(('fixed entry discovers the synthetic function',
                   count is not None and int(count[1]) == 1))
    run(tool, output, 'emit', immediate, 'prefix_a')
    run(tool, output, 'emit', immediate, 'prefix_b')
    code = '\n'.join(p.read_text() for p in (output / 'prefix_a').glob('*.c'))
    # ISO C requires a decimal point or exponent for a float with an f suffix.
    malformed = re.findall(r'psp_vimm\([^,]+,\s*(-?\d+f)\)', code)
    checks.append(('whole-number vector immediate produces invalid C literal',
                   bool(malformed)))
    shared = public_symbols(output / 'prefix_a') & public_symbols(output / 'prefix_b')
    checks.append(('different output prefixes still share public function names',
                   bool(shared)))
    summary = {
        'meaning': 'Known limitations reproduced; no correctness acceptance implied',
        'checks': [{'name': name, 'reproduced': ok} for name, ok in checks],
        'malformed_literal_count': len(malformed),
        'shared_symbol_count': len(shared),
    }
    (output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    for name, ok in checks:
        print(('REPRODUCED: ' if ok else 'NOT REPRODUCED: ') + name)
    return 0 if all(ok for _, ok in checks) else 1


if __name__ == '__main__':
    raise SystemExit(main())

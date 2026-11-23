#!/usr/bin/env python3
"""Name stripped sections in a local analysis copy of the known European ELF.

Never changes its input, entry, program headers or loaded bytes. Only the
checked ULES01456 v1.00 decrypted input is supported. Output must be new and
below this repository's ignored build/ directory. This is metadata preparation,
not relocation, decryption or a game loader. No overlay entry points are guessed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

INPUT_SHA1 = 'd1d930da28e48dd4ba49d6659035d62c803a1edd'
SECTIONS = {
    2: ('.text', 0x08804040, 0x1018, 2499184, 6),
    7: ('.sceStub.text', 0x08A664EC, 0x2634C4, 1824, 6),
    9: ('.rodata.sceModuleInfo', 0x08A66C70, 0x263C48, 52, 3),
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def region(data, offset, size):
    require(offset >= 0 and size >= 0 and offset + size <= len(data),
            'ELF file range is truncated or invalid')
    return data[offset:offset + size]


def layout(data):
    require(len(data) >= 52 and data[:7] == b'\x7fELF\x01\x01\x01',
            'Expected an ELF32 little-endian input')
    header = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
    require(header[0:3] == (2, 8, 1) and header[7] == 52,
            'Expected a fixed-address MIPS executable')
    phoff, shoff = header[4:6]
    phsize, phnum, shsize, shnum, strings = header[8:13]
    require(phsize == 32 and 0 < phnum <= 8 and shsize == 40,
            'Unsupported ELF table layout')
    require(0 < shnum <= 256 and 0 < strings < shnum,
            'Missing or invalid section table')
    region(data, phoff, phsize * phnum)
    region(data, shoff, shsize * shnum)
    programs = [struct.unpack_from('<8I', data, phoff + i * phsize)
                for i in range(phnum)]
    loads = [p for p in programs if p[0] == 1]
    require(loads, 'No load segments')
    for p in loads:
        region(data, p[1], p[4])
        require(p[4] <= p[5] and p[2] + p[5] <= 2**32,
                'Invalid segment extent')
    sections = [struct.unpack_from('<10I', data, shoff + i * shsize)
                for i in range(shnum)]
    table = sections[strings]
    require(table[1] == 3, 'Section names do not refer to a string table')
    region(data, table[4], table[5])
    return header, loads, sections


def named_copy(data, names):
    """Attach explicit names; reject edits that could alter any loaded bytes."""
    header, loads, sections = layout(data)
    shoff, shsize, strings = header[5], header[10], header[12]
    output = bytearray(data)
    old_table = sections[strings]
    table = bytearray(region(data, old_table[4], old_table[5]))
    require(table and table[0] == 0 and table[-1] == 0,
            'Invalid section-name string table')
    edits = []
    for index, name in names.items():
        require(0 < index < len(sections) and index != strings,
                'Invalid target section index')
        require(sections[index][0] == 0, 'Target section already has a name')
        require(name and '\0' not in name, 'Invalid section name')
        address = shoff + index * shsize
        struct.pack_into('<I', output, address, len(table))
        edits.append((address, 4))
        table.extend(name.encode('ascii') + b'\0')
    address = shoff + strings * shsize + 16
    struct.pack_into('<II', output, address, len(data), len(table))
    edits.append((address, 8))
    for offset, size in edits:
        require(offset >= 52, 'Cannot alter the ELF header')
        for p in loads:
            require(offset + size <= p[1] or offset >= p[1] + p[4],
                    'Section metadata overlaps loaded bytes')
        phstart, phend = header[4], header[4] + header[8] * header[9]
        require(offset + size <= phstart or offset >= phend,
                'Section metadata overlaps program headers')
    output.extend(table)
    for p in loads:
        require(output[p[1]:p[1] + p[4]] == data[p[1]:p[1] + p[4]],
                'Loaded bytes changed')
    require(output[:52] == data[:52], 'ELF header changed')
    return bytes(output)


def inspect_main(data):
    require(hashlib.sha1(data).hexdigest() == INPUT_SHA1,
            'Unsupported input hash; expected the recorded European main ELF')
    header, loads, sections = layout(data)
    require(header[3] == 0x08804040 and len(loads) == 2 and len(sections) == 23,
            'Unexpected main executable layout')
    for index, (_, va, offset, size, flags) in SECTIONS.items():
        s = sections[index]
        require((s[0], s[1], s[2], s[3], s[4], s[5]) ==
                (0, 1, flags, va, offset, size), 'Unexpected section facts')

    def file_offset(va, size):
        matches = [p[1] + va - p[2] for p in loads
                   if p[2] <= va and va + size <= p[2] + p[4]]
        require(len(matches) == 1, 'Metadata pointer is unmapped or ambiguous')
        region(data, matches[0], size)
        return matches[0]

    def cstring(va):
        chars = bytearray()
        for i in range(32):
            value = data[file_offset(va + i, 1)]
            if value == 0:
                require(chars, 'Empty library name')
                return chars.decode('ascii')
            require(value == 95 or 48 <= value <= 57 or
                    65 <= value <= 90 or 97 <= value <= 122,
                    'Invalid library identifier')
            chars.append(value)
        raise ValueError('Unterminated library name')

    _, _, name, gp, exports, exports_end, imports, imports_end = struct.unpack_from(
        '<HH28sIIIII', data, SECTIONS[9][2])
    require(name.split(b'\0')[0] == b'DBTVS', 'Unexpected module name')
    file_offset(exports, exports_end - exports)
    require(imports < imports_end, 'Missing import descriptors')
    file_offset(imports, imports_end - imports)
    entries = []
    cursor = imports
    stub_start, stub_size = SECTIONS[7][1], SECTIONS[7][3]
    while cursor < imports_end:
        offset = file_offset(cursor, 20)
        nameva, _, _, words, variables, count, nids, stubs = struct.unpack_from(
            '<IHHBBHII', data, offset)
        stride = words * 4
        require(stride >= 20 and cursor + stride <= imports_end and variables == 0,
                'Unsupported import descriptor')
        lib = cstring(nameva)
        nid_offset = file_offset(nids, count * 4)
        file_offset(stubs, count * 8)
        require(stubs % 4 == 0 and stub_start <= stubs and
                stubs + count * 8 <= stub_start + stub_size,
                'Import thunks are outside their section')
        entries.append({'library': lib, 'function_count': count,
                        'nids': [f'0x{n:08X}' for n in
                                 struct.unpack_from('<' + 'I' * count, data, nid_offset)]})
        cursor += stride
    require(len(entries) == 25 and sum(e['function_count'] for e in entries) == 228,
            'Import inventory does not match the recorded copy')
    return {'input_sha1': INPUT_SHA1, 'entry': f'0x{header[3]:08X}',
            'global_pointer': f'0x{gp:08X}', 'libraries': len(entries),
            'functions': 228, 'imports': entries}


def write_analysis_copy(source, output):
    source = source.resolve(strict=True)
    output = output.resolve()
    build = Path(__file__).resolve().parents[1] / 'build'
    require(output != build.resolve() and output.is_relative_to(build.resolve()),
            'Output must be a new file below this repository build/')
    summary = output.with_suffix('.json')
    require(output != summary and not output.exists() and not summary.exists(),
            'Output or report already exists; choose a new output name')
    data = source.read_bytes()
    report = inspect_main(data)
    result = named_copy(data, {i: row[0] for i, row in SECTIONS.items()})
    report.update(output_sha1=hashlib.sha1(result).hexdigest(),
                  loaded_segments_unchanged=True, entry_unchanged=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('xb') as f:
        f.write(result)
    with summary.open('x', encoding='utf-8') as f:
        json.dump(report, f, indent=2)
        f.write('\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=Path('baserom/recon/EBOOT.elf'))
    parser.add_argument('--output', type=Path,
                        default=Path('build/metadata/main-analysis.elf'))
    args = parser.parse_args()
    try:
        report = write_analysis_copy(args.input, args.output)
    except (OSError, ValueError, struct.error) as exc:
        parser.exit(1, f'Preparation refused: {exc}\n')
    print(f"Prepared analysis copy: {report['functions']} imports / "
          f"{report['libraries']} libraries; loaded bytes and entry unchanged.")


if __name__ == '__main__':
    main()

"""Original synthetic ELF fixtures; no game data is required by these tests."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('prepare', ROOT / 'tools/prepare_main_elf.py')
prepare = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(prepare)


def fixture():
    data = bytearray(0x300)
    data[:16] = b'\x7fELF\x01\x01\x01' + bytes(9)
    struct.pack_into('<HHIIIIIHHHHHH', data, 16,
                     2, 8, 1, 0x08900000, 52, 0x200, 0, 52, 32, 1, 40, 3, 2)
    struct.pack_into('<8I', data, 52,
                     1, 0x100, 0x08900000, 0, 16, 24, 5, 4)
    # Synthetic payload distinguishes it from the zero-filled surrounding metadata.
    data[0x100:0x110] = b'original sample!'
    struct.pack_into('<10I', data, 0x200 + 40,
                     0, 1, 6, 0x08900000, 0x100, 16, 0, 0, 4, 0)
    struct.pack_into('<10I', data, 0x200 + 80,
                     0, 3, 0, 0, 0x180, 1, 0, 0, 1, 0)
    return bytes(data)


class MetadataTests(unittest.TestCase):
    def test_names_preserve_loaded_bytes_and_all_unedited_bytes(self):
        source = fixture()
        result = prepare.named_copy(source, {1: '.text'})
        self.assertEqual(result[:52 + 32], source[:52 + 32])
        self.assertEqual(result[0x100:0x110], source[0x100:0x110])
        mutable = set(range(0x228, 0x22C)) | set(range(0x260, 0x268))
        self.assertTrue(all(result[i] == source[i] for i in range(len(source))
                            if i not in mutable))
        nameoff = struct.unpack_from('<I', result, 0x228)[0]
        tableoff, tablesize = struct.unpack_from('<II', result, 0x260)
        self.assertEqual(result[tableoff + nameoff:tableoff + tablesize], b'.text\0')
        self.assertEqual(source, fixture())

    def test_preserves_existing_names_of_other_sections(self):
        data = bytearray(fixture())
        data[0x180:0x188] = b'\0.names\0'
        struct.pack_into('<I', data, 0x250, 1)
        struct.pack_into('<I', data, 0x264, 8)
        result = prepare.named_copy(bytes(data), {1: '.text'})
        tableoff = struct.unpack_from('<I', result, 0x260)[0]
        self.assertEqual(result[tableoff:tableoff + 8], b'\0.names\0')

    def test_rejects_edit_overlapping_loaded_bytes(self):
        data = bytearray(fixture())
        struct.pack_into('<I', data, 52 + 4, 0x228)
        with self.assertRaisesRegex(ValueError, 'overlaps loaded'):
            prepare.named_copy(bytes(data), {1: '.text'})

    def test_rejects_section_table_overlapping_program_headers(self):
        data = bytearray(fixture())
        struct.pack_into('<I', data, 32, 8)
        with self.assertRaises(ValueError):
            prepare.named_copy(bytes(data), {1: '.text'})

    def test_rejects_truncated_and_wrong_architecture(self):
        for size in (0, 6, 51, 80, 0x210):
            with self.subTest(size=size), self.assertRaises(ValueError):
                prepare.named_copy(fixture()[:size], {1: '.text'})
        data = bytearray(fixture())
        struct.pack_into('<H', data, 18, 3)
        with self.assertRaises(ValueError):
            prepare.named_copy(bytes(data), {1: '.text'})

    def test_rejects_segment_overflow_or_file_larger_than_memory(self):
        for field, value in ((8, 0xFFFFFFFC), (20, 8)):
            data = bytearray(fixture())
            struct.pack_into('<I', data, 52 + field, value)
            with self.subTest(field=field), self.assertRaises(ValueError):
                prepare.named_copy(bytes(data), {1: '.text'})

    def test_rejects_renaming_an_existing_name_or_invalid_slot(self):
        data = bytearray(fixture())
        struct.pack_into('<I', data, 0x228, 1)
        with self.assertRaises(ValueError):
            prepare.named_copy(bytes(data), {1: '.text'})
        for index in (-1, 0, 2, 3):
            with self.subTest(index=index), self.assertRaises(ValueError):
                prepare.named_copy(fixture(), {index: '.text'})

    def test_unknown_game_is_refused_before_any_output_write(self):
        build = ROOT / 'build'
        build.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=build) as directory:
            folder = Path(directory)
            source = folder / 'synthetic.elf'
            source.write_bytes(fixture())
            output = folder / 'output.elf'
            with self.assertRaisesRegex(ValueError, 'Unsupported input hash'):
                prepare.write_analysis_copy(source, output)
            self.assertFalse(output.exists())
            self.assertEqual(source.read_bytes(), fixture())

    def test_refuses_existing_output_and_output_outside_build(self):
        build = ROOT / 'build'
        build.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=build) as directory:
            folder = Path(directory)
            source, output = folder / 'input.elf', folder / 'output.elf'
            source.write_bytes(fixture())
            output.write_bytes(b'keep existing')
            with self.assertRaisesRegex(ValueError, 'already exists'):
                prepare.write_analysis_copy(source, output)
            self.assertEqual(output.read_bytes(), b'keep existing')
            with self.assertRaisesRegex(ValueError, 'below this repository build'):
                prepare.write_analysis_copy(source, ROOT / '.local/refused.elf')


if __name__ == '__main__':
    unittest.main()

import unittest

from tools.verify_ps2_elf import (
    ElfValidationError,
    ELF_HEADER,
    PROGRAM_HEADER,
    validate_elf,
)


def make_elf(
    machine=8,
    elf_class=1,
    endian=1,
    elf_type=2,
    phnum=1,
    segment_type=1,
    segment_offset=0,
    segment_file_size=None,
    segment_memory_size=None,
):
    ident = b"\x7fELF" + bytes([elf_class, endian, 1, 0, 0]) + bytes(7)
    payload_size = ELF_HEADER.size + PROGRAM_HEADER.size
    if segment_file_size is None:
        segment_file_size = payload_size
    if segment_memory_size is None:
        segment_memory_size = segment_file_size
    header = ELF_HEADER.pack(
        ident, elf_type, machine, 1, 0x00100000,
        ELF_HEADER.size, 0, 0x20000001,
        ELF_HEADER.size, PROGRAM_HEADER.size, phnum, 40, 0, 0,
    )
    if not phnum:
        return header
    program_header = PROGRAM_HEADER.pack(
        segment_type, segment_offset, 0x00100000, 0x00100000,
        segment_file_size, segment_memory_size, 5, 16,
    )
    return header + program_header


class Ps2ElfValidationTests(unittest.TestCase):
    def test_accepts_little_endian_mips_executable_with_load_segment(self):
        result = validate_elf(make_elf())
        self.assertEqual(result["file_bytes"], ELF_HEADER.size + PROGRAM_HEADER.size)
        self.assertEqual(result["program_headers"], 1)
        self.assertEqual(result["load_segments"], 1)

    def test_rejects_wrong_machine(self):
        with self.assertRaises(ElfValidationError):
            validate_elf(make_elf(machine=62))

    def test_rejects_elf64(self):
        with self.assertRaises(ElfValidationError):
            validate_elf(make_elf(elf_class=2))

    def test_rejects_big_endian(self):
        with self.assertRaises(ElfValidationError):
            validate_elf(make_elf(endian=2))

    def test_rejects_shared_object(self):
        with self.assertRaises(ElfValidationError):
            validate_elf(make_elf(elf_type=3))

    def test_rejects_missing_program_headers(self):
        with self.assertRaisesRegex(ElfValidationError, "no program header"):
            validate_elf(make_elf(phnum=0))

    def test_rejects_no_loadable_segment(self):
        with self.assertRaisesRegex(ElfValidationError, "no loadable PT_LOAD"):
            validate_elf(make_elf(segment_type=0))

    def test_rejects_load_segment_outside_file(self):
        with self.assertRaisesRegex(ElfValidationError, "extends beyond"):
            validate_elf(make_elf(segment_offset=4096, segment_file_size=16))

    def test_rejects_file_segment_larger_than_memory_segment(self):
        with self.assertRaisesRegex(ElfValidationError, "file size exceeds"):
            validate_elf(make_elf(segment_file_size=32, segment_memory_size=16))

    def test_rejects_truncated_program_headers(self):
        with self.assertRaises(ElfValidationError):
            validate_elf(make_elf()[:-8])


if __name__ == "__main__":
    unittest.main()

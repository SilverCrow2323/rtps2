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
    entry=0x00100000,
    segment_vaddr=0x00100000,
    segment_flags=5,
    segment_align=16,
    flags=0x20920025,
):
    ident = b"\x7fELF" + bytes([elf_class, endian, 1, 0, 0]) + bytes(7)
    payload_size = ELF_HEADER.size + PROGRAM_HEADER.size
    if segment_file_size is None:
        segment_file_size = payload_size
    if segment_memory_size is None:
        segment_memory_size = segment_file_size
    header = ELF_HEADER.pack(
        ident, elf_type, machine, 1, entry,
        ELF_HEADER.size, 0, flags,
        ELF_HEADER.size, PROGRAM_HEADER.size, phnum, 40, 0, 0,
    )
    if not phnum:
        return header
    program_header = PROGRAM_HEADER.pack(
        segment_type, segment_offset, segment_vaddr, segment_vaddr,
        segment_file_size, segment_memory_size, segment_flags, segment_align,
    )
    return header + program_header


class Ps2ElfValidationTests(unittest.TestCase):
    def test_accepts_little_endian_mips_executable_with_load_segment(self):
        result = validate_elf(make_elf())
        self.assertEqual(result["file_bytes"], ELF_HEADER.size + PROGRAM_HEADER.size)
        self.assertEqual(result["program_headers"], 1)
        self.assertEqual(result["load_segments"], 1)

    def test_accepts_ps2_r5900_mips_iii_architecture_flags(self):
        result = validate_elf(make_elf(flags=0x20920025))
        self.assertEqual(result["flags"], 0x20920025)

    def test_rejects_non_mips_iii_architecture_flags(self):
        for flags in (0x00000025, 0x30920025, 0x50920025):
            with self.subTest(flags=hex(flags)):
                with self.assertRaisesRegex(ElfValidationError, "MIPS III"):
                    validate_elf(make_elf(flags=flags))

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

    def test_rejects_entrypoint_outside_executable_segment(self):
        with self.assertRaisesRegex(ElfValidationError, "entry point is not inside"):
            validate_elf(make_elf(entry=0x00200000))

    def test_rejects_non_executable_entry_segment(self):
        with self.assertRaisesRegex(ElfValidationError, "entry point is not inside"):
            validate_elf(make_elf(segment_flags=4))

    def test_rejects_non_power_of_two_load_alignment(self):
        with self.assertRaisesRegex(ElfValidationError, "not a power of two"):
            validate_elf(make_elf(segment_align=24))

    def test_rejects_incongruent_load_alignment(self):
        with self.assertRaisesRegex(ElfValidationError, "violate alignment"):
            validate_elf(make_elf(segment_offset=1, segment_file_size=1))


if __name__ == "__main__":
    unittest.main()

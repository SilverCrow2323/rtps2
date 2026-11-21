import struct
import unittest

from tools.verify_ps2_elf import ElfValidationError, ELF_HEADER, validate_elf


def make_elf(machine=8, elf_class=1, endian=1, elf_type=2, phnum=1):
    ident = b"\x7fELF" + bytes([elf_class, endian, 1, 0, 0]) + bytes(7)
    header = ELF_HEADER.pack(
        ident, elf_type, machine, 1, 0x00100000,
        ELF_HEADER.size, 0, 0x20000001,
        ELF_HEADER.size, 32, phnum, 40, 0, 0,
    )
    return header + (bytes(32) if phnum else b"")


class Ps2ElfValidationTests(unittest.TestCase):
    def test_accepts_little_endian_mips_executable(self):
        result = validate_elf(make_elf())
        self.assertEqual(result["file_bytes"], ELF_HEADER.size + 32)
        self.assertEqual(result["program_headers"], 1)

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

    def test_rejects_truncated_program_headers(self):
        with self.assertRaises(ElfValidationError):
            validate_elf(make_elf()[:-8])


if __name__ == "__main__":
    unittest.main()

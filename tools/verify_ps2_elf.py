#!/usr/bin/env python3
"""Validate that a native binary is a structurally sound little-endian PS2 MIPS ELF."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import Dict

ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
ELF_MAGIC = b"\x7fELF"
EM_MIPS = 8
ET_EXEC = 2


class ElfValidationError(ValueError):
    """The supplied file is not a valid PS2 EE ELF payload."""


def validate_elf(data: bytes) -> Dict[str, int]:
    if len(data) < ELF_HEADER.size:
        raise ElfValidationError("file is shorter than the ELF32 header")
    header = ELF_HEADER.unpack_from(data)
    ident, elf_type, machine, version, entry, phoff, shoff, flags, ehsize, phentsize, phnum, shentsize, shnum, shstrndx = header

    if ident[:4] != ELF_MAGIC:
        raise ElfValidationError("ELF magic is missing")
    if ident[4] != 1:
        raise ElfValidationError("expected ELF32 (EI_CLASS=1)")
    if ident[5] != 1:
        raise ElfValidationError("expected little-endian ELF (EI_DATA=1)")
    if ident[6] != 1 or version != 1:
        raise ElfValidationError("unsupported ELF version")
    if elf_type != ET_EXEC:
        raise ElfValidationError("expected an executable ELF (ET_EXEC)")
    if machine != EM_MIPS:
        raise ElfValidationError("expected EM_MIPS for the PlayStation 2 EE")
    if ehsize < ELF_HEADER.size or ehsize > len(data):
        raise ElfValidationError("invalid ELF header size")
    if phnum and (phentsize < 32 or phoff + phentsize * phnum > len(data)):
        raise ElfValidationError("program header table is outside the file")
    if shnum and (shentsize < 40 or shoff + shentsize * shnum > len(data)):
        raise ElfValidationError("section header table is outside the file")
    if shnum and shstrndx >= shnum and shstrndx != 0xFFFF:
        raise ElfValidationError("section-name string table index is out of range")
    if entry == 0:
        raise ElfValidationError("ELF entry point is zero")

    return {
        "file_bytes": len(data),
        "entry": entry,
        "flags": flags,
        "program_headers": phnum,
        "section_headers": shnum,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", help="path to NEXUS.ELF")
    args = parser.parse_args()
    try:
        data = Path(args.elf).read_bytes()
        result = validate_elf(data)
    except (OSError, ElfValidationError) as exc:
        print("PS2 ELF verification: ERROR: %s" % exc, file=sys.stderr)
        return 1
    print(
        "PS2 ELF verification: OK (ELF32 little-endian MIPS, ET_EXEC; %d bytes; entry 0x%08x; flags 0x%08x)"
        % (result["file_bytes"], result["entry"], result["flags"])
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Validate that a native binary is a structurally sound little-endian PS2 MIPS ELF."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import Dict

ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
PROGRAM_HEADER = struct.Struct("<IIIIIIII")
ELF_MAGIC = b"\x7fELF"
EM_MIPS = 8
ET_EXEC = 2
PT_LOAD = 1
PF_X = 1
ELF32_MAX = (1 << 32) - 1


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
    if phnum == 0:
        raise ElfValidationError("ELF has no program header table")
    if (
        phoff < ehsize
        or phentsize < PROGRAM_HEADER.size
        or phoff + phentsize * phnum > len(data)
    ):
        raise ElfValidationError("program header table is outside the file")

    load_segments = 0
    entry_in_executable_segment = False
    for index in range(phnum):
        offset = phoff + index * phentsize
        p_type, p_offset, p_vaddr, _p_paddr, p_filesz, p_memsz, p_flags, p_align = (
            PROGRAM_HEADER.unpack_from(data, offset)
        )
        if p_type != PT_LOAD:
            continue
        load_segments += 1
        if p_filesz > p_memsz:
            raise ElfValidationError("PT_LOAD file size exceeds its memory size")
        if p_offset > len(data) or p_filesz > len(data) - p_offset:
            raise ElfValidationError("PT_LOAD segment extends beyond the ELF file")
        if p_vaddr > ELF32_MAX - p_memsz:
            raise ElfValidationError("PT_LOAD memory range overflows the ELF32 address space")
        if p_align > 1:
            if p_align & (p_align - 1):
                raise ElfValidationError("PT_LOAD alignment is not a power of two")
            if p_offset % p_align != p_vaddr % p_align:
                raise ElfValidationError("PT_LOAD offset and virtual address violate alignment")
        if p_flags & PF_X and p_vaddr <= entry < p_vaddr + p_memsz:
            entry_in_executable_segment = True

    if load_segments == 0:
        raise ElfValidationError("ELF has no loadable PT_LOAD segment")
    if not entry_in_executable_segment:
        raise ElfValidationError("ELF entry point is not inside an executable PT_LOAD segment")

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
        "load_segments": load_segments,
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
        "PS2 ELF verification: OK (ELF32 little-endian MIPS, ET_EXEC; %d bytes; entry 0x%08x; %d PT_LOAD segment(s); flags 0x%08x)"
        % (result["file_bytes"], result["entry"], result["load_segments"], result["flags"])
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

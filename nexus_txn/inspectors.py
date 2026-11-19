"""Read-only archive inspection for NEXUS Store packages.

ZIP is inspected using the Python standard library. PSU and SAS are recognized
as distinct package formats but intentionally return unsupported until their
real parsers are implemented. Nothing in this module extracts or writes files.
"""
from __future__ import annotations

import hashlib
import os
import re
import stat
import zipfile
import zlib
from pathlib import Path
from typing import Any

PROTECTED = {
    "BOOT", "SYS-CONF", "SYSTEM", "OSDSYS", "FMCB", "FHDB", "PSBBN", "PS2BBL"
}
SUPPORTED_FORMATS = {"zip", "psu", "sas"}


class ArchiveInspectionError(ValueError):
    """The archive is malformed or violates an inspection safety limit."""


class ArchiveFormatUnsupported(ArchiveInspectionError):
    """A known archive format has no implemented parser yet."""


def _safe_member_name(name: str, is_dir: bool) -> str:
    if not isinstance(name, str) or not name or "\x00" in name or "\\" in name:
        raise ArchiveInspectionError("archive contains an empty or unsafe member path")
    if name.startswith("/") or name.startswith("//") or re.match(r"^[A-Za-z]:", name):
        raise ArchiveInspectionError(f"absolute member path rejected: {name!r}")
    if any(ord(ch) < 32 or ord(ch) == 127 for ch in name) or ":" in name:
        raise ArchiveInspectionError(f"control or reserved character in member path: {name!r}")
    trimmed = name[:-1] if is_dir and name.endswith("/") else name
    parts = trimmed.split("/")
    if not trimmed or any(part in {"", ".", ".."} for part in parts):
        raise ArchiveInspectionError(f"traversal or ambiguous member path rejected: {name!r}")
    for part in parts:
        upper = part.upper()
        if upper in PROTECTED or upper.startswith(("BOOT.", "SYS-CONF.")):
            raise ArchiveInspectionError(f"protected system path rejected: {name!r}")
    return "/".join(parts)


def inspect_zip(
    archive_path: os.PathLike[str] | str,
    *,
    max_archive_size: int = 64 * 1024 * 1024,
    max_entries: int = 4096,
    max_member_size: int = 128 * 1024 * 1024,
    max_total_size: int = 512 * 1024 * 1024,
    max_compression_ratio: float = 200.0,
) -> dict[str, Any]:
    """Inspect a ZIP fully without extracting it; stream and hash every file."""
    path = Path(archive_path)
    try:
        archive_size = path.stat().st_size
    except OSError as exc:
        raise ArchiveInspectionError(f"cannot stat archive: {exc}") from exc
    if archive_size > max_archive_size:
        raise ArchiveInspectionError("archive exceeds configured compressed-size limit")
    if not zipfile.is_zipfile(path):
        raise ArchiveInspectionError("file is not a valid ZIP archive")

    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    total_size = 0
    try:
        with zipfile.ZipFile(path, "r") as zf:
            infos = zf.infolist()
            if len(infos) > max_entries:
                raise ArchiveInspectionError("archive exceeds configured entry-count limit")
            for info in infos:
                is_dir = info.is_dir()
                name = _safe_member_name(info.filename, is_dir)
                folded = name.casefold()
                if folded in seen:
                    raise ArchiveInspectionError(f"duplicate or case-colliding archive path: {name}")
                seen.add(folded)

                mode = (info.external_attr >> 16) & 0xFFFF
                kind = stat.S_IFMT(mode)
                if kind == stat.S_IFLNK:
                    raise ArchiveInspectionError(f"symbolic link rejected: {name}")
                allowed_kind = stat.S_IFDIR if is_dir else stat.S_IFREG
                if kind not in (0, allowed_kind):
                    raise ArchiveInspectionError(f"special filesystem entry rejected: {name}")
                if info.flag_bits & 0x1:
                    raise ArchiveInspectionError(f"encrypted ZIP entry is not supported: {name}")
                if is_dir:
                    if info.file_size != 0:
                        raise ArchiveInspectionError(f"directory entry has a payload: {name}")
                    continue
                if info.file_size < 0 or info.compress_size < 0:
                    raise ArchiveInspectionError(f"invalid ZIP entry size: {name}")
                if info.file_size > max_member_size:
                    raise ArchiveInspectionError(f"member exceeds configured size limit: {name}")
                total_size += info.file_size
                if total_size > max_total_size:
                    raise ArchiveInspectionError("archive exceeds configured expanded-size limit")
                ratio = info.file_size / max(info.compress_size, 1)
                if ratio > max_compression_ratio:
                    raise ArchiveInspectionError(f"suspicious compression ratio: {name}")

                digest = hashlib.sha256()
                actual_size = 0
                with zf.open(info, "r") as stream:
                    while True:
                        block = stream.read(1024 * 1024)
                        if not block:
                            break
                        actual_size += len(block)
                        if actual_size > max_member_size or total_size - info.file_size + actual_size > max_total_size:
                            raise ArchiveInspectionError(f"expanded data exceeds configured limit: {name}")
                        digest.update(block)
                if actual_size != info.file_size:
                    raise ArchiveInspectionError(f"declared and actual member sizes differ: {name}")
                entries.append({
                    "path": name,
                    "size": actual_size,
                    "compressed_size": info.compress_size,
                    "sha256": digest.hexdigest(),
                })
    except ArchiveInspectionError:
        raise
    except (OSError, EOFError, RuntimeError, zipfile.BadZipFile, zlib.error) as exc:
        raise ArchiveInspectionError(f"ZIP integrity check failed: {exc}") from exc

    return {
        "status": "ok",
        "format": "zip",
        "archive_size": archive_size,
        "file_count": len(entries),
        "expanded_size": total_size,
        "entries": entries,
        "extraction_performed": False,
        "device_write_performed": False,
    }


def inspect_archive(archive_path: os.PathLike[str] | str, archive_format: str, **limits: Any) -> dict[str, Any]:
    """Dispatch strictly by declared format; never reinterpret PSU/SAS as ZIP."""
    if not isinstance(archive_format, str) or archive_format.lower() not in SUPPORTED_FORMATS:
        raise ArchiveInspectionError(f"unknown archive format: {archive_format!r}")
    archive_format = archive_format.lower()
    if archive_format == "zip":
        return inspect_zip(archive_path, **limits)
    return {
        "status": "unsupported",
        "format": archive_format,
        "reason": f"{archive_format.upper()} parser is not implemented; archive was not opened or extracted",
        "extraction_performed": False,
        "device_write_performed": False,
    }


def match_recipe(inspection: dict[str, Any], recipe: dict[str, Any]) -> dict[str, Any]:
    """Check recipe source paths and hashes against an inspected ZIP report."""
    if inspection.get("status") != "ok" or inspection.get("format") != "zip":
        return {"matched": False, "reason": "a successful ZIP inspection is required"}
    if recipe.get("archive_format") != "zip":
        return {"matched": False, "reason": "recipe format does not match ZIP inspection"}
    actual = {entry["path"].casefold(): entry for entry in inspection.get("entries", [])}
    missing, mismatched = [], []
    mapped = set()
    for item in recipe.get("files", []):
        source = item["source"]
        key = source.casefold()
        mapped.add(key)
        entry = actual.get(key)
        if entry is None:
            missing.append(source)
        elif entry["sha256"].lower() != item["sha256"].lower():
            mismatched.append(source)
    return {
        "matched": not missing and not mismatched,
        "missing_sources": missing,
        "hash_mismatches": mismatched,
        "unmapped_archive_files": sorted(
            entry["path"] for key, entry in actual.items() if key not in mapped
        ),
        "extraction_performed": False,
        "device_write_performed": False,
    }

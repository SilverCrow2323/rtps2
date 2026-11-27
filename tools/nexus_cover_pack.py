#!/usr/bin/env python3
"""Build a deterministic, bounded 128x128 RGBA32 cover-art cache pack for NEXUS."""
from __future__ import annotations
import argparse, hashlib, json, re, sys, os, shutil, tempfile
from pathlib import Path, PurePosixPath
from tools.nexus_asset_pack import AssetError, decode_png_rgba

SIZE = 128
MAX_COVERS = 96
MAX_SOURCE_BYTES = 4 * 1024 * 1024
MAGIC = "NXC1"

class CoverPackError(ValueError):
    pass


class DuplicateJSONKeyError(ValueError):
    """A cover manifest contains an ambiguous duplicate object key."""


def _unique_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise DuplicateJSONKeyError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _strict_json_loads(text):
    return json.loads(text, object_pairs_hook=_unique_json_object)


def slug(stem: str) -> str:
    value = re.sub(r"[^A-Za-z0-9._-]+", "_", stem).strip("._-")
    if not value:
        raise CoverPackError(f"Invalid cover filename: {stem!r}")
    return value[:80]

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def fit_cover(width: int, height: int, rgba: bytes) -> bytes:
    if width < 1 or height < 1 or len(rgba) != width * height * 4:
        raise CoverPackError("Invalid source image dimensions or pixel data")
    scale = min(SIZE / width, SIZE / height)
    out_w = max(1, min(SIZE, round(width * scale)))
    out_h = max(1, min(SIZE, round(height * scale)))
    x0, y0 = (SIZE - out_w) // 2, (SIZE - out_h) // 2
    output = bytearray([0, 0, 0, 255] * (SIZE * SIZE))
    for y in range(out_h):
        sy = min(height - 1, int((y + 0.5) * height / out_h))
        for x in range(out_w):
            sx = min(width - 1, int((x + 0.5) * width / out_w))
            src = (sy * width + sx) * 4
            dst = ((y0 + y) * SIZE + x0 + x) * 4
            output[dst:dst + 4] = rgba[src:src + 4]
    return bytes(output)

def collect_sources(source_dir: Path) -> list[Path]:
    if source_dir.is_symlink():
        raise CoverPackError("Input directory must not be a symlink")
    if not source_dir.is_dir():
        raise CoverPackError(f"Input directory does not exist: {source_dir}")
    sources = sorted(
        (p for p in source_dir.iterdir() if p.suffix.lower() == ".png"),
        key=lambda p: p.name.casefold(),
    )
    if not sources:
        raise CoverPackError("No PNG cover images found")
    if len(sources) > MAX_COVERS:
        raise CoverPackError(f"Cover count exceeds the bounded catalogue limit ({MAX_COVERS})")
    names = set()
    for path in sources:
        if path.is_symlink() or not path.is_file():
            raise CoverPackError(f"Symlinks and non-regular files are not allowed: {path.name}")
        if path.stat().st_size > MAX_SOURCE_BYTES:
            raise CoverPackError(f"Source image exceeds 4 MiB: {path.name}")
        name = slug(path.stem).casefold()
        if name in names:
            raise CoverPackError(f"Duplicate output name after normalization: {path.stem}")
        names.add(name)
    return sources

def publish_pack(output_dir: Path, prepared: list[tuple[str, bytes]], manifest: dict) -> None:
    """Publish payloads and manifest as one rollback-capable filesystem transaction."""
    stage = Path(tempfile.mkdtemp(prefix=".nxc-stage-", dir=str(output_dir)))
    payload_dir = stage / "payload"
    backup_dir = stage / "backup"
    payload_dir.mkdir()
    backup_dir.mkdir()
    staged = []
    for filename, pixels in prepared:
        source = payload_dir / filename
        with source.open("wb") as stream:
            stream.write(pixels)
            stream.flush()
            os.fsync(stream.fileno())
        staged.append((source, output_dir / filename))
    manifest_source = payload_dir / "covers.json"
    with manifest_source.open("wb") as stream:
        stream.write((json.dumps(manifest, indent=2) + "\n").encode("utf-8"))
        stream.flush()
        os.fsync(stream.fileno())
    staged.append((manifest_source, output_dir / "covers.json"))
    backups = []
    committed = []
    try:
        for _source, target in staged:
            if target.is_symlink():
                raise CoverPackError(f"Refusing to replace symlink: {target.name}")
            if target.exists():
                if not target.is_file():
                    raise CoverPackError(f"Refusing to replace non-file: {target.name}")
                backup = backup_dir / target.name
                os.replace(target, backup)
                backups.append((target, backup))
        for source, target in staged:
            os.replace(source, target)
            committed.append(target)
        verify_pack(output_dir)
    except Exception as exc:
        rollback_error = None
        for target in reversed(committed):
            try:
                if target.exists() or target.is_symlink():
                    target.unlink()
            except OSError as rollback_exc:
                rollback_error = rollback_exc
        for target, backup in reversed(backups):
            try:
                if backup.exists():
                    os.replace(backup, target)
            except OSError as rollback_exc:
                rollback_error = rollback_exc
        if rollback_error is not None:
            raise CoverPackError(
                f"Cover pack publish failed and rollback is incomplete; recovery files remain in {stage}: {rollback_error}"
            ) from exc
        raise CoverPackError(f"Cover pack publish failed; previous pack restored: {exc}") from exc
    shutil.rmtree(stage, ignore_errors=True)


def build_pack(source_dir: Path, output_dir: Path) -> dict:
    sources = collect_sources(source_dir)
    if output_dir.is_symlink():
        raise CoverPackError("Output directory must not be a symlink")
    output_dir.mkdir(parents=True, exist_ok=True)
    if output_dir.is_symlink():
        raise CoverPackError("Output directory must not be a symlink")
    manifest_path = output_dir / "covers.json"
    if manifest_path.is_symlink():
        raise CoverPackError("Manifest path must not be a symlink")

    records = []
    prepared = []
    for source in sources:
        try:
            source_bytes = source.read_bytes()
            width, height, rgba = decode_png_rgba(source_bytes)
        except (OSError, AssetError) as exc:
            raise CoverPackError(f"Cannot decode {source.name}: {exc}") from exc
        pixels = fit_cover(width, height, rgba)
        filename = slug(source.stem) + ".rgba"
        target = output_dir / filename
        if target.is_symlink():
            raise CoverPackError(f"Cover output must not be a symlink: {filename}")
        if target.exists() and not target.is_file():
            raise CoverPackError(f"Cover output is not a regular file: {filename}")
        record = {
            "id": slug(source.stem),
            "source": source.name,
            "source_sha256": sha256(source_bytes),
            "output": filename,
            "width": SIZE,
            "height": SIZE,
            "format": "PSMCT32_RGBA32",
            "bytes": len(pixels),
            "pixel_sha256": sha256(pixels),
            "source_dimensions": [width, height],
        }
        prepared.append((filename, pixels))
        records.append(record)

    # Decode and validate the complete input set before publishing anything.
    manifest = {
        "format": MAGIC,
        "version": 1,
        "width": SIZE,
        "height": SIZE,
        "pixel_format": "PSMCT32_RGBA32",
        "max_entries": MAX_COVERS,
        "count": len(records),
        "entries": records,
    }
    publish_pack(output_dir, prepared, manifest)
    verify_pack(output_dir)
    return manifest


def verify_pack(output_dir: Path) -> dict:
    if output_dir.is_symlink():
        raise CoverPackError("Cover pack directory must not be a symlink")
    manifest_path = output_dir / "covers.json"
    if manifest_path.is_symlink():
        raise CoverPackError("Cover manifest must not be a symlink")
    try:
        manifest = _strict_json_loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError, DuplicateJSONKeyError) as exc:
        raise CoverPackError(f"Cannot read covers.json: {exc}") from exc
    if not isinstance(manifest, dict):
        raise CoverPackError("Cover manifest must be an object")
    version = manifest.get("version")
    if (manifest.get("format") != MAGIC or isinstance(version, bool) or not isinstance(version, int) or version != 1 or
            manifest.get("width") != SIZE or manifest.get("height") != SIZE or
            manifest.get("pixel_format") != "PSMCT32_RGBA32" or
            manifest.get("max_entries") != MAX_COVERS):
        raise CoverPackError("Unsupported or inconsistent cover pack format")
    entries = manifest.get("entries")
    count = manifest.get("count")
    if (not isinstance(entries, list) or not 1 <= len(entries) <= MAX_COVERS or
            isinstance(count, bool) or not isinstance(count, int) or count != len(entries)):
        raise CoverPackError("Invalid cover entry count")
    seen = set()
    for entry in entries:
        if not isinstance(entry, dict):
            raise CoverPackError("Cover entry must be an object")
        name = entry.get("output")
        if not isinstance(name, str) or "\\" in name or "\x00" in name:
            raise CoverPackError(f"Unsafe cover output path: {name!r}")
        path = PurePosixPath(name)
        if path.is_absolute() or len(path.parts) != 1 or path.name in ("", ".", "..") or path.suffix != ".rgba":
            raise CoverPackError(f"Unsafe cover output path: {name!r}")
        normalized_name = name.casefold()
        if normalized_name in seen:
            raise CoverPackError(f"Duplicate cover output: {name}")
        seen.add(normalized_name)
        if not re.fullmatch(r"[a-f0-9]{64}", str(entry.get("pixel_sha256", ""))):
            raise CoverPackError(f"Invalid pixel SHA-256: {name}")
        if not re.fullmatch(r"[a-f0-9]{64}", str(entry.get("source_sha256", ""))):
            raise CoverPackError(f"Invalid source SHA-256: {name}")
        target = output_dir / name
        if target.is_symlink():
            raise CoverPackError(f"Cover payload must not be a symlink: {name}")
        if not target.is_file() or target.stat().st_size != SIZE * SIZE * 4:
            raise CoverPackError(f"Missing or incorrectly sized cover payload: {name}")
        payload = target.read_bytes()
        if sha256(payload) != entry.get("pixel_sha256"):
            raise CoverPackError(f"Cover payload SHA-256 mismatch: {name}")
        if (entry.get("width") != SIZE or entry.get("height") != SIZE or
                entry.get("format") != "PSMCT32_RGBA32" or entry.get("bytes") != SIZE * SIZE * 4):
            raise CoverPackError(f"Invalid texture contract: {name}")
    return manifest

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="directory containing user-supplied PNG cover art")
    parser.add_argument("output", type=Path, help="output directory for RGBA32 files and covers.json")
    parser.add_argument("--verify", action="store_true", help="verify an existing output pack instead of building")
    args = parser.parse_args()
    try:
        if args.verify:
            manifest = verify_pack(args.output)
            print(f"Verified {manifest['count']} cover payloads ({SIZE}x{SIZE} RGBA32)")
        else:
            manifest = build_pack(args.source, args.output)
            print(f"Built {manifest['count']} deterministic cover payloads in {args.output}")
    except (OSError, CoverPackError) as exc:
        print(f"cover-pack: ERROR: {exc}", file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

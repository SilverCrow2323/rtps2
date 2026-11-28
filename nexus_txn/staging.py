"""Recipe-driven ZIP staging into an explicit host cache directory.

Only files mapped by a validated recipe are copied. No ZIP extractall call,
device mount, PS2 target path or installer execution is exposed here.
"""
from __future__ import annotations

import hashlib
import os
import shutil
import tempfile
import zipfile
from pathlib import Path
from typing import Any

from .inspectors import ArchiveInspectionError, inspect_archive, match_recipe
from .recipes import RecipeError, validate_recipe


class StagingError(RuntimeError):
    pass


def stage_zip_package(
    archive_path: str | Path,
    recipe: dict[str, Any],
    staging_root: str | Path,
) -> dict[str, Any]:
    """Copy only recipe-mapped ZIP members into a fresh host staging directory."""
    stage_dir: Path | None = None
    try:
        normalized = validate_recipe(recipe)
        if normalized["archive_format"] != "zip":
            raise StagingError("only ZIP packages can be staged; PSU/SAS parsers are not implemented")
        inspection = inspect_archive(archive_path, "zip")
        comparison = match_recipe(inspection, normalized)
        if not comparison["matched"]:
            raise StagingError(f"archive does not match recipe: {comparison}")

        requested_root = Path(staging_root)
        if requested_root.is_symlink():
            raise StagingError("staging root must not be a symbolic link")
        requested_root.mkdir(parents=True, exist_ok=True)
        root = requested_root.resolve()
        if not root.is_dir():
            raise StagingError("staging root is not a directory")

        stage_dir = Path(tempfile.mkdtemp(prefix="nexus-stage-", dir=root)).resolve()
        payload_root = stage_dir / "payload"
        payload_root.mkdir()
        manifest_files = []
        total_written = 0

        with zipfile.ZipFile(archive_path, "r") as zf:
            archive_entries = {
                info.filename.rstrip("/").casefold(): info
                for info in zf.infolist()
                if not info.is_dir()
            }
            for item in normalized["files"]:
                source_name = item["source"]
                info = archive_entries.get(source_name.casefold())
                if info is None:
                    raise StagingError(f"recipe source disappeared from archive: {source_name}")

                source_rel = Path(*source_name.split("/"))
                destination = payload_root / source_rel
                destination.parent.mkdir(parents=True, exist_ok=True)
                if destination.exists() or destination.is_symlink():
                    raise StagingError(f"staging destination already exists: {source_name}")
                digest = hashlib.sha256()
                written = 0
                with zf.open(info, "r") as source, destination.open("xb") as target:
                    while True:
                        block = source.read(1024 * 1024)
                        if not block:
                            break
                        target.write(block)
                        digest.update(block)
                        written += len(block)
                    target.flush()
                    os.fsync(target.fileno())
                if written != info.file_size or digest.hexdigest() != item["sha256"]:
                    raise StagingError(f"staged file size or SHA-256 mismatch: {source_name}")
                total_written += written
                manifest_files.append({
                    "path": item["destination"],
                    "source": source_name,
                    "sha256": item["sha256"],
                })

        return {
            "status": "staged",
            "package_id": normalized["id"],
            "version": normalized["version"],
            "target_class": normalized["target"],
            "stage_dir": str(stage_dir),
            "payload_dir": str(payload_root),
            "manifest": {"files": manifest_files},
            "file_count": len(manifest_files),
            "staged_size": total_written,
            "inspection": {
                "archive_size": inspection["archive_size"],
                "expanded_size": inspection["expanded_size"],
                "file_count": inspection["file_count"],
            },
            "extraction_performed": True,
            "device_write_performed": False,
            "installer_execution_enabled": False,
        }
    except (ArchiveInspectionError, RecipeError, zipfile.BadZipFile, OSError) as exc:
        if stage_dir is not None:
            shutil.rmtree(stage_dir, ignore_errors=True)
        if isinstance(exc, StagingError):
            raise
        raise StagingError(str(exc)) from exc
    except Exception:
        if stage_dir is not None:
            shutil.rmtree(stage_dir, ignore_errors=True)
        raise


def remove_staged_package(stage_dir: str | Path, staging_root: str | Path) -> None:
    """Remove a staging directory only when it is a direct child of staging_root."""
    requested_root = Path(staging_root)
    if requested_root.is_symlink():
        raise StagingError("refusing to use a symbolic-link staging root")
    if not requested_root.is_dir():
        raise StagingError("staging root must be an existing directory")
    root = requested_root.resolve()
    candidate_path = Path(stage_dir)
    if candidate_path.is_symlink():
        raise StagingError("refusing to remove a staging symlink")
    candidate = candidate_path.resolve()
    if candidate == root or root not in candidate.parents or candidate.parent != root:
        raise StagingError("staging directory must be a direct child of staging_root")
    if not candidate.name.startswith("nexus-stage-"):
        raise StagingError("staging directory does not have the NEXUS staging prefix")
    if candidate.exists():
        shutil.rmtree(candidate)

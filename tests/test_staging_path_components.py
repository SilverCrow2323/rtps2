import hashlib
import tempfile
import unittest
import zipfile
from pathlib import Path

from nexus_txn.staging import StagingError, remove_staged_package, stage_zip_package


PAYLOAD = b"NEXUS path-safety payload"
RECIPE = {
    "id": "path-safety",
    "version": "1.0",
    "archive_format": "zip",
    "target": "mass",
    "files": [
        {
            "source": "APP/main.elf",
            "destination": "NEXUS/path-safety/main.elf",
            "sha256": hashlib.sha256(PAYLOAD).hexdigest(),
        }
    ],
}


def make_archive(root: Path) -> Path:
    archive = root / "package.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("APP/main.elf", PAYLOAD)
    return archive


class StagingPathComponentTests(unittest.TestCase):
    def test_stage_rejects_symlinked_ancestor_before_creating_cache(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            real_parent = base / "real-parent"
            real_parent.mkdir()
            alias = base / "alias"
            alias.symlink_to(real_parent, target_is_directory=True)
            archive = make_archive(base)
            requested_root = alias / "cache"

            with self.assertRaisesRegex(StagingError, "path contains a symbolic link"):
                stage_zip_package(archive, RECIPE, requested_root)

            self.assertFalse((real_parent / "cache").exists())

    def test_removal_rejects_symlinked_root_ancestor(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            real_parent = base / "real-parent"
            root = real_parent / "cache"
            root.mkdir(parents=True)
            stage = root / "nexus-stage-keep"
            stage.mkdir()
            alias = base / "alias"
            alias.symlink_to(real_parent, target_is_directory=True)

            with self.assertRaisesRegex(StagingError, "path contains a symbolic link"):
                remove_staged_package(alias / "cache" / stage.name, alias / "cache")

            self.assertTrue(stage.is_dir())

    def test_removal_rejects_symlinked_candidate_ancestor(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            root = base / "cache"
            root.mkdir()
            stage = root / "nexus-stage-keep"
            stage.mkdir()
            alias = base / "cache-alias"
            alias.symlink_to(root, target_is_directory=True)

            with self.assertRaisesRegex(StagingError, "path contains a symbolic link"):
                remove_staged_package(alias / stage.name, root)

            self.assertTrue(stage.is_dir())


if __name__ == "__main__":
    unittest.main()

import hashlib
import tempfile
import unittest
import zipfile
from pathlib import Path

from nexus_txn.staging import StagingError, remove_staged_package, stage_zip_package


PAYLOAD = b"NEXUS staged ELF payload"
RECIPE = {
    "id": "demo",
    "version": "1.2",
    "archive_format": "zip",
    "target": "mass",
    "files": [
        {"source": "APP/main.elf", "destination": "NEXUS/demo/main.elf",
         "sha256": hashlib.sha256(PAYLOAD).hexdigest()}
    ],
}


class StagingTests(unittest.TestCase):
    def make_zip(self, location, entries):
        archive = Path(location) / "demo.zip"
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zf:
            for name, data in entries:
                zf.writestr(name, data)
        return archive

    def test_stages_only_recipe_mapped_files(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            archive = self.make_zip(td, [("APP/main.elf", PAYLOAD), ("README.txt", b"ignored")])
            staged = stage_zip_package(archive, RECIPE, root)
            payload_root = Path(staged["payload_dir"])
            self.assertEqual((payload_root / "APP/main.elf").read_bytes(), PAYLOAD)
            self.assertFalse((payload_root / "README.txt").exists())
            self.assertEqual(staged["manifest"]["files"][0]["path"], "NEXUS/demo/main.elf")
            self.assertEqual(staged["file_count"], 1)
            self.assertTrue(staged["extraction_performed"])
            self.assertFalse(staged["device_write_performed"])
            self.assertFalse(staged["installer_execution_enabled"])

    def test_staged_manifest_is_compatible_with_transaction_engine(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            archive = self.make_zip(td, [("APP/main.elf", PAYLOAD)])
            staged = stage_zip_package(archive, RECIPE, root)
            entry = staged["manifest"]["files"][0]
            self.assertEqual(entry["source"], "APP/main.elf")
            self.assertEqual(entry["sha256"], hashlib.sha256((Path(staged["payload_dir"]) / entry["source"]).read_bytes()).hexdigest())

    def test_hash_mismatch_fails_without_leaving_partial_stage(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            root.mkdir()
            archive = self.make_zip(td, [("APP/main.elf", b"changed")])
            with self.assertRaises(StagingError):
                stage_zip_package(archive, RECIPE, root)
            self.assertEqual(list(root.iterdir()), [])

    def test_missing_recipe_member_fails(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            archive = self.make_zip(td, [("OTHER/file.bin", b"x")])
            with self.assertRaises(StagingError):
                stage_zip_package(archive, RECIPE, root)
            self.assertFalse(root.exists())

    def test_psu_and_sas_are_not_staged(self):
        for fmt in ("psu", "sas"):
            with self.subTest(fmt=fmt), tempfile.TemporaryDirectory() as td:
                recipe = {**RECIPE, "archive_format": fmt}
                with self.assertRaises(StagingError):
                    stage_zip_package("not-opened", recipe, Path(td) / "cache")

    def test_refuses_symlink_staging_root(self):
        with tempfile.TemporaryDirectory() as td:
            real = Path(td) / "real"
            real.mkdir()
            link = Path(td) / "link"
            link.symlink_to(real, target_is_directory=True)
            archive = self.make_zip(td, [("APP/main.elf", PAYLOAD)])
            with self.assertRaises(StagingError):
                stage_zip_package(archive, RECIPE, link)

    def test_removes_only_direct_child_staging_directory(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            archive = self.make_zip(td, [("APP/main.elf", PAYLOAD)])
            staged = stage_zip_package(archive, RECIPE, root)
            stage_dir = Path(staged["stage_dir"])
            remove_staged_package(stage_dir, root)
            self.assertFalse(stage_dir.exists())

    def test_refuses_remove_outside_staging_root(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            other = Path(td) / "nexus-stage-keep"
            other.mkdir()
            with self.assertRaises(StagingError):
                remove_staged_package(other, root)


if __name__ == "__main__":
    unittest.main()

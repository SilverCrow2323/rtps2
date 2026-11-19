import os
import stat
import tempfile
import unittest
import zipfile
from pathlib import Path

from nexus_txn.inspectors import ArchiveInspectionError, inspect_archive, inspect_zip, match_recipe


class InspectorTests(unittest.TestCase):
    def make_zip(self, root, members):
        path = Path(root) / "sample.zip"
        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            for name, data in members:
                zf.writestr(name, data)
        return path

    def test_inspect_hashes_without_extracting(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("APP/BOOT.ELF", b"not-a-protected-root-boot")])
            # BOOT.ELF is deliberately protected at every path depth.
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path)
            self.assertEqual(sorted(os.listdir(td)), ["sample.zip"])

    def test_valid_archive_reports_hash_and_no_writes(self):
        import hashlib
        payload = b"NEXUS read-only package"
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("APP/main.elf", payload), ("README.txt", b"notes")])
            report = inspect_zip(path)
            self.assertEqual(report["status"], "ok")
            self.assertEqual(report["file_count"], 2)
            self.assertEqual(report["entries"][0]["sha256"], hashlib.sha256(payload).hexdigest())
            self.assertFalse(report["extraction_performed"])
            self.assertFalse(report["device_write_performed"])
            self.assertEqual(os.listdir(td), ["sample.zip"])

    def test_rejects_traversal(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("../escape.txt", b"x")])
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path)

    def test_rejects_absolute_paths(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("/absolute.txt", b"x")])
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path)

    def test_rejects_backslash_paths(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [(r"..\\escape.txt", b"x")])
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path)

    def test_rejects_protected_paths(self):
        for name in ("BOOT.ELF", "SYS-CONF/config.dat", "foo/PSBBN/start.elf", "FMCB/config"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as td:
                path = self.make_zip(td, [(name, b"x")])
                with self.assertRaises(ArchiveInspectionError):
                    inspect_zip(path)

    def test_rejects_case_collisions(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("APP/File.ELF", b"a"), ("app/file.elf", b"b")])
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path)

    def test_rejects_expanded_size_limit(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("big.bin", b"A" * 4096)])
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path, max_member_size=1024, max_compression_ratio=10000)

    def test_rejects_entry_count_limit(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("a", b"a"), ("b", b"b")])
            with self.assertRaises(ArchiveInspectionError):
                inspect_zip(path, max_entries=1)

    def test_psu_and_sas_are_explicitly_unsupported(self):
        for fmt in ("psu", "sas"):
            report = inspect_archive("not-opened.bin", fmt)
            self.assertEqual(report["status"], "unsupported")
            self.assertEqual(report["format"], fmt)
            self.assertFalse(report["extraction_performed"])

    def test_rejects_unknown_format(self):
        with self.assertRaises(ArchiveInspectionError):
            inspect_archive("package.bin", "7z")

    def test_recipe_mapping_checks_hashes(self):
        import hashlib
        payload = b"ELF placeholder"
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("APP/main.elf", payload), ("README", b"extra")])
            inspected = inspect_zip(path)
            recipe = {"archive_format": "zip", "files": [
                {"source": "APP/main.elf", "destination": "NEXUS/main.elf",
                 "sha256": hashlib.sha256(payload).hexdigest()}
            ]}
            matched = match_recipe(inspected, recipe)
            self.assertTrue(matched["matched"])
            self.assertEqual(matched["unmapped_archive_files"], ["README"])

    def test_recipe_mapping_rejects_hash_mismatch(self):
        with tempfile.TemporaryDirectory() as td:
            path = self.make_zip(td, [("APP/main.elf", b"payload")])
            matched = match_recipe(inspect_zip(path), {"archive_format": "zip", "files": [
                {"source": "APP/main.elf", "destination": "NEXUS/main.elf", "sha256": "0" * 64}
            ]})
            self.assertFalse(matched["matched"])
            self.assertEqual(matched["hash_mismatches"], ["APP/main.elf"])


if __name__ == "__main__":
    unittest.main()

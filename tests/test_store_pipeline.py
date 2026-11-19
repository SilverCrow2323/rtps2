import hashlib
import tempfile
import unittest
import zipfile
from pathlib import Path

from nexus_txn.store_flow import StoreFlow
from nexus_txn.store_pipeline import (
    StorePipelineError,
    inspect_download_for_store,
    stage_download_for_store,
)


class StorePipelineTests(unittest.TestCase):
    def make_flow(self):
        flow = StoreFlow(selected_ids=["demo"])
        flow.advance("DEPENDENCIES")
        flow.advance("SPACE")
        flow.set_space_check("pass")
        flow.advance("CONFIRM")
        flow.advance("DOWNLOAD")
        flow.set_progress(100)
        flow.advance("VERIFY")
        flow.mark_verified(True)
        flow.advance("INSPECT")
        return flow

    def recipe(self, payload, fmt="zip"):
        return {
            "id": "demo",
            "version": "1.0",
            "archive_format": fmt,
            "target": "mass",
            "files": [{
                "source": "APP/main.elf",
                "destination": "NEXUS/main.elf",
                "sha256": hashlib.sha256(payload).hexdigest(),
            }],
        }

    def test_zip_inspection_and_staging_gate_install_ready(self):
        payload = b"NEXUS ELF payload"
        with tempfile.TemporaryDirectory() as td:
            archive = Path(td) / "demo.zip"
            with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zf:
                zf.writestr("APP/main.elf", payload)
                zf.writestr("README.txt", b"extra")
            flow = self.make_flow()
            inspected = inspect_download_for_store(flow, archive, self.recipe(payload))
            self.assertTrue(inspected["store_ready"])
            flow.advance("STAGE")
            staged = stage_download_for_store(flow, archive, self.recipe(payload), Path(td) / "cache")
            self.assertTrue(staged["store_ready"])
            self.assertEqual(flow.advance("READY_TO_INSTALL")["stage"], "READY_TO_INSTALL")
            self.assertFalse(staged["device_write_performed"])
            self.assertTrue((Path(staged["payload_dir"]) / "APP/main.elf").exists())
            self.assertFalse((Path(staged["payload_dir"]) / "README.txt").exists())

    def test_zip_hash_mismatch_blocks_store(self):
        with tempfile.TemporaryDirectory() as td:
            archive = Path(td) / "demo.zip"
            with zipfile.ZipFile(archive, "w") as zf:
                zf.writestr("APP/main.elf", b"tampered")
            flow = self.make_flow()
            report = inspect_download_for_store(flow, archive, self.recipe(b"expected"))
            self.assertFalse(report["store_ready"])
            self.assertEqual(flow.stage, "FAILED")

    def test_psu_and_sas_fail_closed_without_opening_archive(self):
        for fmt in ("psu", "sas"):
            with self.subTest(fmt=fmt):
                flow = self.make_flow()
                report = inspect_download_for_store(flow, "does-not-exist", self.recipe(b"x", fmt))
                self.assertEqual(report["status"], "unsupported")
                self.assertFalse(report["store_ready"])
                self.assertEqual(flow.stage, "FAILED")

    def test_invalid_zip_fails_flow(self):
        with tempfile.TemporaryDirectory() as td:
            archive = Path(td) / "bad.zip"
            archive.write_bytes(b"not a zip")
            flow = self.make_flow()
            report = inspect_download_for_store(flow, archive, self.recipe(b"x"))
            self.assertEqual(report["status"], "invalid")
            self.assertEqual(flow.stage, "FAILED")

    def test_recipe_validation_failure_fails_flow(self):
        flow = self.make_flow()
        report = inspect_download_for_store(flow, "unused", {"id": "demo"})
        self.assertFalse(report["store_ready"])
        self.assertEqual(flow.stage, "FAILED")

    def test_staging_failure_fails_flow(self):
        payload = b"expected"
        with tempfile.TemporaryDirectory() as td:
            archive = Path(td) / "demo.zip"
            with zipfile.ZipFile(archive, "w") as zf:
                zf.writestr("APP/main.elf", b"changed")
            flow = self.make_flow()
            inspect_download_for_store(flow, archive, self.recipe(payload))
            self.assertEqual(flow.stage, "FAILED")

    def test_requires_verified_inspection_stage(self):
        flow = StoreFlow(selected_ids=["demo"])
        with self.assertRaises(StorePipelineError):
            inspect_download_for_store(flow, "unused", self.recipe(b"x"))

    def test_requires_inspected_staging_stage(self):
        flow = self.make_flow()
        with self.assertRaises(StorePipelineError):
            stage_download_for_store(flow, "unused", self.recipe(b"x"), Path("cache"))


if __name__ == "__main__":
    unittest.main()

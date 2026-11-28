import tempfile
import unittest
from pathlib import Path

from nexus_txn.staging import StagingError, remove_staged_package


class StagingRootSafetyTests(unittest.TestCase):
    def test_removal_refuses_a_symbolic_link_staging_root(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "real-cache"
            root.mkdir()
            stage = root / "nexus-stage-demo"
            stage.mkdir()
            link = Path(td) / "cache-link"
            link.symlink_to(root, target_is_directory=True)

            with self.assertRaisesRegex(StagingError, "symbolic link"):
                remove_staged_package(stage, link)

            self.assertTrue(stage.is_dir(), "refused removal must leave staged data intact")

    def test_removal_still_accepts_a_direct_child_of_real_staging_root(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "cache"
            root.mkdir()
            stage = root / "nexus-stage-demo"
            stage.mkdir()
            (stage / "payload.bin").write_bytes(b"safe test payload")

            remove_staged_package(stage, root)

            self.assertFalse(stage.exists())


if __name__ == "__main__":
    unittest.main()

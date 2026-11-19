import unittest

from nexus_txn.store_flow import StoreFlow, StoreFlowError


class StoreFlowTests(unittest.TestCase):
    def make_confirmed(self):
        flow = StoreFlow(selected_ids=["opl"])
        flow.advance("DEPENDENCIES")
        flow.advance("SPACE")
        flow.set_space_check("pass")
        flow.advance("CONFIRM")
        return flow

    def make_verified(self):
        flow = self.make_confirmed()
        flow.advance("DOWNLOAD")
        flow.set_progress(45)
        flow.set_progress(100)
        flow.advance("VERIFY")
        flow.mark_verified(True)
        return flow

    def test_complete_preinstall_workflow(self):
        flow = self.make_verified()
        flow.advance("INSPECT")
        flow.mark_inspected(True)
        out = flow.advance("READY_TO_INSTALL")
        self.assertEqual(out["stage"], "READY_TO_INSTALL")
        self.assertTrue(out["verified"])
        self.assertTrue(out["inspected"])
        self.assertFalse(out["device_write_performed"])
        self.assertFalse(out["installer_execution_enabled"])

    def test_cannot_confirm_unknown_space(self):
        flow = StoreFlow(selected_ids=["opl"])
        flow.advance("DEPENDENCIES")
        flow.advance("SPACE")
        with self.assertRaises(StoreFlowError):
            flow.advance("CONFIRM")

    def test_cannot_verify_before_download_complete(self):
        flow = self.make_confirmed()
        flow.advance("DOWNLOAD")
        flow.set_progress(50)
        with self.assertRaises(StoreFlowError):
            flow.advance("VERIFY")

    def test_progress_monotonic(self):
        flow = self.make_confirmed()
        flow.advance("DOWNLOAD")
        flow.set_progress(80)
        with self.assertRaises(StoreFlowError):
            flow.set_progress(70)

    def test_failed_verification_blocks_inspection(self):
        flow = self.make_confirmed()
        flow.advance("DOWNLOAD")
        flow.set_progress(100)
        flow.advance("VERIFY")
        flow.mark_verified(False)
        self.assertEqual(flow.stage, "FAILED")

    def test_failed_inspection_blocks_install(self):
        flow = self.make_verified()
        flow.advance("INSPECT")
        flow.mark_inspected(False)
        self.assertEqual(flow.stage, "FAILED")
        with self.assertRaises(StoreFlowError):
            flow.advance("READY_TO_INSTALL")

    def test_cannot_enter_inspection_without_verification(self):
        flow = self.make_confirmed()
        flow.advance("DOWNLOAD")
        flow.set_progress(100)
        flow.advance("VERIFY")
        with self.assertRaises(StoreFlowError):
            flow.advance("INSPECT")

    def test_cannot_skip_inspection(self):
        flow = self.make_verified()
        with self.assertRaises(StoreFlowError):
            flow.advance("READY_TO_INSTALL")

    def test_requires_selection(self):
        with self.assertRaises(StoreFlowError):
            StoreFlow().advance("DEPENDENCIES")

    def test_cannot_skip_stages(self):
        with self.assertRaises(StoreFlowError):
            self.make_confirmed().advance("READY_TO_INSTALL")

    def test_unknown_space_not_accepted(self):
        flow = StoreFlow(selected_ids=["opl"])
        flow.advance("DEPENDENCIES")
        flow.advance("SPACE")
        flow.set_space_check("unknown")
        with self.assertRaises(StoreFlowError):
            flow.advance("CONFIRM")


if __name__ == "__main__":
    unittest.main()

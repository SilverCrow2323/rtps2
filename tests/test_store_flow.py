import unittest
from nexus_txn.store_flow import StoreFlow, StoreFlowError
class StoreFlowTests(unittest.TestCase):
 def make_confirmed(self):
  f=StoreFlow(selected_ids=["opl"]); f.advance("DEPENDENCIES"); f.advance("SPACE"); f.set_space_check("pass"); f.advance("CONFIRM"); return f
 def test_complete_preinstall_workflow(self):
  f=self.make_confirmed(); f.advance("DOWNLOAD"); f.set_progress(45); f.set_progress(100); f.advance("VERIFY"); f.mark_verified(True); out=f.advance("READY_TO_INSTALL")
  self.assertEqual(out["stage"],"READY_TO_INSTALL"); self.assertFalse(out["device_write_performed"]); self.assertFalse(out["installer_execution_enabled"])
 def test_cannot_confirm_unknown_space(self):
  f=StoreFlow(selected_ids=["opl"]); f.advance("DEPENDENCIES"); f.advance("SPACE")
  with self.assertRaises(StoreFlowError): f.advance("CONFIRM")
 def test_cannot_verify_before_download_complete(self):
  f=self.make_confirmed(); f.advance("DOWNLOAD"); f.set_progress(50)
  with self.assertRaises(StoreFlowError): f.advance("VERIFY")
 def test_progress_monotonic(self):
  f=self.make_confirmed(); f.advance("DOWNLOAD"); f.set_progress(80)
  with self.assertRaises(StoreFlowError): f.set_progress(70)
 def test_failed_verification_blocks_install(self):
  f=self.make_confirmed(); f.advance("DOWNLOAD"); f.set_progress(100); f.advance("VERIFY"); f.mark_verified(False)
  self.assertEqual(f.stage,"FAILED")
 def test_requires_selection(self):
  with self.assertRaises(StoreFlowError): StoreFlow().advance("DEPENDENCIES")
 def test_cannot_skip_stages(self):
  with self.assertRaises(StoreFlowError): self.make_confirmed().advance("READY_TO_INSTALL")
 def test_unknown_space_not_accepted(self):
  f=StoreFlow(selected_ids=["opl"]); f.advance("DEPENDENCIES"); f.advance("SPACE"); f.set_space_check("unknown")
  with self.assertRaises(StoreFlowError): f.advance("CONFIRM")
if __name__=="__main__": unittest.main()

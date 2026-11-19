import unittest
from nexus_txn.store import plan_install, plan_remove, profile_packages, StorePlanError
from nexus_txn.tui import render_progress, render_plan

PKGS = [
 {"id":"core","name":"NEXUS Core","version":"1.0","installed_size":100,"tags":["core"]},
 {"id":"opl","name":"OPL","version":"2.0","installed_size":200,"tags":["loader","gaming"],"requires":["core"]},
 {"id":"pops","name":"POPS","version":"1.2","installed_size":300,"tags":["emulator"],"requires":["core"]},
 {"id":"diag","name":"Diagnostics","version":"1.0","installed_size":50,"tags":["diagnostic"]},
]

class StoreTests(unittest.TestCase):
 def test_profile(self): self.assertEqual(profile_packages(PKGS,"base"),["core"])
 def test_dependencies_and_size(self):
  p=plan_install(PKGS,["opl"],free_bytes=500,reserve_bytes=10)
  self.assertEqual([x["id"] for x in p["actions"]],["core","opl"])
  self.assertEqual(p["required_bytes"],310); self.assertEqual(p["space_check"],"pass")
 def test_unknown_space_is_not_pass(self): self.assertEqual(plan_install(PKGS,["core"])["space_check"],"unknown")
 def test_insufficient_space(self): self.assertEqual(plan_install(PKGS,["opl"],free_bytes=200)["space_check"],"insufficient")
 def test_keep_existing(self): self.assertEqual(plan_install(PKGS,["core"],{"core":"1.0"},0)["actions"][0]["action"],"keep")
 def test_update_peak_estimate(self):
  p=plan_install(PKGS,["core"],{"core":"0.9"},free_bytes=200)
  self.assertEqual(p["required_bytes"],200)
 def test_remove_dependency_blocked(self):
  with self.assertRaises(StorePlanError): plan_remove(PKGS,{"core":"1.0","opl":"2.0"},["core"])
 def test_remove_preview(self): self.assertEqual(plan_remove(PKGS,{"core":"1.0"},["core"])["remove"],["core"])
 def test_invalid_catalog_size(self):
  with self.assertRaises(StorePlanError): plan_install([{**PKGS[0],"installed_size":-1}],["core"])
 def test_tui_progress(self):
  out=render_progress("Store","Download",50)
  self.assertIn("50%",out); self.assertIn("[##############",out)
 def test_tui_rejects_percent(self):
  with self.assertRaises(ValueError): render_progress("x","y",101)
 def test_plan_output(self): self.assertIn("NO DEVICE WRITE",render_plan(plan_install(PKGS,["core"])))

if __name__ == "__main__": unittest.main()

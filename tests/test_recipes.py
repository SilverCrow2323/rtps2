import unittest
from nexus_txn.recipes import validate_recipe, RecipeError

BASE = {"id":"sample","version":"1.0","archive_format":"zip","target":"mass",
        "files":[{"source":"bin/app.elf","destination":"APPS/sample/app.elf","sha256":"a"*64}],
        "requires":["runtime"],"post_install":"refresh-index"}

class RecipeTests(unittest.TestCase):
    def test_valid_recipe_is_normalized_and_non_executable(self):
        out = validate_recipe(BASE)
        self.assertEqual(out["files"][0]["destination"], "APPS/sample/app.elf")
        self.assertFalse(out["execution_enabled"])
    def test_reject_unknown_archive_format(self):
        with self.assertRaises(RecipeError): validate_recipe({**BASE,"archive_format":"7z"})
    def test_reject_unknown_target(self):
        with self.assertRaises(RecipeError): validate_recipe({**BASE,"target":"hdd-root"})
    def test_reject_traversal(self):
        bad = {**BASE,"files":[{**BASE["files"][0],"destination":"../BOOT.ELF"}]}
        with self.assertRaises(RecipeError): validate_recipe(bad)
    def test_reject_protected_destination(self):
        bad = {**BASE,"files":[{**BASE["files"][0],"destination":"SYS-CONF/BOOT.ELF"}]}
        with self.assertRaises(RecipeError): validate_recipe(bad)
    def test_reject_duplicate_destinations_case_insensitive(self):
        files = BASE["files"] + [{**BASE["files"][0],"source":"other.elf","destination":"apps/SAMPLE/app.elf"}]
        with self.assertRaises(RecipeError): validate_recipe({**BASE,"files":files})
    def test_reject_bad_hash(self):
        bad = {**BASE,"files":[{**BASE["files"][0],"sha256":"not-a-hash"}]}
        with self.assertRaises(RecipeError): validate_recipe(bad)
    def test_reject_duplicate_dependencies(self):
        with self.assertRaises(RecipeError): validate_recipe({**BASE,"requires":["runtime","runtime"]})
    def test_reject_unapproved_post_install(self):
        with self.assertRaises(RecipeError): validate_recipe({**BASE,"post_install":"run-shell"})

if __name__ == "__main__": unittest.main()

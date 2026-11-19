import unittest
from nexus_txn.dependencies import resolve, DependencyError

class DependencyTests(unittest.TestCase):
    def setUp(self):
        self.pkgs = [
            {'id':'runtime','version':'1.0'},
            {'id':'frontend','version':'2.0','requires':['runtime']},
            {'id':'addon','version':'1.1','requires':['frontend']},
        ]
    def test_topological_order(self):
        self.assertEqual([x['id'] for x in resolve(self.pkgs,['addon'])], ['runtime','frontend','addon'])
    def test_missing_dependency(self):
        self.pkgs[1]['requires']=['missing']
        with self.assertRaises(DependencyError): resolve(self.pkgs,['frontend'])
    def test_cycle(self):
        self.pkgs[0]['requires']=['frontend']
        with self.assertRaises(DependencyError): resolve(self.pkgs,['frontend'])
    def test_conflict(self):
        self.pkgs.append({'id':'bad-addon','version':'1','conflicts':['addon']})
        with self.assertRaises(DependencyError): resolve(self.pkgs,['addon','bad-addon'])
    def test_duplicate_id(self):
        self.pkgs.append(dict(self.pkgs[0]))
        with self.assertRaises(DependencyError): resolve(self.pkgs,['runtime'])

if __name__=='__main__': unittest.main()

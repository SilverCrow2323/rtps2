import hashlib, json, tempfile, unittest
from pathlib import Path
from nexus_txn.engine import Transaction, TransactionError
class TxTests(unittest.TestCase):
 def setUp(self):
  self.t=tempfile.TemporaryDirectory(); self.base=Path(self.t.name); self.root=self.base/'root'; self.root.mkdir(); self.payload=self.base/'payload'; self.payload.mkdir(); self.state=self.base/'state'
  (self.payload/'new.bin').write_bytes(b'new-data'); (self.root/'app.bin').write_bytes(b'old-data')
  self.manifest={'files':[{'path':'apps/demo/app.bin','source':'new.bin','sha256':hashlib.sha256(b'new-data').hexdigest()}]}
 def tearDown(self): self.t.cleanup()
 def test_commit(self):
  tx=Transaction(self.root,self.state); r=tx.install(self.manifest,self.payload); self.assertEqual(r['state'],'COMMITTED'); self.assertEqual((self.root/'apps/demo/app.bin').read_bytes(),b'new-data')
 def test_rollback_new(self):
  tx=Transaction(self.root,self.state)
  with self.assertRaises(TransactionError): tx.install(self.manifest,self.payload,1)
  self.assertFalse((self.root/'apps/demo/app.bin').exists()); self.assertEqual(json.loads(tx.journal.read_text())['state'],'ROLLED_BACK')
 def test_restore_existing(self):
  (self.root/'apps').mkdir(); (self.root/'apps/demo').mkdir(); (self.root/'apps/demo/app.bin').write_bytes(b'original')
  tx=Transaction(self.root,self.state)
  with self.assertRaises(TransactionError): tx.install(self.manifest,self.payload,1)
  self.assertEqual((self.root/'apps/demo/app.bin').read_bytes(),b'original')
 def test_protected(self):
  self.manifest['files'][0]['path']='BOOT/BOOT.ELF'
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
 def test_traversal(self):
  self.manifest['files'][0]['path']='../../BOOT.ELF'
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
 def test_backslash_path_rejected(self):
  self.manifest['files'][0]['path']='apps\\\\demo\\\\app.bin'
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
 def test_hash(self):
  self.manifest['files'][0]['sha256']='0'*64
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
 def test_symlink_escape_rejected(self):
  outside=self.base/'outside'; outside.mkdir(); (self.root/'apps').symlink_to(outside, target_is_directory=True)
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
  self.assertFalse((outside/'demo/app.bin').exists())
 def test_duplicate_targets_rejected(self):
  self.manifest['files'].append(dict(self.manifest['files'][0]))
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
 def test_lock_prevents_concurrent_writer(self):
  tx=Transaction(self.root,self.state); tx.lock.write_text('other process')
  with self.assertRaises(TransactionError): tx.install(self.manifest,self.payload)
 def test_state_tree_separation(self):
  with self.assertRaises(TransactionError): Transaction(self.root,self.root/'state')
 def test_recovery_rejects_tampered_root(self):
  tx=Transaction(self.root,self.state); tx.backups.mkdir(parents=True)
  rec={'schema':1,'id':'bad','state':'APPLYING','root':str(self.base),'entries':[],'completed':0}; tx._write(rec)
  with self.assertRaises(TransactionError): tx.recover()
 def test_source_traversal_rejected(self):
  self.manifest['files'][0]['source']='../../secret'
  with self.assertRaises(TransactionError): Transaction(self.root,self.state).install(self.manifest,self.payload)
 def test_recover_incomplete_journal(self):
  tx=Transaction(self.root,self.state); tx.backups.mkdir(parents=True)
  target=self.root/'apps/demo/app.bin'; target.parent.mkdir(parents=True); target.write_bytes(b'new-data')
  backup=tx.backups/'1.bak'; backup.write_bytes(b'old-version')
  record={'schema':1,'id':'crash-test','state':'APPLYING','root':str(self.root),'completed':1,'entries':[{'path':'apps/demo/app.bin','source':'unused','sha256':'0'*64,'existed':True,'backup':str(backup)}]}
  tx._write(record); out=tx.recover()
  self.assertEqual(out['state'],'ROLLED_BACK'); self.assertEqual(target.read_bytes(),b'old-version')
if __name__=='__main__': unittest.main()

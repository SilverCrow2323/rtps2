import hashlib, tempfile, unittest
from pathlib import Path
from nexus_txn.bridge import BridgeClient, BridgeError

DATA = b"PS2BBN NEXUS package fixture / " * 9
DIGEST = hashlib.sha256(DATA).hexdigest()
PACKAGE = {"id":"demo","version":"1.0","name":"Demo package","size":len(DATA),
           "installed_size":4096,"sha256":DIGEST,"download_path":"/cache/demo.pkg","requires":[]}

class FakeTransport:
    def __init__(self): self.calls=[]; self.fail_status=None
    def __call__(self, method, path, headers, body):
        self.calls.append((method,path,dict(headers)))
        if path == "/api/v0.4/catalog":
            return {"status":200,"headers":{"Content-Type":"application/json"},"body":{"packages":[PACKAGE]}}
        if self.fail_status: return {"status":self.fail_status,"headers":{},"body":b""}
        r=headers.get("Range")
        if not r: return {"status":400,"headers":{},"body":b""}
        start,end=[int(x) for x in r[6:].split("-")]
        chunk=DATA[start:end+1]
        return {"status":206,"headers":{"Content-Range":f"bytes {start}-{start+len(chunk)-1}/{len(DATA)}"},"body":chunk}

class BridgeTests(unittest.TestCase):
    def setUp(self): self.transport=FakeTransport(); self.client=BridgeClient(self.transport,"secret-token")
    def test_catalog_validation(self):
        out=self.client.catalog(); self.assertEqual(out["count"],1); self.assertEqual(out["packages"][0]["id"],"demo")
    def test_store_catalog_requires_separate_install_size(self):
        self.assertEqual(self.client.store_catalog()[0]["installed_size"],4096)
    def test_store_catalog_refuses_missing_install_size(self):
        class NoInstallSize:
            def __call__(self,*args):
                p={k:v for k,v in PACKAGE.items() if k!="installed_size"}
                return {"status":200,"body":{"packages":[p]}}
        with self.assertRaises(BridgeError): BridgeClient(NoInstallSize(),"x").store_catalog()
    def test_auth_header_sent_without_logging_token(self):
        self.client.catalog(); self.assertEqual(self.transport.calls[0][2]["Authorization"],"Bearer secret-token")
    def test_range_download_and_hash(self):
        with tempfile.TemporaryDirectory() as td:
            out=self.client.download(PACKAGE,Path(td)/"demo.pkg",chunk_size=37)
            self.assertTrue(out["verified"]); self.assertEqual((Path(td)/"demo.pkg").read_bytes(),DATA)
            self.assertGreater(len(self.transport.calls),1)
    def test_resume_partial_cache(self):
        with tempfile.TemporaryDirectory() as td:
            dest=Path(td)/"demo.pkg"; part=dest.with_name(dest.name+".part"); part.write_bytes(DATA[:50])
            out=self.client.download(PACKAGE,dest,chunk_size=31)
            self.assertTrue(out["verified"]); self.assertEqual(dest.read_bytes(),DATA)
            self.assertTrue(any("bytes=50-" in c[2].get("Range","") for c in self.transport.calls))
    def test_http_error(self):
        self.transport.fail_status=503
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError): self.client.download(PACKAGE,Path(td)/"demo.pkg")
    def test_reject_bad_content_range(self):
        original=self.transport
        def bad(method,path,headers,body):
            r=original(method,path,headers,body)
            if path=="/cache/demo.pkg": r["headers"]["Content-Range"]="bytes 0-1/999"
            return r
        self.client=BridgeClient(bad,"token")
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError): self.client.download(PACKAGE,Path(td)/"demo.pkg")
    def test_reject_hash_mismatch(self):
        bad={**PACKAGE,"sha256":"0"*64}
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError): self.client.download(bad,Path(td)/"demo.pkg")
    def test_reject_unsafe_package_id(self):
        with self.assertRaises(BridgeError): self.client.download({**PACKAGE,"id":"../oops"},Path("/tmp/oops"))
    def test_reject_unsafe_download_path(self):
        with self.assertRaises(BridgeError): self.client.download({**PACKAGE,"download_path":"/../../secret"},Path("/tmp/oops"))
    def test_catalog_rejects_duplicate_ids(self):
        self.transport.__call__ = lambda *args: {"status":200,"body":{"packages":[PACKAGE,PACKAGE]}}
        class DuplicateTransport:
            def __call__(self,*args): return {"status":200,"body":{"packages":[PACKAGE,PACKAGE]}}
        with self.assertRaises(BridgeError): BridgeClient(DuplicateTransport(),"x").catalog()

if __name__ == "__main__": unittest.main()

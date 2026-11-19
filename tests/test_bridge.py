import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from nexus_txn.bridge import BridgeClient, BridgeError
from nexus_txn.trust import CatalogTrustPolicy


DATA = b"PS2BBN NEXUS package fixture / " * 9
DIGEST = hashlib.sha256(DATA).hexdigest()
PACKAGE = {
    "id": "demo",
    "version": "1.0",
    "name": "Demo package",
    "size": len(DATA),
    "installed_size": 4096,
    "sha256": DIGEST,
    "download_path": "/cache/demo.pkg",
    "requires": [],
}
NOW = 1_800_000_000
_PRIVATE = Ed25519PrivateKey.from_private_bytes(bytes(range(32)))
_PUBLIC = _PRIVATE.public_key().public_bytes(
    encoding=serialization.Encoding.Raw,
    format=serialization.PublicFormat.Raw,
)
TRUST = CatalogTrustPolicy({"test-release": _PUBLIC}, clock=lambda: NOW)


def signed_catalog(packages):
    payload = json.dumps(
        {"issued_at": NOW - 60, "expires_at": NOW + 3600, "packages": packages},
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")
    import base64
    envelope = {
        "schema": "nexus.signed-catalog.v1",
        "key_id": "test-release",
        "algorithm": "Ed25519",
        "payload": base64.urlsafe_b64encode(payload).decode("ascii").rstrip("="),
        "signature": base64.urlsafe_b64encode(_PRIVATE.sign(payload)).decode("ascii").rstrip("="),
    }
    return json.dumps(envelope, separators=(",", ":"))


class FakeTransport:
    def __init__(self, packages=None):
        self.calls = []
        self.fail_status = None
        self.catalog_body = signed_catalog(packages if packages is not None else [PACKAGE])

    def __call__(self, method, path, headers, body):
        self.calls.append((method, path, dict(headers)))
        if path == "/api/v0.4/catalog":
            return {
                "status": 200,
                "headers": {"Content-Type": "application/json"},
                "body": self.catalog_body,
            }
        if self.fail_status:
            return {"status": self.fail_status, "headers": {}, "body": b""}
        request_range = headers.get("Range")
        if not request_range:
            return {"status": 400, "headers": {}, "body": b""}
        start, end = [int(x) for x in request_range[6:].split("-")]
        chunk = DATA[start:end + 1]
        return {
            "status": 206,
            "headers": {"Content-Range": f"bytes {start}-{start + len(chunk) - 1}/{len(DATA)}"},
            "body": chunk,
        }


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.transport = FakeTransport()
        self.client = BridgeClient(self.transport, "secret-token", trust_policy=TRUST)
        self.package = self.client.catalog()["packages"][0]
        self.transport.calls.clear()

    def test_catalog_validation_and_signature_metadata(self):
        out = self.client.catalog()
        self.assertEqual(out["count"], 1)
        self.assertEqual(out["packages"][0]["id"], "demo")
        self.assertEqual(out["trust"]["algorithm"], "Ed25519")
        self.assertTrue(out["trust"]["signature_verified"])

    def test_unsigned_catalog_is_refused(self):
        class UnsignedTransport:
            def __call__(self, *args):
                return {"status": 200, "body": {"packages": [PACKAGE]}}
        client = BridgeClient(UnsignedTransport(), "x", trust_policy=TRUST)
        with self.assertRaises(BridgeError):
            client.catalog()

    def test_catalog_requires_pinned_trust_policy(self):
        client = BridgeClient(self.transport, "x")
        with self.assertRaises(BridgeError):
            client.catalog()

    def test_store_catalog_requires_separate_install_size(self):
        self.assertEqual(self.client.store_catalog()[0]["installed_size"], 4096)

    def test_store_catalog_refuses_missing_install_size(self):
        package = {key: value for key, value in PACKAGE.items() if key != "installed_size"}
        class NoInstallSize:
            def __call__(self, *args):
                return {"status": 200, "body": signed_catalog([package])}
        with self.assertRaises(BridgeError):
            BridgeClient(NoInstallSize(), "x", trust_policy=TRUST).store_catalog()

    def test_auth_header_sent_without_logging_token(self):
        self.client.catalog()
        self.assertEqual(self.transport.calls[0][2]["Authorization"], "Bearer secret-token")

    def test_range_download_and_hash(self):
        with tempfile.TemporaryDirectory() as td:
            out = self.client.download(self.package, Path(td) / "demo.pkg", chunk_size=37)
            self.assertTrue(out["verified"])
            self.assertEqual((Path(td) / "demo.pkg").read_bytes(), DATA)
            self.assertGreater(len(self.transport.calls), 1)
            self.assertEqual(out["catalog_trust"], "ed25519-verified")

    def test_resume_partial_cache(self):
        with tempfile.TemporaryDirectory() as td:
            destination = Path(td) / "demo.pkg"
            partial = destination.with_name(destination.name + ".part")
            partial.write_bytes(DATA[:50])
            out = self.client.download(self.package, destination, chunk_size=31)
            self.assertTrue(out["verified"])
            self.assertEqual(destination.read_bytes(), DATA)
            self.assertTrue(any("bytes=50-" in call[2].get("Range", "") for call in self.transport.calls))

    def test_http_error(self):
        self.transport.fail_status = 503
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError):
                self.client.download(self.package, Path(td) / "demo.pkg")

    def test_reject_bad_content_range(self):
        original = FakeTransport()
        def bad(method, path, headers, body):
            response = original(method, path, headers, body)
            if path == "/cache/demo.pkg":
                response["headers"]["Content-Range"] = "bytes 0-1/999"
            return response
        self.client = BridgeClient(bad, "token", trust_policy=TRUST)
        self.package = self.client.catalog()["packages"][0]
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError):
                self.client.download(self.package, Path(td) / "demo.pkg")

    def test_reject_hash_mismatch_from_a_validly_signed_catalog(self):
        bad_package = {**PACKAGE, "sha256": "0" * 64}
        transport = FakeTransport([bad_package])
        client = BridgeClient(transport, "token", trust_policy=TRUST)
        trusted_record = client.catalog()["packages"][0]
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError):
                client.download(trusted_record, Path(td) / "demo.pkg")

    def test_reject_unsafe_package_id(self):
        with self.assertRaises(BridgeError):
            self.client.download({**self.package, "id": "../oops"}, Path("/tmp/oops"))

    def test_reject_unsafe_download_path(self):
        with self.assertRaises(BridgeError):
            self.client.download({**self.package, "download_path": "/../../secret"}, Path("/tmp/oops"))

    def test_reject_encoded_path_traversal(self):
        with self.assertRaises(BridgeError):
            self.client.download({**self.package, "download_path": "/%2e%2e/secret"}, Path("/tmp/oops"))

    def test_reject_query_in_download_path(self):
        with self.assertRaises(BridgeError):
            self.client.download({**self.package, "download_path": "/cache/pkg?redirect=evil"}, Path("/tmp/oops"))

    def test_download_requires_verified_catalog_metadata(self):
        client = BridgeClient(FakeTransport(), "x", trust_policy=TRUST)
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(BridgeError):
                client.download(PACKAGE, Path(td) / "demo.pkg")

    def test_catalog_rejects_duplicate_ids_even_when_signed(self):
        class DuplicateTransport:
            def __call__(self, *args):
                return {"status": 200, "body": signed_catalog([PACKAGE, PACKAGE])}
        with self.assertRaises(BridgeError):
            BridgeClient(DuplicateTransport(), "x", trust_policy=TRUST).catalog()


if __name__ == "__main__":
    unittest.main()

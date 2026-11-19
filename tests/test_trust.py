import base64
import json
import unittest

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from nexus_txn.trust import CatalogTrustError, CatalogTrustPolicy


PRIVATE = Ed25519PrivateKey.from_private_bytes(bytes(range(32)))
PUBLIC = PRIVATE.public_key().public_bytes(
    encoding=serialization.Encoding.Raw,
    format=serialization.PublicFormat.Raw,
)
NOW = 1_800_000_000


def b64url(raw):
    return base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")


def make_envelope(payload=None, *, key_id="release-2026", algorithm="Ed25519"):
    if payload is None:
        payload = {"issued_at": NOW - 10, "expires_at": NOW + 3600, "packages": []}
    payload_bytes = json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    signature = PRIVATE.sign(payload_bytes)
    envelope = {
        "schema": "nexus.signed-catalog.v1",
        "key_id": key_id,
        "algorithm": algorithm,
        "payload": b64url(payload_bytes),
        "signature": b64url(signature),
    }
    return json.dumps(envelope, separators=(",", ":"))


class CatalogTrustTests(unittest.TestCase):
    def setUp(self):
        self.policy = CatalogTrustPolicy({"release-2026": PUBLIC}, clock=lambda: NOW)

    def test_valid_signature_verifies_and_returns_trust_metadata(self):
        result = self.policy.verify_envelope(make_envelope())
        self.assertEqual(result["catalog"]["packages"], [])
        self.assertTrue(result["trust"]["signature_verified"])
        self.assertEqual(result["trust"]["key_id"], "release-2026")

    def test_tampered_payload_is_rejected(self):
        envelope = json.loads(make_envelope())
        payload = json.loads(base64.urlsafe_b64decode(envelope["payload"] + "=="))
        payload["packages"] = [{"id": "tampered"}]
        envelope["payload"] = b64url(json.dumps(payload, separators=(",", ":"), sort_keys=True).encode())
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(json.dumps(envelope))

    def test_unknown_signing_key_is_rejected(self):
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(make_envelope(key_id="unknown"))

    def test_expired_catalog_is_rejected(self):
        payload = {"issued_at": NOW - 7200, "expires_at": NOW - 1, "packages": []}
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(make_envelope(payload))

    def test_future_issued_catalog_is_rejected(self):
        payload = {"issued_at": NOW + 1000, "expires_at": NOW + 2000, "packages": []}
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(make_envelope(payload))

    def test_excessive_validity_is_rejected(self):
        payload = {"issued_at": NOW - 1, "expires_at": NOW + 8 * 86400, "packages": []}
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(make_envelope(payload))

    def test_duplicate_json_fields_are_rejected(self):
        raw = '{"schema":"nexus.signed-catalog.v1","schema":"nexus.signed-catalog.v1"}'
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(raw)

    def test_wrong_algorithm_is_rejected(self):
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope(make_envelope(algorithm="none"))

    def test_unsigned_object_is_rejected(self):
        with self.assertRaises(CatalogTrustError):
            self.policy.verify_envelope({"packages": []})

    def test_bad_public_key_length_is_rejected(self):
        with self.assertRaises(CatalogTrustError):
            CatalogTrustPolicy({"bad": b"short"}, clock=lambda: NOW)

    def test_empty_keyring_is_rejected(self):
        with self.assertRaises(CatalogTrustError):
            CatalogTrustPolicy({}, clock=lambda: NOW)

    def test_payload_size_limit_is_enforced(self):
        policy = CatalogTrustPolicy({"release-2026": PUBLIC}, clock=lambda: NOW, max_payload_bytes=16)
        with self.assertRaises(CatalogTrustError):
            policy.verify_envelope(make_envelope())


if __name__ == "__main__":
    unittest.main()

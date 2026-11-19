"""Fail-closed Ed25519 trust policy for signed NEXUS catalogs.

The signature covers the exact raw payload bytes, not a re-serialized JSON
object. Public keys are pinned by the caller; this module has no trust-on-first-
use, network key discovery or automatic key rotation.
"""
from __future__ import annotations

import base64
import binascii
import json
import re
import time
from typing import Any, Callable


class CatalogTrustError(ValueError):
    pass


_KEY_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")
_B64URL = re.compile(r"^[A-Za-z0-9_-]+$")


def _reject_constant(value: str):
    raise CatalogTrustError(f"non-standard JSON constant rejected: {value}")


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise CatalogTrustError(f"duplicate JSON field rejected: {key}")
        result[key] = value
    return result


def _decode_json(raw: bytes, label: str) -> Any:
    try:
        return json.loads(
            raw.decode("utf-8"),
            object_pairs_hook=_unique_object,
            parse_constant=_reject_constant,
        )
    except CatalogTrustError:
        raise
    except (UnicodeDecodeError, ValueError, TypeError) as exc:
        raise CatalogTrustError(f"{label} is not strict UTF-8 JSON") from exc


def _decode_b64url(value: Any, label: str) -> bytes:
    if not isinstance(value, str) or not _B64URL.fullmatch(value):
        raise CatalogTrustError(f"{label} must be unpadded base64url")
    try:
        decoded = base64.b64decode(value + "=" * ((4 - len(value) % 4) % 4), altchars=b"-_", validate=True)
    except (ValueError, binascii.Error) as exc:
        raise CatalogTrustError(f"{label} is malformed base64url") from exc
    canonical = base64.urlsafe_b64encode(decoded).decode("ascii").rstrip("=")
    if canonical != value:
        raise CatalogTrustError(f"{label} is not canonical base64url")
    return decoded


class CatalogTrustPolicy:
    """Verify version-1 Ed25519 catalog envelopes against pinned public keys."""

    def __init__(
        self,
        trusted_keys: dict[str, bytes],
        *,
        clock: Callable[[], int | float] = time.time,
        max_payload_bytes: int = 4 * 1024 * 1024,
        max_validity_seconds: int = 7 * 24 * 60 * 60,
        allowed_clock_skew_seconds: int = 300,
    ):
        if not isinstance(trusted_keys, dict) or not trusted_keys:
            raise CatalogTrustError("at least one pinned Ed25519 public key is required")
        normalized = {}
        for key_id, public_key in trusted_keys.items():
            if not isinstance(key_id, str) or not _KEY_ID.fullmatch(key_id):
                raise CatalogTrustError("invalid trusted key ID")
            if not isinstance(public_key, bytes) or len(public_key) != 32:
                raise CatalogTrustError(f"Ed25519 public key {key_id!r} must contain exactly 32 raw bytes")
            normalized[key_id] = bytes(public_key)
        for name, value in (
            ("max_payload_bytes", max_payload_bytes),
            ("max_validity_seconds", max_validity_seconds),
            ("allowed_clock_skew_seconds", allowed_clock_skew_seconds),
        ):
            if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                raise CatalogTrustError(f"{name} must be a non-negative integer")
        if max_payload_bytes < 1 or max_validity_seconds < 1:
            raise CatalogTrustError("payload and validity limits must be positive")
        if not callable(clock):
            raise CatalogTrustError("clock must be callable")
        self._trusted_keys = normalized
        self._clock = clock
        self.max_payload_bytes = max_payload_bytes
        self.max_validity_seconds = max_validity_seconds
        self.allowed_clock_skew_seconds = allowed_clock_skew_seconds

    def verify_envelope(self, envelope: bytes | str) -> dict[str, Any]:
        """Return verified catalog payload and trust metadata; reject unsigned data."""
        if isinstance(envelope, str):
            raw_envelope = envelope.encode("utf-8")
        elif isinstance(envelope, bytes):
            raw_envelope = envelope
        else:
            raise CatalogTrustError("signed catalog envelope must be raw JSON bytes or text")
        if len(raw_envelope) > self.max_payload_bytes + 16 * 1024:
            raise CatalogTrustError("signed catalog envelope exceeds configured size limit")

        outer = _decode_json(raw_envelope, "signed catalog envelope")
        required = {"schema", "key_id", "algorithm", "payload", "signature"}
        if not isinstance(outer, dict) or set(outer) != required:
            raise CatalogTrustError("signed catalog envelope has an invalid schema")
        if outer["schema"] != "nexus.signed-catalog.v1":
            raise CatalogTrustError("unsupported signed catalog schema")
        key_id = outer["key_id"]
        if not isinstance(key_id, str) or not _KEY_ID.fullmatch(key_id):
            raise CatalogTrustError("invalid catalog signing key ID")
        if outer["algorithm"] != "Ed25519":
            raise CatalogTrustError("only Ed25519 catalog signatures are accepted")
        public_key = self._trusted_keys.get(key_id)
        if public_key is None:
            raise CatalogTrustError("catalog signing key is not pinned")

        payload_bytes = _decode_b64url(outer["payload"], "catalog payload")
        signature = _decode_b64url(outer["signature"], "catalog signature")
        if len(payload_bytes) > self.max_payload_bytes:
            raise CatalogTrustError("catalog payload exceeds configured size limit")
        if len(signature) != 64:
            raise CatalogTrustError("Ed25519 signature must contain exactly 64 bytes")

        try:
            from cryptography.exceptions import InvalidSignature
            from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
        except ImportError as exc:
            raise CatalogTrustError("host cryptography dependency is unavailable") from exc
        try:
            Ed25519PublicKey.from_public_bytes(public_key).verify(signature, payload_bytes)
        except (InvalidSignature, ValueError) as exc:
            raise CatalogTrustError("catalog signature verification failed") from exc

        catalog = _decode_json(payload_bytes, "signed catalog payload")
        if not isinstance(catalog, dict) or not isinstance(catalog.get("packages"), list):
            raise CatalogTrustError("signed catalog payload must contain a packages array")
        issued = catalog.get("issued_at")
        expires = catalog.get("expires_at")
        if any(not isinstance(v, int) or isinstance(v, bool) for v in (issued, expires)):
            raise CatalogTrustError("catalog issued_at and expires_at must be integer Unix timestamps")
        if expires <= issued or expires - issued > self.max_validity_seconds:
            raise CatalogTrustError("catalog validity interval is invalid or too long")
        now = self._clock()
        if not isinstance(now, (int, float)) or isinstance(now, bool):
            raise CatalogTrustError("configured clock returned an invalid timestamp")
        if issued > now + self.allowed_clock_skew_seconds:
            raise CatalogTrustError("catalog issue time is in the future")
        if expires <= now:
            raise CatalogTrustError("catalog signature has expired")

        return {
            "catalog": catalog,
            "trust": {
                "key_id": key_id,
                "algorithm": "Ed25519",
                "issued_at": issued,
                "expires_at": expires,
                "signature_verified": True,
            },
        }

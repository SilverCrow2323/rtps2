# NEXUS — verified status snapshot

## M6.7 source
- Signed catalog envelope v1 uses Ed25519. The signature covers the exact payload bytes; JSON is not re-serialized before verification.
- Verification requires a locally pinned 32-byte public key selected by key_id. Unknown keys, unsigned catalog responses, modified payloads, malformed signatures and unsupported algorithms are rejected.
- Signed catalog payloads require issued_at and expires_at. Expired, future-issued, overlong-validity, oversized and duplicate-field JSON is rejected.
- BridgeClient.catalog() now requires an explicit trust policy. Package downloads must match metadata from the most recently verified signed catalog; hand-crafted or modified records are rejected.
- Host verifier dependency is declared in requirements-host.txt and installed in CI.

## M6.6 source
- Read-only ZIP inspector: path traversal, absolute paths, backslashes, control/reserved characters, protected boot/system targets, case collisions, symlinks, special entries and encrypted entries are rejected.
- Archive limits: 64 MiB compressed archive, 4,096 entries, 128 MiB per member, 512 MiB total expanded data and 200:1 compression ratio by default; all are configurable.
- Every ZIP file is streamed and SHA-256 hashed without extraction. Actual expanded sizes are checked against ZIP metadata.
- Recipe matching verifies declared source paths and SHA-256 values. Unmapped archive entries are reported and are never implicitly installed.
- PSU and SAS are recognized as distinct formats but remain explicitly unsupported; they are not passed to the ZIP reader.
- Store flow requires VERIFY → INSPECT → READY_TO_INSTALL. Invalid, mismatched or unsupported archives end in FAILED.

## M6.5 source
- Bridge client core: injected transport, bearer authentication, catalog validation, explicit archive size vs installed size, ranged downloads, resumable PC .part cache and final size/SHA-256 verification.
- Bridge-to-Store adapter requires a separately declared installed size; compressed download size is never treated as installed storage requirement.
- Bridge route compatibility with original M5 remains unverified; route paths and transport are injectable.

## Verification boundary
- M6.7 code passed GitHub Actions on commit 80017d5351da6d46badab3d041761a5cfdc2f1b8 ([run #44484611611](https://github.com/SilverCrow2323/rtps2/actions/runs/44484611611)).
- Both **Unit tests** and **Compile check** passed after installing the host cryptography dependency. The suite contains **100 tests**.
- Host-side Python only. No PS2SDK build or console test is claimed.

## Not implemented
- Actual M5 Companion/Bridge route compatibility (original M5 source is still unavailable).
- SPDW Factory production signing-key generation, storage, publishing, rotation and revocation procedure.
- PSU/SAS parsers or extractors.
- Native PS2 Store UI, PS2 network transport, native installer or device writes.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

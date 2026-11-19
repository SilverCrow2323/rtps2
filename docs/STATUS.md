# NEXUS — verified status snapshot

## M6.8 source
- Recipe-scoped ZIP staging in an explicit host cache directory.
- The archive is re-inspected and matched against the validated recipe at staging time; every mapped member is streamed and SHA-256 hashed again while copied.
- Only recipe-mapped members are staged. Unmapped extras are omitted. No extractall(), archive permission restoration or special-file creation.
- A staging manifest maps validated recipe destinations to staged source files and is consumed by the existing host transaction engine in an isolated test sandbox.
- Store flow now requires VERIFY → INSPECT → STAGE → READY_TO_INSTALL. A file mutation between inspection and staging causes failure and blocks readiness.
- Safe cleanup is limited to a direct NEXUS staging child under the caller-supplied staging root.

## M6.7 source
- Signed catalog envelope v1 uses Ed25519. Signature covers exact payload bytes; the verifier does not re-serialize JSON.
- Verification requires a locally pinned raw 32-byte public key selected by key_id. Unknown keys, unsigned JSON, modified payloads, malformed signatures, duplicate JSON fields and unsupported algorithms are rejected.
- Signed payloads require issued_at and expires_at. Expired, future-issued, overlong-validity and oversized payloads are rejected.
- BridgeClient.catalog() requires an explicit trust policy. Package downloads must match metadata from the most recently verified signed catalog.
- Host cryptography dependency is declared in requirements-host.txt and installed in CI.

## M6.6 source
- Read-only ZIP inspector rejects traversal, absolute paths, backslashes, control/reserved characters, protected system paths, case collisions, symlinks, special entries and encrypted entries.
- Configurable limits cover compressed archive size, entry count, member size, total expanded size and compression ratio.
- Every ZIP file is streamed and SHA-256 hashed without extraction. Recipe matching checks source paths and expected hashes.
- PSU and SAS are recognized as distinct formats but remain explicitly unsupported; they are never treated as ZIP.

## M6.5 source
- Injected Bridge client, bearer authentication, validated catalog metadata, explicit download vs installed size, resumable ranged PC downloads and final size/SHA-256 checks.
- Original M5 Bridge route compatibility is still unverified; route paths and transport remain injectable.

## Verification boundary
- Latest M6.8 source snapshot passed GitHub Actions [run #44484998507](https://github.com/SilverCrow2323/rtps2/actions/runs/44484998507): host unit tests and Python compile check both succeeded.
- The current test tree contains **112 test cases**.
- Host-side Python only. No PS2SDK build, on-console test or device write is claimed.

## Not implemented
- Actual M5 Companion/Bridge route compatibility (original M5 source is still unavailable).
- SPDW Factory production signing-key generation, storage, publishing, rotation and revocation procedure.
- PSU/SAS parsers or extractors.
- Native PS2 Store UI, PS2 network transport, native installer or device writes.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

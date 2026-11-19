# NEXUS — verified status snapshot

## M6.6 source
- Read-only ZIP inspector: path traversal, absolute paths, backslashes, control/reserved characters, protected boot/system targets, case collisions, symlinks, special entries and encrypted entries are rejected.
- Archive limits: 64 MiB compressed archive, 4,096 entries, 128 MiB per member, 512 MiB total expanded data and 200:1 compression ratio by default; all are configurable.
- Every ZIP file is streamed and SHA-256 hashed without extraction. Actual expanded sizes are checked against ZIP metadata.
- Recipe matching verifies declared source paths and SHA-256 values. Unmapped archive entries are reported and are never implicitly installed.
- PSU and SAS are recognized as distinct formats but remain explicitly unsupported; they are not passed to the ZIP reader.
- Store flow now requires VERIFY → INSPECT → READY_TO_INSTALL. Invalid, mismatched or unsupported archives end in FAILED.
- Inspection and recipe matching perform no extraction, device writes or installer execution.

## M6.5 source
- Bridge client core: injected transport, bearer authentication, catalog validation, explicit archive size vs installed size, ranged downloads, resumable PC .part cache and final size/SHA-256 verification.
- Bridge-to-Store adapter: requires a separately declared installed size; it never mistakes compressed download size for installed storage requirement.
- Bridge route compatibility with original M5 remains unverified; route paths and transport are injectable.

## Verification boundary
- GitHub Actions host test workflow previously passed **63/63 tests and compile check** on the M6.5 code snapshot (run [#44463407162](https://github.com/SilverCrow2323/rtps2/actions/runs/44463407162)).
- M6.6 adds new test coverage; its workflow result is pending at the time of this snapshot.
- Host-side Python only. No PS2SDK build or console test is claimed.

## Not implemented
- Actual M5 Companion/Bridge route compatibility (original M5 source is still unavailable).
- TLS implementation and signed catalog / publisher authentication.
- PSU/SAS parsers or extractors.
- Native PS2 Store UI, PS2 network transport, native installer or device writes.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

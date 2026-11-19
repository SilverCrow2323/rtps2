# NEXUS — verified status snapshot

## M6.9 native shell
- Native EE shell source is committed under src/: entry point, libpad DualShock input, portable navigation state machine, dashboard and distinct Games/PS1/Homebrew/Store/Files/System views.
- Controller backend uses SIO2MAN/PADMAN, a 256-byte / 64-byte-aligned DMA work area, PAD_STATE_STABLE reads and edge-triggered button events.
- Input polling uses the EE delaythread.h API for a 10 ms pause.
- Native device slots remain NOT PROBED until real probes exist.
- The actual PS2DEV EE cross-build **passed** and uploaded NEXUS.ELF: [run #44486831714](https://github.com/SilverCrow2323/rtps2/actions/runs/44486831714).
- Artifact: NEXUS-ELF, 396,215-byte ZIP, SHA-256 2e84f1afd9c8e85d8ab392b108301a111767502bd27b472d41c6e8d58bd69add.
- Extracted binary: 1,234,820 bytes, ELF32 little-endian MIPS with R5900/MIPS III flags; SHA-256 06c47a71407582732efb4449230337f6ba4414ee4d12042375af4dc083c7c51f.
- Host test workflow also passed on the following test-fixture commit: 112 Python tests, Python compile, native C navigation test and syntax-only native module compile ([run #44486842812](https://github.com/SilverCrow2323/rtps2/actions/runs/44486842812)).
- This confirms compilation and ELF format, not a physical-console boot test.

## M6.8 source
- Recipe-scoped ZIP staging into an explicit host cache; repeats inspection and hash matching, copies only mapped members and emits a transaction manifest.
- Store flow requires VERIFY → INSPECT → STAGE → READY_TO_INSTALL. Archive modification between inspection and staging blocks readiness.
- End-to-end staging manifest test commits only into a disposable transaction sandbox.

## M6.7 source
- Pinned Ed25519 signed-catalog trust; exact payload bytes are signed and verified. Unsigned catalogs, unknown keys, altered content, invalid times and duplicate JSON fields are rejected.
- Bridge downloads must use metadata from the latest verified catalog record.

## M6.6 source
- Read-only ZIP inspection, safe path/type/size checks and SHA-256 content verification.
- PSU/SAS recognized but unsupported; never treated as ZIP.

## M6.5 source
- Injected host Bridge client, bearer auth, validated catalog, distinct download vs installed size, resumable ranged downloads and final hash/size checks.
- Original M5 Bridge route compatibility remains unverified.

## Verification boundary
- Host suite: 112 tests and both C host checks pass.
- Native EE cross-compile and artifact upload pass.
- No physical PS2 console test is claimed.

## Not implemented
- GS graphical renderer, fonts, textures, artwork, animation or final design system.
- Live disc/MC/HDD/MMCE/USB probing, mounted-volume providers or filesystem access.
- OPL/Neutrino launch adapters.
- Native PS2 network transport, TLS, Ed25519 catalog validation or package installer.
- PSU/SAS parsers.
- Production SPDW Factory signing-key lifecycle.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

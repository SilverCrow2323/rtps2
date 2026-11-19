# NEXUS — verified status snapshot

## M6.9 source
- New native EE shell bootstrap under src/: entry point, DualShock input backend, portable navigation state machine, first dashboard and distinct section screens.
- Controller backend uses PS2SDK libpad, SIO2MAN/PADMAN module loading and a 256-byte, 64-byte-aligned pad work area. It reads only after PAD_STATE_STABLE.
- Bootstrap controls: D-pad selects, Cross opens, L1/R1 switches sections, Circle backs, Start returns home.
- Native device status slots remain NOT PROBED until actual device providers are implemented.
- src/Makefile produces NEXUS.ELF with the PS2SDK sample build rules; requires the official PS2DEV toolchain.
- CI adds a host C compiler test for the portable navigation state machine. This is not a PS2SDK cross-build.

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
- Latest native C navigation CI run is pending at the time of this document update.
- Previous M6.8 source snapshot passed 112 host tests and Python compile check ([run #44484998507](https://github.com/SilverCrow2323/rtps2/actions/runs/44484998507)).
- PS2SDK cross-build, NEXUS.ELF artifact and physical console test are not yet claimed.

## Not implemented
- GS graphical renderer, fonts, textures, artwork, animation or final design system.
- Live disc/MC/HDD/MMCE/USB probing, mounted-volume providers or filesystem access.
- OPL/Neutrino launch adapters.
- Native PS2 network transport, TLS, Ed25519 catalog validation or package installer.
- PSU/SAS parsers.
- Production SPDW Factory signing-key lifecycle.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

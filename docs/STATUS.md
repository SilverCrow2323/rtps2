# NEXUS — verified status snapshot

## M6.13 embedded boot-time atlas
- Six source PNGs are versioned in assets/icons/ and packed by CI to NXA1 before the native build.
- The generated pack is linked into NEXUS.ELF through a 16-byte-aligned .rodata blob; nexus_renderer_init() calls the GS uploader after framebuffer and GIF DMA setup.
- EE-side structure and SHA-256 checks run before VRAM allocation. If upload fails, procedural 5×5 glyphs remain active and renderer startup continues.
- Host CI [#195](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819542) and native EE build [#42](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819621) passed.
- Extracted ELF: 1,724,400 bytes; SHA-256: 51ad1a8c2799aee05452369c60dbf6244b916c4cb2a60919ca62f08673022cc81.
- Actual texture display, PAL/NTSC appearance and startup timing still need a physical PS2 test.
- Details: [M6.13 embedded icon atlas](M6.13-embedded-icon-atlas.md).

## M6.12 native GS atlas upload path
- Added `nexus_atlas.c`: strict NXA1 header/size checks, aligned PSMCT32 texture upload through GIF DMA, texture flush, nearest sampling and six-cell atlas draw helper.
- Dashboard card renderer selects atlas icons when a caller has loaded an atlas; procedural 5×5 glyphs remain the fallback.
- The shell now embeds its six-icon NXA1 pack in the ELF and invokes the uploader during renderer initialization; EE validation checks SHA-256 before allocating VRAM. Real-console visual validation remains pending.
- One atlas VRAM slot only; no cover-art LRU or safe multi-slot allocator yet.
- PS2DEV cross-build passed with the SHA-256 object linked: [native build #33](https://github.com/SilverCrow2323/rtps2/actions/runs/44577115974). Host CI [run #178](https://github.com/SilverCrow2323/rtps2/actions/runs/44577288782) passed all 120 Python tests, C navigation, NXA1 checksum regressions, SHA-256 standard/block-boundary vectors and native C syntax.
- Console visual validation remains pending. M6.13 now embeds and invokes the atlas uploader during boot; see [M6.13](M6.13-embedded-icon-atlas.md).
- Details: [M6.12 native GS atlas](M6.12-native-gs-atlas.md).

## M6.11 host-side asset pipeline
- Added a deterministic PNG decoder/atlas packer with no third-party Python dependency.
- Six 64×64 section icons map to a 256×128 PSMCT32 RGBA atlas; two 64×64 atlas cells remain reserved.
- NXA1 uses a 64-byte little-endian header, 131,072-byte pixel payload and SHA-256 verification. PNG alpha is scaled to the GS 0..128 range.
- Eight focused asset tests passed in the full GitHub Actions host suite [#129](https://github.com/SilverCrow2323/rtps2/actions/runs/44574962515); the workflow passed all 120 Python tests, Python compile, C navigation and native C syntax checks.
- Native GS upload, VRAM allocation/LRU and visual console validation are not implemented in this milestone.
- Details: [M6.11 asset pipeline](M6.11-asset-pipeline.md).

## M6.10 native GS renderer
- Native renderer uses PS2SDK libgraph, libdraw, libfont, libpacket and libdma.
- Double 32-bit framebuffer in GS VRAM, 640 px wide with region-specific height: 512 PAL / 448 NTSC.
- Reusable bounded 60,000-qword GIF render packet; DMA submission and VSync-based page flip.
- KROM ROM font loaded via FontX; debug text remains as fallback if GS or font initialization fails.
- NEXUS dark-navy / cyan / blue-grey / amber palette.
- Home screen: six status chips (Disc, MC0, MC1, HDD, MMCE, USB) and 3×2 module cards. Separate module panels for Games, PS1, Homebrew, Store, Files and System.
- Device values remain NOT PROBED; no live probing or storage writes are claimed.

### Verified GS artifact
- PS2DEV EE cross-build and artifact upload passed: [workflow #44488236616](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236616).
- Source commit: 3c47ae7bbb15b1ab6ff4c58e0c795f41d1bdb901.
- Artifact ID: 14511920550; artifact ZIP: 506,996 bytes.
- ZIP SHA-256: 68cc91c23636e1dad543c0d157ae4070629bab41e3bb0156bd9c378a38c5d0dc.
- Extracted NEXUS.ELF: 1,564,624 bytes, ELF32 little-endian MIPS, N32 ABI, statically linked, R5900/MIPS III flags.
- ELF SHA-256: c313bc7ca7e00cb1c49d30d784d78ab59921132f31f7b55223c59207d49ebf27.
- Host tests and C checks passed: [workflow #44488236424](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236424): 112 Python tests, Python compile, C navigation test and syntax-only compile of all native C modules including GS renderer.

The build confirms compilation/linking and ELF target format, not physical-console startup or visual output. TV/monitor and video-mode checks remain to be done.

## M6.9 native shell
- Native EE shell source: entry point, libpad DualShock input, portable navigation, dashboard and distinct module pages.
- Controller backend loads SIO2MAN/PADMAN, uses 256-byte / 64-byte-aligned DMA work area, PAD_STATE_STABLE reads and edge-triggered input.
- Native device slots remain NOT PROBED until real probes exist.

## M6.8 host staging
- Recipe-scoped ZIP staging into explicit host cache; reinspection and hash matching; only mapped members copied; manifest passed to transaction reference engine inside isolated sandbox.
- Store flow requires VERIFY → INSPECT → STAGE → READY_TO_INSTALL; archive changes between inspect and stage block readiness.

## M6.7 signed catalog
- Ed25519 signature verification with locally pinned public keys; exact payload bytes, validity window, strict JSON and Bridge binding to trusted records.

## M6.6 archive inspection
- Read-only ZIP inspection with safe path/type/size/hash checks. PSU/SAS recognized but unsupported.

## M6.5 host Bridge
- Injected transport, bearer token, range/resume downloads, explicit download vs installed sizes and SHA verification. Actual M5 route compatibility remains unverified.

## Not implemented
- Cover-art cache / texture streaming, multi-slot VRAM management and final animated GS design system.
- Live Disc/MC/HDD/MMCE/USB detection, mounted volume providers and filesystem writes.
- Game catalog scanner and documented OPL/Neutrino handoff.
- Native PS2 Store networking, PS2-side TLS/Ed25519 and installer.
- PSU/SAS parsers.
- Production SPDW Factory signing-key lifecycle.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

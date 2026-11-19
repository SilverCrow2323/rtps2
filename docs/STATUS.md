# NEXUS — verified status snapshot

## M6.20 Files root selector and confirmed read-only scan
- Files now offers a fixed root allowlist: `mass:/` (USB), `mmce0:/` (MMCE slot 0), `mmce1:/` (MMCE slot 1). Up/Down cycles the roots; no free-form path input exists.
- When FileXio is ready, Triangle opens a scan warning, Cross confirms and Circle cancels. Start also cancels and returns Home. If FileXio is inactive, Files directs the user to System instead.
- A confirmed scan invokes only the existing FileXio Dopen/Dread/Dclose read-only adapter, then the M6.17 scanner checks direct `CD/`, `DVD/`, `POPS/` folders for PS2 ISO/ZSO and PS1 VCD. Results are summarized in Files and inserted into the in-memory catalog.
- No USB mass or MMCEMAN driver is started, no volume is mounted/unmounted, no IOP reset occurs, and no filesystem write/format/delete/rename is available through this flow. A root may return unavailable if its driver/filesystem is not already registered.
- Host CI [#344](https://github.com/SilverCrow2323/rtps2/actions/runs/44587509463) passed the full suite including root allowlist, selection, cancel/confirm and scan dispatch tests. Native PS2DEV build [#88](https://github.com/SilverCrow2323/rtps2/actions/runs/44587509595) passed.
- Artifact ZIP: 635,331 bytes; SHA-256 `05665fe9d333d2c88418ec8b4c14df8be74465e3f3773df16b80af5c3d83d1dc`. Extracted ELF: 1,981,436 bytes; SHA-256 `960f22530ebfae80691bb1ffd611cc0ea834444f4b9df0255a1596365d3db3b5` (ELF32 little-endian MIPS N32/R5900/MIPS III).
- The build and host mocks are verified; no PCSX2 boot, live USB/MMCE scan or physical-console test is claimed. Details: [M6.20 Files root scan](M6.20-files-root-scan.md).
## M6.19 user-confirmed FileXio activation
- The System page now exposes the M6.18 adapter through a two-step controller action: Triangle opens the warning, Cross confirms, Circle cancels, and Start cancels/returns Home. Triangle is inert outside System.
- The activation call occurs only after confirmation. The warning states that missing IOP RPC modules may be loaded and SBV hooks may be applied; there is no IOP reset, mount, scan or filesystem write. Once FileXio is ready, activation cannot be triggered again from Triangle.
- The GS renderer and debug fallback show pending, ready and error states. A successful activation only initializes FileXio RPC; it does not start USB/MMCE/APA drivers or scan a device.
- Host CI [#332](https://github.com/SilverCrow2323/rtps2/actions/runs/44586520529) passed the full suite including confirm/cancel/failure/retry and outside-System inertness tests. Native PS2DEV build [#84](https://github.com/SilverCrow2323/rtps2/actions/runs/44586493569) passed.
- Artifact ZIP: 629,839 bytes; SHA-256 `37fc69a9385528621b17d181c795bb8a308b396e29fbd24b55b289bd2be93ba8`. Extracted ELF: 1,968,516 bytes; SHA-256 `436ba1b11ac241685d5e497ccf2e09492dd0b0edcc59310718d5e468671b1ca2` (ELF32 little-endian MIPS N32/R5900/MIPS III).
- PCSX2 visual execution and physical PS2 validation remain pending. Instructions: [MX Linux XFCE](PCSX2-MX-LINUX.md).
- Details: [M6.19 System FileXio confirmation](M6.19-system-filexio-confirm.md).
## M6.18 explicit PS2SDK fileXio adapter
- Added an opt-in EE adapter that embeds PS2SDK `iomanX.irx` and `fileXio.irx`, checks whether the two modules already exist, loads only missing modules and calls `fileXioInit()`.
- It reuses the SIF RPC session initialized by NEXUS bootstrap. Activation is **not called automatically**; it never resets the IOP, starts USB/MMCE/APA block drivers, mounts/unmounts volumes or writes files.
- The scanner adapter exposes only `fileXioDopen`, `fileXioDread` and `fileXioDclose`. It translates IOP dirents, preserves 64-bit sizes, rejects symlinks/special entries and feeds the M6.17 read-only scanner.
- Host CI [#307](https://github.com/SilverCrow2323/rtps2/actions/runs/44585526848) passed the mocked module-activation/idempotence, read-only directory scan and complete host suite. Native PS2DEV build [#70](https://github.com/SilverCrow2323/rtps2/actions/runs/44585518347) passed.
- Artifact ZIP: 625,601 bytes; SHA-256 `e89eddfbb34c236e318a4f9c66e36ceb0a3acdf45f29c4d79411709cf8604d76`. Extracted ELF: 1,956,332 bytes; SHA-256 `6e55ea9628f680255ff796f8ab037df4eb4b8cb4c099329475424ef8ec1749d1` (ELF32 little-endian MIPS N32/R5900/MIPS III).
- This is adapter integration only: it is not activated from the UI or at boot, and no volume drivers are loaded. No live storage scan has run. APA/PFS HDD partitions still need a separate provider.
- Details: [M6.18 fileXio adapter](M6.18-filexio-adapter.md).
## M6.17 read-only game catalog scanner core
- Added a bounded directory scanner over injected read-only open/read/close callbacks. It inspects only explicitly configured root children `CD/`, `DVD/` and `POPS/`; accepts `.iso`/`.zso` for PS2 and `.vcd` for PS1.
- Validates source roots and fixed-width basenames, rejects traversal/separators/control characters, stages each read before catalog insertion, handles missing optional directories, and preserves 64-bit file sizes. No recursion or filesystem write API exists in the scanner contract.
- Native regression uses an in-memory fake filesystem: extension filtering, PS1/PS2 entries, >4 GiB size, malformed names, optional missing directories, staged read failure and path traversal are covered.
- Host CI [#289](https://github.com/SilverCrow2323/rtps2/actions/runs/44583873744) passed the full suite and scanner regression. Native EE build [#63](https://github.com/SilverCrow2323/rtps2/actions/runs/44583873664) passed and published `NEXUS-ELF`.
- Artifact ZIP: 573,905 bytes; SHA-256 `a853fc06580d151ca53b73543196275b8736c38314fb9e717b7ed74bc4d77f27`. Extracted ELF: 1,774,060 bytes; SHA-256 `83ad74bdcb35714a52cbeb04db64971cf970793a9ea00b5f91e70752043b281f` (ELF32 little-endian MIPS N32, R5900/MIPS III).
- The scanner core is connected to the FileXio adapter and M6.19 adds a user-confirmed System action; activation is never automatic. No USB mass/MMCEMAN block driver is started and no real drive has been scanned. APA HDD games need a separate partition provider.
- Details: [M6.17 read-only scanner](M6.17-read-only-scanner.md).
## M6.16 native game-library model
- Added a fixed-capacity 96-entry in-memory catalog with a 64-bit key scoped by source, HDD/MMCE/USB source tags, PS1/PS2 platform tags, AUTO/OPL/Neutrino/POPS backend hints, title/title ID/path/size and favorites.
- Upsert validates enums and fixed-width string termination, replaces `(source, key)` entries while retaining favorites, and maintains deterministic title/ID/source/key ordering.
- View state supports source, platform and favorites-only filters, Grid/Carousel/Coverflow/List modes, visible-entry lookup and circular selection. Games screen now reports whether the native catalog is empty or populated.
- Host CI [#271](https://github.com/SilverCrow2323/rtps2/actions/runs/44582680857) passed the full suite including catalog behavior, cover-cache LRU, NXA1/SHA-256 and PCSX2 launcher tests. Native PS2DEV build [#56](https://github.com/SilverCrow2323/rtps2/actions/runs/44582409082) passed and linked the model into NEXUS.ELF.
- Artifact ZIP: 565,321 bytes; SHA-256 `62c18dfda47556754415376a5f32d71a1488192b0a684bb27ea29e32cfdee4ed`. Extracted ELF: 1,754,284 bytes; SHA-256 `792f8abbd1d773598f0fbb429cc77c5276a82bcd6eee48772246a5aa8d235e53` (ELF32 little-endian MIPS N32, R5900/MIPS III).
- M6.20 can scan already-accessible USB/MMCE folder roots after two explicit confirmations, but does not start mass/MMCEMAN drivers or mount devices. APA/PFS internal-HDD partition discovery, persistent favorites, image decoding and OPL/Neutrino/POPS launch integration remain pending.
- Details: [M6.16 library model](M6.16-library-model.md).
## M6.15 fixed GS cover-art cache
- Added a four-slot LRU texture cache: 128×128 RGBA32 per slot, 256 KiB total. One contiguous 512×128 PSMCT32 allocation avoids arbitrary frees against PS2SDK's FIFO bump allocator.
- Renderer reserves the cache after the two framebuffers and embedded icon atlas; allocation is optional and non-fatal. System diagnostics report whether the four artwork slots were reserved.
- Store API accepts 16-byte-aligned 128×128 RGBA pixels and publishes a slot only after successful GIF DMA. LRU replacement reuses fixed addresses; it never allocates/frees per cover.
- Host CI [#251](https://github.com/SilverCrow2323/rtps2/actions/runs/44581918935) passed LRU policy, PCSX2 launcher and native C syntax checks. PS2DEV EE build [#52](https://github.com/SilverCrow2323/rtps2/actions/runs/44581909557) passed; both the embedded atlas and cover pixel REF sources now receive explicit SyncDCache before GIF DMA.
- Artifact ZIP: 558,304 bytes; SHA-256 e2f5d9b6e17017941b95e020c4a437735283ddb0e0cefc58ba1d7c08be1e7417. Extracted ELF: 1,737,732 bytes; SHA-256 93006b873561f6007f52c3b572281a3b8efcd57d0ecf477677adf3e67beb8deb.
- No game scanner, cover-image decoder or catalog-to-cache integration yet; PCSX2 and physical-console visual checks remain pending.
- Details: [M6.15 cover cache](M6.15-cover-cache.md).

## M6.14 native renderer diagnostics
- The System module reports PAL 640×512 / NTSC 640×448, embedded NXA1 atlas readiness and controller status. Storage devices remain explicitly NOT PROBED. The PCSX2 launcher and mocked tests cover command dispatch, not emulation.
- PS2DEV EE build [#43](https://github.com/SilverCrow2323/rtps2/actions/runs/44580353828) passed and published `NEXUS-ELF` (553,090-byte artifact ZIP; SHA-256 `06bf3e31d1c593e422a9603c5b72cc3a297bd2e9596400cd5ab2a4928c666b81`). Extracted ELF: 1,724,512 bytes; SHA-256 `56375605f8ef1116f3bf44b322085d94a043c076506dcbe192c30db5f7d74b29`. Host CI [#208](https://github.com/SilverCrow2323/rtps2/actions/runs/44580353818) passed.
- PCSX2 setup and ELF launch instructions: [MX Linux XFCE](PCSX2-MX-LINUX.md), now pointing to the M6.20 ELF. Actual PCSX2 boot and physical-console visual validation remain pending.

## M6.13 embedded boot-time atlas
- Six source PNGs are versioned in `assets/icons/` and deterministically packed by CI into NXA1 before the native build.
- The generated pack is embedded in ELF read-only data through a 16-byte-aligned assembly blob. `nexus_renderer_init()` calls the GS uploader after framebuffer and GIF DMA setup.
- EE-side structure and SHA-256 checks run before VRAM allocation. If upload fails, procedural 5×5 glyphs remain active and renderer startup continues.
- Host CI [#195](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819542) and native EE build [#42](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819621) passed.
- Extracted ELF: 1,724,400 bytes; SHA-256: `51ad1a8c2799aee05452369b8b73fe1141ffcf187737227086c66882b4908d1b`. Artifact ZIP SHA-256: `1c7a2ac9be049987f8a379c60dbf6244b916c4cb2a60919ca62f08673022cc81`.
- Actual texture display, alpha appearance, timing and PAL/NTSC output still need a physical PS2 test.
- Details: [M6.13 embedded icon atlas](M6.13-embedded-icon-atlas.md) and [M6.14 System diagnostics](M6.14-system-diagnostics.md).

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
- PCSX2 runtime boot/visual verification and physical PAL/NTSC console validation.
- Native fileXio/IOMANX and MMCEMAN adapter lifecycle; actual read-only HDD/MMCE/USB root configuration and scans; APA/PFS HDD partition provider; persistent favorites; cover-image decoding/population for the four-slot cache; final animated GS design system.
- Live Disc/MC/HDD/MMCE/USB detection, mounted volume providers and filesystem writes.
- Game catalog scanner and documented OPL/Neutrino handoff.
- Native PS2 Store networking, PS2-side TLS/Ed25519 and installer.
- PSU/SAS parsers.
- Production SPDW Factory signing-key lifecycle.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

# NEXUS — roadmap & milestone ledger

## Product direction
NEXUS is a native PS2 shell with a polished PSBBN-inspired dashboard and clear separation between Games, PS1, Homebrew, Store, Files and System. The home screen should feel like a navigable constellation / tree of life rather than a flat settings grid. Device state (disc, memory cards, MMCE, HDD, HDDOSD, PSBBN) should be visible without turning the dashboard into a diagnostic wall.

Games should support storage-source tabs, DualShock L1/R1 switching, grid/carousel/coverflow/list presentation, artwork and metadata, favorites and backend selection. Launching must use documented, version-aware OPL/Neutrino handoff paths; never invent a private command line or claim direct per-title launch where the backend does not expose it.

The Store should provide an online catalog and a dedicated full-screen terminal/TUI workflow: select package → resolve dependencies → check capacity → confirm → download → verify → inspect archive → stage files → install. Every installable package needs a validated recipe describing archive format, destination, dependencies and post-install action.

## Milestone ledger
| Milestone | Scope | Source status |
|---|---|---|
| M1 | Native shell scaffold, dashboard/pages, device indicators, DualShock navigation | Historical description only; original source snapshot pending recovery |
| M2 | Device/provider registry, library catalog, package dry-run planner, path protection | Historical description only; original source snapshot pending recovery |
| M3 | Game scanner/grouping, artwork metadata, OPL/Neutrino adapters, PC Store Bridge baseline | Historical description only; original source snapshot pending recovery |
| M4 | Mounted-filesystem probing, runtime view, NXA1 artwork pipeline, LoadExecPS2 helper | Historical description only; original source snapshot pending recovery |
| M5 | Companion/Bridge API v0.4, authenticated package index, resumable PC downloads, package inspector | Prior archive described as built, but original source tree is not present; recover before claiming merge |
| M6.2 | Host transaction reference, dependency resolver, network provider contract | Source committed |
| M6.3 | Store profiles, install/update/keep planning, space preview, dependency-safe removal | Source committed |
| M6.4 | Strict non-executing install recipe validation | Source committed |
| M6.5 | Injected Bridge client, range/resume download cache, size/SHA verification | Source committed; original M5 route compatibility awaits recovery |
| M6.6 | Read-only ZIP inspection, recipe SHA matching, PSU/SAS fail-closed handling | Source committed |
| M6.7 | Pinned Ed25519 signed-catalog trust and Bridge download binding | Source committed |
| M6.8 | Recipe-scoped host staging, repeated hash checks and transaction-manifest handoff | Source committed |
| M6.9 | Native EE shell, libpad DualShock input, dashboard/pages | PS2DEV build passed |
| M6.10 | PS2SDK GS renderer: double buffering, KROM font, dashboard cards and module panels | PS2DEV build and host checks passed; console visual validation pending |
| M6.11 | Original section icon set, deterministic PNG-to-NXA1 PSMCT32 atlas packer, manifest and SHA-256 verifier | Source and eight unit tests committed |
| M6.12 | Native EE NXA1 structural + SHA-256 validation, GIF DMA upload, GS texture binding and six-cell icon sampling | PS2DEV build and host regressions passed |
| M6.13 | Versioned original PNGs, deterministic CI packing and 16-byte-aligned NXA1 embedding in ELF; boot-time GS upload | Host CI #195 and native PS2DEV build #42 passed |
| M6.14 | System diagnostics for video mode, atlas upload state and controller status; PCSX2 launcher and MX Linux smoke-test guide | Host CI #208 and native PS2DEV build #43 passed; emulator/console smoke test pending |
| M6.15 | Fixed four-slot 128×128 RGBA32 GS artwork cache, contiguous VRAM reservation, LRU replacement and explicit D-cache sync for REF uploads | Host CI #251 and native PS2DEV build #52 passed; image source integration and runtime visual test pending |
| M6.16 | Bounded native game catalog, source/platform/favorites filters, sorted index, view modes and cyclic selection | Host CI #271 and native PS2DEV build #56 passed |
| M6.17 | Bounded read-only directory scanner core for OPL CD/DVD ISO/ZSO and POPS VCD roots; 64-bit file sizes and staged reads | Host CI #289 and native PS2DEV build #63 passed |
| M6.18 | Explicit PS2SDK IOMANX/FileXio IRX integration and read-only Dopen/Dread/Dclose adapter, no IOP reset or mount | Host CI #307 and native PS2DEV build #70 passed; UI activation, storage drivers and live scan pending |

## M6.10 verification
- Native EE workflow [#44488236616](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236616), successful build and uploaded ELF artifact.
- Host test workflow [#44488236424](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236424), passed 112 Python tests, Python compile, C navigation regression and native C syntax checks.
- ELF inspection: ELF32 little-endian MIPS, N32 ABI, static, R5900/MIPS III flags.
- No physical-console boot or visual test yet.

## M6.11 verification
- Host packer and eight focused tests added in `tools/nexus_asset_pack.py` and `tests/test_asset_pack.py`; GitHub Actions run [#129](https://github.com/SilverCrow2323/rtps2/actions/runs/44574962515) passed all 120 Python tests plus Python compile and native C checks.
- NXA1 format: 64-byte little-endian header, 256×128 PSMCT32 RGBA atlas, 131,072-byte pixel payload and SHA-256 validation.
- Local generated pack SHA-256: `246d883d5e7086769ce1adc3630c1fea8aeb3e250cf80be270144475fee2eed9`; pixel payload SHA-256: `c3daf0ae6b208701c8374cd2adf5e98c21d341ab1ee6c57ab90334824543b526`.
- No native ELF rebuild or physical-console test is claimed for M6.11.

## M6.18 verification
- NEXUS embeds PS2SDK IOMANX and FileXio IRXs in the EE ELF using the upstream `bin2c` pattern. `nexus_filexio_activate()` is explicit, reuses the bootstrap SIF RPC session, loads only missing modules and initializes the FileXio RPC client.
- Read-only adapter uses only Dopen/Dread/Dclose; no IOP reset, driver loading, mount, unmount or write calls. Host mock test covers module loading, idempotence, directory read and catalog insertion.
- Host CI [#307](https://github.com/SilverCrow2323/rtps2/actions/runs/44585526848) and native EE build [#70](https://github.com/SilverCrow2323/rtps2/actions/runs/44585518347) passed.
- Artifact ZIP: 625,601 bytes; SHA-256 `e89eddfbb34c236e318a4f9c66e36ceb0a3acdf45f29c4d79411709cf8604d76`. Extracted ELF: 1,956,332 bytes; SHA-256 `6e55ea9628f680255ff796f8ab037df4eb4b8cb4c099329475424ef8ec1749d1`.
- The adapter is not activated from the UI/startup and requires an already registered filesystem/device. No live USB/MMCE/HDD scan or APA/PFS partition discovery is claimed.
- Documentation: [M6.18 fileXio adapter](M6.18-filexio-adapter.md).

## M6.17 verification
- Added a callback-driven, read-only scanner core. It confirms the configured root is openable and scans only its direct `CD/`, `DVD/`, and `POPS/` children; ISO/ZSO and VCD filters are explicit.
- Host test uses a fake filesystem to verify extension/platform mapping, >4 GiB file size preservation, unsafe path/name rejection, optional missing folders, read-error staging and scanner idempotence. Host CI [#289](https://github.com/SilverCrow2323/rtps2/actions/runs/44583873744) passed.
- Native EE build [#63](https://github.com/SilverCrow2323/rtps2/actions/runs/44583873664) passed and uploaded a 573,905-byte artifact ZIP (SHA-256 `a853fc06580d151ca53b73543196275b8736c38314fb9e717b7ed74bc4d77f27`). Extracted ELF: 1,774,060 bytes; SHA-256 `83ad74bdcb35714a52cbeb04db64971cf970793a9ea00b5f91e70752043b281f`.
- The FileXio directory adapter is now integrated in M6.18 but is not activated from the UI/startup; USB mass/MMCEMAN drivers and real source selection remain pending. No live device scan or APA HDD partition scan has been performed.

## M6.16 verification
- Native catalog model holds up to 96 entries with 64-bit `(source, key)` identity, deterministic title ordering, PS1/PS2 and HDD/MMCE/USB tags, backend hints, favorites and fixed-width path/title metadata.
- View state provides source/platform/favorites filters, Grid/Carousel/Coverflow/List mode selection and wraparound movement. Games page exposes catalog-empty vs populated state; no games are fabricated.
- Host CI [#271](https://github.com/SilverCrow2323/rtps2/actions/runs/44582680857) passed the full suite. Native EE build [#56](https://github.com/SilverCrow2323/rtps2/actions/runs/44582409082) passed and uploaded a 565,321-byte artifact ZIP (SHA-256 `62c18dfda47556754415376a5f32d71a1488192b0a684bb27ea29e32cfdee4ed`). Extracted ELF: 1,754,284 bytes; SHA-256 `792f8abbd1d773598f0fbb429cc77c5276a82bcd6eee48772246a5aa8d235e53`.
- The model is not a device scanner. No HDD/MMCE/USB provider, image decoder, persistent favorites or game launcher is connected yet.

## M6.15 verification
- Added four fixed 128×128 RGBA32 slots (256 KiB total) allocated as one contiguous 512×128 PSMCT32 block, respecting PS2SDK's FIFO bump allocator behavior.
- The renderer reserves the cache after the framebuffers and embedded NXA1 atlas; failure is non-fatal. System status exposes cache-reservation state.
- Host CI [#251](https://github.com/SilverCrow2323/rtps2/actions/runs/44581918935) passed the LRU policy test, launcher tests, SHA/atlas/navigation regressions and native C syntax checks.
- Native PS2DEV build [#52](https://github.com/SilverCrow2323/rtps2/actions/runs/44581909557) passed and uploaded a 558,322-byte NEXUS-ELF artifact ZIP (SHA-256 8308f8d60fcadf3a1f634421bbde7e5b5eb346ac43224a4596969daf2a3aaa80). Extracted ELF: 1,738,060 bytes; SHA-256 43f6038be6372d91034ec33760182c079716dbb816bc7061d7f49477f9e6d992.
- NXA1 and cover-art REF payloads receive explicit SyncDCache write-back before GIF DMA; the DMA chain flush alone only covers the chain/tag buffer.
- The cache is prepared but no game scanner/decoder populates it yet. No PCSX2 or physical-console visual validation is claimed.

## M6.14 verification
- The System page reports PAL/NTSC framebuffer geometry, NXA1 atlas-ready/fallback state and controller port-0 status. Storage remains explicitly NOT PROBED.
- PCSX2 launcher helper and MX Linux setup/download guide are included: [PCSX2-MX-LINUX.md](PCSX2-MX-LINUX.md) and [M6.14 System diagnostics](M6.14-system-diagnostics.md).
- Host CI [#208](https://github.com/SilverCrow2323/rtps2/actions/runs/44580353818) and native EE build [#43](https://github.com/SilverCrow2323/rtps2/actions/runs/44580353828) passed. Build #43 produced a 1,724,512-byte ELF; SHA-256 56375605f8ef1116f3bf44b322085d94a043c076506dcbe192c30db5f7d74b29.
- Artifact ZIP SHA-256: 06bf3e31d1c593e422a9603c5b72cc3a297bd2e9596400cd5ab2a4928c666b81.
- Launcher tests for Flatpak/native fallback, paths with spaces and missing files have been added to the host workflow; first run pending.
- CI has not performed a PCSX2 boot or physical-console visual validation.

## M6.13 verification
- The build packs all six original PNGs into `assets/build/NEXUS-ICONS.NXA`, embeds the pack through `src/nexus_asset_blob.S`, and uploads it from `nexus_renderer_init()`.
- The embedded NXA1 is verified on EE (header, payload geometry, reserved bytes and SHA-256) before texture VRAM allocation. Atlas failure remains non-fatal and leaves procedural fallback glyphs available.
- Host CI [#195](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819542) passed; native EE build [#42](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819621) passed and published a 553,030-byte `NEXUS-ELF` artifact. ZIP SHA-256: `1c7a2ac9be049987f8a379c60dbf6244b916c4cb2a60919ca62f08673022cc81`.
- No PCSX2 or physical-console visual validation is claimed yet.

## M6.12 verification
- EE SHA-256 implementation is allocation-free and verifies the 32-byte digest stored at NXA1 header offset 20 against the complete 131,072-byte pixel payload before VRAM allocation.
- Native tests cover empty and `abc` vectors, SHA padding lengths 55/56/63/64/65, the full NXA payload size, and a one-million-byte message; NXA tests reject corrupt payload and digest bytes.
- PS2DEV EE build [#33](https://github.com/SilverCrow2323/rtps2/actions/runs/44577115974) passed and uploaded artifact `NEXUS-ELF` (519,661 bytes).
- Host CI [#178](https://github.com/SilverCrow2323/rtps2/actions/runs/44577288782) passed 120 Python tests plus native SHA/atlas/navigation tests and syntax checks.
- No boot-time pack loader or physical-console visual validation is claimed.

## M6.13 verification
- The six PNG sources are versioned in assets/icons/; CI regenerates assets/build/NEXUS-ICONS.NXA before the EE cross-build.
- src/nexus_asset_blob.S embeds the pack in .rodata at 16-byte alignment. nexus_renderer_init() uploads it after GS/DMA initialization; the existing procedural glyph path remains the fallback if upload fails.
- Host CI [#195](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819542) passed the full 120-test Python suite, native SHA-256 vectors, NXA1 integrity regression, C navigation and C syntax checks.
- Native EE build [#42](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819621) passed NXA generation, PS2DEV compile/link and ELF artifact upload.
- Extracted NEXUS.ELF: 1,724,400 bytes; SHA-256 `51ad1a8c2799aee05452369b8b73fe1141ffcf187737227086c66882b4908d1b`.
- No real-console boot or visual output is claimed.

## M6.10 design boundary
The current GS renderer is a first functional graphical layer: navy and cyan palette, 3×2 home cards, device-status strip and separate section screens. It is not yet the final animated interface. The original icons are now embedded in the native ELF and handed to the GS uploader during renderer initialization. Cover-art cache, multi-slot streaming and animated page backgrounds are still pending.

## Next priorities
1. Run the M6.18 ELF in PCSX2 and on real PAL/NTSC hardware; record atlas, cache, adapter-inactive state, controller mapping, page flips and frame pacing.
2. Add a visible, user-confirmed System/Files action to activate the existing FileXio adapter; never do so automatically at boot.
3. Add already-loaded/explicit provider adapters for USB mass and MMCEMAN, with no implicit mounts; expose only existing accessible roots.
4. Add an APA/PFS-specific read-only provider for traditional internal HDD partitions; the file-based CD/DVD scanner must not pretend to enumerate APA partitions.
5. Connect scanner roots to library source tabs and add atomic stale-entry reconciliation.
6. Add a bounded cover-image decoder/resizer that feeds verified assets into the four cache slots; then connect Grid/Carousel/Coverflow/List UI to real entries.
4. Add documented, version-aware OPL/Neutrino/POPS launch handoff without inventing private commands.
5. Add native PS2 Store networking only after transport and crypto architecture are proven.
6. Recover M1–M5 original source snapshots, especially M5 Bridge API contract.
7. Implement and test genuine PSU/SAS readers.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.


## M6.16 next steps
1. Connect game-library metadata to cover paths, add bounded image decoding and stable artwork keys, and route updates through the four-slot cache.
2. Run the NEXUS ELF in PCSX2 and on PAL/NTSC hardware; record boot, display mode, controller mapping, atlas/cover appearance, frame pacing and input latency before increasing VRAM use.

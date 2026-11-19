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
| M6.18 | Explicit PS2SDK IOMANX/FileXio IRX integration and read-only Dopen/Dread/Dclose adapter, no IOP reset or mount | Host CI #307 and native PS2DEV build #70 passed |
| M6.19 | Two-step System UI action for explicit FileXio activation: Triangle prompt, Cross confirm, Circle cancel; no automatic scan/mount | Host CI #332 and native PS2DEV build #84 passed |
| M6.20 | Files root allowlist for mass:/, mmce0:/ and mmce1:/ plus two-step read-only scan dispatch and result panel | Host CI #344 and native PS2DEV build #88 passed |
| M6.21 | Games catalog navigation, four view modes, favorites, source/platform filters and metadata-only detail | Host CI #367 and native PS2DEV build #101 passed |
| M6.22 | Home navigation rail, contextual hero, status metrics, consistent module panels and cover-style Games placeholders | Host CI #367 and native PS2DEV build #101 passed; PCSX2 visual feedback and real artwork pending |

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

## M6.22 verification
- Home was redesigned into a persistent six-module navigation rail, selected-module hero and three status metrics. PAL/NTSC geometry is calculated from the active region.
- Module pages share an overview panel, three status cards and a context-aware controller footer. Games uses distinct cover-style cards and title metadata across Grid/Carousel/Coverflow/List; labels are width-aware.
- The artwork region currently uses section icons plus ARTWORK NOT CACHED placeholders. The VRAM cache is reserved but real cover image ingestion/decoding has not been implemented.
- Host CI [#367](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741640) passed. Native PS2DEV build [#101](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741740) passed and uploaded a 661,824-byte ELF artifact ZIP (SHA-256 398ff56052ed70be3471bc87e4b32b3fb96fcf530182fc7e9529a253769ad525). Extracted ELF: 2,047,340 bytes; SHA-256 a84f7acf833bc53536f3f081cd00020094fc10fdc6f37f7037c67a6a29653922.
- The user has confirmed the previous NEXUS build runs in PCSX2. The M6.22 redesign still needs a PCSX2 screenshot review; no physical-PS2 test is claimed.
- Details: [M6.22 visual redesign](M6.22-visual-redesign.md).

## M6.21 verification
- Games uses the M6.16 catalog and exposes Grid, Carousel, Coverflow and List; Up/Down selection, Triangle view cycling, Square favorites, L2 source filter, R2 platform filter and Cross metadata details.
- Launch is intentionally not wired; detail screens do not falsely claim an OPL/Neutrino/POPS handoff.
- Host CI [#367](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741640) and native PS2DEV build [#101](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741740) pass with the M6.22 renderer.

## M6.20 verification
- Files offers three fixed roots (`mass:/`, `mmce0:/`, `mmce1:/`) with Up/Down selection. Triangle opens the scan warning; Cross confirms, Circle cancels and Start returns Home.
- The scan calls the existing read-only FileXio adapter only after confirmation. It reads direct CD/DVD/POPS folders into the M6.16 catalog and reports the result/counts.
- No mass/MMCEMAN driver is loaded, no mount/IOP reset or write operation is run. A root succeeds only if already registered and accessible.
- Host CI [#344](https://github.com/SilverCrow2323/rtps2/actions/runs/44587509463) passed the full host suite; native PS2DEV build [#88](https://github.com/SilverCrow2323/rtps2/actions/runs/44587509595) passed and uploaded a 635,331-byte artifact ZIP (SHA-256 `05665fe9d333d2c88418ec8b4c14df8be74465e3f3773df16b80af5c3d83d1dc`). Extracted ELF: 1,981,436 bytes; SHA-256 `960f22530ebfae80691bb1ffd611cc0ea834444f4b9df0255a1596365d3db3b5`.
- No PCSX2 boot or real USB/MMCE/console scan has been performed. Details: [M6.20 Files root scan](M6.20-files-root-scan.md).

## M6.19 verification
- System page now exposes an explicit activation prompt: Triangle opens warning, Cross confirms, Circle cancels and Start cancels/returns Home. Triangle is inert outside System. Renderer and debug fallback both display action states.
- Confirmation warns that missing IOP modules may be loaded and SBV hooks may be applied. FileXio activation is not automatic and does not reset IOP, mount, scan or write a filesystem.
- Host CI [#332](https://github.com/SilverCrow2323/rtps2/actions/runs/44586520529) passed the complete suite including confirm/cancel/failure/retry and outside-System tests. Native EE build [#84](https://github.com/SilverCrow2323/rtps2/actions/runs/44586493569) passed.
- Artifact ZIP: 629,839 bytes; SHA-256 `37fc69a9385528621b17d181c795bb8a308b396e29fbd24b55b289bd2be93ba8`. Extracted ELF: 1,968,516 bytes; SHA-256 `436ba1b11ac241685d5e497ccf2e09492dd0b0edcc59310718d5e468671b1ca2`.
- No PCSX2 boot or physical-console test has been performed.
- Details: [M6.19 System FileXio confirmation](M6.19-system-filexio-confirm.md).

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
- The FileXio directory adapter was integrated in M6.18; M6.19 added user-confirmed System activation and M6.20 added an explicit Files scan action. USB mass/MMCEMAN drivers and actual volume availability remain pending. No real device scan or APA HDD partition scan has been performed.

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

## M6.22 visual boundary
The old 3×2 dashboard/device strip has been replaced by the navigation rail + module hero + status metrics. Module pages share a more structured two-column layout, and Games uses cover-style placeholders across all four views. This is a major first redesign, not the final animated skin: the real artwork decoder/cache feed, subtle transitions, screenshot review and physical PAL/NTSC calibration remain open.

## Next priorities
1. Review M6.22 in PCSX2 using screenshots of Home, Games Grid/Carousel/Coverflow/List, Files and System; fix any observed clipping, scale or spacing issues.
2. Implement a bounded image decoder/resizer and source-to-title artwork mapping that populates the four-slot GS cache; replace ARTWORK NOT CACHED placeholders with actual covers.
3. Add a read-only availability/status probe for mass:/, mmce0:/ and mmce1:/, while keeping device drivers and mounts explicit.
4. Implement explicitly enabled USB mass and MMCEMAN adapters only after checking their installed IRX/version contracts; do not auto-load or mount them at boot.
5. Add a separate read-only APA/PFS provider for traditional internal HDD partitions.
6. Connect documented, version-aware OPL/Neutrino/POPS launch handoff; do not invent private commands.
7. Add subtle animation only after PCSX2/console frame pacing is measured.
8. Add native PS2 Store networking after transport/crypto architecture is proven; recover original M1–M5 source snapshots and genuine PSU/SAS readers.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.


## M6.16 next steps
1. Connect game-library metadata to cover paths, add bounded image decoding and stable artwork keys, and route updates through the four-slot cache.
2. Run the NEXUS ELF in PCSX2 and on PAL/NTSC hardware; record boot, display mode, controller mapping, atlas/cover appearance, frame pacing and input latency before increasing VRAM use.

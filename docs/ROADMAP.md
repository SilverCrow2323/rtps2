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
| M6.13 | Versioned original PNGs, deterministic CI packing and 16-byte-aligned NXA1 embedding in ELF; boot-time GS upload | Host CI #195 and native PS2DEV build #42 passed; physical-console visual test pending |

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
- Extracted NEXUS.ELF: 1,724,400 bytes; SHA-256 `51ad1a8c2799aee05452369c60dbf6244b916c4cb2a60919ca62f08673022cc81`.
- No real-console boot or visual output is claimed.

## M6.10 design boundary
The current GS renderer is a first functional graphical layer: navy and cyan palette, 3×2 home cards, device-status strip and separate section screens. It is not yet the final animated interface. The original icons are now embedded in the native ELF and handed to the GS uploader during renderer initialization. Cover-art cache, multi-slot streaming and animated page backgrounds are still pending.

## Next priorities
1. Implement M6.14: measure the embedded atlas on real PAL/NTSC consoles, then add a strictly bounded cover-art texture cache without disturbing the framebuffers.
2. Build the final dashboard backgrounds and distinct animated section scenes only after M6.14 measures frame pacing and memory budgets.
3. Add read-only device and mounted-volume providers incrementally.
4. Add game discovery, metadata/artwork integration and documented OPL/Neutrino launch handoff.
5. Add native PS2 Store networking only after transport and crypto architecture are proven.
6. Recover M1–M5 original source snapshots, especially M5 Bridge API contract.
7. Implement and test genuine PSU/SAS readers.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.

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
| M6.11 | Original section icon set, deterministic PNG-to-NXA1 PSMCT32 atlas packer, manifest and SHA-256 verifier | Source and eight unit tests committed; native GS upload/cache pending |

## M6.10 verification
- Native EE workflow [#44488236616](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236616), successful build and uploaded ELF artifact.
- Host test workflow [#44488236424](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236424), passed 112 Python tests, Python compile, C navigation regression and native C syntax checks.
- ELF inspection: ELF32 little-endian MIPS, N32 ABI, static, R5900/MIPS III flags.
- No physical-console boot or visual test yet.

## M6.11 verification
- Host packer and eight focused tests added in `tools/nexus_asset_pack.py` and `tests/test_asset_pack.py`; local test run passed 8/8.
- NXA1 format: 64-byte little-endian header, 256×128 PSMCT32 RGBA atlas, 131,072-byte pixel payload and SHA-256 validation.
- Local generated pack SHA-256: `246d883d5e7086769ce1adc3630c1fea8aeb3e250cf80be270144475fee2eed9`; pixel payload SHA-256: `c3daf0ae6b208701c8374cd2adf5e98c21d341ab1ee6c57ab90334824543b526`.
- No native ELF rebuild or physical-console test is claimed for M6.11.

## M6.10 design boundary
The current GS renderer is a first functional graphical layer: navy and cyan palette, 3×2 home cards, device-status strip and separate section screens. It is not yet the final animated interface. The original icon artwork now has a host-side packing path, but no native GS texture upload, cover-art cache or animated page backgrounds are active yet.

## Next priorities
1. Implement M6.12: native GS texture upload, atlas sampling and a bounded VRAM cache; visually validate on a PS2 before claiming the artwork is displayed.
2. Build the final dashboard backgrounds and distinct animated section scenes only after the M6.12 frame pacing and memory budgets are measured.
3. Add read-only device and mounted-volume providers incrementally.
4. Add game discovery, metadata/artwork integration and documented OPL/Neutrino launch handoff.
5. Add native PS2 Store networking only after transport and crypto architecture are proven.
6. Recover M1–M5 original source snapshots, especially M5 Bridge API contract.
7. Implement and test genuine PSU/SAS readers.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.

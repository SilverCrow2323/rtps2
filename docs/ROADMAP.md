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

## M6.10 verification
- Native EE workflow [#44488236616](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236616), successful build and uploaded ELF artifact.
- Host test workflow [#44488236424](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236424), passed 112 Python tests, Python compile, C navigation regression and native C syntax checks.
- ELF inspection: ELF32 little-endian MIPS, N32 ABI, static, R5900/MIPS III flags.
- No physical-console boot or visual test yet.

## M6.10 design boundary
The current GS renderer is a first functional graphical layer: navy and cyan palette, 3×2 home cards, device-status strip and separate section screens. It is not yet the final animated interface and has no original icons, cover artwork, texture cache, or animated page backgrounds.

## Next priorities
1. Add reliable GS texture upload/cache and original NEXUS icon/brand assets.
2. Build the final dashboard backgrounds and distinct animated section scenes only after frame pacing and memory budgets are measured.
3. Add read-only device and mounted-volume providers incrementally.
4. Add game discovery, metadata/artwork integration and documented OPL/Neutrino launch handoff.
5. Add native PS2 Store networking only after transport and crypto architecture are proven.
6. Recover M1–M5 original source snapshots, especially M5 Bridge API contract.
7. Implement and test genuine PSU/SAS readers.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.

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
| M6.9 | Native EE shell, libpad DualShock input, dashboard/pages and PS2DEV cross-build | EE build passed; NEXUS.ELF artifact uploaded |

## M6.9 verification
- Native EE workflow: [run #44486831714](https://github.com/SilverCrow2323/rtps2/actions/runs/44486831714), successful PS2DEV cross-build and artifact upload.
- Host tests and C checks: [run #44486842812](https://github.com/SilverCrow2323/rtps2/actions/runs/44486842812), successful 112 Python tests, compile check, navigation regression and native C syntax check.
- The produced binary is an ELF32 little-endian MIPS executable with R5900/MIPS III flags.
- Physical console execution has not yet been tested.

## M6.9 safety boundary
M6.9 starts a new native source line under src/; it does not reconstruct or claim to merge missing M1–M5 snapshots. The initial renderer is debug text. Device slots remain NOT PROBED until live probes exist. No storage is mounted or written, and no boot manager is changed.

## Recovery rule
Do not recreate missing M1–M5 files from descriptions and label them original. Recover the real source snapshots or reimplement them as new, clearly versioned work with tests.

## Next priorities
1. Replace the debug text renderer with a GS rendering backend and establish resolution/layout/color/font primitives.
2. Implement read-only device and mounted-volume discovery incrementally.
3. Add library scanning and version-aware OPL/Neutrino launch handoffs.
4. Implement PS2-native Store networking and package verification only after selecting and testing transport and cryptographic dependencies.
5. Recover M5 Bridge route contract and M1–M5 source snapshots from original archives.
6. Implement genuine PSU/SAS readers with format-specific regression fixtures.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.

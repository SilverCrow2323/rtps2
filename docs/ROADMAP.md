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
| M6.3 | Store profiles, install/update/keep planning, space preview, dependency-safe removal, TUI renderer | Source committed |
| M6.4 | Strict non-executing install recipe validation | Source committed |
| M6.5 | Injected Bridge client, range/resume download cache, size/SHA verification, Store flow | Source committed; route compatibility awaits M5 recovery |
| M6.6 | Read-only ZIP inspection, recipe SHA matching, PSU/SAS fail-closed handling | Source committed |
| M6.7 | Pinned Ed25519 signed-catalog trust and Bridge download binding | Source committed |
| M6.8 | Recipe-scoped host staging, repeated hash checks and transaction-manifest handoff | Source committed |
| M6.9 | New native EE shell bootstrap, libpad DualShock input, page navigation and text screens | Source committed; host C navigation test added |

## M6.9 safety boundary
M6.9 starts a new native source line under src/; it does not reconstruct or claim to merge missing M1–M5 snapshots. It builds a basic text dashboard and six sections with DualShock controls. The device provider returns NOT PROBED until actual probes are implemented.

## Recovery rule
Do not recreate missing M1–M5 files from descriptions and label them original. Recover the real source snapshots or reimplement them as new, clearly versioned work with tests.

## Next priorities
1. Verify the native EE source with the PS2SDK cross-toolchain and produce NEXUS.ELF; record build output separately from host navigation tests.
2. Replace the debug text renderer with a GS rendering backend and establish the resolution/layout/color/font system.
3. Implement live device providers incrementally, starting with read-only device and mounted-volume discovery.
4. Add library scanning and version-aware OPL/Neutrino launch handoffs.
5. Implement PS2-native Store networking and package verification only after selecting and testing the transport and cryptographic dependencies.
6. Recover the M5 Bridge route contract and M1–M5 source snapshots from their original archives.
7. Implement genuine PSU/SAS readers with format-specific regression fixtures.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.

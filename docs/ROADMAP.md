# NEXUS — roadmap & milestone ledger

## Product direction
NEXUS is a native PS2 shell with a polished PSBBN-inspired dashboard and clear separation between Games, PS1, Homebrew, Store, Files and System. The home screen should feel like a navigable constellation / tree of life rather than a flat settings grid. Device state (disc, memory cards, MMCE, HDD, HDDOSD, PSBBN) should be visible without turning the dashboard into a diagnostic wall.

Games should support storage-source tabs, DualShock L1/R1 switching, grid/carousel/coverflow/list presentation, artwork and metadata, favorites and backend selection. Launching must use documented, version-aware OPL/Neutrino handoff paths; never invent a private command line or claim direct per-title launch where the backend does not expose it.

The Store should eventually provide an online catalog and a dedicated full-screen terminal/TUI workflow: select package → resolve dependencies → check capacity → confirm → download → verify → install. Every package must have a validated recipe describing archive format, destination, dependencies and post-install action.

## Milestone ledger
| Milestone | Scope recorded in project history | Source status |
|---|---|---|
| M1 | Native shell scaffold, dashboard/page renderer, Games/PS1/Store/Homebrew/Files/System pages, device indicators and DualShock navigation | Historical description only; original source snapshot not available |
| M2 | Device/provider registry, library catalog, package dry-run planner and path protection | Historical description only; original source snapshot not available |
| M3 | Game scanner/grouping, artwork metadata, OPL/Neutrino adapters and PC Store Bridge baseline | Historical description only; original source snapshot not available |
| M4 | Mounted-filesystem probing, runtime device view, NXA1 artwork conversion/loader, low-level LoadExecPS2 helper | Historical description only; original source snapshot not available |
| M5 | Companion/Bridge API v0.4, authenticated package index, range-resumable PC downloads, package inspector and expanded tests | Previous archive reported built; only checksum file remains in this workspace. Recover before merging |
| M6.2 | Transaction reference engine, dependency resolver, network provider contract | Source committed |
| M6.3 | Store catalog validation, curated profiles, install/update/keep preview, conservative space estimate, dependency-safe removal preview, TUI renderers | Source committed |
| M6.4 | Strict non-executing package recipe validation, approved target classes, archive metadata, path/hash/dependency/post-install validation | Source committed; 41 local host tests pass |

## Recovery rule
Do not recreate missing M1–M5 files from descriptions and label them original. Recover the real source snapshots or reimplement them as new, clearly versioned work with tests.

## Next priorities
1. Recover and merge M1–M5 source snapshots, especially the M5 Companion/Bridge package catalog and inspector.
2. Bind M6.4 recipes to the real Companion/Bridge catalog and define a signed catalog trust policy.
3. Add format-specific archive inspectors; PSU and SAS must not be treated as ZIP.
4. Implement a transport adapter against the real Companion API and test authentication, range/resume, hash verification and error recovery.
5. Build native PS2-side Store UI as a separate screen with nonblocking progress and explicit confirmation.
6. Add PS2SDK build/CI and test on actual FAT/Slim configurations only when a test build exists.

## Non-goals
- No automatic bootloader/OSDSYS/FMCB/FHDB/PSBBN replacement.
- No implicit mount, IOP reset, device formatting or filesystem repair.
- No assumption that Bluetooth HID compatibility means Wi-Fi support.
- No claim of hardware validation from host tests.

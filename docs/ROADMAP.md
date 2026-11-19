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
| M5 | Companion/Bridge API v0.4, authenticated package index, resumable PC downloads, package inspector | Prior archive described as built, but original source tree is not present in this workspace; recover before claiming merge |
| M6.2 | Host transaction reference, dependency resolver, network provider contract | Source committed |
| M6.3 | Store profiles, install/update/keep planning, space preview, dependency-safe removal, TUI renderer | Source committed |
| M6.4 | Strict non-executing install recipe validation, target/path/hash/dependency validation | Source committed |
| M6.5 | Injected Bridge client, range/resume download cache, size/SHA verification, Store flow | Source committed; route compatibility awaits M5 recovery |
| M6.6 | Read-only ZIP inspection, recipe SHA matching, PSU/SAS fail-closed handling | Source committed |
| M6.7 | Pinned Ed25519 signed-catalog trust, expiry/key policy, Bridge download binding to trusted records | Source committed |
| M6.8 | Recipe-scoped host staging, repeated hash checks, transaction-manifest handoff and Store staging gate | Source committed; 112 host tests and compile check pass ([CI run #44484998507](https://github.com/SilverCrow2323/rtps2/actions/runs/44484998507)) |

## M6.8 safety boundary
Only ZIP is staged. Each mapped entry is streamed into a new host cache directory, verified again and represented in a manifest for the transaction reference engine. The Store will not reach READY_TO_INSTALL if the archive changes between inspection and staging. The test commits only into a disposable sandbox. No real PS2 mount is a valid target for this host reference engine.

READY_TO_INSTALL means staged for a future installer adapter. It does not mean installed. PSU and SAS remain unsupported.

## Recovery rule
Do not recreate missing M1–M5 files from descriptions and label them original. Recover the real source snapshots or reimplement them as new, clearly versioned work with tests.

## Next priorities
1. Recover and merge M1–M5 source snapshots, especially the M5 Companion/Bridge route contract and original package-inspector implementation.
2. Define SPDW Factory release-key custody, signing, rotation and revocation; provision a production public key through a trusted release channel.
3. Implement and independently test genuine PSU and SAS readers, retaining strict target/path protections.
4. Implement a native PS2 network provider/Bridge client only after selecting a supported transport and validating the PS2SDK integration.
5. Build the native PS2 Store screen as a separate full-screen terminal/TUI view with nonblocking progress and explicit confirmation.
6. Add PS2SDK cross-build and integration tests when the native shell source is recovered.

## Non-goals
No automatic bootloader/OSDSYS/FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN replacement. No implicit mount, IOP reset, device formatting or filesystem repair. Bluetooth HID compatibility does not imply Wi-Fi support.

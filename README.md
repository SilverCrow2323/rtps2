# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. The code currently committed is the recoverable M6 transaction, dependency, Store-planning, recipe-validation and network-contract work. Earlier M1–M5 history is documented where original source snapshots are not currently available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | Dashboard, pages, DualShock navigation | M1 scaffold described in roadmap; source snapshot pending recovery |
| Devices & library | Mounted-volume providers, game catalog, metadata | M2–M4 work described; source snapshot pending recovery |
| Artwork | NXA1 texture pipeline and GS upload path | M4 work described; source snapshot pending recovery |
| Store & packages | Catalog profiles, dependency-aware planning and strict recipe validation | Host-side code present |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side reference implementation present |
| Network | Provider-neutral PS2 network contract | Header contract only; no driver or transport |

## Repository structure
- `nexus_txn/` — host-side reference logic for transactions, dependencies, Store planning/TUI, recipe validation, Bridge client and Store workflow state machine.
- `src/include/` — native-facing API contracts.
- `tools/` — host-side simulation CLI.
- `tests/` — automated host regression tests.
- `docs/` — architecture, milestone reports, safety rules and roadmap.

## Quick validation
```sh
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
```
The current local suite contains **41 passing host tests**. GitHub Actions is configured to run the same checks on pushes and pull requests.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The transaction engine is host-only and sandboxed. No PS2 storage is mounted or written by this code. Unknown free space is not treated as sufficient. Package metadata is not a substitute for a validated install recipe.

## Hardware facts supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim. This does **not** imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Current engineering boundary
No `NEXUS.ELF`, PS2SDK cross-build, PS2-side HTTP/TLS client, native archive installer, PSU/SAS parser or on-console package extraction is included in the currently recoverable source snapshot. Do not describe host reference modules as a functioning PS2 installer.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6 transaction/network](docs/M6-transaction-network.md), [M6.3 Store manager](docs/M6.3-store-manager.md), [M6.4 package recipes](docs/M6.4-package-recipes.md) and [M6.5 Bridge client](docs/M6.5-bridge-client.md).

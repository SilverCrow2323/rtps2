# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. The recoverable source now includes host-side transaction, dependency, Store planning, recipe validation, Bridge trust, archive inspection and ZIP staging work. Earlier M1–M5 history is documented where original source snapshots are not available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | Dashboard, pages, DualShock navigation | M1 scaffold described in roadmap; source snapshot pending recovery |
| Devices & library | Mounted-volume providers, game catalog, metadata | M2–M4 work described; source snapshot pending recovery |
| Artwork | NXA1 texture pipeline and GS upload path | M4 work described; source snapshot pending recovery |
| Store & packages | Signed catalog trust, dependency planning, recipes, ZIP inspection and host staging | Host-side code present |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side reference implementation present |
| Network | Provider-neutral PS2 network contract and host Bridge client | Contracts/client only; no PS2 driver or transport |

## Repository structure
- nexus_txn/ — host-side reference logic for transactions, dependencies, Store planning/TUI, recipe validation, Bridge client, catalog trust, archive inspectors, staging and Store workflow.
- src/include/ — native-facing API contracts.
- tools/ — host-side simulation CLI.
- tests/ — automated host regression tests.
- docs/ — architecture, milestone reports, safety rules and roadmap.

## Quick validation
~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
~~~

The M6.8 GitHub Actions run [#44484998507](https://github.com/SilverCrow2323/rtps2/actions/runs/44484998507) passed the **112-test** host suite and Python compile check.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The transaction engine is host-only and sandboxed. No PS2 storage is mounted or written by this code. Unknown free space is not treated as sufficient. ZIP inspection does not extract files; ZIP staging writes only to an explicit host cache. PSU/SAS remain unsupported. The synthetic test signing key is not a production key.

## Hardware facts supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim. This does **not** imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Current engineering boundary
No NEXUS.ELF, PS2SDK cross-build, PS2-side HTTP/TLS/Ed25519 client, native archive installer, PSU/SAS parser or on-console package extraction is included in the currently recoverable source snapshot. Do not describe host reference modules as a functioning PS2 installer.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6 transaction/network](docs/M6-transaction-network.md), [M6.3 Store manager](docs/M6.3-store-manager.md), [M6.4 package recipes](docs/M6.4-package-recipes.md), [M6.5 Bridge client](docs/M6.5-bridge-client.md), [M6.6 archive inspection](docs/M6.6-archive-inspection.md), [M6.7 signed catalog trust](docs/M6.7-signed-catalog.md) and [M6.8 host staging](docs/M6.8-host-staging.md).

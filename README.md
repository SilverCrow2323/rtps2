# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. Recoverable source includes host-side transactions, dependency planning, Store, recipes, Bridge trust, ZIP inspection/staging and a native EE shell with a PS2SDK GS renderer. Earlier M1–M5 history is documented where original source snapshots are not available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | EE entry point, DualShock input, navigation and GS dashboard | PS2DEV cross-build passed; NEXUS.ELF artifact published |
| Graphics | Redesigned GS dashboard, navigation rail, module hero, status metrics, Games card views, NXA1 atlas and four-slot cover cache | Host CI #367 and native PS2DEV build #101 passed; real cover ingestion and visual screenshot review pending |
| Devices & library | 96-entry catalog, Files root selector, read-only scan core and explicit FileXio activation | Host mocks and native cross-build passed; USB/MMCEMAN/APA drivers and live scan availability pending |
| Store & packages | Signed host catalog, dependency planning, recipes, ZIP inspection and host staging | Host-side code present; no PS2 installer |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side sandbox reference |
| Network | Provider-neutral PS2 contract and host Bridge client | Contracts/client only; no native PS2 transport |

## Native build and binary

The native NEXUS.ELF is compiled using the PS2DEV Docker image and the PS2SDK EE toolchain.

- Latest native EE build with the M6.22 visual redesign: [Actions run #101](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741740)
- Latest host verification including Games navigation/views/filters, Files scan dispatch, FileXio adapter mocks, catalog/scanner, cache policy and renderer syntax: [Actions run #367](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741640)
- MX Linux / PCSX2 smoke-test guide: [PCSX2-MX-LINUX.md](docs/PCSX2-MX-LINUX.md); launcher: `tools/run_nexus_pcsx2.sh`
- Renderer notes: [M6.10 GS renderer](docs/M6.10-gs-renderer.md), [M6.12 native GS atlas](docs/M6.12-native-gs-atlas.md), [M6.13 embedded icon atlas](docs/M6.13-embedded-icon-atlas.md), [M6.14 System diagnostics](docs/M6.14-system-diagnostics.md), [M6.15 cover-art cache](docs/M6.15-cover-cache.md), [M6.16 game-library model](docs/M6.16-library-model.md), [M6.17 read-only scanner](docs/M6.17-read-only-scanner.md), [M6.18 FileXio adapter](docs/M6.18-filexio-adapter.md), [M6.19 System activation](docs/M6.19-system-filexio-confirm.md), [M6.20 Files root scan](docs/M6.20-files-root-scan.md), [M6.22 visual redesign](docs/M6.22-visual-redesign.md), [M6.14 renderer diagnostics](docs/M6.14-renderer-diagnostics.md)

Extracted ELF: **2,047,340 bytes**, ELF32 little-endian MIPS N32, R5900/MIPS III flags. SHA-256: a84f7acf833bc53536f3f081cd00020094fc10fdc6f37f7037c67a6a29653922. The binary includes the NXA1 atlas, four-slot GS artwork cache, native catalog/scanner, FileXio adapter, Files scan flow and M6.22 dashboard/module/Games visual redesign. Artifact ZIP SHA-256: 398ff56052ed70be3471bc87e4b32b3fb96fcf530182fc7e9529a253769ad525.

Host CI [#367](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741640) and native PS2DEV build [#101](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741740) both passed. The preceding build's successful PCSX2 boot/run is user-confirmed; the M6.22 visual refresh still needs screenshot review. Physical PS2 rendering remains unverified.

## Host validation

~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
cc -std=c11 -Wall -Wextra -Werror -Isrc/include src/nexus_navigation.c tests/native/test_navigation.c -o /tmp/nexus_navigation_test
/tmp/nexus_navigation_test
~~~

M6.22 host CI [#367](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741640) passed; native EE build [#101](https://github.com/SilverCrow2323/rtps2/actions/runs/44598741740) generated the redesigned ELF. The Games cover regions remain honest placeholders until image ingestion is implemented.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The host transaction engine is sandboxed. No PS2 storage is mounted or written. Native device status remains NOT PROBED until real probes exist. ZIP staging writes only to an explicit host cache. PSU/SAS remain unsupported. The synthetic signing key is not a production key.

## Hardware observations supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim; this does not imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Known native limitations
M6.22 replaces the old 3×2 dashboard with a left navigation rail, contextual hero and lower system metrics. Internal modules use a consistent overview/state-panel layout; Games has cover-style placeholders in all four views. The cover artwork is not real box art yet. The four-slot VRAM cache is reserved but not fed by a decoder/source mapper. FileXio is user-activated through System (Triangle → warning → Cross); Files scans only already-accessible roots after a second confirmation. USB/MMCEMAN/APA block drivers and mounts are not started automatically. The user has confirmed the preceding build boots and runs in PCSX2; M6.22 still needs screenshot feedback, and physical-console rendering remains unverified. APA/PFS partition enumeration, OPL/Neutrino launch handoff, PS2-side HTTP/TLS/Ed25519 and native Store installer remain outstanding.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6.8 host staging](docs/M6.8-host-staging.md), [M6.7 signed catalog trust](docs/M6.7-signed-catalog.md), [M6.9 native shell](docs/M6.9-native-shell.md) and [M6.10 GS renderer](docs/M6.10-gs-renderer.md) and [M6.11 asset pipeline](docs/M6.11-asset-pipeline.md) and [M6.12 native GS atlas](docs/M6.12-native-gs-atlas.md) and [M6.13 embedded icon atlas](docs/M6.13-embedded-icon-atlas.md) and [M6.14 System diagnostics](docs/M6.14-system-diagnostics.md).

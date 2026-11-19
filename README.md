# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. Recoverable source includes host-side transactions, dependency planning, Store, recipes, Bridge trust, ZIP inspection/staging and a native EE shell with a PS2SDK GS renderer. Earlier M1–M5 history is documented where original source snapshots are not available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | EE entry point, DualShock input, navigation and GS dashboard | PS2DEV cross-build passed; NEXUS.ELF artifact published |
| Graphics | PS2SDK GS renderer, KROM font, embedded NXA1 atlas, EE SHA-256 validation, GIF DMA and four-slot cover-art texture cache | Native EE build #63 passed; game-art loading and PCSX2/console visual validation pending |
| Devices & library | 96-entry catalog, metadata, filters and read-only CD/DVD/POPS scan core | Host scanner regression and native cross-build passed; fileXio/MMCEMAN adapters and live device scan pending |
| Store & packages | Signed host catalog, dependency planning, recipes, ZIP inspection and host staging | Host-side code present; no PS2 installer |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side sandbox reference |
| Network | Provider-neutral PS2 contract and host Bridge client | Contracts/client only; no native PS2 transport |

## Native build and binary

The native NEXUS.ELF is compiled using the PS2DEV Docker image and the PS2SDK EE toolchain.

- Latest native EE build with embedded atlas, four-slot cache and read-only scanner core: [Actions run #63](https://github.com/SilverCrow2323/rtps2/actions/runs/44583873664)
- Latest host verification including cache-policy and PCSX2 launcher regressions: [Actions run #251](https://github.com/SilverCrow2323/rtps2/actions/runs/44581918935)
- MX Linux / PCSX2 smoke-test guide: [PCSX2-MX-LINUX.md](docs/PCSX2-MX-LINUX.md); launcher: `tools/run_nexus_pcsx2.sh`
- Renderer notes: [M6.10 GS renderer](docs/M6.10-gs-renderer.md), [M6.12 native GS atlas](docs/M6.12-native-gs-atlas.md), [M6.13 embedded icon atlas](docs/M6.13-embedded-icon-atlas.md), [M6.14 System diagnostics](docs/M6.14-system-diagnostics.md), [M6.15 cover-art cache](docs/M6.15-cover-cache.md), [M6.16 game-library model](docs/M6.16-library-model.md), [M6.17 read-only scanner](docs/M6.17-read-only-scanner.md), [M6.14 renderer diagnostics](docs/M6.14-renderer-diagnostics.md)

Extracted ELF: **1,774,060 bytes**, ELF32 little-endian MIPS, N32 ABI, statically linked, R5900/MIPS III flags. SHA-256: 83ad74bdcb35714a52cbeb04db64971cf970793a9ea00b5f91e70752043b281f. The binary includes the six-icon NXA1 atlas, M6.14 System diagnostics, M6.15 four-slot GS artwork cache, M6.16 native catalog and M6.17 read-only scan core.

Build success proves compilation/linking and target format, not that the binary has been tested on a physical PS2.

## Host validation

~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
cc -std=c11 -Wall -Wextra -Werror -Isrc/include src/nexus_navigation.c tests/native/test_navigation.c -o /tmp/nexus_navigation_test
/tmp/nexus_navigation_test
~~~

M6.16 host CI [#271](https://github.com/SilverCrow2323/rtps2/actions/runs/44582680857) passed, including catalog filters/favorites/views, cover-cache LRU, PCSX2 launcher, NXA1 integrity, native SHA-256, navigation and C syntax. PS2DEV EE build [#56](https://github.com/SilverCrow2323/rtps2/actions/runs/44582409082) linked the catalog model and generated ELF. Artifact ZIP SHA-256: 62c18dfda47556754415376a5f32d71a1488192b0a684bb27ea29e32cfdee4ed. The atlas and cover artwork DMA sources are explicitly synchronized before REF-tagged transfers.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The host transaction engine is sandboxed. No PS2 storage is mounted or written. Native device status remains NOT PROBED until real probes exist. ZIP staging writes only to an explicit host cache. PSU/SAS remain unsupported. The synthetic signing key is not a production key.

## Hardware observations supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim; this does not imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Known native limitations
The current graphical renderer is an initial GS shell, not the final animated frontend. The six built-in section icons are now embedded in NEXUS.ELF and loaded during renderer initialization, with procedural glyphs retained as a failure fallback. The four-slot VRAM cache is reserved and LRU-managed, and the 96-entry catalog model is ready; the fileXio/MMCEMAN adapters are not yet connected, so no real HDD/MMCE/USB scan or image decoder feeds them yet. Animated section scenes remain pending. PCSX2 command dispatch is tested separately; emulator visual output and real-console rendering remain unverified. Live device discovery awaits the fileXio/MMCEMAN adapters and source initialization; APA HDD partition enumeration, OPL/Neutrino launch handoff, PS2-side HTTP/TLS/Ed25519 and a native Store installer also remain outstanding.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6.8 host staging](docs/M6.8-host-staging.md), [M6.7 signed catalog trust](docs/M6.7-signed-catalog.md), [M6.9 native shell](docs/M6.9-native-shell.md) and [M6.10 GS renderer](docs/M6.10-gs-renderer.md) and [M6.11 asset pipeline](docs/M6.11-asset-pipeline.md) and [M6.12 native GS atlas](docs/M6.12-native-gs-atlas.md) and [M6.13 embedded icon atlas](docs/M6.13-embedded-icon-atlas.md) and [M6.14 System diagnostics](docs/M6.14-system-diagnostics.md).

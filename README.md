# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. Recoverable source includes host-side transactions, dependency planning, Store, recipes, Bridge trust, ZIP inspection/staging and a native EE shell with a PS2SDK GS renderer. Earlier M1–M5 history is documented where original source snapshots are not available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | EE entry point, DualShock input, navigation and GS dashboard | PS2DEV cross-build passed; NEXUS.ELF artifact published |
| Graphics | libgraph/libdraw/libfont, double framebuffer, KROM font, embedded NXA1 atlas, EE SHA-256 verification and GIF DMA upload | Boot-time atlas embedding cross-compiles; console visual validation, cover-art cache and animation pending |
| Devices & library | Mounted-volume providers, game catalog and metadata | Not yet implemented natively |
| Store & packages | Signed host catalog, dependency planning, recipes, ZIP inspection and host staging | Host-side code present; no PS2 installer |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side sandbox reference |
| Network | Provider-neutral PS2 contract and host Bridge client | Contracts/client only; no native PS2 transport |

## Native build and binary

The native NEXUS.ELF is compiled using the PS2DEV Docker image and the PS2SDK EE toolchain.

- Latest native EE build with embedded icon atlas: [Actions run #42](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819621)
- Host verification including native digest and NXA1 checks: [Actions run #195](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819542)
- Renderer notes: [M6.10 GS renderer](docs/M6.10-gs-renderer.md), [M6.12 native GS atlas](docs/M6.12-native-gs-atlas.md), [M6.13 embedded icon atlas](docs/M6.13-embedded-icon-atlas.md)

Extracted ELF: **1,724,400 bytes**, ELF32 little-endian MIPS, N32 ABI, statically linked, R5900/MIPS III flags. SHA-256: 51ad1a8c2799aee05452369b8b73fe1141ffcf187737227086c66882b4908d1b. The binary includes the six-icon NXA1 atlas in aligned read-only data.

Build success proves compilation/linking and target format, not that the binary has been tested on a physical PS2.

## Host validation

~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
cc -std=c11 -Wall -Wextra -Werror -Isrc/include src/nexus_navigation.c tests/native/test_navigation.c -o /tmp/nexus_navigation_test
/tmp/nexus_navigation_test
~~~

M6.13 CI verification: [Host tests run #195](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819542) passed 120 Python tests, native SHA-256 known vectors, NXA1 integrity regression, Python compile, C navigation and native C syntax checks. The PS2DEV EE cross-build [#42](https://github.com/SilverCrow2323/rtps2/actions/runs/44579819621) generated NXA1 from the committed PNGs and embedded it in NEXUS.ELF.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The host transaction engine is sandboxed. No PS2 storage is mounted or written. Native device status remains NOT PROBED until real probes exist. ZIP staging writes only to an explicit host cache. PSU/SAS remain unsupported. The synthetic signing key is not a production key.

## Hardware observations supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim; this does not imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Known native limitations
The current graphical renderer is an initial GS shell, not the final animated frontend. The six built-in section icons are now embedded in NEXUS.ELF and loaded during renderer initialization, with procedural glyphs retained as a failure fallback. There is still no cover-art texture cache, multi-slot VRAM allocator or animated section scenes. Live device discovery, filesystem providers, OPL/Neutrino launch handoff, PS2-side HTTP/TLS/Ed25519 and a native Store installer also remain outstanding.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6.8 host staging](docs/M6.8-host-staging.md), [M6.7 signed catalog trust](docs/M6.7-signed-catalog.md), [M6.9 native shell](docs/M6.9-native-shell.md) and [M6.10 GS renderer](docs/M6.10-gs-renderer.md) and [M6.11 asset pipeline](docs/M6.11-asset-pipeline.md) and [M6.12 native GS atlas](docs/M6.12-native-gs-atlas.md) and [M6.13 embedded icon atlas](docs/M6.13-embedded-icon-atlas.md).

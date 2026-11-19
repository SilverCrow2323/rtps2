# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. Recoverable source includes host-side transactions, dependency planning, Store, recipes, Bridge trust, ZIP inspection/staging and a native EE shell with a PS2SDK GS renderer. Earlier M1–M5 history is documented where original source snapshots are not available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | EE entry point, DualShock input, navigation and GS dashboard | PS2DEV cross-build passed; NEXUS.ELF artifact published |
| Graphics | libgraph/libdraw/libfont, double framebuffer, KROM font and NXA1 asset pipeline | Initial native renderer present; GS texture upload/cache still pending |
| Devices & library | Mounted-volume providers, game catalog and metadata | Not yet implemented natively |
| Store & packages | Signed host catalog, dependency planning, recipes, ZIP inspection and host staging | Host-side code present; no PS2 installer |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side sandbox reference |
| Network | Provider-neutral PS2 contract and host Bridge client | Contracts/client only; no native PS2 transport |

## Native build and binary

The native NEXUS.ELF is compiled using the PS2DEV Docker image and the PS2SDK EE toolchain.

- GS renderer build: [Actions run #44488236616](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236616)
- Host verification: [Actions run #44488236424](https://github.com/SilverCrow2323/rtps2/actions/runs/44488236424)
- Renderer notes: [M6.10 GS renderer](docs/M6.10-gs-renderer.md)

Extracted ELF: **1,564,624 bytes**, ELF32 little-endian MIPS, N32 ABI, statically linked, R5900/MIPS III flags. SHA-256: c313bc7ca7e00cb1c49d30d784d78ab59921132f31f7b55223c59207d49ebf27.

Build success proves compilation/linking and target format, not that the binary has been tested on a physical PS2.

## Host validation

~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
cc -std=c11 -Wall -Wextra -Werror -Isrc/include src/nexus_navigation.c tests/native/test_navigation.c -o /tmp/nexus_navigation_test
/tmp/nexus_navigation_test
~~~

M6.11 CI verification: [Host tests run #129](https://github.com/SilverCrow2323/rtps2/actions/runs/44574962515) passed 120 Python tests, Python compile, C navigation regression and native C syntax checks. The prior M6.10 PS2DEV EE cross-build remains the latest verified native ELF build.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The host transaction engine is sandboxed. No PS2 storage is mounted or written. Native device status remains NOT PROBED until real probes exist. ZIP staging writes only to an explicit host cache. PSU/SAS remain unsupported. The synthetic signing key is not a production key.

## Hardware observations supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim; this does not imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Known native limitations
The current graphical renderer is an initial GS shell, not the final animated frontend. The host-side PNG-to-NXA1 icon packer is committed, but the native renderer does not yet upload or draw that atlas and has no cover-art texture cache or animated section scenes. Live device discovery, filesystem providers, OPL/Neutrino launch handoff, PS2-side HTTP/TLS/Ed25519 and a native Store installer also remain outstanding.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6.8 host staging](docs/M6.8-host-staging.md), [M6.7 signed catalog trust](docs/M6.7-signed-catalog.md), [M6.9 native shell](docs/M6.9-native-shell.md) and [M6.10 GS renderer](docs/M6.10-gs-renderer.md) and [M6.11 asset pipeline](docs/M6.11-asset-pipeline.md).

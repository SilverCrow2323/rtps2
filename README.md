# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. Recoverable source includes host-side transactions, dependency planning, Store, recipes, Bridge trust, ZIP inspection/staging and a new native EE shell bootstrap. Earlier M1–M5 history is documented where original source snapshots are not available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | EE entry point, DualShock input, navigation and bootstrap pages | PS2DEV cross-build passed; ELF artifact published |
| Devices & library | Mounted-volume providers, game catalog and metadata | Not yet implemented natively |
| Artwork | NXA1 texture pipeline and GS upload path | Historical M4 description only; source recovery pending |
| Store & packages | Signed host catalog, dependencies, recipes, ZIP inspection and host staging | Host-side code present; no PS2 installer |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side sandbox reference |
| Network | Provider-neutral PS2 contract and host Bridge client | Contracts/client only; no native PS2 transport |

## Native bootstrap

The initial NEXUS.ELF has been cross-compiled using the PS2DEV container/PS2SDK. The native shell currently uses a debug text renderer and DualShock navigation; it is a bootstrapped shell, not the final graphical frontend.

- Native build workflow: [run #44486831714](https://github.com/SilverCrow2323/rtps2/actions/runs/44486831714)
- Host verification: [run #44486842812](https://github.com/SilverCrow2323/rtps2/actions/runs/44486842812)
- Build instructions: [native shell notes](docs/M6.9-native-shell.md)

## Host validation

~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
cc -std=c11 -Wall -Wextra -Werror -Isrc/include src/nexus_navigation.c tests/native/test_navigation.c -o /tmp/nexus_navigation_test
/tmp/nexus_navigation_test
~~~

The host test suite passes 112 Python cases plus C navigation and C syntax checks. The PS2DEV EE cross-build is verified separately. A passing build does not replace a physical-console test.

## Safety line
NEXUS must not silently replace boot-critical assets or rewrite FMCB/FHDB/OpenTuna/FunTuna/PS2BBL/PSBBN/OSDSYS installations. The host transaction engine is sandboxed. No PS2 storage is mounted or written. Native device status remains NOT PROBED until real probes exist. ZIP staging writes only to an explicit host cache. PSU/SAS remain unsupported. The synthetic signing key is not a production key.

## Hardware observations supplied for planning
- FAT SCPH-30004: internal HDD with original Network Adapter, PSBBN, POPS and MMCE reported working.
- Slim SCPH-70004: MMCE status not confirmed.
- Bluetooth dongle with DS3 reported working on both FAT and Slim; this does not imply USB Wi-Fi support.

These are user-reported observations, not CI or independently repeated hardware tests.

## Known native limitations
No GS graphical renderer, final UI design system, live device discovery, filesystem providers, OPL/Neutrino handoff, PS2-side HTTP/TLS/Ed25519 or native Store installer is included yet. The current source is a bootstrapped native shell, not the finished frontend.

See [roadmap](docs/ROADMAP.md), [verified status](docs/STATUS.md), [safety rules](docs/SAFETY.md), [M6.8 host staging](docs/M6.8-host-staging.md), [M6.7 signed catalog trust](docs/M6.7-signed-catalog.md) and [M6.9 native shell](docs/M6.9-native-shell.md).

# PS2BBN NEXUS

**The PS2, rebuilt as a navigable console experience.**  
A native PlayStation 2 frontend and system shell inspired by PSBBN, developed under SPDW Factory.

> This repository is the canonical home for NEXUS. Recoverable source now includes host-side transaction, dependency, Store planning, recipes, Bridge trust, ZIP inspection/staging and a new native EE shell bootstrap. Earlier M1–M5 history is documented where original source snapshots are not currently available; missing code is not fabricated or represented as merged.

## Project map
| Area | Purpose | Current status |
|---|---|---|
| Native shell | EE entry point, DualShock input, navigation and bootstrap pages | New M6.9 source in src/; PS2SDK build pending |
| Devices & library | Mounted-volume providers, game catalog and metadata | Not yet implemented natively |
| Artwork | NXA1 texture pipeline and GS upload path | Historical M4 description only; source recovery pending |
| Store & packages | Signed host catalog, dependencies, recipes, ZIP inspection and host staging | Host-side code present; no PS2 installer |
| Transactions | Journal, backup, verification, rollback and recovery | Host-side sandbox reference |
| Network | Provider-neutral PS2 contract and host Bridge client | Contracts/client only; no native PS2 transport |

## Repository structure
- src/ — new PS2 native EE shell bootstrap, input, navigation, UI and device-provider interfaces.
- nexus_txn/ — host-side reference logic for transactions, dependencies, Store planning/TUI, recipes, Bridge, trust, archive inspection and staging.
- tools/ — host-side simulation CLI.
- tests/ — Python and C host regression tests.
- docs/ — architecture, milestone reports, safety rules and roadmap.

## Native bootstrap

With the official PS2DEV toolchain and PS2SDK installed and configured:

~~~sh
make -C src
~~~

The expected output is src/NEXUS.ELF. The first version uses a text-mode PS2SDK renderer and DualShock navigation. See [native shell notes](docs/M6.9-native-shell.md).

## Host validation

~~~sh
python -m pip install -r requirements-host.txt
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
cc -std=c11 -Wall -Wextra -Werror -Isrc/include src/nexus_navigation.c tests/native/test_navigation.c -o /tmp/nexus_navigation_test
/tmp/nexus_navigation_test
~~~

The host test suite previously reached 112 Python cases; the new C navigation test is added to the current GitHub Actions workflow. A passing host test is not a PS2SDK cross-build or console test.

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

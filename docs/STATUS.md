# NEXUS — verified status snapshot

## Source present

- `nexus_txn/engine.py`: host-only transactional reference engine with manifest/path/hash checks, lock, journal, backups, copy/read-back verification, rollback and constrained recovery.
- `nexus_txn/dependencies.py`: deterministic dependency ordering and missing/cycle/conflict/duplicate checks.
- `nexus_txn/store.py`: catalog validation, curated profiles, dependency-aware install/update/keep preview, conservative peak-space estimation and dependency-safe removal preview.
- `nexus_txn/tui.py`: deterministic text renderers for plan and progress screens.
- `src/include/nexus_network.h`: provider-neutral network contract and capability flags.
- `tools/nexus_txn.py`: sandbox simulation CLI.

## Verification

The current snapshot passes **30 host unit tests** and Python compile checks. These checks do not compile a PS2 ELF and do not validate hardware behavior.

## Not present / not verified

- M1–M5 complete source trees.
- Native PS2 frontend integration of the M6 modules.
- PS2SDK cross-compiled `NEXUS.ELF`.
- PS2-side network transport, TLS, Companion client or real resumable download.
- Native ZIP/PSU/SAS parsing and extraction.
- Real install/remove operations on MC, MMCE, USB, APA/PFS or HDD.
- Console launch tests for OPL/Neutrino or any other backend.

## User-reported hardware observations

FAT SCPH-30004: PSBBN, POPS and MMCE work with internal HDD/original Network Adapter. Slim SCPH-70004: MMCE unconfirmed. DS3 Bluetooth dongle works on FAT and Slim. These notes guide generic compatibility planning only.

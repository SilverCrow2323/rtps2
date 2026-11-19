# NEXUS — verified status snapshot

## M6.5 source
- Bridge client core: injected transport, bearer authentication, catalog validation, explicit archive size vs installed size, ranged downloads, resumable PC `.part` cache and final size/SHA-256 verification.
- Bridge-to-Store adapter: requires a separately declared installed size; it never mistakes compressed download size for installed storage requirement.
- Store workflow state machine: SELECT → DEPENDENCIES → SPACE → CONFIRM → DOWNLOAD → VERIFY → READY_TO_INSTALL. It blocks confirmation if capacity is unknown/insufficient, verification before 100%, and install-ready state before successful verification.
- Previous M6.2 transaction reference engine, dependency resolver, M6.3 Store planning/TUI and M6.4 package recipe validation remain present.
- GitHub Actions host test workflow configured for push/PR.

## Verification boundary
M6.5 currently has fake-transport regression tests committed, but the hosted GitHub Actions run must be checked before marking CI green. The code is host-side Python; no PS2SDK build or console test is claimed.

## Not implemented
- Actual M5 Companion/Bridge route compatibility (the original M5 source is still unavailable).
- TLS implementation, signed catalog trust, publisher authentication beyond the supplied bearer token.
- ZIP/PSU/SAS archive inspection/extraction.
- Native PS2 Store UI, network transport, native installer or device writes.
- Bootloader/OSDSYS/FMCB/FHDB/PSBBN changes.

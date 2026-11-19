# NEXUS M6 — Transaction safety and network contracts

## Transaction reference engine (host only)

The Python reference engine operates only inside an explicitly supplied sandbox root. It is not wired to PS2 mount points. It validates manifest paths and SHA-256 values before applying changes, rejects protected boot/system paths, rejects symlink traversal, serializes writers with an exclusive lock, journals state transitions, backs up replaced regular files, verifies copied data, and rolls back interrupted work.

Recovery validates the journal root and constrains backup paths to the transaction backup directory. A stale lock is deliberately not removed automatically: an operator must inspect it first. The engine is a host reference and still needs a separately designed PS2 C implementation with platform-specific durability semantics before any device write can be considered.

## Dependency resolver

`nexus_txn.dependencies.resolve()` accepts a curated package catalog with `id`, `version`, `requires` and `conflicts`. It returns a deterministic topological installation order and rejects missing dependencies, duplicate IDs, cycles and selected-package conflicts. Version range semantics are intentionally not invented; catalog policy must define and validate them before production use.

## Network provider contract

`src/include/nexus_network.h` separates link state from address readiness and exposes explicit capability flags for DHCP, DNS, HTTP, HTTPS, Range and Wi-Fi. A provider must advertise only implemented capabilities. Bluetooth HID support says nothing about Wi-Fi/network support. No PS2 network driver, PS2-side Bridge client, TLS stack or live network transport is included.

## Validation

```sh
PYTHONPATH=. python3 -m unittest discover -s tests -v
python3 -m compileall -q nexus_txn tools tests
```

## Outside M6 scope

Native PS2 C installer and PS2SDK build; PS2-side HTTP/TLS/Bridge client; actual resumable downloads; PSU parser/SAS integration; on-console archive extraction; UI integration; live device enumeration; any writes to MC/MMCE/USB/PFS/HDD or boot files.

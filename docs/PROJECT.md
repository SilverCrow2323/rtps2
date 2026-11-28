# Project: PS2BBN NEXUS

## Purpose
Build a native PlayStation 2 frontend and system shell inspired by PSBBN, with a controller-driven dashboard for installed games, PS1/PS2 libraries, files, system state and a provider-neutral Store workflow.

## Users and use cases
- PS2 owners using supported storage setups and a DualShock controller.
- Maintainers validating native EE builds, storage probes, package metadata and PC-side staging.
- Contributors integrating PS2SDK modules without assuming that host tools are safe to run on console storage.

## Scope and boundaries
In scope: native EE shell, GS rendering, controller navigation, read-only library scanning, cover metadata/cache, storage inventory, host-side package catalogues, dependency planning and transaction simulations.

Out of scope: redistributing commercial games, keys or proprietary assets; undocumented OPL/Neutrino command lines; automatic boot-chain updates; and treating the host transaction reference as an approved PS2 installer. The native Files scanner currently performs bounded read-only enumeration and does not mount, write or launch content.

## Target
- PlayStation 2 fat/slim hardware and PS2SDK EE toolchain.
- Host development and tests on Linux, including MX Linux; PCSX2 is a smoke-test environment, not physical-console proof.
- DualShock input and PS2 GS renderer constraints.

## Architecture
- Native NEXUS shell: EE entry point, input, navigation and GS renderer.
- Library/device layer: typed catalogue, read-only storage adapters and scan staging.
- Artwork pipeline: NXA1 atlas, SHA-256 verification and bounded cover cache.
- Store/Bridge: host-side catalogue, recipe inspection, dependency planner and staged transaction reference.
- Safety boundary: explicit FileXio activation and read-only scans; no privileged installer in the current phase.

## Dependencies and integration
PS2SDK/PS2DEV for native builds; PCSX2 for host-side smoke testing; OPL/Neutrino only through documented and version-aware handoff contracts. PSBBV is a separate Vita product and shares design lineage, not runtime binaries.

## Safety and quality
Reject traversal, unsafe roots, symlink escapes, untrusted package destinations and missing checksums. Treat unknown capacity as unknown. Do not mount or write during discovery. Host tests and emulator runs must be reported separately from physical PS2 validation. UI remains controller-first and memory-bounded.

## Verified baseline
The repository contains a native EE shell, PS2SDK build path, read-only library scanner, host transaction/catalogue tooling and native C host tests. Published ELF metadata and CI builds do not prove physical PS2 storage-driver or gameplay compatibility.

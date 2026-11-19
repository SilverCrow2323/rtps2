# Native NEXUS shell — bootstrap build

This directory starts a new, explicitly versioned native PS2 shell implementation. It does **not** claim to recover the missing M1–M5 source tree.

## Build

Install the official PS2DEV toolchain and PS2SDK, then configure PS2DEV and PS2SDK as required by your installation. Build the EE executable with:

~~~sh
make -C src
~~~

The output is src/NEXUS.ELF. A clean build can be requested with make -C src clean.

The makefile follows the official PS2SDK sample pattern (Makefile.pref and Makefile.eeglobal) and links libpad plus the debug text renderer.

## Bootstrap controls

- D-pad up/down: move through the home sections
- Cross: open selected section
- L1/R1: previous/next section
- Circle: back to dashboard
- Start: return home

## Current native scope

- PS2SDK libpad input with a 256-byte work area aligned to 64 bytes.
- Reads controller state only when padGetState() reports PAD_STATE_STABLE.
- Loads the ROM SIO2MAN and PADMAN modules using PS2SDK loadfile functions.
- Initial text dashboard and separate placeholder pages for Games, PS1, Homebrew, Store, Files and System.
- Device state remains NOT PROBED until a real device provider has probed the hardware. It does not reuse earlier hardware test reports as a live scan.
- Navigation logic is a platform-independent C module with host unit tests.

## Not yet included

- GS graphical renderer, fonts, artwork, textures or animation.
- Device probing or filesystem mounts.
- OPL/Neutrino launch adapters.
- PS2-side catalog/TLS/Ed25519 client or native package installer.
- PS2SDK cross-build or console execution in CI. Build and hardware testing must be recorded separately once performed.

The PS2SDK controller API contract is defined in upstream ee/rpc/pad/include/libpad.h; the official pad sample uses the same module-loading and aligned-buffer model.

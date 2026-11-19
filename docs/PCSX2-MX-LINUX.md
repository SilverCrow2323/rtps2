# PCSX2 on MX Linux XFCE — NEXUS quick start

This guide installs the official PCSX2 Flatpak, downloads the current NEXUS ELF artifact and boots it directly.

## 1. Install Flatpak and enable Flathub

MX Linux includes Flatpak support in MX Package Installer from MX 18 onward. You can enable Flathub from MX Package Installer → Flatpaks, or use the terminal:

    sudo apt update
    sudo apt install -y flatpak
    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    flatpak install --user flathub net.pcsx2.PCSX2

## 2. First PCSX2 launch

    flatpak run net.pcsx2.PCSX2

Complete the first-run wizard and select a BIOS dumped from your own PS2. PCSX2 does not ship with a BIOS.

## 3. Download the NEXUS build

The current known-good build is Native EE workflow #43. Its `NEXUS-ELF` artifact is retained by GitHub Actions for 30 days from 19 November 2026 (scheduled expiry: 19 December 2026).

Install GitHub CLI and authenticate:

    sudo apt install -y gh
    gh auth login

Download and extract the artifact:

    mkdir -p "$HOME/PS2/NEXUS"
    gh run download 44580353828 --repo SilverCrow2323/rtps2 --name NEXUS-ELF --dir "$HOME/PS2/NEXUS"

The resulting file is:

    $HOME/PS2/NEXUS/NEXUS.ELF

## 4. Boot NEXUS directly

PCSX2's current command line supports `-elf <file>`:

    flatpak run --filesystem="$HOME/PS2/NEXUS" net.pcsx2.PCSX2 -elf "$HOME/PS2/NEXUS/NEXUS.ELF"

No ISO is needed. The emulator still needs a correctly configured BIOS.

## 5. Optional repository helper

After cloning the repository, the launcher helper detects the Flatpak or native Qt executable:

    bash tools/run_nexus_pcsx2.sh

Or pass an explicit ELF path:

    bash tools/run_nexus_pcsx2.sh "$HOME/PS2/NEXUS/NEXUS.ELF"

## First test checklist

- Confirm the NEXUS dashboard appears.
- Open System and check the reported video mode (PAL 640×512 or NTSC 640×448).
- Check whether it reports `NXA1 ICON ATLAS READY`; otherwise the dashboard should use procedural fallback icons.
- Test the DualShock mapping/navigation.

This is an emulator smoke test. It does not replace validation on an actual PS2, especially for controller timing, video output and GS texture appearance.

## Official references

- PCSX2 Linux setup and Flatpak: https://pcsx2.net/docs/setup/running/
- PCSX2 command-line options: https://pcsx2.net/docs/advanced/cli/
- MX Linux Flathub setup: https://flathub.org/setup/MX%20Linux
- PCSX2 Flathub listing: https://flathub.org/apps/net.pcsx2.PCSX2

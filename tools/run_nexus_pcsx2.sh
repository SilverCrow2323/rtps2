#!/usr/bin/env bash
set -euo pipefail

ELF_INPUT="${1:-$HOME/PS2/NEXUS/NEXUS.ELF}"
APP_ID="net.pcsx2.PCSX2"

if [[ ! -f "$ELF_INPUT" || ! -r "$ELF_INPUT" ]]; then
  printf 'NEXUS ELF not found or unreadable: %s\n' "$ELF_INPUT" >&2
  printf 'Usage: %s [/path/to/NEXUS.ELF]\n' "$0" >&2
  exit 2
fi

ELF_DIR="$(cd -- "$(dirname -- "$ELF_INPUT")" && pwd -P)"
ELF_PATH="$ELF_DIR/$(basename -- "$ELF_INPUT")"

if command -v flatpak >/dev/null 2>&1 && flatpak info "$APP_ID" >/dev/null 2>&1; then
  exec flatpak run --filesystem="$ELF_DIR" "$APP_ID" -elf "$ELF_PATH"
fi

if command -v pcsx2-qt >/dev/null 2>&1; then
  exec pcsx2-qt -elf "$ELF_PATH"
fi

cat >&2 <<'EOF'
PCSX2 was not found.
Install the stable Flatpak build with:
  flatpak install --user flathub net.pcsx2.PCSX2
Then run this script again.
EOF
exit 127

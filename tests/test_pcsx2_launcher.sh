#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "$0")/.. && pwd -P)"
LAUNCHER="$ROOT/tools/run_nexus_pcsx2.sh"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

BIN="$TMP/bin"
ELF_DIR="$TMP/NEXUS build with spaces"
ELF="$ELF_DIR/NEXUS test.ELF"
mkdir -p "$BIN" "$ELF_DIR"
: > "$ELF"

cat > "$BIN/flatpak" <<'EOF'
#!/usr/bin/env bash
if [[ "$1" == "info" ]]; then
  exit "$FAKE_FLATPAK_INFO_STATUS"
fi
printf '%s\n' "$@" > "$CAPTURE_ARGS"
EOF
chmod +x "$BIN/flatpak"

cat > "$BIN/pcsx2-qt" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "$@" > "$CAPTURE_ARGS"
EOF
chmod +x "$BIN/pcsx2-qt"

CAPTURE_ARGS="$TMP/flatpak.args" FAKE_FLATPAK_INFO_STATUS=0 PATH="$BIN:$PATH" \
  bash "$LAUNCHER" "$ELF"
[[ "$(wc -l < "$TMP/flatpak.args")" -eq 5 ]]
[[ "$(sed -n '1p' "$TMP/flatpak.args")" == "run" ]]
[[ "$(sed -n '2p' "$TMP/flatpak.args")" == "--filesystem=$ELF_DIR" ]]
[[ "$(sed -n '3p' "$TMP/flatpak.args")" == "net.pcsx2.PCSX2" ]]
[[ "$(sed -n '4p' "$TMP/flatpak.args")" == "-elf" ]]
[[ "$(sed -n '5p' "$TMP/flatpak.args")" == "$ELF" ]]

CAPTURE_ARGS="$TMP/native.args" FAKE_FLATPAK_INFO_STATUS=1 PATH="$BIN:$PATH" \
  bash "$LAUNCHER" "$ELF"
[[ "$(wc -l < "$TMP/native.args")" -eq 2 ]]
[[ "$(sed -n '1p' "$TMP/native.args")" == "-elf" ]]
[[ "$(sed -n '2p' "$TMP/native.args")" == "$ELF" ]]

set +e
PATH="$BIN:$PATH" bash "$LAUNCHER" "$TMP/missing.ELF" >/dev/null 2>&1
STATUS=$?
set -e
[[ "$STATUS" -eq 2 ]]

printf 'PCSX2 launcher tests: 3 scenarios passed\n'

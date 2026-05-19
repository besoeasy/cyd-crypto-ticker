#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE_DIR="${SITE_DIR:-$ROOT_DIR/.web-flasher/site}"
SOURCE_DIR="$ROOT_DIR/web-flasher"
VERSION="${VERSION:-}"

resolve_executable() {
  local explicit_path="$1"
  local fallback_name="$2"
  shift 2

  if [[ -n "$explicit_path" ]]; then
    echo "$explicit_path"
    return 0
  fi

  local candidate
  for candidate in "$@"; do
    if [[ -x "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done

  if command -v "$fallback_name" >/dev/null 2>&1; then
    command -v "$fallback_name"
    return 0
  fi

  return 1
}

resolve_first_file() {
  local explicit_path="$1"
  shift

  if [[ -n "$explicit_path" ]]; then
    echo "$explicit_path"
    return 0
  fi

  local candidate
  for candidate in "$@"; do
    if [[ -f "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done

  return 1
}

PIO_BIN="$(resolve_executable "${PIO_BIN:-}" pio "$HOME/.platformio/penv/bin/pio" || true)"
PLATFORMIO_PYTHON="$(resolve_executable "${PLATFORMIO_PYTHON:-}" python3 "$HOME/.platformio/penv/bin/python" || true)"
ESPTOOL_BIN="$(resolve_first_file "${ESPTOOL_BIN:-}" \
  "$HOME/.platformio/packages/tool-esptoolpy/esptool.py" \
  "$HOME/.platformio/penv/bin/esptool.py")"
BOOT_APP0_BIN="$(resolve_first_file "${BOOT_APP0_BIN:-}" \
  "$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin")"

require_file() {
  local path="$1"
  if [[ ! -f "$path" ]]; then
    echo "Missing required file: $path" >&2
    exit 1
  fi
}

require_dir() {
  local path="$1"
  if [[ ! -d "$path" ]]; then
    echo "Missing required directory: $path" >&2
    exit 1
  fi
}

if [[ -z "$PIO_BIN" ]]; then
  echo "PlatformIO executable not found. Set PIO_BIN or ensure 'pio' is on PATH." >&2
  exit 1
fi

if [[ -z "$PLATFORMIO_PYTHON" ]]; then
  echo "Python 3 executable not found. Set PLATFORMIO_PYTHON or ensure 'python3' is on PATH." >&2
  exit 1
fi

require_file "$PIO_BIN"
require_file "$PLATFORMIO_PYTHON"
require_file "$ESPTOOL_BIN"
require_file "$BOOT_APP0_BIN"
require_dir "$SOURCE_DIR"

if [[ -z "$VERSION" ]]; then
  VERSION="$(git -C "$ROOT_DIR" describe --tags --always --dirty 2>/dev/null || git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo dev)"
fi

BUILD_DATE="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

rm -rf "$SITE_DIR"
mkdir -p "$SITE_DIR/firmware"
cp "$SOURCE_DIR/index.html" "$SITE_DIR/index.html"

build_env() {
  local env_name="$1"
  local env_dir="$ROOT_DIR/.pio/build/$env_name"
  local output_dir="$SITE_DIR/firmware/$env_name"
  local merged_bin="$output_dir/merged-firmware.bin"

  "$PIO_BIN" run -e "$env_name"

  require_dir "$env_dir"
  require_file "$env_dir/bootloader.bin"
  require_file "$env_dir/partitions.bin"
  require_file "$env_dir/firmware.bin"

  mkdir -p "$output_dir"

  "$PLATFORMIO_PYTHON" "$ESPTOOL_BIN" --chip esp32 merge_bin \
    -o "$merged_bin" \
    --flash_mode dio \
    --flash_freq 40m \
    --flash_size 4MB \
    0x1000 "$env_dir/bootloader.bin" \
    0x8000 "$env_dir/partitions.bin" \
    0xe000 "$BOOT_APP0_BIN" \
    0x10000 "$env_dir/firmware.bin"

  cat > "$output_dir/manifest.json" <<EOF
{
  "name": "CYD Crypto Ticker",
  "version": "$VERSION",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        {
          "path": "merged-firmware.bin",
          "offset": 0
        }
      ]
    }
  ]
}
EOF
}

build_env cyd
build_env cyd2usb

cat > "$SITE_DIR/version.json" <<EOF
{
  "version": "$VERSION",
  "builtAt": "$BUILD_DATE",
  "targets": [
    {
      "id": "cyd",
      "label": "CYD"
    },
    {
      "id": "cyd2usb",
      "label": "CYD2USB"
    }
  ]
}
EOF

echo "Web flasher site written to $SITE_DIR"
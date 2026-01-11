#!/usr/bin/env bash
set -euo pipefail

DEV="${1:-}"
WIPE_METHOD="${2:-unknown}"
LOGFILE="${3:-/dev/null}"

if [ -z "$DEV" ]; then
  echo "Usage: $0 <device_node> <wipe_method> <logfile>"
  exit 2
fi

OWNER="${SUDO_USER:-$USER}"
GROUP="$(id -gn "$OWNER" 2>/dev/null || echo "$OWNER")"

sha256_file() { sha256sum "$1" 2>/dev/null | awk '{print $1}'; }
now_ts() { date --utc +"%Y-%m-%dT%H:%M:%SZ"; }

LSBLK_OUT=$(lsblk -P -b -o NAME,KNAME,MODEL,SERIAL,SIZE,VENDOR "$DEV" 2>/dev/null || true)
parse_field(){ local key=$1; echo "$LSBLK_OUT" | sed -n "s/.*${key}=\"\\([^\"]*\\)\".*/\\1/p" || true; }

NAME="$(parse_field NAME)"
KNAME="$(parse_field KNAME)"
MODEL="$(parse_field MODEL)"
SERIAL="$(parse_field SERIAL)"
SIZE="$(parse_field SIZE)"

if command -v udevadm >/dev/null 2>&1; then
  UDEV_OUT=$(udevadm info --query=property --name="$DEV" 2>/dev/null || true)
  u_model=$(echo "$UDEV_OUT" | awk -F= '/ID_MODEL=/{print $2; exit}' || true)
  u_serial=$(echo "$UDEV_OUT" | awk -F= '/ID_SERIAL=/{print $2; exit}' || true)
  MODEL="${MODEL:-$u_model}"
  SERIAL="${SERIAL:-$u_serial}"
fi

SYSBASE="/sys/block/$(basename "$DEV")/device"
if [ -d "$SYSBASE" ]; then
  [ -f "$SYSBASE/model" ] && [ -z "$MODEL" ] && MODEL=$(cat "$SYSBASE/model" 2>/dev/null || true)
  [ -f "$SYSBASE/serial" ] && [ -z "$SERIAL" ] && SERIAL=$(cat "$SYSBASE/serial" 2>/dev/null || true)
  [ -f "$SYSBASE/wwid" ] && [ -z "$SERIAL" ] && SERIAL=$(cat "$SYSBASE/wwid" 2>/dev/null || true)
fi

MODEL="${MODEL:-unknown}"
SERIAL="${SERIAL:-unknown}"
SIZE="${SIZE:-0}"
KNAME="${KNAME:-unknown}"

case "$DEV" in
  /dev/nvme*) DRVTYPE="nvme" ;;
  /dev/sd*)   DRVTYPE="ata" ;;
  /dev/loop*) DRVTYPE="loopback" ;;
  *)          DRVTYPE="block" ;;
esac

FINGERPRINT_INPUT="${SERIAL}|${MODEL}|${SIZE}|${DEV}"
FINGERPRINT=$(echo -n "$FINGERPRINT_INPUT" | sha256sum | awk '{print $1}')

SAMPLE_HASH="unavailable"
if [ -b "$DEV" ]; then
  SAMPLE_TMP=$(mktemp)
  dd if="$DEV" of="$SAMPLE_TMP" bs=1M count=1 status=none 2>/dev/null || true
  if [ -s "$SAMPLE_TMP" ]; then SAMPLE_HASH=$(sha256_file "$SAMPLE_TMP"); fi
  rm -f "$SAMPLE_TMP"
else
  SAMPLE_HASH="not_block_device"
fi

NVME_VER="$(nvme version 2>/dev/null || echo 'nvme-N/A')"
HDPARM_VER="$(hdparm -v 2>/dev/null | head -n1 || echo 'hdparm-N/A')"
FASTBOOT_VER="$(fastboot --version 2>/dev/null | head -n1 || echo 'fastboot-N/A')"
ADB_VER="$(adb --version 2>/dev/null | head -n1 || echo 'adb-N/A')"
OS_INFO="$(uname -a 2>/dev/null || echo 'os-N/A')"

TS="$(now_ts)"
ATTEST_JSON="$(mktemp --suffix=.json)"
cat > "$ATTEST_JSON" <<EOF
{
  "sentinel_version": "PoC-2025-10",
  "timestamp_utc": "$TS",
  "device_node": "$DEV",
  "kname": "$KNAME",
  "model": "$MODEL",
  "serial": "$SERIAL",
  "device_type": "$DRVTYPE",
  "size_bytes": $SIZE,
  "device_fingerprint": "$FINGERPRINT",
  "wipe_method": "$WIPE_METHOD",
  "post_wipe_sample_sha256": "$SAMPLE_HASH",
  "tool_versions": {
    "nvme": "$NVME_VER",
    "hdparm": "$HDPARM_VER",
    "fastboot": "$FASTBOOT_VER",
    "adb": "$ADB_VER",
    "os": "$OS_INFO"
  },
  "logfile": "$LOGFILE"
}
EOF

mkdir -p keys
if [ ! -f keys/commander_key.pem ]; then
  echo "[*] generating demo RSA key at keys/commander_key.pem"
  openssl genpkey -algorithm RSA -out keys/commander_key.pem -pkeyopt rsa_keygen_bits:2048
  openssl rsa -in keys/commander_key.pem -pubout -out keys/commander_pub.pem
fi

SIG_FILE="${ATTEST_JSON}.sig"
openssl dgst -sha256 -sign keys/commander_key.pem -out "$SIG_FILE" "$ATTEST_JSON"

find_removable_mount() {
  while read -r src tgt rest; do
    devbase="$(basename "$src" | sed 's/[0-9]*$//')"
    if [ -e "/sys/block/$devbase/removable" ]; then
      if [ "$(cat /sys/block/$devbase/removable 2>/dev/null || echo 0)" = "1" ]; then
        echo "$tgt"
        return 0
      fi
    fi
  done < <(findmnt -rn -o SOURCE,TARGET)
  return 1
}

DEST_DIR=""
REM_MOUNT="$(find_removable_mount || true)"
if [ -n "$REM_MOUNT" ] && [ -w "$REM_MOUNT" ]; then
  DEST_DIR="${REM_MOUNT}/sentinel_attestations"
else
  DEST_DIR="$(pwd)/attestations"
fi

mkdir -p "$DEST_DIR"
OUT_BASE="$DEST_DIR/attest-$(basename "$DEV")-$(date +%Y%m%dT%H%M%SZ)"

cp "$ATTEST_JSON" "${OUT_BASE}.json"
cp "$SIG_FILE" "${OUT_BASE}.sig"
cp keys/commander_pub.pem "${OUT_BASE}.pub.pem"

if command -v chown >/dev/null 2>&1; then
  sudo -n true 2>/dev/null || true
  chown "${OWNER}:${GROUP}" "${OUT_BASE}.json" "${OUT_BASE}.sig" "${OUT_BASE}.pub.pem" 2>/dev/null || true
fi
chmod 644 "${OUT_BASE}.json" "${OUT_BASE}.sig" "${OUT_BASE}.pub.pem" 2>/dev/null || true

echo "[*] Attestation written:"
echo "    JSON : ${OUT_BASE}.json"
echo "    SIG  : ${OUT_BASE}.sig"
echo "    PUB  : ${OUT_BASE}.pub.pem"

rm -f "$ATTEST_JSON" "$SIG_FILE"
exit 0

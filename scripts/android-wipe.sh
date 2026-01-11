#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-fastboot}"
DEV="${2:-}"

LOG="/tmp/sentinel-android-$(date +%s).log"
echo "[*] android-wipe -> mode:$MODE dev:$DEV" | tee "$LOG"

if [ "$MODE" = "fastboot" ]; then
  echo "[*] Looking for fastboot devices" | tee -a "$LOG"
  fastboot devices 2>&1 | tee -a "$LOG" || true
  if [ -n "$DEV" ]; then
    echo "[*] Erasing userdata on $DEV" | tee -a "$LOG"
    fastboot -s "$DEV" erase userdata 2>&1 | tee -a "$LOG" || fastboot -s "$DEV" format userdata 2>&1 | tee -a "$LOG"
    SERIAL="$DEV"
  else
    SERIAL="$(fastboot devices 2>/dev/null | awk 'NR==1{print $1}' || true)"
    if [ -z "$SERIAL" ]; then
      echo "[!] No fastboot device found." | tee -a "$LOG"
      exit 1
    fi
    fastboot -s "$SERIAL" erase userdata 2>&1 | tee -a "$LOG" || fastboot -s "$SERIAL" format userdata 2>&1 | tee -a "$LOG"
  fi
elif [ "$MODE" = "adb" ]; then
  echo "[*] Attempting adb-based wipe (requires device with adb & permissions)" | tee -a "$LOG"
  
  echo "[*] Explicitly starting adb server daemon..." | tee -a "$LOG"
  adb start-server 2>&1 | tee -a "$LOG"
  sleep 1 
  
  echo "[*] Listing adb devices..." | tee -a "$LOG"
  adb devices 2>&1 | tee -a "$LOG"
  SERIAL="$(adb devices 2>/dev/null | awk 'NR==2{print $1}' || true)"
  if [ -z "$SERIAL" ]; then
    echo "[!] No adb device found." | tee -a "$LOG"
    exit 1
  fi
  echo "[*] Rebooting into recovery" | tee -a "$LOG"
  adb -s "$SERIAL" reboot recovery 2>&1 | tee -a "$LOG" || true
else
  echo "usage: $0 fastboot|adb [device]" | tee -a "$LOG"
  exit 1
fi

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
bash "$SCRIPT_DIR/attest.sh" "/dev/unknown-${SERIAL}" "android-${MODE}" "$LOG" || echo "[!] android attestation failed" | tee -a "$LOG"

exit 0

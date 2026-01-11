#!/usr/bin/env bash
set -euo pipefail
MODE="${2:-adb}"
LOG="/tmp/sentinel-detect-android-$(date +%s).log"

echo "[*] android-detect -> mode:$MODE" | tee "$LOG"

if [ "$MODE" = "fastboot" ]; then
  echo "[*] Looking for fastboot devices..." | tee -a "$LOG"
  fastboot devices 2>&1 | tee -a "$LOG"
  SERIAL="$(fastboot devices 2>/dev/null | awk 'NR==1{print $1}' || true)"
  if [ -n "$SERIAL" ]; then
    echo "[+] Found fastboot device: $SERIAL" | tee -a "$LOG"
  else
    echo "[!] No fastboot device detected." | tee -a "$LOG"
  fi
elif [ "$MODE" = "adb" ]; then
  echo "[*] Looking for adb devices..." | tee -a "$LOG"
  adb devices 2>&1 | tee -a "$LOG"
  SERIAL="$(adb devices 2>/dev/null | awk 'NR==2{print $1}' || true)"
  if [ -n "$SERIAL" ]; then
    echo "[+] Found adb device: $SERIAL" | tee -a "$LOG"
  else
    echo "[!] No adb device detected." | tee -a "$LOG"
  fi
else
  echo "usage: $0 fastboot|adb" | tee -a "$LOG"
  exit 1
fi

exit 0

#!/usr/bin/env bash
set -euo pipefail
DEV="$1"
METHOD="${2:-overwrite-random}"
LOG="/tmp/sentinel-wipe-$(date +%s).log"
echo "[*] wipe-device.sh -> dev:$DEV method:$METHOD" | tee "$LOG"

ROOT_DEV="$(findmnt -n -o SOURCE / | sed -n '1p' || true)"
if [ "$DEV" = "$ROOT_DEV" ]; then
  echo "Refusing to wipe running root device ($DEV). Use --allow-real or set FORCE_REAL=1 if you know what you are doing." | tee -a "$LOG"
  exit 2
fi

IS_LOOP=0
if [[ "$DEV" == /dev/loop* ]]; then IS_LOOP=1; fi

if [[ "$IS_LOOP" -eq 0 && "${FORCE_REAL:-0}" != "1" && "$*" != *"--allow-real"* ]]; then
  echo "Refusing to operate on real device $DEV without FORCE_REAL=1 or --allow-real. Use demo loopback or pass FORCE_REAL=1 intentionally." | tee -a "$LOG"
  exit 3
fi

echo "[*] unmounting partitions for $DEV" | tee -a "$LOG"
for p in $(lsblk -ln -o NAME "${DEV}" 2>/dev/null | tail -n +2); do
  mp=$(lsblk -ln -o MOUNTPOINT "/dev/$p" 2>/dev/null)
  if [ -n "$mp" ]; then
    echo "[*] unmounting /dev/$p -> $mp" | tee -a "$LOG"
    umount -l "/dev/$p" || true
  fi
done

case "$METHOD" in
  overwrite-random)
    echo "[*] Overwrite with random" | tee -a "$LOG"
    dd if=/dev/urandom of="$DEV" bs=4M oflag=direct status=progress conv=fdatasync 2>&1 | tee -a "$LOG"
    ;;
  overwrite-zero)
    echo "[*] Overwrite with zeros" | tee -a "$LOG"
    dd if=/dev/zero of="$DEV" bs=4M oflag=direct status=progress conv=fdatasync 2>&1 | tee -a "$LOG"
    ;;
  wipefs-zap)
    echo "[*] wipefs + sgdisk zap all" | tee -a "$LOG"
    wipefs -a "$DEV" 2>&1 | tee -a "$LOG" || true
    sgdisk --zap-all "$DEV" 2>&1 | tee -a "$LOG" || true
    blkdiscard "$DEV" 2>&1 | tee -a "$LOG" || true
    SECS=$(blockdev --getsz "$DEV" 2>/dev/null || echo 0)
    if [ "$SECS" -gt 4096 ]; then
      dd if=/dev/zero of="$DEV" bs=512 count=2048 conv=fdatasync status=none || true
      BACKUP_START=$((SECS - 2048))
      dd if=/dev/zero of="$DEV" bs=512 seek=$BACKUP_START count=2048 conv=fdatasync status=none || true
      partprobe "$DEV" 2>/dev/null || true
    fi
    ;;
  nvme-format)
    echo "[*] nvme format" | tee -a "$LOG"
    nvme format "$DEV" --ses=1 2>&1 | tee -a "$LOG"
    ;;
  ata-secure-erase)
    echo "[*] ATA secure erase" | tee -a "$LOG"
    hdparm --user-master u --security-set-pass P "$DEV" 2>&1 | tee -a "$LOG"
    hdparm --security-erase P "$DEV" 2>&1 | tee -a "$LOG"
    ;;
  *)
    echo "Unknown method: $METHOD" | tee -a "$LOG"
    exit 4
    ;;
esac

sync
echo "[*] done. log: $LOG" | tee -a "$LOG"

if command -v bash >/dev/null 2>&1; then
  echo "[*] creating attestation..."
  bash "$(dirname "$(realpath "$0")")/attest.sh" "$DEV" "$METHOD" "$LOG" || echo "[!] attestation failed; check scripts/attest.sh" | tee -a "$LOG"
else
  echo "[!] attest.sh not run (bash missing?)" | tee -a "$LOG"
fi

exit 0

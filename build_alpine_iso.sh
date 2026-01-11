#!/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Try: sudo ./build.sh"
  exit 1
fi

if [ -z "$SUDO_USER" ] || [ "$SUDO_USER" = "root" ]; then
  echo "ERROR: Run as normal user with sudo, not as root directly."
  exit 1
fi

ISO_NAME="sentinel-live-$(date +%Y%m%d-%H%M%S).iso"

SCRIPT_SOURCE_DIR=$(eval echo ~"$SUDO_USER")/Work/minimal-sentinal
if [ ! -f "${SCRIPT_SOURCE_DIR}/main.cpp" ]; then
  SCRIPT_SOURCE_DIR=$(pwd)
fi
WORK_DIR="${SCRIPT_SOURCE_DIR}/build_temp"

TINYCORE_ISO="TinyCorePure64-15.0.iso"
TINYCORE_URL="http://distro.ibiblio.org/tinycorelinux/15.x/x86_64/release/${TINYCORE_ISO}"

echo "Hope for best my Nigga"

if [ ! -f "${SCRIPT_SOURCE_DIR}/main.cpp" ]; then
  echo "ERROR: main.cpp missing in ${SCRIPT_SOURCE_DIR}"
  exit 1
fi

if [ ! -d "${SCRIPT_SOURCE_DIR}/scripts" ] || [ -z "$(ls -A "${SCRIPT_SOURCE_DIR}/scripts/"*.sh 2>/dev/null)" ]; then
  echo "ERROR: scripts/ folder missing or empty"
  exit 1
fi

for tool in docker wget advdef mkisofs; do
  if ! command -v $tool &>/dev/null; then
    echo "Missing tool: $tool"
    if [ "$tool" = "mkisofs" ]; then
      echo "Something Fucked: sudo apt install genisoimage"
    elif [ "$tool" = "advdef" ]; then
      echo "Something Fucked: sudo apt install advancecomp"
    fi
    exit 1
  fi
done

if ! command -v 7z &>/dev/null && ! command -v bsdtar &>/dev/null && ! command -v xorriso &>/dev/null; then
  echo "ERROR: Need one of: 7z, bsdtar, or xorriso for ISO extraction"
  echo "Something Fucked: sudo apt install p7zip-full"
  exit 1
fi

if ! sudo -u "$SUDO_USER" docker info >/dev/null 2>&1; then
  echo "ERROR: Docker not running or user not in docker group"
  exit 1
fi

echo "[1/6] Setting up workspace..."
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"/{iso,extract,newiso/boot}
chmod -R 777 "$WORK_DIR"
cd "$WORK_DIR"


echo "[2/6] Building sentinel-gui..."
sudo -u "$SUDO_USER" docker run --rm \
  -v "${SCRIPT_SOURCE_DIR}":/src:ro \
  -v "${WORK_DIR}":/out \
  -w /build \
  alpine:latest sh -c "
    apk add --no-cache g++ ncurses-dev ncurses-static
    g++ -std=c++17 -static -o /build/sentinel-gui /src/main.cpp -lncursesw
    cp /build/sentinel-gui /out/sentinel-gui
    chmod 755 /out/sentinel-gui
"
if [ ! -f "${WORK_DIR}/sentinel-gui" ]; then
  echo "ERROR: Binary compilation failed"
  exit 1
fi
chown "$SUDO_USER:$(id -g "$SUDO_USER")" "${WORK_DIR}/sentinel-gui"
echo "[*] Binary size: $(du -h ${WORK_DIR}/sentinel-gui | cut -f1)"

echo "[3/6] Downloading TinyCore Linux..."
if [ -f "/tmp/${TINYCORE_ISO}" ] && [ ! -s "/tmp/${TINYCORE_ISO}" ]; then
  echo "[*] Removing empty cached file..."
  rm -f "/tmp/${TINYCORE_ISO}"
fi
if [ ! -f "/tmp/${TINYCORE_ISO}" ]; then
  echo "[*] Downloading from: $TINYCORE_URL"
  if ! wget --tries=3 --timeout=30 --show-progress "$TINYCORE_URL" -O "/tmp/${TINYCORE_ISO}"; then
    echo "ERROR: Download failed"
    rm -f "/tmp/${TINYCORE_ISO}"
    exit 1
  fi
  if [ ! -s "/tmp/${TINYCORE_ISO}" ]; then
    echo "ERROR: Downloaded file is empty"
    rm -f "/tmp/${TINYCORE_ISO}"
    exit 1
  fi
  SIZE=$(stat -c%s "/tmp/${TINYCORE_ISO}")
  if [ "$SIZE" -lt 10000000 ]; then
    echo "ERROR: Wrong file Downloaded (size: $SIZE bytes)"
    rm -f "/tmp/${TINYCORE_ISO}"
    exit 1
  fi
else
  echo "[*] Using cached ISO"
fi
if ! file "/tmp/${TINYCORE_ISO}" | grep -q "ISO 9660"; then
  echo "ERROR: Downloaded file is not a valid ISO"
  echo "File type: $(file /tmp/${TINYCORE_ISO})"
  echo "Removing corrupted download..."
  rm -f "/tmp/${TINYCORE_ISO}"
  exit 1
fi
echo "[*] ISO verified: $(du -h /tmp/${TINYCORE_ISO} | cut -f1)"

echo "[4/6] Extracting ISO..."
EXTRACTED=false
# Method 1: xorriso
if command -v xorriso &>/dev/null && [ "$EXTRACTED" = "false" ]; then
  echo "[*] Extracting with xorriso..."
  if xorriso -osirrox on -indev "/tmp/${TINYCORE_ISO}" -extract / "${WORK_DIR}/newiso" 2>&1 | grep -q "nodes read"; then
    EXTRACTED=true
  fi
fi
# Method 2: 7z
if command -v 7z &>/dev/null && [ "$EXTRACTED" = "false" ]; then
  echo "[*] Extracting with 7z..."
  if 7z x -o"${WORK_DIR}/newiso" "/tmp/${TINYCORE_ISO}" -y >/dev/null 2>&1; then
    EXTRACTED=true
  fi
fi
# Method 3: bsdtar
if command -v bsdtar &>/dev/null && [ "$EXTRACTED" = "false" ]; then
  echo "[*] Extracting with bsdtar..."
  if bsdtar -xf "/tmp/${TINYCORE_ISO}" -C "${WORK_DIR}/newiso" 2>/dev/null; then
    EXTRACTED=true
  fi
fi
if [ "$EXTRACTED" = "false" ]; then
  echo "ERROR: All extraction methods failed"
  echo "The ISO may be corrupted. Try: rm /tmp/${TINYCORE_ISO}"
  exit 1
fi
if [ ! -f "${WORK_DIR}/newiso/boot/core.gz" ]; then
  echo "ERROR: core.gz not found after extraction"
  echo "Contents of newiso/boot:"
  ls -la "${WORK_DIR}/newiso/boot/" 2>/dev/null || echo "boot directory doesn't exist"
  exit 1
fi

echo "[5/6] Customizing TinyCore rootfs..."
cd "${WORK_DIR}/extract"
CORE_FILE=$(find "${WORK_DIR}/newiso/boot" -name "core*.gz" -o -name "corepure*.gz" | head -1)
if [ -z "$CORE_FILE" ]; then
  echo "ERROR: Cannot find core.gz or similar"
  echo "Boot directory contents:"
  ls -la "${WORK_DIR}/newiso/boot/"
  exit 1
fi
echo "[*] Extracting: $(basename $CORE_FILE)"
zcat "$CORE_FILE" | cpio -idm
mkdir -p opt/sentinel/{bin,scripts}
cp "${WORK_DIR}/sentinel-gui" opt/sentinel/bin/
cp "${SCRIPT_SOURCE_DIR}/scripts/"*.sh opt/sentinel/scripts/
chmod +x opt/sentinel/bin/* opt/sentinel/scripts/*

#Auto start script
mkdir -p home/tc/.local/bin
cat > home/tc/.local/bin/sentinel-start.sh <<'AUTOSTART'
#!/bin/sh
clear
cat <<'BANNER'
╔════════════════════════════════════════════════╗
║         Sentinel Live USB System               ║
║     Secure Device Wipe & Attestation           ║
╚════════════════════════════════════════════════╝

Starting Sentinel GUI...
BANNER
sleep 1
exec /opt/sentinel/bin/sentinel-gui
AUTOSTART
chmod +x home/tc/.local/bin/sentinel-start.sh

cat >> home/tc/.profile <<'PROFILE'

# Auto-start Sentinel GUI
if [ "$(tty)" = "/dev/tty1" ]; then
  /home/tc/.local/bin/sentinel-start.sh
fi
PROFILE

cat > opt/bootlocal.sh <<'BOOTLOCAL'
#!/bin/sh
# Sentinel boot setup
chown -R tc:staff /home/tc
chmod +x /opt/sentinel/bin/*
chmod +x /opt/sentinel/scripts/*
BOOTLOCAL
chmod +x opt/bootlocal.sh

echo "[*] Repacking rootfs..."
find . | cpio -o -H newc | gzip -9 > "$CORE_FILE"

# Optimize compression
advdef -z4 "$CORE_FILE"

CORE_SIZE=$(du -h "$CORE_FILE" | cut -f1)
echo "[*] New rootfs size: ${CORE_SIZE}"

echo "[*] Updating boot menu..."

KERNEL_FILE=$(ls "${WORK_DIR}/newiso/boot/" | grep -E "^vmlinuz" | head -1)
if [ -z "$KERNEL_FILE" ]; then
  echo "ERROR: No kernel found in boot directory"
  ls -la "${WORK_DIR}/newiso/boot/"
  exit 1
fi

INITRD_FILE=$(ls "${WORK_DIR}/newiso/boot/" | grep -E "^core" | head -1)
if [ -z "$INITRD_FILE" ]; then
  echo "ERROR: No initrd found in boot directory"
  ls -la "${WORK_DIR}/newiso/boot/"
  exit 1
fi

echo "[*] Using kernel: ${KERNEL_FILE}, initrd: ${INITRD_FILE}"

if [ -f "${WORK_DIR}/newiso/boot/isolinux/isolinux.cfg" ]; then
  cat > "${WORK_DIR}/newiso/boot/isolinux/isolinux.cfg" <<ISOLINUX
default sentinel
label sentinel
  kernel /boot/${KERNEL_FILE}
  initrd /boot/${INITRD_FILE}
  append quiet loglevel=3

timeout 30
prompt 0
ISOLINUX
  echo "[*] BIOS boot configured"
fi

if [ -f "${WORK_DIR}/newiso/boot/grub/grub.cfg" ]; then
  cat > "${WORK_DIR}/newiso/boot/grub/grub.cfg" <<GRUBCFG
set timeout=3
set default=0

menuentry "Sentinel Live System" {
  linux /boot/${KERNEL_FILE} quiet loglevel=3
  initrd /boot/${INITRD_FILE}
}
GRUBCFG
  echo "[*] EFI boot configured"
fi

echo "[6/6] Building final ISO..."

cd "${WORK_DIR}"

mkisofs -l -J -R -V "SENTINEL_LIVE" \
  -no-emul-boot -boot-load-size 4 -boot-info-table \
  -b boot/isolinux/isolinux.bin \
  -c boot/isolinux/boot.cat \
  -o "${SCRIPT_SOURCE_DIR}/${ISO_NAME}" \
  newiso/ 2>&1 | grep -v "^Warning: creating filesystem"

if command -v isohybrid &>/dev/null; then
  isohybrid "${SCRIPT_SOURCE_DIR}/${ISO_NAME}" 2>/dev/null || true
fi

cd "${SCRIPT_SOURCE_DIR}"
#rm -rf "$WORK_DIR"

if [ -f "${ISO_NAME}" ]; then
  chown "$SUDO_USER:$(id -g "$SUDO_USER")" "${ISO_NAME}"
  ISO_SIZE=$(du -h "${ISO_NAME}" | cut -f1)
  
  echo "  ISO BUILD COMPLETE"
  echo "  ISO:  ${ISO_NAME}"
  echo "  Size: ${ISO_SIZE}"
  echo "  Core: ${CORE_SIZE}"
else
  echo "ERROR: ISO was not created!"
  exit 1
fi
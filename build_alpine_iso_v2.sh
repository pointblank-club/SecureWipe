#!/bin/bash

set -e

if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Must be run with sudo. Try: sudo ./build.sh"
  exit 1
fi

if [ -z "$SUDO_USER" ] || [ "$SUDO_USER" = "root" ]; then
  echo "ERROR: \$SUDO_USER not set or is 'root'. Run as normal user with sudo."
  exit 1
fi

ALPINE_VERSION="3.20"
ALPINE_RELEASE="3.20.0"
ISO_NAME="sentinel-live-$(date +%Y%m%d-%H%M%S).iso"

SCRIPT_SOURCE_DIR=$(eval echo ~"$SUDO_USER")/Work/minimal-sentinal
if [ ! -f "${SCRIPT_SOURCE_DIR}/main.cpp" ]; then
  SCRIPT_SOURCE_DIR=$(pwd)
fi
WORK_DIR="${SCRIPT_SOURCE_DIR}/build_temp"

ALPINE_PACKAGES="alpine-base build-base ncurses-dev ncurses-static libstdc++ bash util-linux nvme-cli hdparm openssl coreutils ncurses-libs jq android-tools"

echo "════════════════════════════════════════════════"
echo " Sentinel Alpine ISO Builder (v5.3 - OVERLAY FIX)"
echo "════════════════════════════════════════════════"
echo ""

if [ ! -f "${SCRIPT_SOURCE_DIR}/main.cpp" ]; then
  echo "ERROR: main.cpp missing."
  exit 1
fi
if [ ! -d "${SCRIPT_SOURCE_DIR}/scripts" ] || [ -z "$(ls -A "${SCRIPT_SOURCE_DIR}/scripts/"*.sh 2>/dev/null)" ]; then
  echo "ERROR: scripts/ folder missing or empty."
  exit 1
fi

for tool in docker wget xorriso tar; do
  if ! command -v $tool &>/dev/null; then
    echo "Missing tool: $tool"
    exit 1
  fi
done

if ! sudo -u "$SUDO_USER" docker info >/dev/null 2>&1; then
  echo "ERROR: Docker not running or user not in docker group."
  exit 1
fi

echo "[2/8] Setting up workspace..."
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"/{iso_extract,overlay}
chmod -R 777 "$WORK_DIR"
cd "$WORK_DIR"

echo "[3/8] Building mini-rootfs in Docker..."
MINI_ROOTFS_TAR="mini-rootfs.tar.gz"

sudo -u "$SUDO_USER" docker run --rm \
  -v "${SCRIPT_SOURCE_DIR}":/src \
  -v "${WORK_DIR}":/export \
  -e HOST_UID=$(id -u "$SUDO_USER") \
  -e HOST_GID=$(id -g "$SUDO_USER") \
  alpine:$ALPINE_VERSION sh -c "
  set -e
  apk add --no-cache $ALPINE_PACKAGES

  g++ -std=c++17 -static -o /usr/local/bin/sentinel-gui \
      /src/main.cpp -lncursesw -lstdc++fs

  mkdir -p /usr/local/share/sentinel/scripts
  cp /src/scripts/*.sh /usr/local/share/sentinel/scripts/
  chmod +x /usr/local/share/sentinel/scripts/*.sh

  cat > /etc/inittab <<'INITTAB'
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
tty1::respawn:/sbin/agetty --autologin root --noclear tty1
tty2::respawn:/sbin/agetty 38400 tty2
::ctrlaltdel:/sbin/reboot
::powerfail:/sbin/poweroff
INITTAB

  mkdir -p /etc/profile.d
  cat > /etc/profile.d/sentinel-start.sh <<'AUTOSTART'
#!/bin/sh
clear
cat <<'BANNER'
╔════════════════════════════════════════════════╗
║         Sentinel Live USB System              ║
║     Secure Device Wipe & Attestation          ║
╚════════════════════════════════════════════════╝
System: Alpine Linux (Offline Boot)
Tools:  All tools pre-installed.
Boot:   Loading overlay...

Auto-starting Sentinel GUI...
BANNER
sleep 1
exec /usr/local/bin/sentinel-gui
AUTOSTART
  chmod +x /etc/profile.d/sentinel-start.sh

  tar -czf /export/${MINI_ROOTFS_TAR} -C / \
    --exclude=./proc --exclude=./sys --exclude=./dev \
    --exclude=./tmp --exclude=./run --exclude=./src --exclude=./export .

  chown \$HOST_UID:\$HOST_GID /export/${MINI_ROOTFS_TAR}
"

if [ ! -f "${WORK_DIR}/${MINI_ROOTFS_TAR}" ]; then
  echo "ERROR: Docker build failed."
  exit 1
fi

ALPINE_ISO="alpine-standard-${ALPINE_RELEASE}-x86_64.iso"
ALPINE_URL="https://dl-cdn.alpinelinux.org/alpine/v${ALPINE_VERSION}/releases/x86_64/${ALPINE_ISO}"
echo "[4/8] Downloading Alpine base..."
if [ ! -f "/tmp/$ALPINE_ISO" ]; then
  wget -q --show-progress "$ALPINE_URL" -O "/tmp/$ALPINE_ISO"
fi

echo "[5/8] Extracting ISO..."
xorriso -osirrox on -indev "/tmp/$ALPINE_ISO" -extract / iso_extract/

echo "[6/8] Creating overlay..."

rm -rf "${WORK_DIR}/overlay"
mkdir -p "${WORK_DIR}/overlay"
tar -xzf "${WORK_DIR}/${MINI_ROOTFS_TAR}" -C "${WORK_DIR}/overlay"


echo "[*] Creating apkovl tarball with correct structure..."
(
  cd "${WORK_DIR}/overlay" || exit 1
  
  tar --numeric-owner -czf "${WORK_DIR}/iso_extract/sentinel.apkovl.tar.gz" \
    --exclude='./proc/*' \
    --exclude='./sys/*' \
    --exclude='./dev/*' \
    --exclude='./tmp/*' \
    --exclude='./run/*' \
    --exclude='./media/*' \
    --exclude='./mnt/*' \
    .
)

if [ ! -f "${WORK_DIR}/iso_extract/sentinel.apkovl.tar.gz" ]; then
  echo "ERROR: Failed to create apkovl!"
  exit 1
fi

OVERLAY_SIZE=$(du -h "${WORK_DIR}/iso_extract/sentinel.apkovl.tar.gz" | cut -f1)
echo "[*] Overlay created: sentinel.apkovl.tar.gz (${OVERLAY_SIZE})"

echo "[*] Overlay contents (first 20 entries):"
tar -tzf "${WORK_DIR}/iso_extract/sentinel.apkovl.tar.gz" | head -20

echo "[7/8] Patching bootloaders..."

GRUB_CFG="${WORK_DIR}/iso_extract/boot/grub/grub.cfg"
SYSLINUX_CFG="${WORK_DIR}/iso_extract/boot/syslinux/syslinux.cfg"

[ -f "$GRUB_CFG" ] && cp -a "$GRUB_CFG" "$GRUB_CFG.orig"
[ -f "$SYSLINUX_CFG" ] && cp -a "$SYSLINUX_CFG" "$SYSLINUX_CFG.orig"

if [ -f "$GRUB_CFG" ]; then
  sed -i -E 's|^([[:space:]]*linux[[:space:]]+/boot/vmlinuz-lts[[:space:]]+.*)$|\1 apkovl=/sentinel.apkovl.tar.gz|' "$GRUB_CFG"
  sed -i 's/timeout=5/timeout=3/' "$GRUB_CFG"
  echo "[*] GRUB config patched"
fi

if [ -f "$SYSLINUX_CFG" ]; then
  sed -i -E 's|^([[:space:]]*APPEND[[:space:]]+.*)$|\1 apkovl=/sentinel.apkovl.tar.gz|' "$SYSLINUX_CFG"
  sed -i 's/TIMEOUT [0-9]*/TIMEOUT 30/' "$SYSLINUX_CFG"
  echo "[*] SYSLINUX config patched"
fi

if [ -f "$GRUB_CFG.orig" ]; then
  echo "[*] GRUB changes:"
  diff -u "$GRUB_CFG.orig" "$GRUB_CFG" || true
fi

echo "[8/8] Building final ISO: ${ISO_NAME}..."
xorriso -as mkisofs \
  -o "${SCRIPT_SOURCE_DIR}/${ISO_NAME}" \
  -J -l -R -V "SENTINEL_LIVE" \
  -b boot/syslinux/isolinux.bin \
  -c boot/syslinux/boot.cat \
  -no-emul-boot -boot-load-size 4 -boot-info-table \
  -eltorito-alt-boot \
  -e boot/grub/efi.img \
  -no-emul-boot \
  -isohybrid-mbr iso_extract/boot/syslinux/isohdpfx.bin \
  -isohybrid-gpt-basdat \
  iso_extract/ 2>&1 | grep -v "NOTE"

echo ""
echo "[*] Verifying overlay in ISO..."
if xorriso -indev "${SCRIPT_SOURCE_DIR}/${ISO_NAME}" -find / -name "sentinel.apkovl.tar.gz" 2>/dev/null | grep -q sentinel; then
  echo "    ✓ Overlay file found in ISO"
else
  echo "    ✗ WARNING: Overlay file NOT found in ISO!"
fi

cd "$SCRIPT_SOURCE_DIR"
rm -rf "$WORK_DIR"
chown "$SUDO_USER:$(id -g "$SUDO_USER")" "${SCRIPT_SOURCE_DIR}/${ISO_NAME}"

ISO_SIZE=$(du -h "${SCRIPT_SOURCE_DIR}/${ISO_NAME}" | cut -f1)

echo ""
echo "════════════════════════════════════════════════"
echo "  ✓ ISO BUILD COMPLETE — Sentinel v5.3"
echo "════════════════════════════════════════════════"
echo "  ISO:  ${ISO_NAME}"
echo "  Size: ${ISO_SIZE}"
echo "  Overlay: ${OVERLAY_SIZE}"
echo "════════════════════════════════════════════════"
echo "  Ready to flash and boot."
echo "  Should auto-login as root and launch GUI."
echo ""
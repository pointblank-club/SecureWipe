#!/usr/bin/env bash
set -euo pipefail


if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Run with sudo: sudo ./build.sh"
  exit 1
fi

if [ -z "${SUDO_USER:-}" ] || [ "$SUDO_USER" = "root" ]; then
  echo "ERROR: Run as normal user via sudo, not root directly."
  exit 1
fi

SCRIPT_SOURCE_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK_DIR="${SCRIPT_SOURCE_DIR}/build_iso_tmp"

ISO_NAME="sentinel-live-$(date +%Y%m%d-%H%M%S).iso"

TINYCORE_ISO="TinyCorePure64-15.0.iso"
TINYCORE_URL="http://distro.ibiblio.org/tinycorelinux/15.x/x86_64/release/${TINYCORE_ISO}"


for tool in docker wget mkisofs advdef xorriso; do
  command -v "$tool" >/dev/null || {
    echo "ERROR: Missing tool: $tool"
    exit 1
  }
done


echo "[1/6] Preparing workspace..."
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"/{extract,newiso/boot}
chmod -R 777 "$WORK_DIR"


echo "[2/6] Building Sentinel (glibc) and extracting runtime..."

sudo -u "$SUDO_USER" docker run --rm \
  -v "${SCRIPT_SOURCE_DIR}":/src \
  -v "${WORK_DIR}":/out \
  -w /src \
  debian:bookworm-slim sh -c "
    apt-get update
    apt-get install -y \
      build-essential \
      libncursesw5-dev libncurses-dev \
      linux-headers-amd64

    make clean
    make

    cp bin/sentinel /out/sentinel

    mkdir -p /out/runtime/lib64 /out/runtime/lib

    # glibc loader + libc
    cp /lib64/ld-linux-x86-64.so.2 /out/runtime/lib64/
    cp /lib/x86_64-linux-gnu/libc.so.6 /out/runtime/lib/
    cp /lib/x86_64-linux-gnu/libpthread.so.0 /out/runtime/lib/

    # ncurses runtime (THIS WAS MISSING)
    cp /lib/x86_64-linux-gnu/libncurses.so.6 /out/runtime/lib/
    cp /lib/x86_64-linux-gnu/libtinfo.so.6 /out/runtime/lib/
"

chmod 755 "$WORK_DIR/sentinel"


echo "[3/6] Downloading TinyCore..."

if [ ! -f "/tmp/${TINYCORE_ISO}" ]; then
  wget --tries=3 --timeout=30 "$TINYCORE_URL" -O "/tmp/${TINYCORE_ISO}"
fi


echo "[4/6] Extracting ISO..."

xorriso -osirrox on -indev "/tmp/${TINYCORE_ISO}" \
  -extract / "${WORK_DIR}/newiso"

echo "[5/6] Injecting Sentinel and runtime into rootfs..."

cd "$WORK_DIR/extract"

CORE_FILE="$(find "$WORK_DIR/newiso/boot" -name 'core*.gz' | head -1)"
zcat "$CORE_FILE" | cpio -idm

# Sentinel
mkdir -p opt/sentinel/bin opt/sentinel/scripts
cp "$WORK_DIR/sentinel" opt/sentinel/bin/sentinel
chmod +x opt/sentinel/bin/sentinel

# Runtime (glibc + ncurses)
mkdir -p lib64 lib/x86_64-linux-gnu
cp "$WORK_DIR/runtime/lib64/"* lib64/
cp "$WORK_DIR/runtime/lib/"* lib/x86_64-linux-gnu/

# Autostart
mkdir -p home/tc/.local/bin
cat > home/tc/.local/bin/sentinel-start.sh <<'EOF'
#!/bin/sh
clear
echo "Starting Sentinel..."
exec /opt/sentinel/bin/sentinel
EOF
chmod +x home/tc/.local/bin/sentinel-start.sh

cat >> home/tc/.profile <<'EOF'
if [ "$(tty)" = "/dev/tty1" ]; then
  /home/tc/.local/bin/sentinel-start.sh
fi
EOF

# Repack rootfs
find . | cpio -o -H newc | gzip -9 > "$CORE_FILE"
advdef -z4 "$CORE_FILE"

echo "[6/6] Building ISO..."

cd "$WORK_DIR"

mkisofs -l -J -R -V "SENTINEL_LIVE" \
  -b boot/isolinux/isolinux.bin \
  -c boot/isolinux/boot.cat \
  -no-emul-boot -boot-load-size 4 -boot-info-table \
  -o "${SCRIPT_SOURCE_DIR}/${ISO_NAME}" \
  newiso/

echo
echo "====================================="
echo " ISO BUILD COMPLETE"
echo " File: ${ISO_NAME}"
echo "====================================="

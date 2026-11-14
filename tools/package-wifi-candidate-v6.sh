#!/bin/bash
# Build the Wi-Fi candidate v6 for topaz (SM6225 / tapas).
#
# v6 fixes:
#   - Robust WLAN interface discovery in init (does not depend on ARPHRD type 802)
#   - Fix ConfigFS mount for USB serial console gadget (ttyGS0)
#   - Fix /run/wpa_supplicant directory creation for wpa_cli control socket
#   - Add firmware fallback symlinks for wlanmdsp.mbn (root /lib/firmware) and board.bin
#   - Start rmtfs with verbose flag (-v) for detailed EFS diagnostic
#   - Mirror logs to /tmp/meow-wifi.log and open interactive shells
#   - Keeps all v4 DT fixes (WLAN rails always-on + clocks)
#
# Stable images and v5 are NOT touched.
set -euo pipefail
ROOT="/home/satanas/linux-topaz"
MEOW="/home/satanas/meow-os"
KERNEL="$HOME/Documents/PostmarketOS-redmi-note-12-4g/work/kernel"
cd "$ROOT"

DTB="$ROOT/sm6225-xiaomi-tapas-wifi-v4.dtb"
KERNEL_IMG="$ROOT/kernel-wifi-Image-v4"
INITRD="$ROOT/rd-wifi-v6/initrd-wifi-v6.cpio.gz"
PAYLOAD="$ROOT/linux-payload-wifi-v6.bin"
CANDIDATE="$ROOT/meow-linux-wifi-v6.img"

# 1. kernel: reuse pristine v4 kernel
[ -f "$KERNEL_IMG" ] || { echo "missing $KERNEL_IMG" >&2; exit 1; }

# 2. DTB: the v4 wifi DT (rails always-on + clocks).
[ -f "$DTB" ] || { echo "missing $DTB" >&2; exit 1; }

# 3. initramfs
[ -f "$INITRD" ] || { echo "missing $INITRD" >&2; exit 1; }

# 4. payload
python3 "$MEOW/tools/mklinux.py" \
	"$KERNEL_IMG" "$INITRD" "$PAYLOAD" \
	--dtb "$DTB" --dtb-pa 0x94000000 \
	--cmdline 'console=tty0 console=ttyGS0 panic=30 pd_ignore_unused clk_ignore_unused' \
	--kernel-pa 0x85000000 --initrd-pa 0x90000000

# 5. boot image through the MEOW shim
(cd "$MEOW/shim" && \
	MKBOOTIMG="python3 /home/satanas/mkbootimg_src/mkbootimg.py" \
	bash build.sh "$PAYLOAD")
cp "$MEOW/shim/out/shim-boot.img" "$CANDIDATE"

echo "=========================================================="
echo "Candidate v6 successfully built: $CANDIDATE"
md5sum "$CANDIDATE"
echo "=========================================================="


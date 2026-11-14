#!/bin/bash
# Build the Wi-Fi candidate v5 for topaz (SM6225 / tapas).
#
# v5 = v4 (DT fix: WLAN rails always-on + `clocks` instead of `clock`) plus:
#   - /etc/wpa_supplicant-network.conf  (test SSID, PSK stored as PBKDF2 hash)
#   - dropbear (SSH server) + libz, /etc/passwd, authorized_keys (key auth only)
#   - the init starts wpa_supplicant (nl80211), waits for COMPLETED, runs
#     udhcpc, prints the address, then starts dropbear on 0.0.0.0:22.
#
# Nothing here touches the stable image or the stable initramfs.
set -euo pipefail
ROOT="/home/satanas/linux-topaz"
MEOW="/home/satanas/meow-os"
KERNEL="$HOME/Documents/PostmarketOS-redmi-note-12-4g/work/kernel"
cd "$ROOT"

DTB="$ROOT/sm6225-xiaomi-tapas-wifi-v4.dtb"
KERNEL_IMG="$ROOT/kernel-wifi-Image-v4"
INITRD="$ROOT/rd-wifi-v5a/initrd-wifi-v5a.cpio.gz"
PAYLOAD="$ROOT/linux-payload-wifi-v5a.bin"
CANDIDATE="$ROOT/meow-linux-wifi-v5a.img"

# 1. kernel: reuse the pristine v4 build unless the source is newer.
if [ ! -f "$KERNEL_IMG" ] || [ -n "$(find "$KERNEL/drivers/net/wireless/ath/ath10k" -newer "$KERNEL_IMG" -print -quit)" ]; then
	make -C "$KERNEL" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image
	install -m 644 "$KERNEL/arch/arm64/boot/Image" "$KERNEL_IMG"
fi

# 2. DTB: the v4 wifi DT (rails always-on + `clocks`).
[ -f "$DTB" ] || { echo "missing $DTB" >&2; exit 1; }

# 3. initramfs
[ -f "$INITRD" ] || { echo "missing $INITRD" >&2; exit 1; }

# 4. payload
python3 "$MEOW/tools/mklinux.py" \
	"$KERNEL_IMG" "$INITRD" "$PAYLOAD" \
	--dtb "$DTB" --dtb-pa 0x94000000 \
	--cmdline 'console=tty0 console=ttyGS0 panic=20 pd_ignore_unused clk_ignore_unused' \
	--kernel-pa 0x85000000 --initrd-pa 0x90000000

# 5. boot image through the MEOW shim
(cd "$MEOW/shim" && \
	MKBOOTIMG="python3 /home/satanas/mkbootimg_src/mkbootimg.py" \
	bash build.sh "$PAYLOAD")
cp "$MEOW/shim/out/shim-boot.img" "$CANDIDATE"

echo "candidate image: $CANDIDATE"
md5sum "$CANDIDATE"

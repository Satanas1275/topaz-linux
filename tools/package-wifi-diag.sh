#!/bin/bash
# Diagnostic candidate for topaz (SM6225 / tapas).
#
# Same kernel + initramfs as v5, but:
#   - watchdog@f017000 is status = "disabled" in the DT (so qcom-wdt cannot
#     reboot the phone if the bootloader already started the APSS WDT)
#   - panic=60 instead of panic=20 (a full minute to read the panic on screen)
#
# Use this when the phone reboots on its own and we cannot see why.
set -euo pipefail
ROOT="/home/satanas/linux-topaz"
MEOW="/home/satanas/meow-os"
cd "$ROOT"

DTB="$ROOT/sm6225-xiaomi-tapas-wifi-diag.dtb"
KERNEL_IMG="$ROOT/kernel-wifi-Image-v4"
INITRD="$ROOT/rd-wifi-v5/initrd-wifi-v5.cpio.gz"
PAYLOAD="$ROOT/linux-payload-wifi-diag.bin"
CANDIDATE="$ROOT/meow-linux-wifi-diag.img"

[ -f "$DTB" ]      || { echo "missing $DTB" >&2; exit 1; }
[ -f "$KERNEL_IMG" ] || { echo "missing $KERNEL_IMG" >&2; exit 1; }
[ -f "$INITRD" ]   || { echo "missing $INITRD" >&2; exit 1; }

python3 "$MEOW/tools/mklinux.py" \
	"$KERNEL_IMG" "$INITRD" "$PAYLOAD" \
	--dtb "$DTB" --dtb-pa 0x94000000 \
	--cmdline 'console=tty0 console=ttyGS0 panic=60 pd_ignore_unused clk_ignore_unused' \
	--kernel-pa 0x85000000 --initrd-pa 0x90000000

(cd "$MEOW/shim" && \
	MKBOOTIMG="python3 /home/satanas/mkbootimg_src/mkbootimg.py" \
	bash build.sh "$PAYLOAD")
cp "$MEOW/shim/out/shim-boot.img" "$CANDIDATE"

echo "diagnostic image: $CANDIDATE"
md5sum "$CANDIDATE"

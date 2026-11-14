#!/bin/bash
# Build the Wi-Fi candidate v4 for topaz (SM6225 / tapas).
#
# v4 = pristine ath10k_snoc (the opt-in "skip WLFW_CAP_REQ" patch is NOT applied)
#      + a DT fix: the WLAN rails are kept enabled at boot and the wifi node's
#      clock property uses the name the clock framework actually parses.
#
# Root cause addressed: the modem's WLAN PD (wlanmdsp.mbn) runs its CCPM clock
# init while the PMIC rails that feed the WLAN chip are still off, so the WLFW
# service answers WLFW_CAP_REQ with QMI_ERR_PLAT_CCPM_CLK_INIT_FAILED (0x77 =
# 119, "RF card not present").  The vendor icnss driver powers the WLAN on
# before the QMI sequence; ath10k_snoc only powers it on after FW_READY, which
# is too late.  Skipping CAP (the v3 experiment) only hides the symptom and the
# next step (BDF download) then fails with QMI_ERR_INCOMPATIBLE_STATE (90).
#
# Nothing here touches the stable image or the stable initramfs.
set -euo pipefail
ROOT="/home/satanas/linux-topaz"
MEOW="/home/satanas/meow-os"
KERNEL="$HOME/Documents/PostmarketOS-redmi-note-12-4g/work/kernel"
cd "$ROOT"

DTB="$ROOT/sm6225-xiaomi-tapas-wifi-v4.dtb"
KERNEL_IMG="$ROOT/kernel-wifi-Image-v4"
INITRD="$ROOT/rd-wifi-v4/initrd-wifi-v4.cpio.gz"
PAYLOAD="$ROOT/linux-payload-wifi-v4.bin"
CANDIDATE="$ROOT/meow-linux-wifi-v4.img"

# 1. kernel: rebuild if the pristine source is newer than the stored Image
if [ ! -f "$KERNEL_IMG" ] || [ -n "$(find "$KERNEL/drivers/net/wireless/ath/ath10k" -newer "$KERNEL_IMG" -print -quit)" ]; then
	make -C "$KERNEL" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image
	install -m 644 "$KERNEL/arch/arm64/boot/Image" "$KERNEL_IMG"
fi

# 2. DTB: build from the pmOS DTB + the memory patch, then apply the fix.
#    sm6225-xiaomi-tapas-wifi-v4.dtb is generated once from
#    sm6225-xiaomi-tapas-mem.dtb; see tools/notes below for the exact edits.
[ -f "$DTB" ] || { echo "missing $DTB (generate it once with tools/make-wifi-v4-dtb.sh)" >&2; exit 1; }

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

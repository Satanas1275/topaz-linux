#!/bin/bash
set -euo pipefail
ROOT="/home/satanas/linux-topaz"
MEOW="/home/satanas/meow-os"
cd "$ROOT"

DTB="$ROOT/sm6225-xiaomi-tapas-wifi-v3.dtb"
install -m 755 "$ROOT/rd-wifi/init-v3" "$ROOT/rd-wifi-v3/stage/init"
(cd "$ROOT/rd-wifi-v3/stage" && \
	find . | cpio -o -H newc --quiet | gzip -9 > "$ROOT/rd-wifi-v3/initrd-wifi-v3.cpio.gz")
cp "$ROOT/sm6225-xiaomi-tapas-mem.dtb" "$DTB"
fdtput "$DTB" /soc/wifi@c800000 qcom,snoc-capability-skip-quirk
fdtget -p "$DTB" /soc/wifi@c800000 | grep -qx qcom,snoc-capability-skip-quirk

python3 "$MEOW/tools/mklinux.py" \
	"$ROOT/kernel-wifi-Image" \
	"$ROOT/rd-wifi-v3/initrd-wifi-v3.cpio.gz" \
	"$ROOT/linux-payload-wifi-v3.bin" \
	--dtb "$DTB" --dtb-pa 0x94000000 \
	--cmdline 'console=tty0 console=ttyGS0 panic=20 pd_ignore_unused clk_ignore_unused' \
	--kernel-pa 0x85000000 --initrd-pa 0x90000000

(cd "$MEOW/shim" && \
	MKBOOTIMG="python3 /home/satanas/mkbootimg_src/mkbootimg.py" \
	bash build.sh "$ROOT/linux-payload-wifi-v3.bin")
cp "$MEOW/shim/out/shim-boot.img" "$ROOT/meow-linux-wifi-v3.img"
echo "candidate image: $ROOT/meow-linux-wifi-v3.img"

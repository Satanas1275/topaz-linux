#!/bin/bash
set -euo pipefail
ROOT="/home/satanas/linux-topaz"
KERNEL="$HOME/Documents/PostmarketOS-redmi-note-12-4g/work/kernel"
cd "$ROOT"
python3 tools/patch-wifi-qmi.py "$KERNEL"
make -C "$KERNEL" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image
install -m 644 "$KERNEL/arch/arm64/boot/Image" "$ROOT/kernel-wifi-Image"
echo "candidate kernel built at $ROOT/kernel-wifi-Image"

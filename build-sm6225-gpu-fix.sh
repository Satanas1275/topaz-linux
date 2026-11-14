#!/bin/bash
# Build script for SM6225 with GPU hardware acceleration fixes
# Adapté pour ton workflow mklinux.py + shim

set -e

echo "=========================================================="
echo "Building SM6225 Kernel with GPU Hardware Acceleration Fix"
echo "=========================================================="
echo

# Paths
KERNEL_SRC="$HOME/Documents/PostmarketOS-redmi-note-12-4g/work/kernel"
LINUX_TOPAZ="$HOME/linux-topaz"
MEOW_OS="$HOME/meow-os"
MKBOOTIMG="$HOME/mkbootimg_src/mkbootimg.py"

# Check paths
if [ ! -d "$KERNEL_SRC" ]; then
    echo "Error: Kernel source not found at $KERNEL_SRC"
    exit 1
fi

if [ ! -d "$MEOW_OS" ]; then
    echo "Error: meow-os not found at $MEOW_OS"
    exit 1
fi

if [ ! -f "$MKBOOTIMG" ]; then
    echo "Error: mkbootimg.py not found at $MKBOOTIMG"
    exit 1
fi

# Check if initrd exists
INITRD="$LINUX_TOPAZ/initrd.cpio.gz"
if [ ! -f "$INITRD" ]; then
    echo "Warning: $INITRD not found, using initrd-drm.cpio.gz"
    INITRD="$LINUX_TOPAZ/initrd-drm.cpio.gz"
fi

if [ ! -f "$INITRD" ]; then
    echo "Error: No initrd found!"
    exit 1
fi

echo "[1/4] Building kernel with GPU fixes..."
cd "$KERNEL_SRC"
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KCFLAGS=-Wno-error -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Error: Kernel build failed!"
    exit 1
fi

echo
echo "[2/4] Creating Linux payload (kernel + initrd + DTB)..."
cd "$LINUX_TOPAZ"

# Use the updated DTB with GPU fixes
DTB="$KERNEL_SRC/arch/arm64/boot/dts/qcom/sm6225-xiaomi-tapas.dtb"

python3 "$MEOW_OS/tools/mklinux.py" \
    "$KERNEL_SRC/arch/arm64/boot/Image" \
    "$INITRD" \
    linux-payload-gpu-fix.bin \
    --dtb "$DTB" \
    --dtb-pa 0x94000000

if [ $? -ne 0 ]; then
    echo "Error: Payload creation failed!"
    exit 1
fi

echo
echo "[3/4] Building shim + boot image..."
cd "$MEOW_OS/shim"
MKBOOTIMG="python3 $MKBOOTIMG" ./build.sh "$LINUX_TOPAZ/linux-payload-gpu-fix.bin"

if [ $? -ne 0 ]; then
    echo "Error: Shim build failed!"
    exit 1
fi

echo
echo "[4/4] Copying final image..."
cp out/shim-boot.img "$LINUX_TOPAZ/meow-linux-hardware-acceleration1.img"

echo
echo "=========================================================="
echo "SUCCESS!"
echo "=========================================================="
echo
echo "New boot image created:"
echo "  $LINUX_TOPAZ/meow-linux-hardware-acceleration1.img"
echo
echo "Size: $(du -h $LINUX_TOPAZ/meow-linux-hardware-acceleration1.img | cut -f1)"
echo
echo "GPU fixes applied:"
echo "  ✓ pil_gpu_mem region marked as shared-dma-pool"
echo "  ✓ qcom_scm dload-mode configured"
echo "  ✓ DTB with hardware acceleration support"
echo
echo "Test with:"
echo "  fastboot boot ~/linux-topaz/meow-linux-hardware-acceleration1.img"
echo
echo "Or flash permanently:"
echo "  fastboot flash boot ~/linux-topaz/meow-linux-hardware-acceleration1.img"
echo

#!/bin/bash
# Build the Wi-Fi initramfs for the mainline (postmarketOS 7.2.9) SM6225/tapas
# kernel, booted from the MEOW shim.
#
#   rd-wifi/init                 the /init script (busybox sh)
#   rd/bin/busybox               static busybox (already in the repo)
#   firmware tarball             firmware-xiaomi-tapas-1.tar.xz (PMOS)
#   APK mirrors                  musl / iw / libnl / wpa_supplicant (Alpine),
#                                tqftpserv / rmtfs / qrtr-libs (postmarketOS)
#
# Output: rd-wifi/initrd-wifi.cpio.gz
set -euo pipefail
cd "$(dirname "$0")/.."

STAGE="$PWD/rd-wifi/stage"
OUT="$PWD/rd-wifi/initrd-wifi.cpio.gz"
CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/linux-topaz-wifi"
FW_TARBALL="$HOME/Documents/PostmarketOS-redmi-note-12-4g/firmware/firmware-xiaomi-tapas-1.tar.xz"

PMOS_MIRROR="http://mirror.nura.eco/postmarketos/v24.06/aarch64"
ALPINE_MIRROR="https://dl-cdn.alpinelinux.org/alpine/v3.20"

mkdir -p "$CACHE" "$STAGE"

# --- 1. download + unpack the userspace packages -------------------------
fetch_apk() { # <url> <name>
	local url="$1" name="$2"
	[ -s "$CACHE/$name.apk" ] || curl -fsSL -m 120 -o "$CACHE/$name.apk" "$url"
}
fetch_apk "$PMOS_MIRROR/tqftpserv-0_git20240423-r0.apk"           tqftpserv
fetch_apk "$PMOS_MIRROR/rmtfs-0.2_git20220718-r2.apk"              rmtfs
fetch_apk "$PMOS_MIRROR/qrtr-libs-1.1-r0.apk"                      qrtr-libs
fetch_apk "$ALPINE_MIRROR/main/aarch64/musl-1.2.5-r4.apk"          musl
fetch_apk "$ALPINE_MIRROR/main/aarch64/iw-6.9-r0.apk"              iw
fetch_apk "$ALPINE_MIRROR/main/aarch64/libnl3-3.9.0-r1.apk"        libnl3
fetch_apk "$ALPINE_MIRROR/main/aarch64/eudev-libs-3.2.14-r2.apk"   eudev-libs
fetch_apk "$ALPINE_MIRROR/main/aarch64/wpa_supplicant-2.10-r11.apk" wpa_supplicant
fetch_apk "$ALPINE_MIRROR/main/aarch64/libcrypto3-3.3.7-r0.apk"    libcrypto3
fetch_apk "$ALPINE_MIRROR/main/aarch64/libssl3-3.3.7-r0.apk"       libssl3
fetch_apk "$ALPINE_MIRROR/main/aarch64/dbus-libs-1.14.10-r1.apk"   dbus-libs
fetch_apk "$ALPINE_MIRROR/main/aarch64/pcsc-lite-libs-2.2.3-r0.apk" pcsc-lite-libs

rm -rf "$CACHE/x"; mkdir -p "$CACHE/x"
for a in "$CACHE"/*.apk; do tar -xzf "$a" -C "$CACHE/x" 2>/dev/null || true; done
X="$CACHE/x"

# --- 2. stage the rootfs -------------------------------------------------
rm -rf "$STAGE"; mkdir -p "$STAGE"/{bin,lib,usr/bin,usr/sbin,usr/lib,etc,proc,sys,dev,tmp,run,var/lib/tqftpserv,usr/share/rmtfs}

install -m 755 rd/bin/busybox "$STAGE/bin/busybox"
install -m 755 rd-wifi/init    "$STAGE/init"

# musl
install -m 755 "$X/lib/ld-musl-aarch64.so.1" "$STAGE/lib/ld-musl-aarch64.so.1"
ln -sf ld-musl-aarch64.so.1 "$STAGE/lib/libc.musl-aarch64.so.1"

# QRTR / modem services
install -m 755 "$X/usr/bin/tqftpserv" "$STAGE/usr/bin/tqftpserv"
install -m 755 "$X/usr/sbin/rmtfs"     "$STAGE/usr/sbin/rmtfs"
install -m 755 "$X/usr/lib/libqrtr.so.1.1" "$STAGE/usr/lib/libqrtr.so.1.1"
ln -sf libqrtr.so.1.1 "$STAGE/usr/lib/libqrtr.so.1"
install -m 755 "$X/lib/libudev.so.1.6.3" "$STAGE/lib/libudev.so.1.6.3"
ln -sf libudev.so.1.6.3 "$STAGE/lib/libudev.so.1"

# WLAN userspace
install -m 755 "$X/usr/sbin/iw" "$STAGE/usr/sbin/iw"
install -m 755 "$X/usr/lib/libnl-3.so.200.26.0"      "$STAGE/usr/lib/libnl-3.so.200.26.0"
install -m 755 "$X/usr/lib/libnl-genl-3.so.200.26.0" "$STAGE/usr/lib/libnl-genl-3.so.200.26.0"
ln -sf libnl-3.so.200.26.0      "$STAGE/usr/lib/libnl-3.so.200"
ln -sf libnl-genl-3.so.200.26.0 "$STAGE/usr/lib/libnl-genl-3.so.200"
install -m 755 "$X/sbin/wpa_supplicant" "$STAGE/usr/sbin/wpa_supplicant"
install -m 755 "$X/sbin/wpa_cli"        "$STAGE/usr/sbin/wpa_cli"
install -m 755 "$X/lib/libcrypto.so.3"   "$STAGE/lib/libcrypto.so.3"
install -m 755 "$X/lib/libssl.so.3"      "$STAGE/lib/libssl.so.3"
install -m 755 "$X/usr/lib/libdbus-1.so.3.32.4" "$STAGE/usr/lib/libdbus-1.so.3.32.4"
ln -sf libdbus-1.so.3.32.4 "$STAGE/usr/lib/libdbus-1.so.3"
install -m 755 "$X/usr/lib/libpcsclite.so.1" "$STAGE/usr/lib/libpcsclite.so.1"

cat > "$STAGE/etc/wpa_supplicant.conf" <<'EOF'
ctrl_interface=/run/wpa_supplicant
ctrl_interface_group=0
update_config=1
EOF

# --- 3. firmware ---------------------------------------------------------
[ -f "$FW_TARBALL" ] || { echo "missing $FW_TARBALL" >&2; exit 1; }
mkdir -p "$CACHE/fw"
tar -xf "$FW_TARBALL" -C "$CACHE/fw"
F="$CACHE/fw"
mkdir -p "$STAGE/lib/firmware/ath10k/WCN3990/hw1.0/xiaomi-tapas"
install -m 644 "$F"/ath10k/WCN3990/hw1.0/xiaomi-tapas/* \
	"$STAGE/lib/firmware/ath10k/WCN3990/hw1.0/xiaomi-tapas/"
mkdir -p "$STAGE/lib/firmware/qcom/sm6225/XIAOMI/TAPAS"
install -m 644 "$F"/qcom/sm6225/XIAOMI/TAPAS/modem.mdt \
	"$F"/qcom/sm6225/XIAOMI/TAPAS/modem.b* \
	"$F"/qcom/sm6225/XIAOMI/TAPAS/wlanmdsp.mbn \
	"$STAGE/lib/firmware/qcom/sm6225/XIAOMI/TAPAS/"

# --- 4. pack -------------------------------------------------------------
(cd "$STAGE" && find . | cpio -o -H newc --quiet | gzip -9 > "$OUT")
echo "$OUT: $(stat -c%s "$OUT") bytes"

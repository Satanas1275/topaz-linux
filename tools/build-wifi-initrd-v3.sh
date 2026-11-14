#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."

STAGE="$PWD/rd-wifi-v3/stage"
OUT="$PWD/rd-wifi-v3/initrd-wifi-v3.cpio.gz"

if [ -e "$STAGE" ]; then
	echo "refusing to overwrite existing candidate stage: $STAGE" >&2
	exit 1
fi
mkdir -p "$PWD/rd-wifi-v3"
cp -a rd-wifi/stage "$STAGE"
install -m 755 rd-wifi/init-v3 "$STAGE/init"

cat > "$STAGE/etc/udhcpc.script" <<'EOF'
#!/bin/busybox sh
case "$1" in
deconfig)
	busybox ip addr flush dev "$interface"
	;;
bound|renew)
	mask=${subnet:-255.255.255.0}
	oldifs=$IFS; IFS=.; set -- $mask; IFS=$oldifs
	prefix=0
	for octet in "$@"; do
		case "$octet" in
		255) prefix=$((prefix + 8));;
		254) prefix=$((prefix + 7)); break;;
		252) prefix=$((prefix + 6)); break;;
		248) prefix=$((prefix + 5)); break;;
		240) prefix=$((prefix + 4)); break;;
		224) prefix=$((prefix + 3)); break;;
		192) prefix=$((prefix + 2)); break;;
		128) prefix=$((prefix + 1)); break;;
		0) break;;
		*) break;;
	esac
	done
	busybox ip addr flush dev "$interface"
	busybox ip addr add "$ip/$prefix" dev "$interface"
	busybox ip link set "$interface" up
	busybox ip route del default 2>/dev/null
	for gateway in $router; do busybox ip route add default via "$gateway" dev "$interface"; break; done
	: > /etc/resolv.conf
	for server in $dns; do echo "nameserver $server" >> /etc/resolv.conf; done
	;;
esac
exit 0
EOF
chmod 755 "$STAGE/etc/udhcpc.script"

if [ -n "${WIFI_SSID:-}" ]; then
	if [ -z "${WIFI_PSK:-}" ]; then
		read -r -s -p "Passphrase for test SSID '$WIFI_SSID': " WIFI_PSK
		echo
	fi
	if [ "${#WIFI_PSK}" -lt 8 ] || [ "${#WIFI_PSK}" -gt 63 ]; then
		echo "WPA2 passphrase must be 8-63 characters" >&2
		exit 1
	fi
	WIFI_SSID="$WIFI_SSID" WIFI_PSK="$WIFI_PSK" WIFI_CONFIG="$STAGE/etc/wpa_supplicant-network.conf" \
		python3 - <<'PY'
import hashlib, os
ssid = os.environ["WIFI_SSID"]
psk = os.environ["WIFI_PSK"]
digest = hashlib.pbkdf2_hmac("sha1", psk.encode(), ssid.encode(), 4096, 32).hex()
with open(os.environ["WIFI_CONFIG"], "w") as f:
    f.write('ctrl_interface=/run/wpa_supplicant\n')
    f.write('network={\n')
    f.write('    ssid="' + ssid.replace('\\', '\\\\').replace('"', '\\"') + '"\n')
    f.write('    key_mgmt=WPA-PSK\n')
    f.write('    psk=' + digest + '\n')
    f.write('    scan_ssid=1\n')
    f.write('}\n')
PY
	unset WIFI_PSK
fi

(cd "$STAGE" && find . | cpio -o -H newc --quiet | gzip -9 > "$OUT")
unset WIFI_PSK 2>/dev/null || true
echo "$OUT: $(stat -c%s "$OUT") bytes"

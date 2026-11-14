#!/usr/bin/env python3
"""Generate sm6225-xiaomi-tapas-wifi-v4.dtb from the pmOS DTB.

Three edits, all opt-in / corrective:

1.  The wifi node's XO clock is declared as `clock = <...>` (singular).  The
    clock framework only parses the `clocks` property (drivers/clk/clk.c,
    of_parse_clkspec()), so ath10k_snoc's devm_clk_bulk_get_optional() never
    finds "cxo_ref_clk_pin" and the XO reference clock is never enabled.
    Rename it to `clocks`.

2.  The WLAN rails (pm6125 l8 / l16 / l17 / l23) are not always-on, so they
    are still off when the modem's WLAN PD (wlanmdsp.mbn) runs its CCPM clock
    init.  The WLFW service then rejects WLFW_CAP_REQ with
    QMI_ERR_PLAT_CCPM_CLK_INIT_FAILED (0x77 = 119).  Keep them enabled at
    boot, like the SDM845 DTS does for its WLAN rails.

3.  Do NOT carry the v3 `qcom,snoc-capability-skip-quirk` (it masks the error
    and makes the following BDF download fail with error 90).

Base DTB: sm6225-xiaomi-tapas-mem.dtb (pmOS DTB with the /memory node patched).
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE = ROOT / "sm6225-xiaomi-tapas-mem.dtb"
OUT = ROOT / "sm6225-xiaomi-tapas-wifi-v4.dtb"

TARGET_REGULATORS = {"l8", "l16", "l17", "l23"}


def main():
    if not BASE.exists():
        sys.exit(f"missing base DTB: {BASE}")
    dts = subprocess.run(["dtc", "-I", "dtb", "-O", "dts", str(BASE)],
                         capture_output=True, text=True, check=True).stdout

    # 1. drop the skip-CAP quirk if a previous candidate added it
    dts = dts.replace("\t\t\tqcom,snoc-capability-skip-quirk;\n", "")

    # 2. `clock` -> `clocks` in the wifi node
    dts = dts.replace("\t\t\tclock = <0x2b 0x10>;\n",
                      "\t\t\tclocks = <0x2b 0x10>;\n")

    # 3. keep the WLAN rails enabled at boot
    lines = dts.splitlines()
    out, i, inserted = [], 0, []
    while i < len(lines):
        line = lines[i]
        m = re.match(r"^(\t+)([A-Za-z0-9_,]+) \{$", line)
        if m and m.group(2) in TARGET_REGULATORS:
            indent = m.group(1)
            j = i + 1
            while j < len(lines) and lines[j] != indent + "};":
                j += 1
            block = lines[i:j + 1]
            if (any("regulator-min-microvolt" in b for b in block)
                    and not any("regulator-always-on" in b for b in block)):
                block.insert(len(block) - 1, indent + "\tregulator-always-on;")
                inserted.append(m.group(2))
            out.extend(block)
            i = j + 1
        else:
            out.append(line)
            i += 1
    dts = "\n".join(out) + "\n"

    if set(inserted) != TARGET_REGULATORS:
        sys.exit(f"expected to patch {sorted(TARGET_REGULATORS)}, patched {inserted}")

    tmp = Path("/tmp/wifi-v4-generated.dts")
    tmp.write_text(dts)
    subprocess.run(["dtc", "-I", "dts", "-O", "dtb", "-o", str(OUT), str(tmp)],
                   check=True)
    print(f"wrote {OUT} ({OUT.stat().st_size} bytes); rails kept on: {inserted}")


if __name__ == "__main__":
    main()

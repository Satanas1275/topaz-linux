#!/usr/bin/env python3
"""Apply an opt-in ath10k QMI capability fallback for topaz WLAN firmware."""
from pathlib import Path
import sys

root = Path(sys.argv[1])

def replace_once(name, old, new):
    path = root / "drivers/net/wireless/ath/ath10k" / name
    source = path.read_text()
    if new in source:
        return
    if old not in source:
        raise SystemExit(f"expected source context not found: {path}")
    path.write_text(source.replace(old, new, 1))

replace_once(
    "snoc.h",
    "\tATH10K_SNOC_FLAG_SKIP_HOST_CAP_QUIRK,\n};",
    "\tATH10K_SNOC_FLAG_SKIP_HOST_CAP_QUIRK,\n"
    "\tATH10K_SNOC_FLAG_SKIP_CAPABILITY_QUIRK,\n};",
)
replace_once(
    "snoc.c",
    "\tif (of_property_read_bool(dev->of_node, \"qcom,snoc-host-cap-skip-quirk\"))\n"
    "\t\tset_bit(ATH10K_SNOC_FLAG_SKIP_HOST_CAP_QUIRK, &ar_snoc->flags);\n}",
    "\tif (of_property_read_bool(dev->of_node, \"qcom,snoc-host-cap-skip-quirk\"))\n"
    "\t\tset_bit(ATH10K_SNOC_FLAG_SKIP_HOST_CAP_QUIRK, &ar_snoc->flags);\n\n"
    "\tif (of_property_read_bool(dev->of_node, \"qcom,snoc-capability-skip-quirk\"))\n"
    "\t\tset_bit(ATH10K_SNOC_FLAG_SKIP_CAPABILITY_QUIRK, &ar_snoc->flags);\n}",
)
replace_once(
    "qmi.c",
    "\tret = ath10k_qmi_cap_send_sync_msg(qmi);\n"
    "\tif (ret)\n"
    "\t\tgoto err_setup_msa;\n\n"
    "\treturn;\n\n"
    "err_setup_msa:",
    "\tif (test_bit(ATH10K_SNOC_FLAG_SKIP_CAPABILITY_QUIRK,\n"
    "\t\t     &ar_snoc->flags)) {\n"
    "\t\t/* topaz firmware rejects WLFW_CAP_REQ (reported error 119).\n"
    "\t\t * Continue with unknown QMI IDs so ath10k can try the\n"
    "\t\t * variant board.bin fallback after firmware-ready.\n"
    "\t\t */\n"
    "\t\tqmi->chip_info.chip_id = 0xff;\n"
    "\t\tqmi->board_info.board_id = 0xff;\n"
    "\t\tath10k_warn(ar, \"skipping rejected WLFW capability request by DT quirk\\n\");\n"
    "\t} else {\n"
    "\t\tret = ath10k_qmi_cap_send_sync_msg(qmi);\n"
    "\t\tif (ret)\n"
    "\t\t\tgoto err_setup_msa;\n"
    "\t}\n\n"
    "\treturn;\n\n"
    "err_setup_msa:",
)
print("patched ath10k: opt-in skip for rejected WLFW capability request")

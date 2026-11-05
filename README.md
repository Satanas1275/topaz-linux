# linux-topaz

**Linux on the Redmi Note 12 4G** (`topaz` / `tapas`, SM6225 "khaje" / Snapdragon 685).

A separate project from `ios-topaz` / `meow-os`: instead of porting Apple's XNU, boot a
Linux kernel — which already has every driver this phone needs.

## Why this is easier than the iOS port

The XNU port had to write drivers from scratch (GIC, UFS, panel, touch, modem). Linux
already supports the SM6225 ("khaje" / "Bengal_515"): MDP/DSI display, FocalTech FT5452
touch, UFS, Adreno 610, Wi-Fi, battery. The work here is packaging and boot, not drivers.

## Two routes

### A. `fastboot boot` a Linux boot image (simplest)

ABL loads the boot image itself; no shim involved.

- Problem: the topaz ABL ignores boot image v1/v2 and drops the v4 boot ramdisk (it only
  passes vendor_boot's). So a Linux boot needs a **vendor_boot** image carrying the DTB
  plus a **boot** image carrying the kernel and the initrd — the same packaging problem
  the ios-topaz shim solves by appending its payload to the kernel Image.

### B. Boot Linux from the MEOW shim (recommended)

The shim already: receives a DTB from ABL, wakes the Gold core, has a working framebuffer,
and loads an arbitrary payload.

- Add a Linux payload loader: kernel Image at a known PA, `x0` = DTB address (the ABL DTB
  or a topaz DTB), jump to the kernel entry — the standard arm64 Linux boot protocol.
- Linux then drives the panel and can draw a framebuffer console.
- Bonus: MEOW OS becomes a bootloader that can start either XNU or Linux.

## What is needed

| Piece | Source |
| --- | --- |
| Kernel | Xiaomi `android_kernel_xiaomi_sm6225-devicetrees` (5.15), or the prebuilt `android_device_xiaomi_topaz-kernel`; mainline `sm6225` is a third option |
| DTB | `android_device_xiaomi_topaz`, or the DTB from the running ROM |
| initrd | busybox + `/init` + a shell |
| Packaging | `mkbootimg.py` (already at `~/mkbootimg_src`) |

## First milestone

1. Get a kernel Image + DTB: extract them from the phone's own `boot` partition, or use
   the prebuilt kernel from GitHub.
2. Build a busybox initrd that prints to the screen and starts a shell.
3. Boot it — route A (`fastboot boot`) or route B (from the shim).
4. Success = a Linux shell on the phone screen.

## Open questions

- **Which kernel**: prebuilt 5.15 (fast, known-good hardware support) or mainline (newer,
  but does mainline carry a topaz DTB?).
- **Console**: the phone has no serial. The vendor kernel's panel driver should give a
  framebuffer console; otherwise a USB gadget serial console.
- **Root filesystem**: initrd only (RAM) at first — `userdata` currently holds the iOS
  HFS+ root, so nothing destructive.

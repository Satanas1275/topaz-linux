# linux-topaz — notes

Goal: Linux (Arch or Debian, arm64) on the Redmi Note 12 4G (`topaz`/`tapas`, SM6225),
with phosh and as much working hardware as possible.

## What I found (2026-11-05)

- **The vendor kernel is available prebuilt**: `xiaomi-6225-AD/android_device_xiaomi_topaz-kernel`
  (branch `lineage-21`) → `kernel.img`, 46 MB, valid arm64 Image, 4K pages, text_offset 0.
  Downloaded to `kernel.img`; its `.config` is in `kernel.config`.
- **The display driver is a module, not built in**: `# CONFIG_DRM_MSM is not set`,
  `# CONFIG_FB is not set`, `# CONFIG_VT is not set`, `# CONFIG_SERIAL_MSM is not set`.
  But `vendor_dlkm/` in the same repo has the modules, including:
  `msm_drm.ko` (display), `msm_kgsl.ko` (Adreno GPU), `dwc3-msm.ko` (USB),
  `msm_geni_serial.ko` (UART), `i2c-msm-geni.ko` (touch bus), `msm_gpi.ko`,
  `panel_event_notifier.ko`, plus audio/charger/camera modules.
  → load them from the initrd and the hardware comes alive.
- **No topaz DTB in the repos.** The device DTS sources *do* exist:
  `android_kernel_xiaomi_sm6225-devicetrees` → `qcom/topaz-khaje-idp-nopmi.dtsi`,
  `.../display/display/topaz-khaje-sde-display-idp-nopmi-overlay.dts`,
  `.../audio/topaz-khaje-audio-idp-nopmi.dts`, and the `tapas-*` equivalents.
  The real device DTB is the one inside the phone's `boot` image.
- **Mainline has no topaz support**: `torvalds/linux` `arch/arm64/boot/dts/qcom` has no
  `sm6225`/`khaje`/`topaz`/`tapas` file. So no mainline shortcut.

## The plan, in stages

1. **Boot a kernel + initrd to a shell.** Use the prebuilt kernel; initrd = busybox +
   the `vendor_dlkm` modules + an `/init` that loads them. Console over USB
   (`dwc3-msm` + `g_serial`, `console=ttyGS0`) because the phone has no serial port.
2. **Bring up the hardware** by loading the modules: display (`msm_drm`), GPU
   (`msm_kgsl`), touch (`i2c-msm-geni` + the FocalTech driver), USB, audio, charger.
3. **A real rootfs**: Arch Linux ARM or Debian arm64, mounted from an initrd or a
   partition (`userdata` still holds the iOS HFS+ root, so initrd first).
4. **phosh**: needs a working DRM/KMS + wlroots + phoc + phosh, plus a mobile
   userspace (NetworkManager, ModemManager, pipewire, bluez).
5. **WiFi / Bluetooth / audio**: firmware blobs + userspace daemons.

## How to boot it

- **Route A** — `fastboot boot` a boot image. The topaz ABL ignores boot image v1/v2 and
  drops the v4 boot ramdisk (it only passes vendor_boot's), so this needs a vendor_boot
  image carrying the DTB plus a boot image with the kernel and initrd.
- **Route B** — load it from the MEOW shim (recommended): the shim already gets the
  phone's real DTB from ABL, wakes the Gold core, and can load an arbitrary payload.
  Add a Linux payload: kernel Image at a known PA, initrd somewhere, `x0` = DTB,
  jump to the kernel entry (the standard arm64 Linux boot protocol).

## First milestone

Linux boots and gives a shell on the phone (over USB serial at first).

## Honest scope

This is a real project, not a weekend: the drivers exist, but the packaging, the console,
the rootfs and the mobile userspace are all ours to do. Steps 1-2 are achievable in this
project; step 4 (phosh) is the long pole.

# FRF EMU68AGA Pi4 R111 — native USB mass storage (read-only)

**Engineering test**, based on verified R110 AGABOOT scanline fixes, with identical Pi4 HVS renderer, 1280x720@50 HDMI and native xHCI keyboard/mouse/gamepad code.

## Function

- Native ARM CPU1 VL805/xHCI, not Pi3 DWC2/USPi; one boot-connected USB MSC device supporting USB Bulk-Only Transport / SCSI transparent command set (class 08, subclass 06, protocol 50).
- Retains a dedicated USB disk xHCI slot and bulk rings while HID stays active, including supported Pi4 internal/external USB2 hub topologies.
- Sends READ CAPACITY(10) and READ(10) up to 16 x 512-byte sectors per request, via the existing FRUM v1 64 KiB virtual Zorro-II mailbox and the matching **read-only** AmigaOS `usbmass.device`.
- All USB commands owned by CPU1; guest sees only the mailbox. SCSI commands/CBW/CSW are checked for proper signatures, exact tag, residue, stage length and success.
- Write attempts are rejected by both Pi4 ARM and AmigaOS drivers. No filesystem formatting, no boot-disk mutations, no arbitrary USB bootloader flashing.
- `POST` version displays **R111**, and the USB count reports one MSC only after 512-byte READ CAPACITY succeeds.

## Boundaries

**Hardware not tested yet.** Cross-build and simulated protocol tests pass; do not claim working AmigaOS disk mounting until actual Pi4 confirms.

R111 supports **one boot-present drive** (LUN 0). Requires USB BOT/SCSI, 512-byte logical blocks, capacity addressable by SCSI READ CAPACITY(10), no hotplug, no write or multi-drive, and no automatic AmigaOS partition mounting. Existing Pi4 USB hub enumeration is reused; unsupported hubs may fail.

A USB drive with an Amiga RDB has LBA0 signature `RDSK`; a PC MBR uses `55 AA`; reading LBA0 is the basic transport test. Recognising a stick does not automatically make FAT32/NTFS readable to AmigaOS. For FAT32, the guest needs a compatible AmigaOS filesystem and mountlist; existing Workbench/AGS support is a separate layer.

## Kernel installation (Pi4 only)

On the FAT boot partition, copy `BOOT/emu68-pi4-r111-usbmass-ro-test.img` to the root. In the **existing `[pi4]` section** of `config.txt` set:

```ini
kernel=emu68-pi4-r111-usbmass-ro-test.img
```

**Keep** your original `hdmi_group=1`, `hdmi_mode=19`, `disable_overscan=1` and `dtoverlay=emmc,unit0=rw` with `overlays/emmc.dtbo` installed. Don't change Pi3/Pi5 settings. Keep the previous R110 kernel on the card for rollback.

## AmigaOS guest-side driver and test

Transfer `DEVS/usbmass.device` (FRUM v1 **RO**, version 0.1) to `DEVS:usbmass.device`. Transfer `C/usbmass-info-pi4` to `C:`. Run:

```
C:usbmass-info-pi4
```

This checks `OpenDevice("usbmass.device",0)`, `TD_GETGEOMETRY`, `CMD_READ` LBA0, and reports `RDSK` or MBR when found. **Do not use the older Pi3 RW usbmass.device** for this R111 RO test. No automatic install into CaffeineOS or RDB partitions is performed by copying to the FAT boot partition.

## Report after Pi4 hardware test

- POST version R111; display scanlines still visible?
- Mouse and keyboard still operate together?
- USB stick connected *before* Pi4 power on; USB POST MSC count?
- `usbmass-info-pi4` geometry, sector 0 result and any `[R111-USBMASS]` log lines.
- Whether the drive is direct connected or through a hub.

## Rollback

In the original `[pi4]` section, restore:

```ini
kernel=emu68-pi4-r110-agaboot-control-only-test.img
```

Do not delete either kernel. This R111 release must stay experimental until Pi4 boot, HID and USB-disk reads are demonstrated on hardware.

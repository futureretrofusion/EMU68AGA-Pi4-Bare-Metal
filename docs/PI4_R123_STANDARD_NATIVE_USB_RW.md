# EMU68AGA Bare Metal Pi4 — R123 standard native USB (non-legacy)

**Experimental test kernel — 14 November 2026.** This is the normal Pi4/400 native USB build, **not** the R122 older-Pi4B rev1.1 diagnostic. It has no persistent POST/overlay.

## R123 build choices

- Native Pi4 VL805 xHCI USB host, including keyboard, mouse, gamepad, USB hubs and one enumerated USB mass-storage disk.
- SCSI READ(10) and WRITE(10) through the FRUM virtual Zorro-II mailbox. **This USB write path has passed software mocks but is not confirmed working on your physical Pi4.**
- **Non-legacy:** `FRF_PI4_LEGACY_REV_DIAG=OFF`. No R117-only old Pi4B firmware recovery/reset workaround. Intended for the newer Pi4B/400 native USB path, not as a fix for a rev1.1 failure.
- Quiet standard startup: `FRF_BOOT_POST=OFF`, `FRF_PI4_SUPPRESS_DIAGNOSTIC_POST=ON`, a brief `FRF PI4 R123 USB RW` splash identifier. Keyboard/mouse source-level telemetry remains compiled for compatibility, but on-screen USB diagnostic overlay is off.
- No changes to the HDMI audio implementation, 1280×720p50 HDMI, integer scaling, native renderer, AGABOOT scanline controls, or P96 handoff.

## Install the Pi4 kernel

On the verified Pi4 EMU68 FAT32 **boot partition**, copy the R123 image to its root:

`emu68-pi4-r123-standard-usb-rw.img`

Back up `config.txt` and change **only** the selected line within `[pi4]`:

```ini
kernel=emu68-pi4-r123-standard-usb-rw.img
```

Do not overwrite the complete `[pi4]` block. Keep `hdmi_group=1`, `hdmi_mode=19`, `hdmi_drive=2`, `dtoverlay=emmc,unit0=rw` and the existing AGABOOT/integer/scanline settings. Older R121 and R117 kernels stay on the FAT volume for rollback.

## Install the matching AmigaOS driver

The Pi4 FAT32 **boot** partition is **not** the AmigaOS `SYS:` filesystem. Merely copying a .device into its `FRF_USB` staging directory does not install it to `SYS:Devs`.

1. On AmigaOS, back up any existing `DEVS:usbmass.device` before copying the package `DEVS/usbmass.device` (internal resident version **0.5**) into `SYS:Devs/usbmass.device`.
2. Make sure FAT95 is already installed at `L:fat95`. Copy the package's `Storage/DOSDrivers/USB0` into `SYS:Storage/DOSDrivers/USB0` for a manual test. Back up any prior USB0 file.
3. **Disable `USBP4` before activating `USB0`**. Two FAT95 mounts on the same physical disk can corrupt data.
4. Power-cycle the Pi4 so a previously loaded `pi4usbmass.device` or older `usbmass.device` cannot stay resident.
5. Check `Version DEVS:usbmass.device FULL` and run `C:frf-usb121-rwcheck` from the package. It should say `OpenDevice=0` and `TD_PROTSTATUS protected=0`.
6. If the correct kernel and driver are present and USB0 is not already mounted, use `Mount USB0: FROM SYS:Storage/DOSDrivers/USB0`. Test reads first.
7. **Only with a backed-up, expendable FAT32 stick**, run `C:frf-usb121-filetest` to write/flush/read back/delete its own 4096-byte file. That test refuses to overwrite an existing `USB0:FRF121.TST`. A successful result must then be checked again after a cold reboot for persistence before declaring USB writing stable.

Don't modify partition tables, format storage, or enable automatic mounts before validation. The R123 firmware does not forcibly disable device write protection; the guest returns `TDERR_WriteProt` unless the ARM kernel advertises `PI4W` and media is present.

## Software verification

- Big-endian ARM64 kernel builds and reports R123; no R117 legacy recovery compile define.
- The SCSI WRITE(10) mock validates CBW/data OUT/CSW, readback, LBA bounds, and fail-closed handling; read-only transport regression also passes.
- Existing USB xHCI topology and AGABOOT/scanline protocol regressions pass.
- The bundled 68k `usbmass.device` v0.5 compiled earlier, and the AmigaDOS 4 KB file-test utility passed against an isolated emulator-mounted volume.
- **The Pi4 user previously reported that R121 remained read-only. This release does not establish that real Pi4 writing is fixed.** It removes the legacy path and packages the correct guest driver with standard native USB enabled.

## Rollback

Restore `kernel=emu68-pi4-r121-usbmass-rw-test.img` in `[pi4]` and the matching guest driver if required. For the proven read-only configuration, R117 with `pi4usbmass.device` v0.4 remains in the earlier R120 package.

## No card changes during publication

The Studio PC USB reader reported a zero-byte/no-card `/dev/sdh`; this package is for installation or copying when the Pi4 card is available. The attached user data and Amiga RDB partition were not modified.

# Future Retro Fusion EMU68AGA Pi4 — R121 native USB mass-storage read/write test

**Experimental, 14 November 2026. Read-only FAT32 mounting and file loading have been confirmed by the user on R117 with the R120 `pi4usbmass.device` v0.4 guest driver. Pi4 hardware writes have NOT yet been confirmed.**

## Purpose

Upgrade the successfully mounted Pi4 USB drive to read/write and restore the familiar AmigaOS driver name **`usbmass.device`**, with volume **`USB0:`**. This needs BOTH a matching R121 ARM-native Pi4 xHCI kernel and the new 68k `usbmass.device` v0.5. Renaming R120's read-only binary is not enough.

### Contents

- `BOOT/emu68-pi4-r121-usbmass-rw-test.img` — big-endian ARM64 native Pi4 kernel, retains R117 older-Pi4B handling, HID, HD720p50, AGABOOT, scanlines and working HDMI audio.
- `DEVS/usbmass.device` — **v0.5** 68k read/write driver, internal resident name is `usbmass.device`, not a renamed `pi4usbmass.device`.
- `Storage/DOSDrivers/USB0` — FAT95 mount with **exact working R120 USBP4 geometry**, changing only the device name to `usbmass.device`. FAT32 first partition auto-selection, `HighCyl=1`, `DosType=0x46415401`.
- `C/frf-usb121-rwcheck` — non-writing check of `OpenDevice`, disk geometry, and `TD_PROTSTATUS`. Must report **protected=0** before any write test.
- `C/usbmass-info-pi4` — non-writing capacity and sector-zero reader.
- `C/usbmass-rw-mountinfo` — non-writing FAT partition structure check.
- `C/frf-usb121-filetest` — file-level 4096-byte write/flush/close/readback/compare/delete using **only `USB0:FRF121.TST`**. REFUSES to overwrite a pre-existing file.
- `C/usbmass-z2-pi4` — FRUM Zorro board diagnostic.
- `BACKUP_RO/emu68-pi4-r117-legacy-rev11-usb-test.img` and `BACKUP_RO/pi4usbmass.device` — tested R117 and R120 read-only fallback copies.
- `SOURCE/` contains the Pi4 ARM driver, 68k source, test sources, and the manifest.

## Write transport / safety

R117's native xHCI `frf_pi4_usb_mass.c` always rejected `FRF_UM_WRITE` with error -30. R121 adds the correct SCSI **WRITE(10)** block path with USB Bulk-Only Transport:

1. The Amiga guest stages up to 16×512-byte sectors into the existing **FRUM v1** mailbox, outside its Forbid/Permit lock.
2. ARM CPU1 copies those bytes to a private 64-byte-aligned DMA buffer and constructs **CBW (31 bytes; direction=OUT; opcode 0x2A)**, followed by **data OUT** and **CSW IN (13 bytes)**.
3. Host validates signature, matching tag, zero residue and success status; only then completes the guest request. The xHCI data buffers get proper cache clean/invalidation.
4. Invalid LBA/range and oversized requests are refused without USB I/O. A failed/ambiguous write permanently quarantines the endpoint until cold boot; **there are no automatic WRITE10 retries**.
5. R121 advertises `PI4W` as a capability tag in the existing `FRUM reserved[1]` field; R117 advertises `PI4R`. The new guest driver's **TD_PROTSTATUS returns protected** and write requests return `TDERR_WriteProt` unless a matching RW kernel is running. This prevents accidental write attempts on an older RO kernel.
6. The old read-only path is unchanged at the protocol level and remains the default when building with `FRF_PI4_USB_MASS_RW=OFF`.

**There are no automatic USB formats, raw disk writes, forced EEPROM updates or boot-time write tests.** FAT95 is not redistributed; use the working `L:fat95` already in AmigaOS. **The existing USB data should be backed up** before enabling writes: any new storage transport must pass live tests before it is used for important files.

## Installation on the Pi4

**Back up existing R117 and R120 first.** Place the R121 test kernel in the **verified Pi4 EMU68 FAT32 boot partition**. Change only the active `[pi4]` line to:

`kernel=emu68-pi4-r121-usbmass-rw-test.img`

Keep earlier kernel images and `config.txt` backup for rollback. Preserve `hdmi_group=1`, `hdmi_mode=19`, `hdmi_drive=2`, scanlines/integer scaling settings, the `emmc,unit0=rw` overlay, and the existing Amiga RDB partition.

The AmigaOS `DEVS:` and `C:` locations are **NOT** the Raspberry Pi FAT boot partition. From a directory accessible inside AmigaOS, back up the current `DEVS:usbmass.device` if it exists and copy the packaged **v0.5** to `DEVS:usbmass.device`, copying utilities into `C:`. No filename renaming is needed.

### Avoid double-mount corruption

The R120 `USBP4:` volume uses `pi4usbmass.device`; R121 `USB0:` uses `usbmass.device`. **Do not mount both USBP4 and USB0 against the same FAT32 stick at the same time.** If `USBP4` is in `DEVS:DOSDrivers`, back it up and disable its automatic mount first. Retain its original driver and mount entry in `Storage` for rollback. Ensure only `USB0` is active on the next cold boot.

Copy the supplied `Storage/DOSDrivers/USB0` to `SYS:Storage/DOSDrivers/USB0` for **manual testing**, backing up an existing USB0 file before overwriting. Do not automatically enable or mount until you have cold-booted with v0.5 and the R121 kernel.

### Mandatory hardware test sequence (same FAT32 USB stick)

1. **Cold boot Pi4** with the FAT32 drive connected. POST should say `R121`, `HOST RUN`, `MSC READY 01`, and **`MSC TRANSPORT RW READY`**.
2. From AmigaShell, check:
   `Version DEVS:usbmass.device FULL`
   `C:usbmass-z2-pi4`
   `C:frf-usb121-rwcheck`
   `C:usbmass-info-pi4`
   `C:usbmass-rw-mountinfo`
3. Confirm driver v0.5, OpenDevice=0, geometry=512 bytes and capacity nonzero, `TD_PROTSTATUS protected=0`, and readable LBA0.
4. With `USBP4` **not mounted**, mount the test DOSDriver:
   `Mount USB0: FROM SYS:Storage/DOSDrivers/USB0`
   `Info`
   `List USB0:`
   Skip the Mount command if USB0 is already mounted. Do **not** format the drive.
5. Test write + readback on only a new test file:
   `C:frf-usb121-filetest`
   **PASS** means 4096-byte file write/flush/readback succeeded and test file was removed. If it fails, stop testing and retain the file for recovery.
6. For persistence across a cold reboot, copy a small expendable file to USB0, close it and allow storage activity to stop, reboot the Pi4 cleanly, then verify the file is present and correct before deleting it. An in-memory FAT cache could otherwise mask a failed media commit.
7. Only after reliable repeated file creation, edits, deletes, and reboot persistence should `USB0` be put in `DEVS:DOSDrivers` for automatic mounting and the release promoted to public testers.

If **any** write stage fails or `BOT ERR` reports 11–16, stop writing, **do not retry writes on a possibly completed command**, and cold boot only after reverting to RO for diagnosis. Do not run filesystem repair until the underlying storage is stable.

## Rollback

Set `kernel=emu68-pi4-r117-legacy-rev11-usb-test.img` on the Pi4 boot partition; restore `DEVS:pi4usbmass.device` v0.4 and the original `USBP4` FAT95 mount. Disable R121 `USB0` mount while reverting. The new v0.5 driver is read-only by design with an R117 kernel, but the proven combination for rollback is R117 + R120 v0.4.

## Software validation

- R121 big-endian ARM64 kernel builds with native xHCI and `FRF_PI4_USB_MASS_RW=ON`, while R115 read-only test still passes with flag OFF.
- Direct host mock runs the actual Pi4 native BOT worker, tests WRITE10 CBW/OUT/CSW, two-sector write/readback, invalid ranges, and no retry after a failed data OUT.
- 68k `usbmass.device` v0.5 compiles as a proper AmigaOS HUNK resident with internal name and version, byte-aligned mailbox and read/write commands.
- AmigaDOS file test runs against a disposable emulator-backed volume, confirms 4096-byte write/readback/delete.
- R117 legacy USB, R112 xHCI topology and R110 AGABOOT/scanline regressions pass; **no audio source modifications**.

**R121 WRITE10 has not been tested on physical Pi4 USB flash media yet; this is a prerelease for controlled tester validation, not a confirmed stable writable storage release.**

# Pi4 R126 — restore boot POST/version info, keep quiet Workbench

Experimental non-legacy Pi4 native USB RW kernel, 14 November 2026.

## Fixed from R123

R123 was built with `FRF_BOOT_POST=OFF`, which removed the splash-time EMU68AGA SYSTEM POST information screen along with the permanent HUD. R126 restores **only** the splash-time POST, while keeping `FRF_PI4_SUPPRESS_DIAGNOSTIC_POST=ON` so there is no permanent AGA USB diagnostic overlay during Workbench or games.

The boot POST includes:
- **EMU68AGA SYSTEM POST** heading.
- **FRF PI4 BARE METAL: R126**.
- **AGA-STORM PORT: v1.121** (upstream port version, separate from kernel version).
- **PI4 NATIVE USB RW / NON-LEGACY**.
- Keyboard/mouse/gamepad, USB host and mass-storage detection fields.
- Brief splash identification `FRF PI4 R126 USB RW / POST`.

This restores the version/status screen during boot, **not a persistent Workbench overlay**. If the boot is too fast to read the full panel, record a short boot video; no extra boot delay has been added to hold the frame.

## Unchanged from R123

- Pi4 / Pi400 native VL805/xHCI host, USB HID, hubs and USB mass-storage detection.
- `FRF_PI4_USB_MASS_RW=ON`; guest-visible FRUM `PI4W` capability and SCSI WRITE(10), with the same error quarantine. **Real Pi4 FAT32 writes remain unconfirmed.**
- Old Pi4B rev1.1 firmware recovery disabled (`FRF_PI4_LEGACY_REV_DIAG=OFF`). This is the newer/non-legacy build.
- `usbmass.device` v0.5 and `USB0` FAT95 mount remain the matching guest files.
- HDMI audio, 1280x720p50, integer scaling, scanlines, AGA-Storm / AGABOOT settings and P96 handoff unchanged.
- No EEPROM or partition changes. This build does not fix the older board's VL805 handoff issue.

## Install on Pi4

Copy `BOOT/emu68-pi4-r126-usb-rw-boot-post.img` to the FAT32 EMU68 **boot partition root**, not the Amiga RDB partition. Back up `config.txt` first, and change only the existing active `[pi4]` kernel setting:

```ini
kernel=emu68-pi4-r126-usb-rw-boot-post.img
```

Preserve the existing `hdmi_group=1`, `hdmi_mode=19`, `hdmi_drive=2`, `dtoverlay=emmc,unit0=rw`, AGABOOT settings, and screen timing.

Rollback: `kernel=emu68-pi4-r123-standard-usb-rw.img` (when that kernel remains on the card), or use the previously installed R121 kernel.

## AmigaOS driver

The package includes `DEVS/usbmass.device` v0.5, `Storage/DOSDrivers/USB0`, `C/frf-usb121-rwcheck` and `C/frf-usb121-filetest`. If the matching driver is already installed, **do not reinstall it just to restore POST**: this build only changes boot UI. If not installed, copy these to the actual AmigaOS filesystem, not merely into the Pi FAT boot folder. Avoid mounting USBP4 and USB0 simultaneously for the same stick.

## Test status

Compiled as big-endian ARM64 kernel; boot-version, quiet HUD, Pi4 xHCI topology, AGABOOT/scanline and native WRITE10 mock regressions passed. No real Pi4 boot or USB-sector write was performed during packaging. Exact kernel is identified by the SHA256 manifest.

This is a test build until the user confirms the return of the POST and existing Pi4 USB input.

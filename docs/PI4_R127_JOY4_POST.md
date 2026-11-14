# Pi4 R127 — Four-button joystick compatibility + restored boot POST

Experimental native Pi4 build, 14 November 2026. Based on R126 (which restored the boot-only version information), using the non-legacy R123 native VL805/xHCI USB implementation. No legacy recovery code and no persistent USB diagnostic overlay.

## Fixes

The Pi4 HID bridge previously forwarded only HID button 1 to Amiga CIA-A PRA7 (port-2 primary fire). HID buttons 2 and 3 were routed solely to POTGOR14 / POTGOR12, and HID button 4 did not affect primary fire. Because most classic Amiga games examine only the CIA fire input, three face buttons seemed non-functional.

**R127 compatibility mapping:**

| USB HID face button | Amiga joystick port 2 |
|---|---|
| 1 | CIA-A PRA7 primary fire (active low) |
| 2 | CIA-A PRA7 primary fire **and** POTGOR bit 14 (active low) |
| 3 | CIA-A PRA7 primary fire **and** POTGOR bit 12 (active low) |
| 4 | CIA-A PRA7 primary fire (active low) |

The OR-to-primary mapping deliberately favours single-button classic games. When a game uses the distinct second/third POTGOR signals, pressing button 2 or 3 will also assert primary fire. This is NOT four independent Amiga hardware actions; the conventional Amiga joystick port has only three button inputs. A later selectable independent-only profile can be introduced if required.

HID pad direction, axes, hats, gamepad report parsing, and release/disconnect handling are not changed. Generic HID four-button and ShanWan 2563:0576 report tests pass.

## Version screen

R126's boot-only **EMU68AGA SYSTEM POST** is preserved and now identifies `FRF PI4 BARE METAL: R127` and `AGA-STORM PORT: v1.121`. The first boot screen also displays `PI4 NATIVE USB RW / NON-LEGACY` and the splash label `FRF PI4 R127 USB RW / 4BTN`. The permanent USB overlay remains suppressed.

## AmigaOS hardware test

The package includes `C/frf-joy127-check`, an AmigaOS 68k program that reads only CIA-A PRA and POTGOR, displaying active-low port-2 fire and two extra-button signals as they change for 15 seconds. No disk writes or USB controller operations. Copy to actual AmigaOS `C:`, then run:

```
C:frf-joy127-check
```

Press each face button 1, 2, 3 and 4 individually. Each should show `FIRE=1`. Button 2 should also show `EXTRA2=1`; button 3 should show `EXTRA3=1`. Upon release, the fields should return to 0.

If a controller uses a proprietary report format or a different HID button ordering, the specific VID:PID and a photo of the output will be needed for a device-specific mapping. This tool proves guest-visible register signals, not individual raw HID bit identities.

## Install

Copy `BOOT/emu68-pi4-r127-usb-rw-joy4-post.img` to the root of the Pi4 EMU68 FAT32 boot partition. Back up `config.txt`, then change only the existing `[pi4]` kernel entry:

```ini
kernel=emu68-pi4-r127-usb-rw-joy4-post.img
```

Keep 1280x720p50, HDMI audio, integer scaling, scanlines, AGABOOT picture controls and existing Amiga RDB untouched.

The package also includes the unchanged `DEVS/usbmass.device` v0.5, `Storage/DOSDrivers/USB0`, the non-writing storage status checker and the temporary USB file test. Use the AmigaOS driver only if it is already proven to open. FAT32 hardware writes remain **unverified**; use a backed-up stick and check `TD_PROTSTATUS` before testing any writes. Never mount both USBP4 and USB0 simultaneously.

Rollback: select `emu68-pi4-r123-standard-usb-rw.img` if present (or R121 original). R127 is not intended for the older Pi4B rev1.1 VL805 boot problem.

## Validation and limitations

- Big-endian ARM64 kernel built successfully; embedded version strings verified.
- 18 HID bridge/gamepad mock input events passed, covering buttons 1–4, direction, release, generic four-button HID and ShanWan HID.
- R126 boot POST/version checks, xHCI USB topology, R110 scanline/AGABOOT and native SCSI WRITE10 mock tests passed.
- The physical Pi4 and its particular joypad have **not** yet confirmed the new mapping.
- No physical FAT32 WRITE10 or boot-speed test was performed.

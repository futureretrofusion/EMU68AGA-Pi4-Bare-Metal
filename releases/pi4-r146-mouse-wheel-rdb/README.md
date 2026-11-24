# EMU68AGA Pi4 R146 — Mouse Wheel + RDB

**Experimental bare-metal Raspberry Pi 4 prerelease — 24 November 2026**

This package combines the Pi4 **native ARM-side xHCI USB mouse scroll-wheel
bridge**, **NewMouse-compatible CIA keyboard events**, **R143 analogue stick
vertical-axis correction**, and the **R140–R142 multi-LUN/RDB USB storage
updates**. It extends the R139 usbmass.device v0.8 PFS3 Direct SCSI
guest-driver work.

## What's new

- USB scroll-wheel reports feed the existing ARM mouse bridge, producing
  NewMouse make/break events through the virtual CIA keyboard queue.
  Wheel-up uses raw key $7A (release $FA); wheel-down uses $7B
  (release $FB). R146 **reverses the HID wheel delta sign** relative
  to R145 after a physical Pi4 test showed inverted scrolling.
- R145 pointer fix: **keep mice in proven HID boot protocol**.
  R144's report-mode switch caused a nonworking pointer. The
  report-mode experiment is disabled in this release, and pointer
  X/Y and all three mouse buttons are unchanged.
- R143: correct both analogue joystick Y axes for generic HID,
  ShanWan and Switch Pro while preserving D-pad and button behaviour.
- R140/R141: improve card-reader multi-LUN readiness and avoid
  incorrectly disabling siblings on repeated PFS3 SCSI CHECKs.
- R142: **opt-in experimental RDB writing**, enabled in the supplied
  kernel with FRF_PI4_RDB_WRITE_EXPERIMENTAL=1. Do not treat this as
  proof of stable filesystem writes.
- No Poseidon or separate AmigaOS USB stack for mouse/joystick.
  Included guest usbmass.device v0.8 handles PFS3 storage only.

## Contents

- BOOT/emu68-pi4-r146-wheel-direction-mouse-rdb-rw.img: Pi4 AArch64
  big-endian bare-metal kernel.
- DEVS/usbmass.device: 68k resident v0.8 USB mass-storage/PFS3 driver,
  installed separately inside AmigaOS (not the Pi4 boot FAT).
- TESTS/: host-side R143/R146 regression source.
- SHA256SUMS.txt: individual file checksums.

## Installation

1. Back up the original Pi4 boot config.txt/kernel and all USB/RDB
   media. Do not format or repartition.
2. Copy the .img from BOOT/ into the Pi4 FAT32 boot partition.
3. In config.txt, change only the active [pi4] kernel entry to:
   kernel=emu68-pi4-r146-wheel-direction-mouse-rdb-rw.img
4. Keep existing 1280x720p50 HDMI, integer scaling, scanlines,
   AGABOOT, ROMs, FAT32 shared and Amiga RDB partitions intact.
5. For PFS3 mass-storage support, back up DEVS:usbmass.device on the
   Amiga and install the included guest driver there. The driver copied
   to Pi FAT32 is NOT automatically installed in AmigaOS. Cold boot
   to replace the resident driver. Leave USBR0..USBR4 DOSDrivers as
   currently configured unless separately diagnosed.
6. Test movement, left/right/middle buttons, NewMouse/FreeWheel
   scroll, joystick axes, RDB mounts and backup-first media writes.

## Test status

- Pi4 real hardware: R145 restored mouse movement, and wheel input
  reached Workbench, but vertical wheel direction was inverted.
- R146 host tests PASS for corrected wheel sign, NewMouse make/break
  keycodes, 3-/4-byte mouse reports, report-ID layout, pointer/buttons.
- R143 analog-stick and R142 USB transport/RDB write-routing
  regression tests PASS. AArch64 big-endian Pi4 kernel crossbuild PASS.
- Exact R146 kernel was copied to the Pi4 FAT32 boot partition and
  SHA256 rechecked after a read-only remount.
- Still pending user confirmation of R146 direction after cold boot;
  exhaustive PFS3 write testing and universal mouse/hub compatibility.
  This is an EXPERIMENTAL prerelease, not a stable filesystem release.

## Rollback

Previous known-working pointer:
  kernel=emu68-pi4-r145-mouse-boot-protocol-rdb-rw.img

Previous R142 RDB write candidate:
  kernel=emu68-pi4-r142-rdb-write-experimental.img

Avoid R144 because its HID report-mode switch disabled the pointer.
Restore the previous DEVS:usbmass.device if guest driver compatibility
changes. Back up important RDB data before any experimental writes.

## Rebuild and licensing

Built with TARGET=raspi64, VARIANT=pistorm-classic,
FRF_PI4_XHCI_HID=ON, FRF_VIRTUAL_AGA=ON,
FRF_PI4_USB_MASS_MULTI=ON, FRF_PI4_USB_MASS_RO=ON,
FRF_PI4_USB_MASS_RW=ON, FRF_BOOT_POST=ON,
FRF_RELEASE_QUIET=ON and compile flag
-DFRF_PI4_RDB_WRITE_EXPERIMENTAL=1.
The experimental report-protocol mouse flag is OFF.
Rebuilding without the RDB flag disables experimental RDB writes.

No Kickstart, Workbench, commercial ADF/HDF media or external
firmware images are included. See LICENSE, THIRD_PARTY_NOTICES.md
and the machine subdirectory's source/license notices.

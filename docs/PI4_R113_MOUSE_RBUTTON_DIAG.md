# FRF EMU68AGA Pi4 R113 — wireless mouse right-button diagnostic

**R113 is diagnostic, not yet a verified right-button fix.** It inherits the R112 native Pi4 xHCI USB/MSC test and the hardware-confirmed R110 scanline code. The issue reported: USB wireless mouse dongle moves the pointer and left-click works, but right-click is not detected.

## Why this needs two probes

The existing Pi4 input pipeline takes button bit 1 (USB HID `0x02`), passes it to `aga_virtual_mouse_update()`, and in the Amiga custom-register hook clears `POTGOR` bit 10 (`0x0400`) when RMB is pressed. LMB is a separate CIA-A PRA bit 6 (`0x40`). The source looks correct for normal **three-byte HID boot-mouse reports**, but certain wireless receivers use report IDs or nonstandard button layouts. We need the dongle's **actual raw report** and the **Amiga register read** before changing the mapping. Blind remapping might damage functioning left-click or keyboard data.

## R113 Pi4 HDMI test

The diagnostic kernel enables a temporary on-screen USB HUD, labelled **R113**, while preserving the R112 storage code and R110 scanline source. The HUD will obscure part of the picture; this is intentional for testing and can be reverted by switching back to R112 or the stable R110 kernel.

- **USB MOUSE RAW**: latest 4 raw mouse bytes as a hex word, after optional HID report-ID stripping
- **BTN**: interpreted HID button mask; `01` is left, `02` is right, `04` is middle; `03` is both
- **L / R**: accumulated left and right button-press counters; R should increase by one each right click
- **AMIGA POTGOR**: read-only sample from the *same input hook* used by guest register `DFF016`. Right press should clear `0x0400`
- **RMB**: `01` when the emulated right-button line is low, `00` when released
- **HID REPORT BYTES**: report length; e.g. 3/4 for a boot-protocol mouse

For a working right click, pressing the right button alone should show `BTN=02`, increment `R`, show `RMB=01`, and make the `AMIGA POTGOR` value have bit `0x0400` clear.

**If BTN remains 00 and R does not change:** the failure is upstream in the dongle HID report, report ID, enumeration or protocol selection. Save a photo of the raw report while holding the right button and tell us the dongle VID:PID.

**If BTN becomes 02 but RMB stays 00:** the host-to-virtual-chipset handoff is failing.

**If BTN becomes 02 and RMB becomes 01 but AmigaOS does not see it:** inspect guest trap/register reads and the particular Workbench/game application.

## Additional independent AmigaOS register test

A 68k executable **C/frf-mouse-buttons-pi4** has been compiled. Copy it into the actual AmigaOS `C:` directory. Run:

```
C:frf-mouse-buttons-pi4
```

The program reads CIA-A PRA at `$BFE001` and Paula POTGOR at `$DFF016` every 5 Amiga ticks for up to approximately 20 seconds. It prints **L, R and M** transitions and raw register values. No writes or USB resets; Ctrl-C stops it. Test each button separately. In AmigaOS R should be `1` while right is held and `0` when released.

Example expected shape (not from actual hardware):

```
sample=... CIA=... POTGOR=... L=0 R=0 M=0
sample=... CIA=... POTGOR=... L=0 R=1 M=0
sample=... CIA=... POTGOR=... L=0 R=0 M=0
```

**Important:** the executable copied onto the Pi4 FAT boot partition is not automatically visible in AmigaOS `C:`. Transfer it to your Workbench system directory using the normal Amiga file-transfer mechanism.

## Installing and rollback

This test is **Pi4 only**. Copy `BOOT/emu68-pi4-r113-mouse-rmb-diag.img` into the existing Pi4 FAT boot partition; then change **only the active `[pi4]`** kernel line in `config.txt`:

```ini
kernel=emu68-pi4-r113-mouse-rmb-diag.img
```

Keep `hdmi_group=1`, `hdmi_mode=19` (720p50), `disable_overscan=1`, `dtoverlay=emmc,unit0=rw` and the existing emmc.dtbo, and leave the [pi3] stanza unchanged. Do not delete R112, R111 or R110 images.

Rollback when done: `kernel=emu68-pi4-r112-usbmass-detect-test.img` (current USB test) or `kernel=emu68-pi4-r110-agaboot-control-only-test.img` (hardware-confirmed scanline baseline).

## Status

- R113 Pi4 big-endian native kernel cross-build: passed
- AmigaOS m68k mouse-register diagnostic cross-build: passed
- No physical Pi4 button test yet: right-button defect has **not** been proven fixed
- No USB HDD hardware test yet: R112 mass storage is still experimental and read-only.

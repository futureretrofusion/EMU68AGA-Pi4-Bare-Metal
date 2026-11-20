# FRF EMU68AGA Pi4 R133 — Multi-MSC and RDB detection (experimental)
**21 November 2026 — BUILT and HOST-MOCK-TESTED; NOT hardware-tested.**

This is a Pi4 (BCM2711/VL805 xHCI) development candidate based on the user's confirmed R132 Pi4 boot/USB-write kernel. This version is NOT a stable release and must not be installed onto a Pi5 boot card.

## Included
- BOOT/emu68-pi4-r133-multidrive-rdb-test.img — Pi4 64-bit big-endian bare-metal kernel. Boot POST identifies R133 MULTI-MSC; retains R131 two-second final POST wait and Pi4 HDMI 1280x720p50/scanlines.
- DEVS/usbmass.device — matching AmigaOS resident v0.6, with multiple trackdisk units 0..7.
- C/frf-usb133-units — READ-ONLY enumeration/RDSK diagnostic that lists each host slot/LUN and scans sectors 0..15. It never writes any USB sectors.
- Storage/DOSDrivers/USB0 — old FAT95 mount configuration for *verified FAT32 unit0 only*; do not use it for an RDB unit.
- SOURCE — sources for the multi-BOT worker, diagnostic and driver.

## Storage architecture
- 4 independent physical USB mass-storage xHCI slots, each with dedicated bulk IN/OUT TRB rings.
- USB BOT GET_MAX_LUN; up to 8 logical disk units total (LUN0..7 cap per physical device).
- READ CAPACITY(10), READ(10), and WRITE(10) for 512-byte logical sectors with 32-bit LBA only. UAS protocol 0x62 and unsupported sector sizes still require future implementation.
- Original FRUM v1 board/transport remains unchanged; FRUM v2 unit metadata is an additive structure at Zorro board offset 0x2200. The v0.6 guest writes unit selector before each FRUM request.
- A read-only 16-block RDSK scan protects detected RDB media from WRITE(10). Units 1..7 are **always write-protected** in this build.
- Only non-RDB *unit0* may be writable; do not assume FAT32 stays at unit0 when inserting devices in another order. ALWAYS check with the diagnostic before mounting or writing.
- Up to 12 bounded capacity attempts per empty/slow LUN; first attempts rotate fairly instead of repeatedly retrying the same empty reader.

## First hardware test on the Pi4 (NO writes to the RDB)
1. Back up the Pi4 SD's FAT32 config.txt. Preserve the known working R132 kernel and config for rollback.
2. Copy BOOT/emu68-pi4-r133-multidrive-rdb-test.img to the actual **Pi4** boot FAT32 root. Select it with kernel=emu68-pi4-r133-multidrive-rdb-test.img in the active Pi4 section, changing no display, dtparam, SD, or AGA options.
3. Boot with keyboard, mouse, gamepad, the FAT32 stick, and the USB RDB card reader attached. Photograph the POST MSC SEEN / CFG / READY and R133 version.
4. Back up the AmigaOS SYS:Devs/usbmass.device (NOT the Pi boot FAT). Install packaged DEVS/usbmass.device v0.6 to AmigaOS SYS:Devs/usbmass.device, and C/frf-usb133-units to SYS:C. Cold boot before checking Version DEVS:usbmass.device FULL.
5. Run C:frf-usb133-units in an AmigaShell and capture all lines. It should list USB unit numbers with slot/LUN and a line like "AMIGA RDB HEADER at LBA0" for the populated reader. This diagnostic never writes.
6. Verify FAT32 mounts/read operations on the correct unit only. **Do not mount USBP4 and USB0 against the same physical disk simultaneously.**
7. For an existing Amiga RDB partition mountlist, set its Device to usbmass.device and the Unit to the R133 diagnostic's RDB unit number; keep all other geometry/filesystem details intact until separately tested. R133 exposes raw disk sectors and detects RDSK; **it does not automatically parse RDB PART blocks or install RDB bootnodes**.
8. Do not write any RDB sectors. Secondary unit write protection is intentional. If no RDB is found, capture the per-unit output and POST, revert to R132 if boot or USB input regresses.

## Software verification (PASS)
- Pi4 big-endian AArch64 cross-build with FRF_PI4_USB_MASS_MULTI=ON and FRF_PI4_USB_MASS_RW=ON.
- AmigaOS 68k v0.6 built as a valid HUNK/loadseg device; console RDB probe built.
- Host mock ran the actual new CPU1 mass-storage worker: two physical MSC host slots, a reader with LUN0/LUN1, RDB at LBA3, distinct slot identities, FAT unit0 writable, RDB unit1 protected.
- Second mock scenario: card reader LUN0 absent while LUN1 has RDB; fair retries correctly discover it.
- No physical Pi4 boot test with this R133 build yet. Gamepad unplug/replug handling is still an independent unfinished task.

## Checksums
Kernel SHA256: 96348e6006c915cf59dcb4a44eb22c17ae1a8b6a71753572701a33ecef204f01
v0.6 driver SHA256: 97cae88ec295a53daf2cd2b10a0d31ab1f8eea0c599a7b43acc739995e6da872
Diagnostic SHA256: c97c2a33329bfeb8b006e0dccb57d66c302de20e269d4d176e6b0908ffea4efd

## Unmodified working baseline
The published R132 branch and kernel must remain available until hardware regression passes. The SD currently mounted on the Studio PC has a Pi5-oriented config.txt and has **not** been modified by this R133 work.

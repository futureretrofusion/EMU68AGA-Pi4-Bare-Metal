# R138 PFS3 / RDB Unit0 compatibility candidate

EMU68AGA Pi4 R138 - PFS3 RDB guest unit0 compatibility TEST
22 November 2026. For Raspberry Pi 4 / Pi400 with native ARM VL805 xHCI.
EXPERIMENTAL - Big-Endian ARM64 + AmigaOS 68k cross-compiled, host mock tested;
NOT yet verified to fix PFS3 initialization on actual Pi4 hardware.

Reason for repair:
CaffeineOS Pi3 AmigaOS mount files USBR0..USBR4 expect
Device = usbmass.device, Unit = 0 for the PDS/3 RDB disk.
Pi4 R133 (present in R136) instead numbers physical USB storage by
enumeration order. When FAT32 enumerates first and RDB comes later,
guest unit0 points at the wrong disk, producing PFS3 initialization failure.
A multi-LUN card reader could also detect LUN1 too late.

Changes:
1) Pi4 ARM CPU1 now probes each advertised LUN once before USB boot-ready.
   READ CAPACITY and RDSK detection ONLY, no writes to media.
2) AmigaOS usbmass.device v0.7 preferentially maps the first ready RDB
   to logical guest Unit0. The other physical USB units remain reachable
   at 1..7, reordered accordingly. With no RDB present, identity
   mapping preserves the old FAT32 Unit0 behavior.
3) Each successful OpenDevice pins its selected physical disk for the
   lifetime of that open, so a late USB event cannot swap a live mount
   to a different disk.
4) Read-only RDB diagnostic displays physical/guest map and RDB PART/FSHD.

Pi4 boot SD install:
Back up current FAT32 config.txt. Keep all older kernels for rollback.
Copy BOOT/emu68-pi4-r138-rdb-unit0-compat-test.img to FAT32 root and
change ONLY the existing [pi4] kernel= line to:
    kernel=emu68-pi4-r138-rdb-unit0-compat-test.img
Do not touch the Amiga RDB partition or display/scanline settings.

AmigaOS install:
1. Back up the current DEVS:usbmass.device (the Pi4 v0.6).
2. Copy DEVS/usbmass.device v0.7 to DEVS:usbmass.device.
3. Copy C/frf-usb138-rdbdiag to C:
4. Cold reboot to replace loaded resident device.
5. Leave your five CaffeineOS USBR0..USBR4 PFS3 mount files UNCHANGED.
6. With the RDB card and any other USB disks connected at BOOT, run
      Version DEVS:usbmass.device FULL
      C:frf-usb138-rdbdiag
   Capture output, particularly guest0 vs physical RDB mapping and PDS/3.
7. Check CaffeineOS mounts RDB partitions. Test READ operations only
   before any write test; do not use HDToolBox/Format/Initialize.

IMPORTANT SAFETY / LIMITS:
- RDB remains READ-ONLY in this candidate. PFS3 may require writes;
  successful PFS3 initialization is NOT yet proven and write support is
  intentionally NOT enabled until the mapping is confirmed on hardware.
- usbmass.device has no HD_SCSICMD, same as inspected Pi3 0.3 driver.
- FAT32 device previously Unit0 may become Unit1 when an RDB disk exists.
  If FAT mounts in your system also use Unit0, their mount files would need
  an explicit unit selection or a separate alias; do not mount both
  against the same logical unit when an RDB disk is present.
- No USB MSC hotplug; connect drives before ARM boot. HID hotplug from
  R136 is preserved but still not hardware-verified as fixed.
- Up to 4 physical BOT devices and 8 logical LUNs, only 512B sectors.
- Unexpected extra card-reader LUNs or slow media may need a later retry.
- Previously known Pi4 FAT32 vfat boot corruption is NOT repaired here.
- This is a new paired kernel + driver: DO NOT replace only one side.

Validation:
- AmigaOS v0.7 crossbuild: PASS, valid loadseg() format.
- AArch64 big-endian R138 kernel crossbuild: PASS.
- R138 RDB-first unit-map host test: PASS.
- R133 multi-device, LUN1 RDB, write-safety host test: PASS.
- Physical PFS3 mount: PENDING.
- Pi4 / Amiga SD card: not written (absent from Studio Main).
Rollback:
Restore DEVS:usbmass.device v0.6 and restore original [pi4] kernel
selection, preserving mount files unchanged.

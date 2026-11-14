FRF EMU68AGA Pi4 - R120 read-only FAT32 USB mass driver test
=========================================================
14 November 2026. EXPERIMENTAL: physical Pi4 OpenDevice and FAT32
mount have NOT been confirmed. No HDMI audio, AGABOOT or USB host changes.

WHAT HARDWARE HAS PROVED
R115/R117: HOST RUN, MSC SEEN/CFG/READY 01, SCSI TRY 01, BOT ERR 00.
USB10 guest: FRUM Zorro 6D73:0069 at $EA0000; present=1; capacity=245760000,
slot=4. Both previous drivers returned OpenDevice -1. Only AmigaOS driver
loading/opening and sector0/FS mounting remain to be validated.

R120 COMPONENTS
DEVS/pi4usbmass.device        v0.4 Pi4-native FRUM read-only test driver;
                              corrected guarded HUNK entry and timeout logic,
                              explicit OpenDevice error stage codes
C/frf-usb120-loadtrace        reports DEVS LoadSeg, resident and OpenDevice
C/pi4usbmass-info            tests OpenDevice, geometry and READ LBA0
C/pi4usbmass-mountinfo       partition layout (no writes)
C/usbmass-z2-pi4             verifies guest Zorro mailbox
Storage/DOSDrivers/USBP4     read-only FAT95 first FAT partition mount
DEVS/frfusbtest.device        optional independent R118 v0.3 loader probe
C/frf-usb118-loadtrace       optional v0.3 test utility
BOOT/emu68-pi4-r117-legacy-rev11-usb-test.img
                              unchanged kernel for independent Pi4 testers
SOURCE/                      reproducible driver and loader-test source
SHA256SUMS                   file integrity manifest

INSTALL ON ACTUAL AmigaOS FILESYSTEM (not simply EMU68 Pi FAT boot)
1. Check L:fat95 exists. Keep the present Pi4 R117 kernel.
2. Backup your current DEVS:pi4usbmass.device v0.2:
       Copy DEVS:pi4usbmass.device SYS:Storage/pi4usbmass.device.v02-backup
3. Copy supplied DEVS/pi4usbmass.device to AmigaOS DEVS: under EXACT
   name pi4usbmass.device. Do not rename any executable.
4. Copy all test tools in package C/ to AmigaOS C:.
5. Copy USBP4 to SYS:Storage/DOSDrivers/USBP4 (not DEVS:DOSDrivers yet).
6. COLD BOOT Pi4 so any previously loaded v0.2 is no longer resident.
7. In AmigaShell:
       Version DEVS:pi4usbmass.device FULL
       C:usbmass-z2-pi4
       C:frf-usb120-loadtrace
   Capture entire output. STOP if OpenDevice fails; don't attempt mount.
8. If OpenDevice is 0:
       C:pi4usbmass-info
       C:pi4usbmass-mountinfo
   STOP if LBA0 or FAT partition recognition fails. If both pass:
       Mount USBP4: FROM SYS:Storage/DOSDrivers/USBP4
       Info
       List USBP4:
9. Enable auto mount ONLY after a successful repeat test; then copy USBP4
   into DEVS:DOSDrivers. If unsuccessful, keep manual USBP4 mount.

ERROR CODES
0: opened successfully
-20: unit or IORequest size is invalid
-21: expansion.library v36 could not open
-22: Zorro FRUM board not found or unmapped
-23: FRUM version/magic/layout wrong
-24: FRUM valid but media absent/not ready
-1: loader, resident, or generic open failure not differentiated.
If -1 persists, separately install DEVS/frfusbtest.device and run
C:frf-usb118-loadtrace to inspect R118 v0.3 independent-name loader.

SAFETY / COMPATIBILITY
The driver refuses CMD_WRITE, TD_WRITE64, TD_FORMAT and related write
commands. It does NOT format disks, change partitions, write USB sectors
or modify AmigaOS startup sequence. Existing v0.2 backup can be restored.
It shares the native Pi4 R117 FRUM/Zorro bridge, NOT Pi3 DWC2 USB host code.
Pi4 HDMI audio is working: R119 audio test is NOT installed or included.
Keep 1280x720p50, integer scaling, scanlines and R117 boot settings.
Third-party L:fat95 is not redistributed. Existing USB0/Pi3 mount is not
overwritten; first test USBP4 manually.

STATUS: Compiler, resident format, read-only safety and source tests are
complete. The Pi4 hardware MUST still demonstrate OpenDevice=0, a correct
512-byte LBA0 read and a mounted FAT32 volume before calling this fixed.

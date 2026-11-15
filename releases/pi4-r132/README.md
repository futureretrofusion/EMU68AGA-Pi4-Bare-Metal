# EMU68AGA Pi4 R132 — USB write + POST hardware-confirmed preview
Date: 15 November 2026

## Confirmed by Pi4 tester
- Boot POST shows valid USB enumeration information.
- USB mass-storage writes work with this R132 kernel.
- Earlier R130 fast native xHCI handshake and R131 2-second POST hold are retained.
- Original 1280x720p50 / AGABOOT native picture settings are preserved.
- AGA runtime debug HUD remains suppressed.

## Package
- emu68-r132-pi4-rw-staging.img: exact checksum-matched R132 test binary
- DEVS/usbmass.device: AmigaOS resident v0.5 with PI4W capability check
- Storage/DOSDrivers/USB0: FAT95 manual mount config
- C/frf-usb121-rwcheck: non-writing driver/capability test
- C/frf-usb121-filetest: controlled expendable-media write test

## Install
1. Back up the Pi4 FAT32 boot partition config.txt.
2. Copy the kernel to the FAT32 boot partition and set
   kernel=emu68-r132-pi4-rw-staging.img in the active Pi4 config section.
3. For AmigaOS RW, back up and install the matching guest driver to
   SYS:Devs/usbmass.device; a file copied to Pi firmware FAT is not
   automatically installed into the AmigaOS SYS: filesystem.
4. Do NOT have USBP4 and USB0 mounted simultaneously for the same device.
5. Read and verify a removable FAT32 disk first. Back up all media before
   attempting writing, and avoid first write tests on valuable RDB disks.

## Remaining limitations
- Only ONE native xHCI MSC slot is exposed to AmigaOS, unit 0, LUN 0;
  a second USB RDB card reader may not appear.
- Physical USB unplug/replug re-enumeration is not complete. The existing
  gamepad receiver can still fail to reconnect; wireless syncing under
  four-device hub load is intermittent.
- This is a developer preview, not a claim of full Pi3 USB feature parity.
- No copyrighted Kickstart ROMs, Amiga disk images, or third-party
  Broadcom firmware binaries are packaged here.

## Software and hardware verification
- Cross compiled for Pi4 bare-metal big-endian AArch64.
- HOST mock PASS SCSI WRITE(10) / READ(10), 1024-byte readback,
  LBA bounds, fail-closed transport error recovery.
- User physically confirmed valid POST info and USB writes on Pi4.

Kernel SHA256:
70aabfcb2eef859305814aaf92c72f30689c0b756dfd57d39a0dd010aad28c44
usbmass.device SHA256:
a1ae49632102776d7f17bcd69c326f5aed763f023cfe003ad9b162e6593bb36d

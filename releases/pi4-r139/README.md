# Pi4 R139: usbmass.device v0.8 PFS3 Direct SCSI candidate
**22 November 2026 – Experimental test candidate.**

R138 fixed Pi4 unit0 mapping and boot-time RDB readiness but the user
reports PFS3 still fails initialization.

## Critical install detail

R138 installed ONLY the ARM kernel onto the Pi4 FAT32 partition.
The matching AmigaOS v0.7 usbmass.device was STAGED under
R138_AMIGAOS_INSTALL/ and was NOT copied into CaffeineOS DEVS:.
If not manually copied, the older v0.6 driver remained in use.

Original CaffeineOS Pi3 mount files USBR0..USBR4 use:
Device = usbmass.device; Unit = 0; DosType = 0x50445303 (PDS/3).
PFS3 Direct SCSI normally uses HD_SCSICMD, absent from Pi4 v0.6/v0.7.

## Contents

- DEVS/usbmass.device: v0.8, AmigaOS 68k resident driver.
- C/frf-usb139-scsi-probe: read-only INQUIRY, READ CAPACITY, READ(10)
  versus CMD_READ, and RDSK detection at LBA0..15.
- C/frf-usb138-rdbdiag: read-only RDB unit mapping and filesystem metadata.
- README.md: this note.

Uses the existing R138 Pi4 ARM kernel; guest-driver-only update.

## Installation

1. On the Amiga, back up DEVS:usbmass.device.
2. Copy DEVS/usbmass.device v0.8 into DEVS:usbmass.device.
3. Copy the two diagnostics into C: if desired.
4. Leave original USBR0..USBR4 CaffeineOS mount files unchanged.
5. Cold ARM reboot to unload the resident old driver.
6. Run Version DEVS:usbmass.device FULL; expect v0.8.
7. Run C:frf-usb138-rdbdiag and C:frf-usb139-scsi-probe,
   and capture the full output before a mount attempt if feasible.
8. Attempt CaffeineOS PFS3 mounts; report the exact error message.

## Implementation

HD_SCSICMD (command 28) translates SCSI READ6/10/12 through the same
Pi4 mailbox as TD64 read. INQUIRY, TEST UNIT READY, READ CAPACITY10,
REQUEST SENSE, MODE SENSE6/10, START STOP and SYNCHRONIZE CACHE
have basic synthetic safe responses. WRITE6/10/12 retains existing
write protections, and RDB writes are forbidden. Unknown SCSI commands
return CHECK CONDITION / ILLEGAL REQUEST.

## Safety / limitations

- This does not prove PFS3 will mount: actual Pi4 hardware remains untested.
- The RDB is deliberately read-only in R138/R139. Some PFS3 installations
  may need writes for startup or journal recovery and still fail.
- Only 512-byte logical sectors, 32-bit LBA and READ CAPACITY10 supported.
- No USB MSC hotplug; connect at ARM power-on.
- Do not initialize, format, repair or write to RDB media for this test.
- With an RDB disk, guest unit0 selects RDB; FAT USB devices shift to 1+.
- R138 ARM HID hotplug remains unchanged / hardware-unverified.
- PFS3 handler availability was not verified on this remote CaffeineOS.
- Tests: AmigaOS v0.8 crossbuilt PASS; R139 CDB parser host PASS;
  R138 and R133 multi-drive host mock tests PASS; AmigaOS mount PENDING.

## Rollback

Restore previous backed-up AmigaOS v0.7/v0.6 DEVS:usbmass.device,
cold ARM reboot. R138 kernel is unchanged. SD RDB remains untouched.

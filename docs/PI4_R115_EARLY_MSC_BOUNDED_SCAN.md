# FRF Pi4 R115 — Early USB MSC Probe + Bounded Hub Enumeration

**Pi4 experimental diagnostic build, 14 November 2026. Hardware boot and FAT32 mounting remain unverified.**

## Hardware evidence from R114

A tester's Pi4 display showed:

- **R114 boots quickly** with the same FAT32 USB drive that mounts automatically on Pi3.
- `MSC SEEN=01` and `MSC CFG=01`: one BOT mass-storage USB interface was discovered and its bulk endpoints configured.
- `MSC READY=00`, `SCSI TRY=00`, `BOT ERR=00`: at the instant of the photograph, no SCSI READ CAPACITY had been sent.
- `HOST SCAN`, `XHCI STAGE=00000032` (decimal 50), `USB PHASE=40`, `HUBS=02`, `HID/KB/MS=00`: initial root + cascaded hub enumeration was still underway. A single photograph does not prove the scanner remained there permanently.
- The PFS-III initialization requester was for an unrelated unconnected mount; do not change PFS-III.
- R114's native Pi4 POST was confirmed visible, proving the R113 missing HUD was corrected.

This is a **transport-to-storage scheduling problem first**, not evidence of a FAT32 filesystem error. Even a successful SCSI capacity probe alone is **not enough** to mount the FAT32 volume inside AmigaOS: `usbmass.device`, `L:fat95` and a correct `USB0` DOSDriver must actually exist on the AmigaOS partition, not merely in the Pi boot FAT partition.

## Changes from R114

1. **Early read-only SCSI capacity probe:** when a Pi4 native xHCI BOT interface is configured on a root-hub child or an external-hub child, perform precisely one 350 ms-per-stage bounded BOT `READ CAPACITY(10)` before enumerating more potentially nonresponsive ports. No HID interrupt polling is running at this early point. The xHCI slot is preserved for subsequent guest reads.
2. **Retain the early result:** when the USB worker finishes its global scan, `frf_pi4_um_init()` retains early capacity, attempt count, and fail-closed transport state rather than incorrectly resetting the device to no media.
3. **Real SCSI retries remain:** a valid SCSI `CHECK CONDITION` can still be retried every two seconds, up to twelve attempts. Failed CBW/data/CSW transport remains fail-closed; no endlessly repeated xHCI waits. No write commands are issued.
4. **Bounded initial enumeration:** the Pi4 test build limits the initial root and cascaded USB2-hub power/status/child scan to a shared **12-second elapsed-time budget**, rather than an arbitrary 3–4-device count. Already-configured mass-storage and HID slots remain registered. A single in-flight control request can run until its normal timeout; this is not a hard real-time deadline. Other Pi4 production kernel versions are unaffected.
5. **Live POST details:** screenshot the new `SCAN DEPTH`, `PORT`, `CAP` (scan budget hit), `HUBS`, `STEPS`, `MSC SEEN`, `CFG`, `READY`, `MSC VID:PID`, `SCSI TRY`, `BOT ERR`, `MSC TRANSPORT`, `MOUSE RAW` and `AMIGA POT` fields. Depth 0 denotes the Pi4 internal root hub, 1 denotes an external hub, 2 denotes a cascaded hub. `CAP=01` means the scan was cut short by the elapsed-time budget, not that the drive is ready.
6. The Pi4 R110 1280×720p50 HDMI, integer scaling, scanlines, AGABOOT protocol and RW emmc overlay are unchanged. The Pi3 implementation is **not used** for Pi4 native USB host operations.

## Test procedure

- Cold boot the Pi4 with the known-working FAT32 USB drive attached as in R114 and let the screen settle. Keep the working mouse dongle/hub attached.
- Compare **SCSI TRY**, **MSC READY**, **BOT ERR**, **HOST RUN/SCAN**, **SCAN DEPTH / PORT / CAP**, and **HID**. Send a screenshot after the scan finishes; if it does not, the progressing depth/port indicates where it stopped.
- `MSC READY=01` validates SCSI geometry only. It **does not** confirm FAT32 mounting. Next run `C:usbmass-z2-pi4` followed by `C:usbmass-info-pi4` inside AmigaOS to verify FRUM and read-only LBA0. Then check `DEVS:usbmass.device`, `L:fat95`, and `Storage/DOSDrivers/USB0` or `DEVS:DOSDrivers/USB0` on the real AmigaOS partition.
- If `SCSI TRY>0` and `BOT ERR=01,02,03,04,05`, the Pi4 xHCI transport is failing (not FAT32). `BOT ERR=06` means valid SCSI CHECK CONDITION, usually media not yet ready.
- If `CAP=01` and HID=00, the diagnostic timeout may have shortened enumeration before all input interfaces were registered; a later build will need incremental USB discovery rather than suppressing ports.

## Installation

Copy `BOOT/emu68-pi4-r115-early-msc-scan-budget.img` to the verified Pi4 FAT32 boot partition. Change only the active `[pi4]` `kernel=` entry to that filename. Preserve `hdmi_mode=19`, `hdmi_group=1`, `dtoverlay=emmc,unit0=rw`, RDB partition and all previous kernel files.

**Rollback:** `kernel=emu68-pi4-r114-usb-no-stall-live-post.img`. R113, R112 and R110 remain available.

The test package includes the same **read-only** AmigaOS `usbmass.device`, `usbmass-z2-pi4`, `usbmass-info-pi4`, and the Pi3-proven FAT32 `USB0` DOSDriver **for reference and manual integration**. Copying them to the Pi boot partition will not automatically install them into the AmigaOS file system.

**Validation:** Pi4 big-endian ARM64 compile, real native MSC worker tested against host BOT mock, R114 fail-closed, late spin-up, R115 early capacity and persistence, R112 24-slot topology, R110 AGABOOT/scanline protocol checks. Actual Pi4 drive detection, AmigaOS sector reading and FAT32 mounting still require hardware tests.

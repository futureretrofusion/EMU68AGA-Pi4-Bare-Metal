# FRF EMU68AGA Pi4 R114 — USB no-stall + live POST diagnostic

**Experimental Pi4-only test. Physical Raspberry Pi 4 boot and FAT32 mass-storage operation are not yet verified.** Derived from R113 (mouse HID/POTGOR diagnostic) + R112 (native Pi4 read-only xHCI MSC support) while keeping R110 AGABOOT/scanline exceptions and native 720p50 video code unchanged.

## Why R113 could delay startup and show all-zero POST

1. When a drive connected, the R113/112 CPU1 storage worker called USB **SCSI READ CAPACITY synchronously during USB startup**. Each Bulk-Only Transport transfer stage could wait **3,000 ms**, making the three-stage transaction potentially block the shared HID/MSC worker for several seconds.
2. The storage worker retried a failed capacity command up to **12 times** at two-second intervals, whether the failure was a legitimate SCSI **CHECK CONDITION** or an xHCI bulk transfer timeout. A dead endpoint could therefore stall the worker again and again. Actual HID event processing is on that same ARM CPU1 worker.
3. Root USB enumeration was configured for up to **five** failed reset/address retries per port, including recovery delays. The R114 diagnostic variant caps these to **three** attempts for the first device and **two** for ports scanned after at least one HID interface was already registered.
4. The persistent HDMI USB HUD in R113 was inadvertently suppressed by **FRF_RELEASE_QUIET** even though it was explicitly requested in the diagnostic build.
5. R113's enlarged persistent HUD panel was **337 pixels tall**, but the source AGA framebuffer is only **286 lines**, so the added mouse/status rows could never appear. Some 16-bit boot POST values also reflected only the initial zero snapshot.

## R114 corrections

- **Deferred SCSI:** USB enumeration returns to AmigaOS without waiting for SCSI READ CAPACITY. The first capacity probe is scheduled 250 ms after CPU1 begins ordinary input polling.
- **Bounded bulk:** test-mode xHCI Bulk-Only Transport completion timeout reduced from 3,000 ms to **350 ms per stage**. No HID control/interrupt transfer timeout is modified.
- **Fail closed:** a CBW/DATA/CSW timeout, bad signature or tag/residue mismatch permanently marks that MSC transport as failed. No repeated probes or reads are submitted to that suspect endpoint. Subsequent guest requests fail immediately instead of repeatedly waiting. USB keyboard/mouse polling remains enabled. A cold reboot resets this test state.
- **Retry genuine SCSI not-ready:** when a **valid CSW** carries a SCSI CHECK CONDITION (BOT error stage 6), the worker retries READ CAPACITY after two-second delays, up to twelve attempts. Only this class of failure gets repeated probes.
- **Shorter root reset/address retries:** 3 attempts for first USB device; 2 when a HID device is already live. This is enabled only by the Pi4 diagnostic build; older working kernels are untouched.
- **Persistent live POST:** explicit diagnostic HUD now bypasses the release-quiet suppression, and the reorganised panel fits wholly within **800×286** AGA source pixels. It samples the live counters every rendered AGA frame; it never pokes USB hardware or the 68k guest.
- **Boot POST updates:** SCSI probe results refresh the original 16-bit boot POST panel; the no-HID path publishes an error stage rather than leaving an ambiguous untouched zero snapshot.
- **R113 RMB diagnostics retained:** live USB raw mouse bytes, interpreted button mask, left/right click counters and emulated POTGOR state.

## Read R114 POST

On the **live AGA diagnostic panel** look for these entries:

| Field | Interpretation |
| --- | --- |
| HOST RUN/SCAN | CPU1 native USB worker finished basic startup or is still enumerating |
| HID, KB, MS, GP | Registered interface counts; zero may mean no device or failed enumeration |
| USB ROOT PORT / XHCI STAGE | Hub status and last enumeration stage, including error codes |
| MSC SEEN | USB mass-storage **BOT** interface discovered, independently of filesystem |
| MSC CFG | xHCI bulk endpoints successfully configured |
| READY | SCSI capacity obtained (512-byte sectors, one LUN) |
| MSC VID:PID | Last storage VID:PID discovered |
| MSC SLOT / UAS | Configured bulk slot; count of unsupported UAS interfaces |
| SCSI TRY / BOT ERR | Capacity attempts / last failure stage |
| MSC TRANSPORT | NOT SEEN, CONFIG ERR, PROBING, READY or FAIL CLOSED |
| CFG ERROR / BOT FAIL | Endpoint setup failures or fatal transport failures |
| MOUSE RAW / BTN / L / R | Live wireless-dongle mouse report and click counters |
| AMIGA POT / RMB | Guest-emulated Paula right-button state |

**BOT ERR:** 1=CBW OUT failure; 2=DATA IN failure; 3=CSW IN failure; 4=bad CSW signature; 5=tag/residue mismatch; 6=valid SCSI CHECK CONDITION. BOT errors 1–5 are fail-closed, not endlessly retried.

The **boot splash** POST has MSC SEEN/CFG/READY/UAS/TRY/BOT STAGE. The live HUD is drawn through the native AGA renderer; if P96/RTG owns the HDMI display, the native AGA HUD cannot override P96's screen. It is still available on AGA screens or after switching back from RTG.

## What remains unsupported

- **USB Mass Storage is read-only**, one device, LUN0, 512-byte logical blocks, SCSI READ(10), READ CAPACITY(10) (up to 2 TiB). No hotplug, auto-mount, writes, multi-unit, or UAS protocol.
- A slow USB bridge requiring over 350 ms for a BOT stage may be **reported as failed instead of delaying the OS**. This is a diagnostic/performance tradeoff, not a guaranteed drive-compatibility fix.
- A zero MSC READY counter does not prove the FAT32 filesystem is bad. Determine MSC SEEN then CFG first.
- The existing AmigaOS driver is the tested FRUM v1 **read-only** usbmass.device. Copy to actual AmigaOS `DEVS:` and use the included `usbmass-z2-pi4` and `usbmass-info-pi4` test programs from actual AmigaOS `C:`. Files staged on the Pi boot FAT partition are not automatically installed inside the Amiga RDB partition.
- The existing Pi3 `USB0` DOSDriver uses `FileSystem = L:fat95`, `Device = usbmass.device`, `Unit = 0` and `DosType = 0x46415401`. `Storage/DOSDrivers/USB0` in this kit is only a **reference**. Do not automatically add it to DEVS: or WBStartup until native MSC READ10 and the guest FRUM bridge are confirmed.

## Test sequence

1. Cold-boot the Pi4 with the same FAT32 USB drive and your working USB mouse/hub attached. Compare the time between Wi-Fi connection and completion of Workbench startup against R113; do not assume the delay is fixed until timed on hardware.
2. Read POST: **MSC SEEN → CFG → READY** and XHCI STAGE or BOT ERR. A photograph after startup is ideal. If no USB devices show, the stage code is more useful than the count.
3. Move and right-click with the wireless mouse. The new HUD should show `BTN=02`, increased `R`, `RMB=01` if the HID and chipset paths work.
4. When READY becomes nonzero, run `C:usbmass-z2-pi4`, then `C:usbmass-info-pi4` on AmigaOS (test files are included). This verifies the Zorro mailbox and raw sector 0 without writing to the USB drive.
5. **Only after successful sector reads**, compare and install the Pi3 `fat95` FAT32 `USB0` DOSDriver on the AmigaOS volume with the correct partition layout.

## Pi4 boot installation

Copy `BOOT/emu68-pi4-r114-usb-no-stall-live-post.img` to the **correct** Pi4 FAT32 boot partition and update **only** the active `[pi4]` kernel line:

```ini
kernel=emu68-pi4-r114-usb-no-stall-live-post.img
```

**Do not modify** `hdmi_group=1`, `hdmi_mode=19` (1280×720p50), integer scaling, AGABOOT, scanline commands, `dtoverlay=emmc,unit0=rw` or `overlays/emmc.dtbo`. Keep R110, R111, R112 and R113 kernels for rollback.

**Rollback:** `kernel=emu68-pi4-r113-mouse-rmb-diag.img` (previous diagnostic), `kernel=emu68-pi4-r112-usbmass-detect-test.img` (previous MSC), or `kernel=emu68-pi4-r110-agaboot-control-only-test.img` (hardware-confirmed scanline base).

## Software checks

- Pi4 big-endian ARM64 cross-build
- Real MSC worker in a host bulk-transport mock: deferred init, SCSI CBW/CSW and READ CAPACITY, bounded spin-up retry, raw LBA reads, reject writes, quarantine bad CSW, fail closed on stalled CBW with no additional transport attempts
- R112 topology regression (8 HID interfaces, 24 slots, no premature HID-full stop)
- R110 AGABOOT protocol regression (101 scanline settings and all picture command selectors)
- R114 HVS layout assertions that all diagnostic rows fit inside the 800×286 frame

**Hardware validation and FAT32 mount remain outstanding.**

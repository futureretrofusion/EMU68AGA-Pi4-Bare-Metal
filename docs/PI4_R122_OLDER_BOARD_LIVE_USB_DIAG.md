# EMU68AGA Bare Metal Pi4 — R122 LIVE older-Pi4B USB diagnostic

**Experimental diagnostic kernel — 14 November 2026.** The **4 GB Raspberry Pi 4B PCB revision 1.1** still reports no USB input. R122 is a separate diagnostic image derived from R121/R117, **not a demonstrated fix**. Hardware should be tested on both old rev1.1 and known-working newer rev1.5.

## What this build does

- **Persistent USB diagnostic overlay ON**, from native AGA video startup and continuously while the native AGA display is active. Unlike R121's quiet boot, it is **not hidden after AmigaOS starts** and does not require a new config overlay.
- Early, detailed **boot POST ON**, so a photograph of a stalled boot can show PCIe/USB progress even if Workbench never appears.
- Prominent `FRF PI4 R122 OLD USB DIAG` title.
- Older VL805 firmware / PCIe compatibility instrumentation from R117, plus added USB root-poll results and address/enable completion codes.
- R115 read-only native MSC host, R117 rev1.1/1.2 guarded firmware reset notification, Pi4 HID and hubs code remain. Write commands disabled for diagnostic safety.
- The current normal **R121 USB read/write kernel remains unmodified**. Do not replace its `usbmass.device` or change AmigaOS DEVS: for this board-level diagnostic.
- **HDMI sound, 1280×720p50, integer scaling, scanlines and AGABOOT** code are the same as R121. All platform code remains Pi4-specific.

**Display limitation:** The continuous panel is composited into the Pi4's native AGA frame. It remains on native AGA Workbench and games, but **an external P96/RTG display list can take ownership of HDMI**; this diagnostic does not force an overlay over RTG because doing so could hide the RTG desktop. The boot POST is an earlier separate phase.

## Boot setup

On a backed-up Pi4 microSD card with FAT32 boot partition, copy the image:

`emu68-pi4-r122-legacy-usb-live-diag.img`

into the root alongside the existing R121 and R117 kernels. Change **only** the active Pi4 kernel entry in `config.txt`:

```ini
[pi4]
kernel=emu68-pi4-r122-legacy-usb-live-diag.img
```

Do **not** replace the entire `[pi4]` section or `config.txt` with this small sample. Preserve HDMI 1280×720p50 mode (`hdmi_group=1`, `hdmi_mode=19`, `hdmi_drive=2`), scanline settings, integer scaling, AGABOOT, Amiga RDB and all existing boot overlays. No boot EEPROM or VL805 EEPROM updates.

To return to the verified quiet RW build, restore the **original** `[pi4]` line:

`kernel=emu68-pi4-r121-usbmass-rw-test.img`

The diagnostic image does not enable writes to USB storage. Your normal R121 USB RW setup can be restored by selecting the R121 kernel.

## How to test the old 4GB Pi4B rev1.1

1. Cold power off. Insert the R122 card and attach a simple USB keyboard and mouse **directly** to Pi4 USB-A, not via a hub.
2. Keep the micro-HDMI cable and monitor unchanged. The USB boot POST appears on startup; the **R122** persistent panel remains on the native AGA screen.
3. Photograph the complete boot POST and then the persistent diagnostic panel, **especially if neither keyboard nor mouse responds**. Allow the USB scan to complete; older boards use an extended diagnostic enumeration budget.
4. Repeat with **only the keyboard** plugged in. Then, if needed, a different USB-A port. If HID works only without a hub, test a hub separately.
5. Run the **same kernel and SD card** on the known-working Pi4B revision 1.5 with the same keyboard and mouse, and send comparable photographs.

## Persistent panel fields

| Field | Explanation |
|---|---|
| `BOARD` / `PCB` / `RAM` | Firmware revision register and decoded Pi4B PCB/RAM code; distinguish rev1.1 from rev1.5 |
| `FW PRE` / `POST` | PCI config VL805 firmware version before and after conditional firmware notification |
| `PCIE` | PCIe root-controller link/status bits |
| `REC` | 00=no recovery attempted, 01=in progress, 02=capability recovered, 03=recovery failed |
| `DMA` | Inbound DMA mapping rejection mask; nonzero merits investigation |
| `HOST` / `ERR` / `XHCI STAGE` | Host startup, error and last completed xHCI stage |
| `HID` / `KB` / `MS` / `GP` | Native USB HID enumeration and keyboard/mouse/gamepad counts |
| `ROOT` / `P1` | Root port status and initial port-status snapshot when no keyboard or mouse appears |
| `POLL` / `FAIL` | Root port polls succeeded/failed; **displayed instead of button stats when no HID** |
| `P1 EN` / `ADDR` / `PWR` | Port 1 enable and address completion code, and root-port power cycle count |
| `FWRC` / `CAPS` | Firmware notification return code and raw xHCI capability register if no HID |
| `MSC SEEN` / `CFG` / `READY` | Drive discovery, USB config, SCSI media readiness (separate from HID failure) |
| `BOT ERR` / `USB PHASE` | Mass transport code and USB host progress |

**Useful distinctions:**

- `BOARD=00000000`: firmware did not supply a standard revision DT property; conservative legacy-specific recovery is not attempted. Don't mistake this for a confirmed newer Pi4.
- Firmware nonzero but `CAPS` invalid suggests xHCI capability decode or PCIe MMIO mapping, not an AmigaOS keyboard mapping error.
- Root port reporting no connection with powered devices suggests VBUS, reset/port enable, firmware or cable behaviour; we need the full `ROOT/P1/PCIE` values.
- A detected port with a failed `P1 EN` or `ADDR` code points to xHCI slot/endpoint bring-up. Even `HOST RUN` does **not** prove HID configured.
- `HID>0` but `KB=0` or `MS=0` points to descriptor parsing or input classification.

## Build verification

Built with `FRF_PI4_XHCI_HID=ON`, `FRF_PI4_USB_MASS_RO=ON`, `FRF_PI4_USB_MASS_RW=OFF`, `FRF_PI4_MOUSE_DIAG=ON`, `FRF_PI4_LEGACY_REV_DIAG=ON`, `FRF_PI4_SUPPRESS_DIAGNOSTIC_POST=OFF`, `FRF_BOOT_POST=ON`, `FRF_RELEASE_QUIET=ON`.

Native big-endian AArch64 Pi4 build and existing R115 MSC, R117 board-revision, R112 USB topology and R110 AGABOOT tests all pass. **No physical older-Pi4B USB result yet.**

The file is a diagnostic test image, **not a stable release or a confirmed fix**.

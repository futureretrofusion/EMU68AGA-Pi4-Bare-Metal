# Pi4B rev1.1 USB firmware handoff diagnostics — R125 (R115-derived)

**Experimental, 14 November 2026. Hardware boot NOT YET confirmed.** R117 and R122 were reported black-screening on the older Pi4B. R125 is deliberately built from the earlier **R115** USB implementation (commit `4addde6`), not from R117/R121/R122. The package also includes an **untouched, SHA-verified R115** fallback. Both kernels retain the live native AGA USB diagnostic panel.

## Verified hardware difference

Official Raspberry Pi documentation says Pi4B PCB revision **1.3 and earlier** use a **separate VL805 USB firmware EEPROM**; revision **1.4 and later** load firmware via the bootloader EEPROM. This was verified against:
- https://github.com/raspberrypi/documentation/blob/master/documentation/asciidoc/computers/raspberry-pi/boot-eeprom.adoc
- https://github.com/raspberrypi/rpi-eeprom/blob/master/imager/README-2711.txt

The actual older-board firmware version is still unknown. Raspberry Pi OS's non-mutating `sudo rpi-eeprom-update` shows whether firmware is using the separate VL805 EEPROM; never use `-a` until a decision to update firmware has been made. Read PCI config with `sudo setpci -s 01:00.0 50.L`, and `lspci -nn -s 01:00.0`, if available. An `0x000138c0` value on a newer board is not proof of the older board's version.

## Root-cause investigation

On the existing R115 source:
- `root_prepare()` **avoids PCIe fundamental reset when the link is already live**; it repairs inbound DMA mapping and keeps the outgoing PCIe link.
- It reads firmware at PCI config `0x50`. If the value is nonzero it does NOT send `NOTIFY_XHCI_RESET`.
- If `0x50 == 0`, it sends a firmware property tag `0x00030058` with PCI dev address `0x00100000` once, and restores BAR0 / PCI command bits.
- Original R115 waited only up to **200 ms** for firmware `0x50` to become nonzero. Original xHCI host waited **500 ms** each for reset completion and `CNR` clearing.
- Crucially, the **original firmware mailbox send and receive helpers have no timeout**; a firmware failure or unavailable response can stall the CPU1 USB setup indefinitely.
- R117 added a second guarded firmware-reset notification on early boards after failed xHCI caps validation. R125 intentionally **does not carry that change**.

This explains a credible failure mechanism, but it **does not prove that mailbox or xHCI timeout caused the old Pi4's black screen**. Boot logs and hardware tests are still necessary.

## R125 changes — bounded, no risky recovery loop

- Adds **timeout-limited mailbox transmission (500 ms) and reply (5000 ms)** only to the xHCI firmware reset notification. `-3` means TX timeout (no command sent), `-4` RX timeout (the firmware may have seen the command; no retry).
- Keeps one notification only if `PCI config 0x50 == 0`. **No unconditional notify**, no second notify, no EEPROM writes and no additional fundamental PCIe reset.
- Polls for firmware register `0x50` up to **3000 ms** after an actual notification.
- Waits up to **2000 ms** for xHCI HCRST and **2000 ms** for `CNR` clear. Existing timeout/error handling remains; no infinite poll.
- Samples board revision from the FDT `/system/linux,revision`, avoiding an additional VideoCore mailbox request from the USB core.
- The always-on native AGA HUD shows: `FW0` (pre-notify firmware), `FW1` (post-notify), `N` (notify result), `PH` (progress), `MS` (notify latency), `CAPS` (raw xHCI capability), `STS` (xHCI USBSTS after CNR), `BOARD` (device-tree hardware revision), `CNR` (CNR wait ms), when no HID is seen. The preceding rows retain USB `HOST`, `HID`, `KB`, `MS`, `MSC`, `USB PHASE` and error metrics.
- With working HID input, the last rows revert to existing mouse/joypad counters.
- The HUD is permanent for **native AGA** output. An external RTG/P96 display can replace the AGA output; the diagnostic cannot cover it.
- R115 read-only USB storage, HDMI audio, existing 1280×720p50 output, AGABOOT picture controls, scanlines, integer scaling and USB HID code are unchanged.

**R125 POST `PH` fields:** 01 = entering xHCI probe; 02 = controller mapped, firmware version decision; 03 = VideoCore notification in progress; 04 = notification returned; 05 = polling firmware complete; 06 = capabilities valid; E6 = capabilities invalid. `N=7FFFFFFF` means notification was skipped because firmware was already present. `N=FFFFFFFC` is mailbox RX timeout; `N=FFFFFFFD` is TX timeout. `STS` bit 11 is CNR (controller not ready), bit 0 is halted.

## Safest boot procedure

The preferred **first A/B recovery** is the original R115 file in `FALLBACK_ORIGINAL/`, with no source changes; its SHA-256 must be:

`8d13476e73fc6212c025ee66bab579fc00cae46401977213bb84665605f2e5dd`

Copy either image into the root of an existing **Pi4 EMU68 FAT32 boot partition**, and change **only** the `kernel=` line inside the existing `[pi4]` section of `config.txt`. For original R115:

`kernel=emu68-pi4-r115-early-msc-scan-budget.img`

For the experimental bounded-mailbox R125:

`kernel=emu68-pi4-r125-vl805-bounded-handoff-diag.img`

**Do not** change the entire config, HDMI mode, AGABOOT, flash EEPROM, format the SD card or overwrite the Amiga RDB partition. The current R121 and R123 newer-Pi4 builds are separate. Both R115 and R125 include native USB diagnostic output without any `dtoverlay=diagnostic,hud` command.

1. Try **original R115 first**, with one wired USB keyboard plugged directly into the older Pi4B. Record whether it reaches the boot splash and native AGA POST.
2. If R115 boots, test **R125** on the same older board and power supply; take clear photographs of the live diagnostic panel (particularly FW0/FW1/N/PH/CAPS/STS/BOARD/CNR).
3. Compare R125 against the known-working newer Pi4B with the same USB devices and cable if possible.
4. If R115 *also* black screens, stop substituting newer USB code; check Pi boot card/config/firmware with a known-working Pi4 image and a separate U-Boot or Raspberry Pi OS USB test. A black screen can occur before the native AGA HUD is available.

## Software validation

- Cross-compiled ARM64 **big-endian** `Emu68.img` for Pi4 from R115 source.
- Native xHCI USB HID and read-only mass-storage code included; **no write support**.
- R110 AGABOOT and scanline tests, R112 topology, R115 BOT MSC transport tests, and R125 static firmware timeout checks pass.
- Build CMake flags: `FRF_PI4_XHCI_HID=ON`, `FRF_PI4_USB_MASS_RO=ON`, `FRF_PI4_MOUSE_DIAG=ON`, `FRF_PI4_SAFE_VL805_NOTIFY=ON`, `FRF_BOOT_POST=ON`, `FRF_RELEASE_QUIET=ON`.
- **No direct boot test on physical Pi4B rev1.1 yet.** Do not promote as a fixed release until a successful old-board test and repeatable USB enumeration.

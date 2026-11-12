# EMU68AGA Bare Metal Pi4 R105 — unmodified-board USB2 compatibility

This is a **test kernel**, branched from the known-working Pi4 R104 build (`pi4-v121-r104-version-post`). It preserves the Pi3 v1.121 HID-port, HVS scanline and POST-version changes. R104 is not modified.

## Problem targeted

Some Pi4B boards have intact blue USB3 ports; one such user reported `NO USB`, while the developer's Pi4 with its USB3 connectors previously removed reports working keyboard/mouse. That physical difference may affect VL805 link/port timing or which root ports report connected first. It is *not* evidence users should remove USB3 ports. The original host startup picked the highest-ranked root and could fall back to SuperSpeed. If no supported HID was enumerated, CPU1 parked forever while POST still said ONLINE.

## R105 changes

- Parse xHCI Supported Protocol extended capabilities to identify **USB2** versus **USB3** root-port ranges when advertised. Fall back to USB speed-ID classification when capabilities are absent.
- Scan all reported root ports, **never choose SuperSpeed** as the primary boot HID root when looking for the Pi4's internal USB2 hub. Prefer an attached high-speed root with a short 100ms settle; allow a full 2500ms discovery period for low/full-speed alternatives or slow USB2 hub power-up.
- If the controller is already running but no root USB2 device was attached, CPU1 polls for an eligible later USB2 connection every ~2 seconds and attempts root-port reset/attach **without restarting VL805 HCRST**.
- If the internal USB2 hub was enumerated but no native HID was found, CPU1 polls its port status and rescans only on a new connection (or reconnect), only when no native HID or mass-storage operation is active. It will not tear down a live keyboard.
- Native HID direct-root fallback is shared between initial and late-attach scans.
- R105 boot POST uses separate state flags: `HID READY` only when supported input exists, `WAIT USB2` when no root selected, `WAIT HID` for an empty hub, `HOST ERROR` if controller startup failed. Added `USB2 ROOT` and `XHCI ERR` two-digit diagnostics.
- POST versions are `FRF PI4 BARE METAL: R105` and `AGA-STORM PORT: v1.121`.

## Test sequence: unmodified Pi4

1. Back up the currently bootable FAT32 `emu68-*.img` and leave `config.txt` and Amiga storage untouched.
2. Boot R105 with a wired USB keyboard connected to a **black USB2** socket before power-on; photograph POST. Expected: `HID READY`, nonzero keyboard count.
3. Reboot with the same keyboard in a **blue USB3** socket. USB2 backwards compatibility should still expose the HID interface through the onboard USB2 hub.
4. Connect a separate USB3 device in a remaining blue port, leaving the keyboard on black USB2. Test whether `USB2 ROOT` still identifies the USB2 hub and keyboard works.
5. Reboot with **no keyboard**, then attach one to black USB2. Expected `WAIT HID` followed by `HID READY` if the hub was initially empty.
6. Recheck Microsoft Nano Transceiver `045E:0800` keyboard + wireless trackpad and AGABOOT native scanlines as R104 regressions.
7. For failures, record both `USB2 ROOT` and `XHCI ERR` plus the actual `KEYBOARD`, `MOUSE`, and `HID` counts. Those separate root-discovery errors from HID report decoding.

## Scope / limitations

- Successfully cross-built with AArch64 big-endian GCC Release settings; this is **not hardware validation** on stock Pi4 boards.
- This fixes the **initially empty/no-HID USB2** recovery path and protects currently active HID endpoints. It does not yet provide general USB3-only HID enumeration, arbitrary multi-tier USB hotplug, or dynamic replacement after active HID devices disconnect.
- R105 does not reinitialize the VL805 after initial ownership. Hardware-controller startup failure stays fail-closed; display reports HOST ERROR rather than pretending USB works.
- Keep R104 as immediate rollback. No change to the Pi3 build or Amiga partitions.

## Developer diagnostics

Logs of interest: `[R105-PORT] roots=... USB2=... USB3=...`, `[R81-USB2] candidate root...`, `[R105-USB2] late USB2 hub connect...`, `[R105-USB2] recovered HID without HCRST`.

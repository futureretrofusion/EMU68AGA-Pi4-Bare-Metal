# Pi4 R108 — honest deferred USB POST, proven R107 HID preserved

## Hardware evidence from 2026-11-13

The tester reports that the **five-port USB hub on a Pi4 USB2 port works with keyboard, mouse and wireless gamepad simultaneously** on R107. When this hub is connected, the other physical Pi4 USB2 port fails to respond. The boot POST displays zero USB devices although all hub-connected HID input works after boot.

## Confirmed source explanation

Pi4 USB host initial discovery is started on CPU1 *after* the Amiga guest starts, followed by another two-second delay in `frf_pi4_xhci_hid_main`. The boot-splash POST is painted before this enumeration, so counts of 0 on the splash are **not proof of missing USB**.

The R107 native USB implementation performs a one-time scan of the internal 4-port USB2 hub. It snapshots `port_connected[]` once at boot before enumerating external hub children. The quiet release does not run the R99 diagnostic link monitor, and even that monitor does **not** enumerate newly plugged devices. Later USB2 attachments therefore lack a general hotplug enumeration path.

Native HIDs are limited to `FRF_NATIVE_HID_MAX=4` endpoints; hubs may expose multiple interfaces. For a separate physical-port failure, the next discriminating test is to connect the hub and the extra wired HID **before power-on**, then compare with hotplug after the Amiga is running. If even pre-boot fails, inspect four-HID exhaustion, child DMA bank allocation, root hub EP0 restoration after r97 recursion, and the R90 per-port status/Address Device error snapshots. Do not rewrite or reset a live controller without evidence.

## Changes in R108

Based directly on R107 (commit 5137fea). Changes **only**:
- POST status displays `DEFERRED` / `SCAN AFTER BOOT` while worker has not scanned
- Keyboard, mouse, gamepad, HID and external-hub counts show `--` rather than false zero until scan completed
- Root and xHCI error fields show `--` before controller initialization
- Label becomes `R108`.

**No USB/xHCI/HID driver code changed; R107 remains the stable baseline.**
This does not display live counts after AGA takes over, and it does not fix physical-port hotplug.

## Tests

Cross-build passes, USB code source byte-identical to R107. Hardware testing of R108 pending. Do not publish as a functional host-hotplug fix.

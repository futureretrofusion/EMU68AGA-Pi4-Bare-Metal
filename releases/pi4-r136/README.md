# Pi4 R136 — ARM-native USB HID hotplug, external USB2 hub support (EXPERIMENTAL)

**Experimental pre-release • 22 November 2026 • NOT verified as a working mouse-hotplug fix.**

Targets Raspberry Pi 4 / Pi 400 EMU68AGA Bare Metal, not Pi3 or Pi5. USB is implemented directly on ARM through VL805/xHCI, not AmigaOS Poseidon.

## Changes

- R135 implemented selective hotplug recovery on the Pi4 internal USB2 hub. The user's Pi4 test FAILED: mouse still needed ARM reset after reconnection.
- Source review revealed that external USB hub HID devices lacked physical parent-hub/port identifiers and the runtime monitor only polled the internal hub.
- R136 records the parent xHCI hub slot and downstream port for each boot-detected HID interface.
- It polls connection-change status on up to two chained external USB2 hubs as well as the internal hub, then attempts to rebuild the affected HID xHCI slot and interrupt endpoints, retaining input indices and unrelated USB devices.
- The existing R133 multi-drive USB storage code is enabled. Bundle includes optional AmigaOS usbmass.device v0.6 and read-only diagnostics.

## Unverified functionality / limitations

**R136 hotplug has NOT yet passed the Pi4 unplug/replug mouse test.** This is a candidate for testers, not a confirmed hotplug fix.

- HID reconnection requires the SAME boot-detected mouse/keyboard in the SAME port. New HID devices and unplugging/replugging an entire external hub are not supported.
- USB mass-storage HOTPLUG is not implemented. Attach storage before starting the Pi4.
- R133 supports up to four USB BOT devices and eight logical units in code. Secondary logical units and RDB media remain read-only, and unit index can change between boots.
- Raw Amiga RDB is detected but PART/FSHD auto-mount and bootnode installation are NOT implemented. PFS3/RDB initialization remains unresolved.
- usbmass.device v0.6 does NOT implement HD_SCSICMD needed by PFS3 Direct SCSI PDS/3. Do not format, initialize or repair an RDB disk in response to a mount failure.
- Existing Pi4 display settings (1280x720p50, integer scaling and scanlines), Amiga ROMs and RDB partition should be kept unchanged.

## Install Pi4 boot kernel

1. Back up the FAT32 boot partition and config.txt. Leave the Amiga RDB partition alone.
2. Copy BOOT/emu68-pi4-r136-native-hid-hub-hotplug.img to the Pi4 FAT32 boot partition root.
3. In the existing [pi4] section of config.txt change ONLY the kernel selection to:

    kernel=emu68-pi4-r136-native-hid-hub-hotplug.img

4. Boot with keyboard, mouse, gamepad, USB hub and storage already connected.
5. Unplug only the mouse for at least 3 seconds; reconnect the same mouse to the SAME USB port. Check mouse movement, both buttons, and uninterrupted keyboard/gamepad/USB storage. If possible repeat directly in a Pi4 socket.
6. Report Pi4 revision, hub VID:PID and mouse VID:PID, devices attached, result, and any POST/USB diagnostics.

Rollback: select retained emu68-pi4-r135-native-hid-hotplug-test.img or another known-working Pi4 kernel in [pi4]. Do not delete rollback images.

## Optional AmigaOS storage driver / diagnostics

- DEVS/usbmass.device is R133 **v0.6**, NOT a new R136 guest driver. Save the prior device, install to DEVS: and cold reboot only if testing the experimental multi-device path.
- C/frf-usb133-units: read-only logical-unit and raw RDSK listing.
- C/frf-usb137-rdbdiag: read-only RDB PART, FSHD, PFS3 identifier and partition-boundary diagnostic. Cross-compiled but not hardware-tested.
- Never write to unknown USB logical units/RDB media. Do not mount duplicate DOSDrivers for the same device.

## Validation and release status

- Big-endian ARM64 kernel cross-build: PASS.
- R133 multi-device / multi-LUN mass-storage host regression: PASS (mock, not physical hardware).
- AmigaOS R137 diagnostic cross-build: PASS (not yet hardware tested).
- R135 mouse physical reconnect: FAIL (user hardware report).
- R136 physical reconnect: NOT YET VERIFIED.
- PFS3/RDB mount: UNRESOLVED.

This release is deliberately tagged as PRE-RELEASE, not stable.

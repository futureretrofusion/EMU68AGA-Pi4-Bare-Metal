# Pi4 native xHCI — v1.121 HID integration (development branch)

Source reference: EMU68AGA Bare Metal Pi3 v1.121 RC13, archive
`EMU68AGA_BARE_METAL_v1.121_RC13_DF1_PROTOCOL_TEST/SOURCE/EMU68AGA_v1.121_RC13_source.tar.gz`.
Pi3 controller remains DWC2/TinyUSB; the Pi4 VL805/xHCI controller remains native.

## Integrated for Pi4
- Pi3 v1.121 `frf_r23_keyboard_guard.h` brought across unchanged; eight-byte keyboard and Nintendo Pro gamepad false-key safety.
- Pi3-style per-*interface* HID registration (rather than one HID per USB receiver).
- Separate xHCI interrupt IN endpoints/transfer rings within a shared device slot; both boot keyboard and mouse interfaces configured together using one Configure Endpoint command.
- Single USB SetConfiguration per composite receiver, SetProtocol(BOOT) per boot HID interface.
- Shared control endpoint ring state synchronised after fetching per-interface report descriptors.
- Handles 3-byte boot mouse reports and 4-byte Report-ID+mouse reports (report ID verified against actual HID report descriptor).
- Existing Pi4 single-interface, gamepad, hub, and boot paths preserved.

## Known Microsoft Nano Transceiver v2.0 receiver
USB 045e:0800 (inspected on Studio Main on 12 Nov 2026):
- HID interface 0 / keyboard / endpoint 0x81 / 8 bytes / report descriptor 57 bytes
- HID interface 1 / mouse or trackpad / endpoint 0x82 / 10 bytes / report descriptor 223 bytes (contains mouse report ID 0x1a)
- HID interface 2 / vendor or consumer controls / endpoint 0x83 / 32 bytes / report descriptor 319 bytes

The Pi4 HID registration includes interfaces 0 and 1 and deliberately does not interpret the vendor/consumer-control interface as gamepad input.

## Hardware verification required
The AArch64 big-endian cross-build is checked, but no Pi4 peripheral hardware run has been performed for this branch. Confirm the keyboard still works, the trackpad moves and clicks, and other USB devices enumerate. In boot log expect two `[R121-HID] LIVE` records on one slot, protocol1 and protocol2, with different endpoint numbers and rings. The `[PI4-HID]` log will report descriptor lengths and report IDs.

This development test is separate from stable Pi4 R101. Preserve the current CaffeineOS FAT32 `emu68-*.img` and `config.txt`. To try the test image, rename it to precisely the existing configured `emu68-*.img` name, retaining the saved original for rollback. Do not modify the ROM bundle or Amiga partitions.

This is an HID compatibility integration, not an unreviewed bulk copy of the Pi3 SOC, DWC2 or framebuffer drivers into the Pi4 hardware target.

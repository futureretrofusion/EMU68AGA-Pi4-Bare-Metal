# Pi4 R106 — fix R105 zero-speed root-port regression

## Confirmed source defect

The known-working Pi4 startup diagnostic includes xHCI PORTSC `0x400202E1`. Its CCS bit is set (device connected) and its speed-ID field, bits 13:10, reads **0**. R104 accepts this initial connected root and resets it, allowing the VL805 USB2 hub to enumerate. R105 incorrectly treats speed-ID 0 as ineligible, so it may skip the only attached root and never detect any keyboard, mouse, or gamepad.

R106 allows a connected **unknown-speed** (speed 0) root port to progress to port reset, prioritizing known USB2 high-speed roots and xHCI protocol-tagged USB2 candidates when available. It still avoids choosing a root known to be SuperSpeed (speed >= 4). The xHCI Supported Protocol masks are **preferences**, not mandatory filters.

Initial R105 xHCI classification logic, empty-host idle polling, truthful POST error state, two version labels (R106, AGA-Storm v1.121), native wireless HID compatibility, and v1.121 native HVS scanline port remain.

## Validation and next test

- Compiled AArch64 big-endian Emu68 successfully; expected known-linker RWX warning remains.
- Static regression: connected PORTSC 0x400202E1 -> speed0 -> **eligible**; speed 4 root remains excluded.
- NOT YET VALIDATED ON PI4 HARDWARE.
- The known-working R104 image is unchanged. Preserve backup before testing R106.
- Boot with a wired keyboard in a black USB2 port. Test wireless mouse after keyboard; then blue USB3 socket with a USB2 HID device. Record POST 'USB HOST', 'USB2 ROOT', 'XHCI ERR', 'KEYBOARD' and 'MOUSE' if any failure.
- Avoid offering R105 to affected users; it has a known source-level regression.

## Source history

Branch `pi4-r106-usb-unknown-speed-fix` based on `pi4-r105-usb2-compat`. Main changes `machine/Emu68/src/frf_xhci/frf_xhci_host.c`, version header `machine/Emu68/include/frf_pi4_post_versions.h`.

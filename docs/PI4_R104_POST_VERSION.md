# Pi4 R104 — POST version labels

The Pi4 boot-time splash POST window now displays two version lines:

- `FRF PI4 BARE METAL: R104` — independent FRF Pi4 kernel build/test revision.
- `AGA-STORM PORT: v1.121` — Pi3 AGA-Storm source/feature compatibility level ported into this Pi4 tree.

These are deliberately distinct: the Pi4 binary is not a byte-for-byte full Pi3 v1.121 build. The Pi4-specific VL805 xHCI driver, native HDMI output, and cross-compiled kernel remain independent.

The panel retains the USB host, keyboard, mouse, gamepad, HID, hub, boot-stage and status diagnostics. The optional native HVS debug POST panel also names both revisions. Maintain both strings in `machine/Emu68/include/frf_pi4_post_versions.h`.

Hardware verification is required before promoting this branch. Preserve your working `emu68-*.img` and replace only that configured FAT32 kernel filename when testing.

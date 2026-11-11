# EMU68AGA Pi4 v1.121 AGABOOT scanlines backport

## Why the Pi4 HID build showed no scanlines

The Pi4 R101 HDMI/HVS code was from an older AGA-PiStorm snapshot: it supported the legacy 0-4 `$598n` scanline strength API, but it did not decode the Pi3 v1.121 AGABOOT `$5000` / `$5300` selective 1% scanline protocol. New AGABOOT therefore had no effect when it sent the percentage-based controls.

This branch extends the proven `pi4-v121-composite-hid` code, so its wireless keyboard/mouse compatibility is retained.

## Backported from Pi3 v1.121 RC13

- AGABOOT `$5000` scanline target selection and `$5300` percentage value (0-100), in both ARM data-abort handlers
- `$517n` NTSC scanline override and PAL fallback
- 1% brightness/opacity mapping and interlaced scanline levels
- Pi3 v1.121 accurate Even/Crisp/Whole and Beam line mask generation for the native HDMI/HVS overlay
- Top/bottom scanline orientation
- Legacy `$598n` and `$59En` commands kept (multiples of 25%, 0-4)
- All Pi4 VL805/xHCI HID changes from previous branch

The render path uses the native AGA video plane on Pi4. Picasso96 RTG mode is a distinct external VideoCore.card path and may have its own scanline configuration. Do not assume this modification changes the external RTG driver.

## Hardware test

1. Save your current *working* CaffeineOS FAT32 `emu68-*.img` and `config.txt`.
2. Copy the test kernel onto FAT32 under precisely the configured `emu68-*.img` filename, replacing only the backed-up kernel.
3. Boot the Pi4 with the same AGABOOT v1.121 settings.
4. Open AGABOOT Zoom / RTG Image, select native Amiga display, change Scanlines from Off to 50-100%, and confirm a visible pattern. Test Even, Crisp, Whole, Beam where available. Then test 27% (CRT preset).
5. Verify the already-working wireless mouse still moves and clicks, and the keyboard is functional.
6. If comparing with Picasso96 Workbench/RTG rather than the native AGA screen, check whether your VideoCore.card build offers and applies scanlines; that is not an ARM native overlay.

Build: Pi4 native AArch64 BE Emu68, Release -O3, same key CMake options as R101. Successful source cross-link is *not* a Pi4 hardware test. Keep R101 and the working USB kernel for rollback.

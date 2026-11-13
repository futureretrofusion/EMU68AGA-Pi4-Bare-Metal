# Pi4 R109 — Native AGA scanlines / VC6 HVS test

**Status:** built and source-level tested. **Not yet validated on physical Pi4 HDMI.**
Branch: `pi4-r109-native-scanline-hvs-fix`
Baseline: **R107** (`5137fea`), which has confirmed working native Pi4 xHCI USB and USB hub HID input.
AGA-Storm API: **1.121**

## Reported symptom

On R107, `C:agaboot ZOOM ... SCANPCT=100` does not visibly apply scanlines even though `MASK=n` (CRT aperture mask) **does** work with the same tool. CaffeineOS already has the official, correct `C:agaboot` v1.121; replacing it is not a fix. Firmware mode is 1280×720p50; preserve **INTEGER** and the original 25% EVEN scanline preference.

## Source diagnosis and change

Both scanlines and the CRT mask are overlaid on the native AGA HVS display list, but only **scanlines** require a scaled plane. The unscaled CRT grille uses its own valid unity plane.

R107's `scan_plane()` in `machine/arm/aga_hvs.c` was derived from the Pi3 VC4 layout and emitted only:

```
pitch → horizontal PPF → 4 kernel pointers
```

while Pi4 VC6's working `build_list()` plane emits:

```
pitch → LBM → horizontal PPF → vertical PPF → scratch → 4 kernel pointers
```

The compact H-PPF/V-NONE descriptor is a **suspected compatibility issue**, not a proven one; VC6 has multiple legal layouts. R109 experimentally switches this plane to the same full-format descriptor as the working scaled picture plane. The hardware comparison will determine whether this addresses the invisible scanlines. The vertical sampling factor is 1.0, because the scratch buffer contains a row for each output row. Pi4's extra vertical-scaling plane also uses a separate HVS5 line-buffer base (4096 words), avoiding collision with the main picture's initial line-buffer region. The scanline plane control word now matches the main plane instead of setting the old generic `CTL_SCL` bits.

No CRT mask changes, no USB changes, no ROM changes and no WiFiPi changes.

## Testing and evidence

- Pi4 cross-compile: **PASS**. Label `FRF PI4 BARE METAL: R109`.
- Static HVS layout test: **PASS**. Check LBM, horizontal PPF, vertical unity PPF, scratch and 4 kernels.
- Static PAL 720p50 INTEGER geometry: **PASS**. Expected native output image 832×572 with 286 dark scanline rows at 100% EVEN strength.
- `git diff --check`: **PASS**.
- Binary and runtime HDMI output: **NOT TESTED**. Do not claim fixed before repeatable hardware test.

## Installer/test procedure

1. **Keep the working R107 image and existing CaffeineOS Amiga partition.** On the FAT32 boot partition, add `emu68-pi4-r109-scanline-hvs-test.img` without deleting `emu68-pi4-r107-r104-usb-post-diagnostic.img`.
2. Make a copy of `config.txt`, then edit only the active `[pi4]` `kernel=` line to select the new test image.
3. Preserve `hdmi_group=1`, `hdmi_mode=19`, `disable_overscan=1`. These are **1280×720p50**, not 1080p.
4. Cold boot on the Pi4. Verify keyboard, mouse and gamepad (same R107 USB behaviour).
5. In CaffeineOS AmigaShell, for a high-contrast all-mode test:

   ```
   C:agaboot ZOOM FULL SMOOTH INTEGER SCANPCT=100 LACEDPCT=100 NTSCPCT=100 SCANSHAPE=CRISP MASK=0
   ```

   Start a **native AGA game/demo**, not just the Picasso96/RTG Workbench. If the HVS scan plane now works, every source line should have a clearly black output row at 100%.

6. Disable to compare:

   ```
   C:agaboot ZOOM SCANPCT=0 LACEDPCT=0 NTSCPCT=0 MASK=0
   ```

7. Restore the user's saved baseline (25% EVEN INTEGER):

   ```
   C:agaboot ZOOM FULL SMOOTH INTEGER SCANLINES=1 LACED=0 NTSCLINES=PAL SCANSHAPE=EVEN MASK=0
   ```

   The existing `S:Startup-Sequence` also contains `C:agaboot zoom scanlines=1 integer` and was **not modified**.

8. If the image does not boot or USB regresses, revert `[pi4]` `kernel=` to the R107 image. If native scanlines remain invisible, capture a photo and note whether the CRT mask still works. Don't reinstall `agaboot`.

No SD cards were modified in this R109 work; this is an independent candidate image. The user must validate the display on their Pi4.

**Source:** see the branch and the one-file `machine/arm/aga_hvs.c` diff.

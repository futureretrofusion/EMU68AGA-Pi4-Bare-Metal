# Pi4 R110: Native AGABOOT picture-command gate correction

Status: cross-built, decoder unit tested. NOT YET VERIFIED ON PHYSICAL PI4 HDMI.
Base: R107 commit 5137fea. AGA-Storm/AGABOOT 1.121.

## Source diagnosis
AGABOOT 1.121 sends picture commands as 16-bit writes to DFF1F2. The v1.121 scanline protocol uses 5000-series selector and 5300-series percentage commands. In standalone EMU68AGA the machine is running with aga_mode=1. Both ARM exception-handler paths previously gated the normal zoom, scanline, integer scaling, and crop handling behind if (!aga_mode). The CRT mask has a separate handler outside this block, explaining why MASK responds and scanlines do not.

R110 moves only the native picture-command handling into a small shared decoder ahead of the legacy guard, in both ARM exception write paths. The decoder covers scan percentages, legacy PAL/interlace/NTSC strengths, zoom, scaling, look, custom crop, position, borders, flicker and related video picture controls. Legacy sandbox, redirect and unrelated private commands stay restricted. The CRT mask handler stays unchanged.

This control-only test deliberately retains R107's original HVS scanline plane, and does NOT include the experimental R109 HVS-plane layout modification. USB/xHCI, Wi-Fi and ROM code unchanged.

## Tests and build
Cross-compiled big-endian ARM64 Pi4 kernel with VARIANT=pistorm-classic: PASS.
Source decoder harness executed all percentages 0..100, eight selectors, zoom, INTEGER, scanlines and laced, custom crop and guarded invalid/legacy writes: PASS.
Both write-handler call sites reach the decoder before the existing !aga_mode gate: PASS.
Native 1280x720p50 integer scanline geometry analysis: 832x572 picture with 286 expected dark rows at full strength.
Real Pi4 HDMI scanline output, boot and USB regressions: NOT TESTED.

Binary: emu68-pi4-r110-agaboot-control-only-test.img
SHA256: 163e11731c4c71d03f5eb84ef28df895d08895369ef20d116d25b3e135af05d6
Expected startup label: FRF PI4 BARE METAL: R110.

## Pi4-only test installation
Copy the image to the FAT32 boot partition of the actual Pi4 CaffeineOS card without removing R107. Back up config.txt. Update only the active [pi4] kernel setting to:

    [pi4]
    kernel=emu68-pi4-r110-agaboot-control-only-test.img

Keep HDMI 1280x720 at 50 Hz (hdmi_group=1, hdmi_mode=19), and preserve existing overscan/INTEGER settings and Amiga partitions. Do not use a Pi5 card.

Boot Pi4 and confirm R110 POST, USB keyboard/mouse/gamepad and native AGA game boot. In AmigaShell set high-contrast scanlines:

    C:agaboot ZOOM FULL SMOOTH INTEGER SCANPCT=100 LACEDPCT=100 NTSCPCT=100 SCANSHAPE=CRISP MASK=0

Test on an actual native AGA game/demo; Picasso96/RTG Workbench is a different video path. To compare, turn lines off:

    C:agaboot ZOOM FULL SMOOTH INTEGER SCANPCT=0 LACEDPCT=0 NTSCPCT=0 MASK=0

Restore usual scanlines:

    C:agaboot ZOOM FULL SMOOTH INTEGER SCANPCT=25 LACEDPCT=0 NTSCLINES=PAL SCANSHAPE=EVEN MASK=0

Per-game configuration can override global settings on game launch. Rollback safely by restoring only the previous [pi4] kernel= line selecting R107.

Interpretation: visible scanlines and restored zoom response in R110 indicate missing command handling was the primary issue. If zoom responds but 100% native scanlines remain invisible, the HVS overlay remains a separate candidate for further tests. Do NOT claim a hardware fix until a repeatable Pi4 test.

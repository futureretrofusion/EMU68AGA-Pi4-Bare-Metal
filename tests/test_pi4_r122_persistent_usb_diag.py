#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
cm=(r/"machine/Emu68/CMakeLists.txt").read_text()
h=(r/"machine/arm/aga_hvs.c").read_text()
g=(r/"machine/arm/aga_glue.c").read_text()
boot=(r/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
assert '"R122"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
assert '"FRF PI4 R122 OLD USB DIAG"' in h
assert '"POLL:"' in h and '"FAIL:"' in h
assert '"P1 EN:"' in h and '"ADDR:"' in h and '"PWR:"' in h
assert '"BOARD:"' in h and '"FW PRE:"' in h
assert '"PCIE:"' in h and '"REC:"' in h and '"DMA:"' in h
assert '"ROOT:"' in h and '"FWRC:"' in h and '"CAPS:"' in h
assert 'defined(FRF_PI4_LEGACY_REV_DIAG)' in h
assert '#elif defined(FRF_PI4_USB_HUD)' in g
assert 'frf_hud = 1;' in g
assert 'aga_video_overlay_enable(frf_hud)' in g
assert '#if defined(FRF_PI4_USB_HUD)' in h
assert 'r94_post_panel(dst)' in h
assert 'FRF PI4 R122 REV1.1 USB LIVE DIAGNOSTIC' in boot
assert 'FRF_PI4_SUPPRESS_DIAGNOSTIC_POST' in cm
cfg=(r/"build-pi4-r122/CMakeCache.txt").read_text()
for name,value in (
    ("FRF_BOOT_POST","ON"),
    ("FRF_PI4_MOUSE_DIAG","ON"),
    ("FRF_PI4_LEGACY_REV_DIAG","ON"),
    ("FRF_PI4_USB_MASS_RO","ON"),
    ("FRF_PI4_USB_MASS_RW","OFF"),
    ("FRF_PI4_SUPPRESS_DIAGNOSTIC_POST","OFF"),
    ("FRF_RELEASE_QUIET","ON"),
    ("FRF_RTG_HANDOFF","ON")
):
    assert name+":BOOL="+value in cfg,(name,value)
assert '#if defined(FRF_PI4_LEGACY_REV_DIAG)' in h
assert 'frf_r99_root_poll_ok' in h
assert 'frf_r98_root_addr_cc[0]' in h
assert 'frf_r98_root_enable_cc[0]' in h
print('PASS R122 source: always-on AGA diagnostic, legacy VL805/PCIe, live root-port failures, boot POST')

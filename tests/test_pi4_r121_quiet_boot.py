#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
c=(r/"machine/Emu68/CMakeLists.txt").read_text()
g=(r/"machine/arm/aga_glue.c").read_text()
h=(r/"machine/arm/aga_hvs.c").read_text()
assert 'option(FRF_PI4_SUPPRESS_DIAGNOSTIC_POST' in c
assert 'FRF_PI4_SUPPRESS_DIAGNOSTIC_POST=1' in c
assert '#if defined(FRF_PI4_SUPPRESS_DIAGNOSTIC_POST)' in g
assert 'frf_hud=0;' in g[g.index('#if defined(FRF_PI4_SUPPRESS_DIAGNOSTIC_POST)'):g.index('#elif defined(FRF_PI4_USB_HUD)')]
assert h.count('defined(FRF_PI4_SUPPRESS_DIAGNOSTIC_POST))')==2
assert 'FRF PI4 R121 USB RW' in h
splash=(r/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
assert 'defined(FRF_PI4_SUPPRESS_DIAGNOSTIC_POST) && defined(FRF_PI4_USB_MASS_RW)' in splash
assert 'FRF PI4 R121 USB RW\\n' in splash
assert 'frf_pi4_legacy_vl805_eeprom' in (r/"machine/Emu68/src/frf_xhci/frf_xhci_probe.c").read_text()
print("PASS R121 quiet boot: no forced or requested diagnostic overlay; Pi4 early-board USB code preserved")

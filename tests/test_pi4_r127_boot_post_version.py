#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
source=(r/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
header=(r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
glue=(r/"machine/arm/aga_glue.c").read_text()
cfg=(r/"build-pi4-r127/CMakeCache.txt").read_text()
assert '"R127"' in header
assert '"v1.121"' in header
assert 'FRF_POST_FRF_LABEL,green' in source
assert 'FRF_POST_AGA_LABEL,yellow' in source
assert '"EMU68AGA SYSTEM POST"' in source
assert '"PI4 NATIVE USB RW / NON-LEGACY"' in source
assert 'FRF PI4 R127 USB RW / 4BTN' in source
assert 'frf_boot_post_stage(1u)' in source
assert 'frf_post_active,1u' in source
assert 'frf_post_draw()' in source
assert 'frf_hud=0;' in glue
for name,value in {
 "FRF_BOOT_POST":"ON",
 "FRF_PI4_SUPPRESS_DIAGNOSTIC_POST":"ON",
 "FRF_PI4_XHCI_HID":"ON",
 "FRF_PI4_USB_MASS_RO":"ON",
 "FRF_PI4_USB_MASS_RW":"ON",
 "FRF_PI4_LEGACY_REV_DIAG":"OFF",
 "FRF_PI4_MOUSE_DIAG":"ON",
 "FRF_RELEASE_QUIET":"ON",
 "FRF_RTG_HANDOFF":"ON",
 }.items():
 assert name+":BOOL="+value in cfg,(name,value)
old=Path("/home/fff2025/Downloads/EMU68AGA_PI4_R123_STANDARD_USB_RW")
for name in ("machine/arm/aga_audio.c",
             "machine/arm/aga_hdmi.c",
             "machine/Emu68/src/frf_xhci/frf_pi4_usb_mass.c",
             "machine/Emu68/src/frf_xhci/frf_xhci_host.c",
             "machine/arm/aga_glue.c"):
 assert (r/name).read_bytes()==(old/name).read_bytes(),name
print("PASS R127: splash POST version restored; native Pi4 USB RW and persistent HUD OFF retained")

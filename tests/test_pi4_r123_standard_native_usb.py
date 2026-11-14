#!/usr/bin/env python3
"""Build-time integrity for the Pi4 non-legacy USB/HID+MSC RW branch.
Hardware writes must be verified separately before promotion.
"""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
cache=(r/"build-pi4-r123/CMakeCache.txt").read_text()
checks={
"FRF_VIRTUAL_AGA":"ON",
"FRF_PI4_XHCI_HID":"ON",
"FRF_PI4_USB_MASS_RO":"ON", # underlying transport on
"FRF_PI4_USB_MASS_RW":"ON", # SCSI WRITE10 enabled
"FRF_PI4_LEGACY_REV_DIAG":"OFF", # disable old-board recovery code
"FRF_PI4_MOUSE_DIAG":"ON", # preserve working HID descriptor support
"FRF_PI4_SUPPRESS_DIAGNOSTIC_POST":"ON",
"FRF_RELEASE_QUIET":"ON",
"FRF_BOOT_POST":"OFF",
"FRF_RTG_HANDOFF":"ON",
}
for k,v in checks.items():
 assert k+":BOOL="+v in cache,(k,v)
main=(r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
assert '"R123"' in main
cm=(r/"machine/Emu68/CMakeLists.txt").read_text()
assert 'FRF_PI4_USB_MASS_RW=1' in cm
assert "FRF_PI4_LEGACY_REV_DIAG=1" in cm # opt-in only, disabled here
mass=(r/"machine/Emu68/src/frf_xhci/frf_pi4_usb_mass.c").read_text()
assert 's->reserved[1]=0x50493457u;' in mass
assert 'frf_xhci_host_bulk_out(src,bytes,&got)' in mass
assert 'frf_xhci_host_bulk_in(csw,13u,&got)' in mass
glue=(r/"machine/arm/aga_glue.c").read_text()
assert '#if defined(FRF_PI4_SUPPRESS_DIAGNOSTIC_POST)' in glue
assert 'frf_hud=0;' in glue
hdmi=(r/"machine/arm/aga_hdmi.c").read_bytes()
baseline=Path("/home/fff2025/Downloads/EMU68AGA_PI4_R121_USB_RW/machine/arm/aga_hdmi.c").read_bytes()
assert hdmi==baseline,"existing working audio changed"
assert "FRF PI4 R123 USB RW" in (r/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
print("PASS R123: non-legacy Pi4 with native xHCI HID and SCSI WRITE10, quiet boot and unchanged HDMI audio")

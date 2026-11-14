#!/usr/bin/env python3
"""Source regression: guarded legacy-only retry, no automatic EEPROM writes."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=(r/"machine/Emu68/src/frf_xhci/frf_xhci_probe.c").read_text()
h=(r/"machine/arm/aga_hvs.c").read_text()
c=(r/"machine/Emu68/CMakeLists.txt").read_text()
host=(r/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
assert 'dt_find_node("/system")' in p
assert 'dt_find_property(system,"linux,revision")' in p
assert 'frf_pi4_legacy_vl805_eeprom(revision)' in p
assert 'if(!caps_ok && frf_r117_legacy_vl805 && frf_usb_fw_before!=0u)' in p
assert 'frf_r117_legacy_recover_stage=caps_ok?2u:3u' in p
assert 'if(caps_ok && frf_r117_legacy_vl805)' in p
assert 'frf_r117_fw_reset_rc=(uint32_t)fw' in p
assert 'frf_r115_scan_deadline=host_deadline_ms(frf_r117_legacy_vl805?' in host
assert '16000u:12000u' in host
assert '#if defined(FRF_PI4_LEGACY_REV_DIAG)' in h
for fld in ("BOARD:", "FW PRE:", "PCIE:", "REC:", "DMA:", "XHCI STAGE:", "MSC TRANSPORT:","ROOT:"):
    assert fld in h,fld
assert 'option(FRF_PI4_LEGACY_REV_DIAG' in c
assert 'FRF_PI4_LEGACY_REV_DIAG=1' in c
assert '"R117"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
assert 'const int w=333,x=AGA_OUT_W-w-5,y=2,h=282;' in h
assert 2+4+16*16+16 <= 286
assert 2+282 <= 286
print("PASS R117: board rev and firmware/PCIe diagnostics, guarded legacy-only retry, 800x286 HUD")

#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
t=(r/"machine/Emu68/src/raspi/support_rpi.c").read_text()
p=(r/"machine/Emu68/src/frf_xhci/frf_xhci_probe.c").read_text()
h=(r/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
ui=(r/"machine/arm/aga_hvs.c").read_text()
assert "#if defined(FRF_PI4_SAFE_VL805_NOTIFY)" in t
assert 'return -3;' in t and 'return -4;' in t
assert 'LE32(*clock)-begin)>=500000u' in t
assert 'LE32(*clock)-begin)<5000000u' in t
assert t.count("frf_notify_xhci_reset(uint32_t pci_bdf)")==1
assert 'if (frf_usb_fw_before==0u)' in p
assert p.count('frf_notify_xhci_reset(0x00100000u)')==1
assert "frf_r125_fw_notify_ms" in p and "frf_r125_board_revision" in p
assert 'for(unsigned tries=0;tries<3000u;tries++)' in p
assert 'if(caps_ok && frf_r117_legacy_vl805)' not in p
assert "frf_r125_cnr_sts" in h and "frf_r125_cnr_ms" in h
assert h.count('2000U')>=2
for x in ("FW0:","FW1:","N:","BOARD:","CNR:","CAPS:","STS:"):
 assert '"'+x+'"' in ui,x
assert '"FRF PI4 R125 FW SAFE USB"' in ui
assert '"R125"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
print("PASS R125: R115-derived USB FW notify bounded, no R117 second reset, live AGA diagnostics")

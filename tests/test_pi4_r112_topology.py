#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
v=(r/"machine/Emu68/src/frf_xhci/frf_xhci_hid.c").read_text()
p=(r/"machine/Emu68/src/frf_xhci/frf_native_usb_pad.c").read_text()
assert "#define FRF_NATIVE_HID_MAX 8U" in h
assert "#define XHCI_MAX_SLOTS_USED          24U" in h
assert "#define R97_CHILD_CONTEXT_BANKS       24U" in h
assert "#define FRFXHCI_HUB_MAX_DEVICES 24U" in (r/"machine/Emu68/include/frf_xhci/frf_xhci_host.h").read_text()
for name in ("r97_scan_hub", "hub_scan_children"):
    part=h[h.index("static int "+name+"("):]
    if name=="r97_scan_hub":part=part[:part.index("/* Every USB2.0 socket on Pi4B")]
    else:part=part[:part.index("uint32_t frf_xhci_host_device_count")]
    assert "if(g_native_count>=FRF_NATIVE_HID_MAX" not in part, name
    assert "++frf_r112_msc_seen" in part, name
assert "int usable_hid=(g_native_count<FRF_NATIVE_HID_MAX)" in h
assert "int usable=he && hm && hiface!=0xffffffffu" in h
assert "usable && g_native_count<FRF_NATIVE_HID_MAX" in h
assert "if(count>8u)count=8u" in v
assert "pad_ready[8]" in v
assert "pads[8]" in p
assert "frf_pi4_um_retry(log_now,log_hz)" in v
print("PASS R112: 8 HID interfaces, 24 xHCI slots, no HID-full storage skip, per-slot bounds")

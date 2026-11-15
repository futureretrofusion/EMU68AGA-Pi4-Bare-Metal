#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
main=(r/"machine/Emu68/src/frf_xhci/frf_xhci_hid.c").read_text()
host=(r/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
post=(r/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
flag=(r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
cfg=(r/"build-pi4-r130-legacy/CMakeCache.txt").read_text()
assert "while (!__atomic_load_n(&frf_usb_host_start_allowed, __ATOMIC_ACQUIRE))" in main
assert "frf_timebase_calibrate();" in main
assert "boot_hz*2ULL" not in main
assert "frf_boot_stage(9);" in main
assert "frf_usb_boot_ready,1u" in main
assert "frf_r130_post_hid_discovered" in host
assert host.count("frf_r130_post_hid_discovered();")==3
assert "frf_post_usb_devices(kb,mouse,pad,frf_r76_usb_mass_count);" in host
assert "frf_r130_pad_discovered,pad,__ATOMIC_RELEASE" in host
assert "frf_r130_pad_vidpid=" in host
assert "frf_r91_last_pad_vidpid:frf_r130_pad_vidpid" in post
assert "h->protocol==0u" in host
assert "frf_r92_schedule_recovery(i,0xffu,1)" not in host
assert "USB_HUB_FEAT_PORT_POWER,0" not in host
for field in ("LAST USB VIDPID","PAD VIDPID","PAD SEEN","PAD READY","PAD RX","ACCEPT",
              "LEGACY VL805 DMA DIAGNOSTIC","BAD PHYS","DMA REASON"):
 assert field in post,field
assert "const int w=320,h=620" in post
assert 'discovered?"ENUM HID"' in post
assert "frf_boot_post_devices();" in (r/"machine/Emu68/src/frf_xhci/frf_xhci_compat.c").read_text()
assert "FRF_BOOT_POST:BOOL=ON" in cfg
assert "FRF_USB_HOST:BOOL=OFF" in cfg
assert "FRF_PI4_SAFE_VL805_NOTIFY:BOOL=ON" in cfg
assert '"R130 FAST"' in flag
print("PASS LEGACY R130: zero gratuitous 2s wait, incremental HID POST, candidate vs ready, Pi3 radio safety retained")

#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
host=(r/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
hud=(r/"machine/arm/aga_hvs.c").read_text()
pi3=(r/"machine/arm/frf_potgor_model.h").read_text()
assert "frf_r129_gamepad_idle_pending(h->protocol,h->pending" in host
assert "++frf_r129_gamepad_idle_kept;" in host
assert "frf_r92_schedule_recovery(i,0xffu,1)" not in host
assert "frf_native_usb_pad_release();" not in host
assert "frf_r92_schedule_recovery(i,cc,0)" in host
assert "XHCI_CMD_RESET_ENDPOINT" in host
assert "XHCI_CMD_SET_TR_DEQUEUE" in host
assert "USB_HUB_FEAT_PORT_POWER,0" not in host
assert "frf_r129_port_power_off_avoided" in host
assert "frf_r129_gamepad_idle_kept,4,green" in hud
assert "v &= (uint16_t)~(pad_mask & 0x4000u);" in pi3
assert '"R130 FAST"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
print("PASS R129: NAK-idle no reset, state preserved during actual xHCI retries, no hub power cycling, Pi3 B1/B2/B3 kept")

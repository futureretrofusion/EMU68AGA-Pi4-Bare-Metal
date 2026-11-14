#!/usr/bin/env python3
"""Targeted source/layout regression for Pi4 R115. No hardware assertion."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
mass=(r/"machine/Emu68/src/frf_xhci/frf_pi4_usb_mass.c").read_text()
host=(r/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
hvs=(r/"machine/arm/aga_hvs.c").read_text()
post=(r/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
assert 'const uint32_t timeout_ms=350u;' in host
assert 'const unsigned r114_retries=g_native_count?2u:3u;' in host
assert 'const uint32_t timeout_ms=3000u;' in host
assert 'transport_dead=1u;' in mass
assert 'if(capacity_sectors || transport_dead' in mass
fn=mass[mass.index('void frf_pi4_um_init('):mass.index('void frf_pi4_um_retry(')]
assert 'try_read_capacity();' not in fn, "SCSI probe still blocking USB startup"
assert 'frf_pi4_um_retry(' in mass
assert 'frf_boot_post_devices();' in mass
assert '#if defined(FRF_RELEASE_QUIET) && !defined(FRF_PI4_USB_HUD)' in hvs
assert hvs.count('#if defined(FRF_RELEASE_QUIET) && !defined(FRF_PI4_USB_HUD)')==2
assert 'const int w=333,x=AGA_OUT_W-w-5,y=2,h=282;' in hvs
assert 'const uint32_t ym=4u;' in hvs
# Topaz glyphs 16 px high; source bitmap 286 px high
assert 2+4+16*16+16<=286, "last diagnostic row outside framebuffer"
assert 2+282<=286,"live panel border outside framebuffer"
for label in ("MSC SEEN","BOT ERR:","MSC TRANSPORT:","MOUSE RAW:","AMIGA POT:"):
 assert label in hvs,label
assert 'MSC SEEN:' in post and 'BOT STAGE:' in post
assert "frf_pi4_um_probe_inline();" in host
assert "frf_r115_scan_deadline=host_deadline_ms(12000u);" in host
assert "host_timed_out(frf_r115_scan_deadline)" in host
assert "frf_r115_scan_budget_hit=1u;" in host
assert '"FRF PI4 R115 LIVE USB POST"' in hvs
assert '"SCAN DEPTH:"' in hvs and '"PORT:"' in hvs and '"CAP:"' in hvs
assert '"R115"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
print("PASS R115: no blocking SCSI init, fail-closed BOT, live quiet-mode HUD, 286-line bounds, all POST fields")

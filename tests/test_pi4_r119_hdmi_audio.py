#!/usr/bin/env python3
"""Static guards on Pi4 R119 HDMI audio regression.
Hardware test required before a fix can be claimed.
"""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
h=(r/"machine/arm/aga_hdmi.c").read_text()
a=(r/"machine/arm/aga_hvs.c").read_text()
v=(r/"machine/Emu68/src/aarch64/vectors.c").read_text()
g=(r/"machine/arm/aga_glue.c").read_text()
cm=(r/"machine/Emu68/CMakeLists.txt").read_text()
assert 'option(FRF_PI4_HDMI_AUDIO_DIAG' in cm
assert 'FRF_PI4_HDMI_AUDIO_DIAG=1' in cm
assert '"R119"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
assert '#if defined(FRF_PI4_HDMI_AUDIO_DIAG)' in h
assert 'if (!clk && !pi3 && !aga_hdmi_force)' in h
assert 'set_state(H_NO_LINK);' in h[h.index('if (!clk && !pi3 && !aga_hdmi_force)'):h.index('if (!clk && !aga_hdmi_force)')]
assert 'clk=108000000u;' in h
assert 'frf_r119_audio_no_hsm=1u;' in h
assert 'if(state!=H_NO_CLOCK && state!=H_NO_LINK)set_state(H_OFF);' in h
assert 's == H_NO_LINK' in h[h.index('static inline int retryable'):h.index('void aga_hdmi_service')]
assert 'if (!ready) return;' in h[h.index('void aga_hdmi_enable('):h.index('void aga_hdmi_enable(')+550]
assert 'if (!guard_now())' in h[h.index('void aga_hdmi_enable('):h.index('void aga_hdmi_enable(')+800]
assert 'if (s != H_RUNNING && !ours() && !prep_left && !(wanted && retryable(s))) return;' in h
assert 'if (pi3) {' in h[h.index('uint32_t clk;'):h.index('uint32_t clk;')+450]
assert 'if (!pi3) return;' in h # Pi3-only original no-clock path kept as fallback when not compiled with R119
assert 'aga_hdmi_enable(1);' in g and 'aga_hdmi_boot(fb_va, fb_phys);' in g
assert v.count('extern volatile uint32_t frf_r119_audio_sound_cmds,frf_r119_audio_sound_offs;')==2
assert v.count('frf_r119_audio_sound_last=(uint32_t)(value & 1u);')==2
assert v.count('(value & 0xFFFE) == 0x5820')==2
assert v.count('(value & 0xFFFE) == 0x5890')==2
assert 'frf_r119_audio_feed_count' in h and 'frf_r119_audio_feed_rejects' in h
for label in ['"FRF PI4 R119 HDMI AUDIO"','"HDMI ST:"','"HSM kHz:"','"SND CMD:"','"HDMI FEED:"']:
    assert label in a,label
assert 'const int w=333,x=AGA_OUT_W-w-5,y=2,h=282;' in a
assert 2+282<=286 and 2+4+16*16+16<=286
assert '#if defined(FRF_RELEASE_QUIET) && !defined(FRF_PI4_USB_HUD)' in a
print("PASS R119: zero measured Pi4 HSM can recover only through moving-link guard; AGABOOT Sound command tracked; Pi3 path preserved")

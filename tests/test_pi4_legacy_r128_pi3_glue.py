#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=(r/"machine/arm/aga_glue.c").read_text()
m=(r/"machine/arm/frf_potgor_model.h").read_text()
h=(r/"machine/arm/aga_hvs.c").read_text()
assert '#include "frf_potgor_model.h"' in p
assert 'if(port==1 && (buttons&2u))mask|=0x4000u;' in p
assert 'if(port==1 && (buttons&4u))mask|=0x1000u;' in p
assert 'return frf_potgor_model(pg,mb,pad);' in p
assert 'if (reg == POTGOR) return frf_virtual_potgor_value();' in p
assert 'frf_virtual_potgor_value();' in p
assert 'v &= (uint16_t)~(pad_mask & 0x4000u);' in m
assert 'if (!(pg & 0x2000u)) v &= (uint16_t)~(pad_mask & 0x1000u);' in m
assert 'extern uint32_t frf_pi4_guest_pad_lines_snapshot(void);' in h
assert '"FRF PI4 R130 HANDSHAKE"' in h
assert '"R130 FAST"' in (r/"machine/Emu68/include/frf_pi4_post_versions.h").read_text()
print("PASS: legacy R128 Pi3 port2 3-button mapping, identical POTGOR helper and registered HUD")

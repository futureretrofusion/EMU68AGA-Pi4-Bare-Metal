#!/usr/bin/env python3
"""R109 static HVS scanline layout/geometry sanity checks; no hardware claims."""
from pathlib import Path
src=(Path(__file__).resolve().parents[1]/"machine/arm/aga_hvs.c").read_text()
scan=src.split("static uint32_t scan_plane(uint32_t o)",1)[1].split("/* The picture check",1)[0]
pic=src.split("static void build_list(uint32_t block, uintptr_t frame_phys)",1)[1].split("/* Length of a plane list",1)[0]
must=["hvs_gen == 4 ? 0u : 4096u", "PPF_SCALER | PPF_PHASE", "dl_wr(o++, (0x10000u << 8)", "for (int i = 0; i < 4; i++)"]
for marker in must:
    assert marker in scan,marker
assert "CTL_SCL(SCL_H_PPF_V_NONE)" not in scan
assert "BLW(0, 1); " in pic
assert "BLW((scale_x << 8)" in pic and "BLW((scale_y << 8)" in pic
assert "BLW(0, 0); " in pic
# Compare source dimensions at 720p50 PAL FULL, INTEGER; this is a model, not a hardware test.
aw,ah,src_w,src_h=1280,720,800,286
sw,sh=src_w*520,src_h*1000
w=ah*sw//sh
h=ah
h=(h//src_h)*src_h
w=h*sw//sh
assert (w,h)==(832,572),(w,h)
# HVS 1:1 vertical overlay rows; each PAL source row should get 2 screen rows.
lines=src_h
alpha=255
mask=[]
for row in range(h):
    lo=row*lines
    hi=lo+lines
    end=(lo//h+1)*h
    gap=end-lines
    s0=max(lo,gap)
    e0=min(hi,end)
    mask.append((alpha*(e0-s0)+lines//2)//lines if e0>s0 else 0)
assert mask[:10]==[0,255,0,255,0,255,0,255,0,255],mask[:10]
assert mask.count(255)==286
print(f"STATIC_PASS: 720p50 integer PAL FULL => HVS picture {w}x{h}, 286 dark scanline rows out of {h} at 100% EVEN")
print("STATIC_PASS: scanline plane now has LBM, PPF0, PPF1, scratch, 4 kernel pointers like the working picture plane")
print("HARDWARE NOT TESTED: requires Pi4 R109 boot and AGA image comparison")

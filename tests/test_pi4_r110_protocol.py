#!/usr/bin/env python3
"""Host-side execution of the exact native Pi4 AGABOOT decoder in vectors.c.

Does not claim physical HDMI output: exercises real C helper, checks both trap paths.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess

root = Path(__file__).resolve().parents[1]
vectors = (root / "machine/Emu68/src/aarch64/vectors.c").read_text()
signature = "static int frf_pi4_agaboot_picture_write("
assert vectors.count(signature) == 1
helper = signature + vectors.split(signature, 1)[1].split("\n#endif", 1)[0]
assert vectors.count("if (frf_pi4_agaboot_picture_write(far, size, value)) return 1;") == 2
assert vectors.count("if (!aga_mode) {") >= 2
for part in vectors.split("if (frf_pi4_agaboot_picture_write(far, size, value)) return 1;")[1:]:
    assert part.index("if (!aga_mode) {") < part.index("if (far == 0xDFF1F2 && size == 2 && (value & 0xFF02u) == 0x5000u)")
prefix = r"""
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
static int hit, last, cnt;
static void rec(int id, int v) { hit=id; last=v; ++cnt; }
#define aga_video_scan_select(v) rec(1, v)
#define aga_video_scan_value(v) rec(2, v)
#define aga_video_set_ntsc_scanlines(v) rec(3, v)
#define aga_video_set_scanlines(v) rec(4, v)
#define aga_video_set_laced_scanlines(v) rec(5, v)
#define aga_video_cut_data(v) rec(6, v)
#define aga_video_cut_store(v) rec(7, v)
#define aga_video_crop_screen(v) rec(8, v)
#define aga_video_set_zoom(v) rec(9, v)
#define aga_video_set_sharp(v) rec(10, v)
#define aga_video_set_look(v) rec(11, v)
#define aga_video_set_scale(v) rec(12, v)
#define aga_video_set_pos_x(v) rec(13, v)
#define aga_video_set_pos_y(v) rec(14, v)
#define aga_video_calibrate(v) rec(15, v)
#define aga_video_set_border(a,b) rec(16, (a)*100+(b))
#define aga_flickerfix(v) rec(17, v)
#define aga_brightness(v) rec(18, v)
#define aga_video_standard(v) rec(19, v)
#define aga_video_hdmi_sync(v) rec(20, v)
"""
main = r"""
static uint32_t enc(uint32_t v) {return (v & 1u) | ((v & 0x7eu) << 1);}
int main(void) {
    for (int val = 0; val <= 100; val++) {
        assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5000 | enc(0)) == 1);
        assert(hit == 1 && last == 0);
        assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5300 | enc(val)) == 1);
        assert(hit == 2 && last == val);
    }
    for (int sel = 0; sel <= 7; sel++) {
        assert(frf_pi4_agaboot_picture_write(0xDFF1F2,2,0x5000 | enc(sel)) == 1);
        assert(last == sel);
    }
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5300 | enc(127)) == 1);
    assert(hit == 2 && last == 127);
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5900) == 1);
    assert(hit == 10 && last == 0); /* FULL & SMOOTH */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5908) == 1);
    assert(hit == 10 && last == 1); /* SHARP */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5971) == 1);
    assert(hit == 12 && last == 1); /* INTEGER */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5984) == 1);
    assert(hit == 4 && last == 4); /* legacy 100% */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x59E4) == 1);
    assert(hit == 5 && last == 4); /* interlace */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x58C1) == 1);
    assert(hit == 6 && last == 1); /* custom crop */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x59B0) == 1);
    assert(hit == 16 && last == 0); /* border x */
    int before = cnt;
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 1, 0x5301) == 0);
    assert(frf_pi4_agaboot_picture_write(0xDFF1F0, 2, 0x5301) == 0);
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5A0B) == 0); /* sandbox */
    assert(frf_pi4_agaboot_picture_write(0xDFF1F2, 2, 0x5101) == 0); /* CRT mask handled earlier */
    assert(cnt == before);
    puts("PASS: 101 scanline levels, 8 selectors, crop/zoom/integer/laced, rejected legacy sandbox and wrong access");
    return 0;
}
"""
with TemporaryDirectory() as tmp:
    c = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    c.write_text(prefix + helper + "\n" + main)
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    str(c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS: shared decoder executes BEFORE legacy !aga_mode in both exception handlers")

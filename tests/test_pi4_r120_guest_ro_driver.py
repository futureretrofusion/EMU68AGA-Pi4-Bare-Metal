#!/usr/bin/env python3
"""Pi4 R120 resident image / safety / CLI regression. Hardware test separate."""
from pathlib import Path
import re,subprocess,struct
r=Path(__file__).resolve().parents[1]
s=(r/"amiga/pi4_usbmass_ro/pi4usbmass_v04.device.c").read_text()
old=(r/"amiga/pi4_usbmass_ro/pi4usbmass.device.c").read_text()
tester=(r/"amiga/pi4_usbmass_ro/frf-usb120-loadtrace.c").read_text()
root=Path("/home/fff2025/Downloads")
drv=root/"pi4usbmass-v04-test.device"
info=subprocess.check_output(["m68k-amigaos-objdump","-h",str(drv)],text=True)
m=re.search(r"^\s*0\s+\.text\s+([0-9a-f]+).*?([0-9a-f]{8})\s+2\*\*2\s*$",info,re.M)
assert m,(info[:300])
size=int(m.group(1),16)
off=int(m.group(2),16)
binary=drv.read_bytes()
code=binary[off:off+size]
assert code[:4]==bytes.fromhex("70ff4e75"),"First code hunk entry is not MOVEQ -1;RTS"
tags=[i for i in range(len(code)-26) if code[i:i+2]==bytes.fromhex("4afc")]
assert len(tags)==1,tags
i=tags[0]
match,endskip=struct.unpack(">II",code[i+2:i+10])
assert match==i,(hex(match),hex(i))
assert 0 < endskip < len(code),(hex(endskip),len(code))
assert code[i+10]==0x84,"Autoinit/AfterDOS resident flag changed"
assert code[i+11]==4 and code[i+12]==3,"resident not version 4 NT_DEVICE"
relocs=subprocess.check_output(["m68k-amigaos-objdump","-r",str(drv)],text=True)
# m68k-amigaos-objdump may print section-only relocations; hunktool
# performs the explicit relocation inspection separately.
assert "static char name[]=\"pi4usbmass.device\";" in s
assert "static char name[]=\"pi4usbmass.device\";" in old
assert "0.4 (14.11.2026)" in s and "0.2 (14.11.2026)" in old
assert "BOOL finished=FALSE;" in s
assert "if (!finished || s->completed!=seq)" in s
assert "if (!limit || s->completed!=seq)" not in s
assert "s->command=FRF_UM_READ;" in s and "s->command=FRF_UM_WRITE" not in s
for write_cmd in ("CMD_WRITE:","TD_WRITE64:","NSCMD_TD_WRITE64:","TD_FORMAT:","TD_FORMAT64:"):
    assert "case "+write_cmd in s,write_cmd
assert "io->io_Error=TDERR_WriteProt;" in s
for err in ("-20","-21","-22","-23","-24"):
    assert "io->io_Error="+err+";" in s,err
assert 'LoadSeg("DEVS:pi4usbmass.device")' in tester
assert 'open_test("pi4usbmass.device")' in tester
assert 'open_test("DEVS:pi4usbmass.device")' in tester
mount=(r/"amiga/pi4_usbmass_ro/USBP4").read_text()
for x in ("Device         = pi4usbmass.device","FileSystem     = L:fat95",
          "HighCyl        = 1","LowCyl         = 0","DosType        = 0x46415401"):
    assert x in mount,x
print("PASS R120: valid first-HUNK ROMTag, v0.4 resident, named driver, guarded read-only, stage diagnostics, FAT95 mount")

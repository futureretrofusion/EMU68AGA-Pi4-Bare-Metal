#!/usr/bin/env python3
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[1]
q=r/"amiga/pi4_usbmass_ro"
source=(q/"frfusbtest.device.c").read_text()
original=(q/"pi4usbmass.device.c").read_text()
utility=(q/"frf-usb118-loadtrace.c").read_text()
assert "static char name[]=\"pi4usbmass.device\";" in original
assert "static char name[]=\"frfusbtest.device\";" in source
assert "RTF_AUTOINIT|RTF_AFTERDOS,1,NT_DEVICE" in source
for code in ["-20", "-21", "-22", "-23", "-24"]:
    assert "io->io_Error="+code+";" in source
assert "TDERR_WriteProt;" in source and "s->command=FRF_UM_READ;" in source
assert "s->command=FRF_UM_WRITE" not in source
assert "LoadSeg(\"DEVS:frfusbtest.device\")" in utility
assert "open_test(\"frfusbtest.device\")" in utility
assert "open_test(\"DEVS:frfusbtest.device\")" in utility
print("PASS R118 source: safe independent test driver, stage-specific Open, no USB writes")

#!/usr/bin/env python3
"""Source-level regression: preserve Pi3 and make Pi4 read-only, FAT95 safe."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
q=r/"amiga/pi4_usbmass_ro"
pi3=(q/"usbmass.device.c").read_text()
p4=(q/"pi4usbmass.device.c").read_text()
assert 'static char name[]="usbmass.device";' in pi3
assert 'static char name[]="pi4usbmass.device";' in p4
assert 'FindConfigDev(NULL,FRF_UM_VENDOR,FRF_UM_PRODUCT)' in p4
assert "case CMD_WRITE:" in p4 and "io->io_Error=TDERR_WriteProt;" in p4
assert "s->command=FRF_UM_READ" in p4
assert "s->command=FRF_UM_WRITE" not in p4
mount=(q/"USBP4").read_text()
for line in ("FileSystem     = L:fat95","Device         = pi4usbmass.device",
             "Unit           = 0","LowCyl         = 0","HighCyl        = 1",
             "Surfaces       = 1","BlocksPerTrack = 1","DosType        = 0x46415401"):
    assert line in mount,line
assert "Device         = usbmass.device" not in mount
inf=(q/"pi4usbmass-info.c").read_text()
diagnostic=(q/"pi4usbmass-mountinfo.c").read_text()
assert 'OpenDevice("pi4usbmass.device"' in inf
assert 'OpenDevice("pi4usbmass.device"' in diagnostic
assert "case 11:case 12:" in diagnostic
assert "protective" in diagnostic.lower()
for x in ("Install-USBP4-Manual","Enable-USBP4-At-Boot"):
    assert (q/"S"/x).exists()
print("PASS R116: Pi4 isolated device name, read-only, FAT95 mount geometry, MBR probe, explicit manual install")

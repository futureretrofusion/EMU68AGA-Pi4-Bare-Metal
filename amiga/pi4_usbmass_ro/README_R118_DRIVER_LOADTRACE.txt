FRF EMU68AGA Pi4 R118 guest-side driver loader diagnostic
=============================================================
For the R117 card whose USB10 diagnostic showed:
 BOARD=FOUND at 00EA0000, valid FRUM magic/version/8256 bytes,
 present=1, capacity=245760000 and OpenDevice both names=-1.
The actual installed Pi4-specific driver is pi4usbmass.device v0.2.

WHY THIS BUILD EXISTS
The earlier guest driver sets IOERR_OPENFAIL(-1) for all failures from its
Open() callback, and AmigaOS also uses -1 when it fails to load/register a
device. Merely showing Version DEVS:pi4usbmass.device 0.2 confirms the file
but not that Exec ever loaded its resident code.

R118 builds a SIDEBYSIDE frfusbtest.device 0.3 test driver that retains
read-only USB semantics and does not overwrite pi4usbmass.device.
A first-CODE-hunk safety prologue is added for AmigaOS InitResident.
Diagnostic Open() sets distinct return codes before each step.

INSTALL (actual AmigaOS filesystem, not Pi boot FAT staging only)
1. Keep existing DEVS:pi4usbmass.device untouched.
2. Copy DEVS/frfusbtest.device to SYS:Devs/frfusbtest.device.
3. Copy C/frf-usb118-loadtrace to SYS:C/frf-usb118-loadtrace.
4. Run: C:frf-usb118-loadtrace
5. Photograph full output, particularly RESIDENT and OPEN rc fields.

Open result:
 0     driver loaded, found FRUM, ready to read sector 0 later.
 -20   Open() entered, but IOReq length or unit invalid.
 -21   Open() entered, cannot open expansion.library v36.
 -22   Open() entered, FindConfigDev returned no FRUM board.
 -23   Open() entered, FRUM header invalid.
 -24   Open() entered, valid FRUM but not reporting media.
 -1    Open() did not report any R118 stage: loader/Resident or Exec failure.

Note: if a hard-coded -1 with missing resident is reported, check that the
new file really exists in the AmigaOS DEVS: assignment. This is a diagnostic
not an established fix; the actual cause is determined from hardware output.

No boot kernel changes; R117 remains installed. No writes to USB drive.

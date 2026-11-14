# Future Retro Fusion — R116 Pi4 native USB FAT32 mount kit

The Pi4 R115 HDMI POST proved: MSC SEEN=01, CFG=01, READY=01, SCSI TRY=01, BOT ERR=00, HOST=RUN. This proves native xHCI READ CAPACITY, not guest AmigaOS mounting.

The original R111-R115 packages staged AmigaOS files only on the Raspberry Pi FAT boot partition; the Amiga RDB filesystem could not see them without explicitly copying them. This kit contains a unique Pi4 device name to avoid overwriting the established Pi3 usbmass.device, preserving the user's existing working Pi3 driver.

The Pi4-specific 68k driver is DEVS/pi4usbmass.device (read-only, Zorro II 6D73:0069, unit0). It will NOT work until the matching R115 kernel's virtual FRUM board is detected. It never writes to USB disk. Replacing this driver with Pi3 usbmass.device is not safe.

A separate USBP4 DOSDriver uses the Pi3 R101 FAT95 `FAT\\1` MBR-aware mount format (HighCyl=1), Device=pi4usbmass.device, Unit=0, dosType=0x46415401. The older example copied into R111-R115 had HighCyl=0; this is a changed mount geometry based on Pi3 R101 source, not proof of successful mounting on this Pi4.

BEFORE mounting:
1. Copy this kit into a directory accessible from inside AmigaOS. Copying onto the Pi FAT boot partition DOES NOT make it accessible automatically.
2. Check `L:fat95` exists in the AmigaOS installation. This kit does NOT include a fat95 filesystem binary. Obtain/reuse your licensed copy from your known-working Pi3 CaffeineOS filesystem if necessary.
3. Run `C/frf-mouse-buttons-pi4` only if still diagnosing mouse; `C/usbmass-z2-pi4` to verify native FRUM board, then the Pi4-specific `C/pi4usbmass-info` to open the device and read LBA0.
4. Run `C/pi4usbmass-mountinfo`; it reports whether LBA0 is an MBR FAT partition, a direct FAT superfloppy, GPT protective MBR, or invalid geometry. **STOP** for GPT/no FAT and do not guess the offset.
5. After successful device reads and a valid FAT95-compatible partition, copy `DEVS/pi4usbmass.device` to `SYS:Devs/pi4usbmass.device`. Copy `Storage/DOSDrivers/USBP4` to `SYS:Storage/DOSDrivers/USBP4` and run `Mount USBP4: FROM SYS:Storage/DOSDrivers/USBP4`, then `Info` / `List USBP4:`. This is a read-only mount; the driver rejects all write requests.
6. When the read-only mount has been validated, move `USBP4` to `SYS:Devs/DOSDrivers/USBP4` for normal boot-time mounting. Back up an existing `USBP4` file first. Do not configure automatic mounting while failures remain.

The kit deliberately does NOT automatically write to DEVS:, change S:Startup-Sequence, copy/replace any Pi3 usbmass.device, or format a USB device. Check Pi4 FRUM board visibility, Fat95 presence and MBR partition layout before enabling. Do not modify PFS-III.

Files: DEVS/pi4usbmass.device, Storage/DOSDrivers/USBP4, C/pi4usbmass-info, C/pi4usbmass-mountinfo, C/usbmass-z2-pi4, README.md.

Status: tools cross-compiled and source regression tested, NOT tested on the real Pi4 AmigaOS filesystem or live USB FAT32 drive. If OpenDevice fails, the FRUM virtual Zorro board or guest file installation still needs debugging.

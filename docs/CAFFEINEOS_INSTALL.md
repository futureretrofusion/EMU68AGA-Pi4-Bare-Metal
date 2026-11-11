# Install EMU68AGA Bare Metal Pi4 R101 with CaffeineOS

For an existing working CaffeineOS Raspberry Pi 4 SD card:

1. Shut down the Pi4 and insert its SD card into your computer.
2. Open the FAT32 boot partition and identify the existing **`emu68-*.img`** kernel filename.
3. Back up that image and `config.txt`.
4. Download the Pi4 R101 kernel, **rename it to exactly the same filename as your existing `emu68-*.img`**, and replace the old file on the FAT32 partition.
5. Leave `config.txt`, your ROM/boot bundle and all Amiga partitions unchanged.
6. Safely eject the card and boot the Pi4.

The exact filename may differ between CaffeineOS versions. If you want to keep the old kernel alongside the new one, instead use a different filename for the new kernel and update the existing `kernel=` entry in `config.txt` to match. Do not add a second kernel entry. **Do not rename the kernel to `kernel8.img` unless your own boot configuration explicitly calls for that filename.**

## Update VideoCore.card for Picasso96 RTG

The reference driver is **VideoCore.card 1.5 from AGA-PiStorm universal v1.120**. Its Amiga destination is `LIBS:Picasso96/VideoCore.card`. Back up the old file first. If the existing driver is already the same working version, leave it unchanged.

### Method A — Transfer via the FAT partition

Copy `VideoCore.card` to the FAT boot partition using your PC. In CaffeineOS, or an early boot environment that can mount both FAT and Amiga filesystems, copy the file to `LIBS:Picasso96/VideoCore.card`. The FAT volume name varies; don't assume it is mounted automatically.

### Method B — Transfer using an Amiga emulator

Open an image backup or the Amiga RDB disk in an emulator with RDB support. Boot an AmigaOS environment, provide the new driver through a host-directory mount, and copy it to `LIBS:Picasso96/VideoCore.card`. Do not format or repartition the disk. Shut down cleanly and return the SD card to the Pi4.

Cold boot and verify both native AGA and Picasso96 RTG screens. If RTG fails, restore the backed-up VideoCore.card. If the Pi4 fails to boot, restore the backed-up `emu68-*.img` kernel.

No Kickstart ROM, CaffeineOS installation, AmigaOS media or commercial games are supplied. Driver credit: AGA-PiStorm contributors; the separate download includes the original GPL-2.0 licence and source archive.

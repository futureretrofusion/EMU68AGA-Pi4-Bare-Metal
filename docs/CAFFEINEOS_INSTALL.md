# Install EMU68AGA Bare Metal Pi4 R101 with CaffeineOS

This guide assumes you already have a working **CaffeineOS Amiga installation** on an SD card for the Raspberry Pi 4, including its Amiga RDB partitions, Picasso96 installation and a legally obtained Kickstart ROM. **Do not format the SD card or overwrite the Amiga partitions.** Back up the entire SD card first.

## 1. Update the Pi4 boot kernel

1. Shut down the Pi4, remove its SD card and insert it into your PC.
2. Open the **FAT/FAT32 boot partition** (typically labelled `EMU68`). Windows and Linux can access this partition without Amiga filesystem tools.
3. Back up `config.txt` and your currently working kernel.
4. Copy the downloaded `kernel8_R101_PI4_ORIGINAL_SPLASH_BOOT_POST.img` onto the FAT partition.
5. Edit the existing `config.txt`. In its **[pi4]** section, set:

   ```ini
   [pi4]
   kernel=kernel8_R101_PI4_ORIGINAL_SPLASH_BOOT_POST.img
   ```

   **Do not replace the entire config.txt** with this snippet. Preserve its other settings, the [pi3] and [all] sections, and especially its existing `initramfs ... followkernel` entry.
6. Make sure the file named by `initramfs` **actually exists** on the FAT partition. This is the ROM/boot bundle required by your particular CaffeineOS installation. It is **not** the same file as the Pi4 kernel. Keep your working bundle and legal Kickstart files; none are provided in this release.
7. Safely eject the card and start the Pi4.

## 2. Update VideoCore.card for Picasso96 RTG

**Important:** CaffeineOS users who want Picasso96/RTG should check their installed `VideoCore.card`. The reference updated driver is **VideoCore.card 1.5 from AGA-PiStorm universal v1.120** (30 September 2026, 30,392 bytes; SHA-256 `298df59cbc628488e8d1484acf616ac48a6d198131ebf2fca961ffc5ceb994e9`).

The Amiga destination is:

```text
LIBS:Picasso96/VideoCore.card
```

`LIBS:` means the AmigaOS logical assignment for its libraries directory; on many installations this corresponds to `SYS:Libs/`. The driver is **not** installed in `DEVS:Monitors/`. Keep the existing Picasso96 monitor setup.

Back up your existing `LIBS:Picasso96/VideoCore.card` before replacing it. If the installed driver is already the same working version, no replacement is necessary. Native AGA output does not require this driver; Picasso96 RTG does.

### Method A — FAT partition transfer, then copy in AmigaOS

This avoids accessing the Amiga RDB partition from your PC.

1. With the SD card in your PC, copy the updated `VideoCore.card` onto the **root of the FAT boot partition** next to `config.txt`.
2. Boot CaffeineOS. If your FAT boot volume is already mounted inside AmigaOS, open that volume and copy `VideoCore.card` to `LIBS:Picasso96/`, replacing the old file after making a backup.
3. If the FAT partition is **not** mounted inside AmigaOS, use the system's early boot environment or boot menu **if it provides access to the FAT boot volume and an Amiga filesystem copy utility**. The FAT partition is not automatically an AmigaOS volume merely because the Pi firmware can read it. Identify the source volume and destination before copying. Do not guess a device or volume name.
4. From an Amiga Shell, **once the FAT volume is visible**, replace `BOOTFAT:` below with its actual mounted AmigaOS volume name:

   ```text
   Copy LIBS:Picasso96/VideoCore.card LIBS:Picasso96/VideoCore.card.bak
   Copy BOOTFAT:VideoCore.card LIBS:Picasso96/VideoCore.card
   ```

5. Cold reboot and test Workbench RTG, native AGA screens and switching back to RTG.

**Early-boot caveat:** The early boot screen alone cannot copy a file unless its environment exposes both filesystems and provides a file-copy command. If it does not, use Method B.

### Method B — Mount the Amiga partition in an emulator

1. Shut down the Pi4, remove its SD card and attach it to a PC. Preferably work from a full **image backup** of the card.
2. Open the card or its image in an Amiga emulator that supports **Amiga RDB hard disks** (for example WinUAE, or an appropriately configured FS-UAE). Use the emulator's hard-drive configuration to attach the **whole SD image/device as an RDB disk**, not the FAT partition as a replacement for the Amiga disk.
3. **Do not initialise, partition or format** the disk if the emulator offers to do so. Use read/write access only after confirming the correct image or device, and ideally operate on a backup first.
4. Start an AmigaOS environment that can read the CaffeineOS system partition. Transfer the updated `VideoCore.card` into that environment through an emulator host-directory mount, shared folder or virtual floppy.
5. Back up the existing `LIBS:Picasso96/VideoCore.card` and copy the updated driver into the same directory.
6. Shut down the emulator cleanly, write the modified image back to the SD card **only if you worked on a copy**, safely eject, and boot the Pi4.

## 3. Verify and recover

- Verify the original splash appears, followed by the compact boot POST window, and CaffeineOS starts.
- Test keyboard, mouse, a directly connected gamepad and both AGA and Picasso96/RTG display modes.
- Cascaded Huasheng 214b:7260 USB hubs may disconnect; use direct USB connections during initial setup.
- If the Pi4 does not boot, restore your previous `kernel=` line in `config.txt` and confirm the configured `initramfs` file exists.
- If RTG fails but AGA still works, restore the backed-up `VideoCore.card`.

The kernel and VideoCore driver are separate components. Neither includes a proprietary Kickstart ROM, CaffeineOS installation, AmigaOS, or commercial games.

**Driver provenance:** The VideoCore.card 1.5 reference comes from AGA-PiStorm universal v1.120, not a new build by Future Retro Fusion. The driver is distributed with its original GPL-2.0 licence, source archive and attribution.

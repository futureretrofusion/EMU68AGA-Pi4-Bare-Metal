# FRF EMU68AGA Pi4 R112 — USB mass storage enumeration + HDD spin-up diagnostic

**Experimental Pi4-only read-only test**, based on R111 which derives from **R110 (hardware-confirmed Pi4 scanlines)**. No changes to the Pi4 AGA HVS renderer, scanline command decoder, or Pi4 1280x720/50 HDMI configuration.

## Root causes and corrections

1. **R111 had a global four-HID-interface gate in both hub enumeration loops.** If four HID interfaces were registered, the loop could skip the drive on a later port. R112 removes those gates. A full HID table no longer prevents storage discovery.
2. **R111 reserved only eight xHCI slots/context banks**, and only eight hub-child entries. R112 allocates 24 slots, 24 independent child-context/DMA banks and 24 entries; the actual number remains limited by the hardware-reported xHCI MaxSlots and available USB ports/hub routes. Eight concurrent HID interfaces are supported in the host arrays and native per-device pad/keyboard/mouse state. **This is not unlimited USB support**.
3. R111 reset its software host state to the first HID and consequently could print `msc=0` in the general enumeration summary **even after a mass-storage bulk slot was configured**. R112 preserves the MSC-ready host indicator alongside HID.
4. USB hard disks may need time to spin up. R112 performs up to 12 bounded SCSI READ CAPACITY(10) attempts, separated by at least two seconds after the initial request; it updates the guest mailbox and the mass-storage ready counter when the disk becomes ready later. It identifies BOT failure stage and SCSI CHECK CONDITION separately.
5. R112's original Pi4 boot POST adds **MSC SEEN / CFG / READY / UAS / TRY / BOT STAGE**, so the failure can be localized from a boot photo rather than guessing at a filesystem.

## Compatibility and limits

- One **boot-attached** disk, LUN0, USB mass storage class 08/subclass 06/BOT protocol 50, 512-byte logical blocks, READ CAPACITY(10), READ(10) up to 16 sectors/operation.
- **Read-only.** The AmigaOS `usbmass.device` is the tested FRUM v1 RO driver. The ARM USB layer rejects write requests too.
- USB UAS protocol 62 is **not yet supported** (reported as `UAS` in POST). A USB 2.0 BOT-compatible mode may work, but is not guaranteed.
- This does not provide hotplug, multiple mass-storage units, or automatic NTFS/FAT/RDB filesystem mounting. The hardware, hub and keyboard/mouse coexistence **have not yet been verified on a real Pi4 in R112**.
- A spinning, bus-powered HDD may need a powered USB hub or separate power supply; do not assume data/protocol failure if the drive cannot spin up.

## Pi4 SD boot installation

Copy `BOOT/emu68-pi4-r112-usbmass-detect-test.img` into the FAT32 boot partition root and update **only the Pi4** config stanza:

```ini
[pi4]
kernel=emu68-pi4-r112-usbmass-detect-test.img
```

**Keep** Pi4 `hdmi_group=1`, `hdmi_mode=19` (720p50), `disable_overscan=1`, `dtoverlay=emmc,unit0=rw`, and installed `overlays/emmc.dtbo`. Do not change [pi3], [pi5], or other entries. R110 and R111 kernels must remain available for rollback.

## AmigaOS test files

**Important:** Merely copying the files to the Pi4 FAT boot partition is **not** installing them in AmigaOS. Transfer `DEVS/usbmass.device` to the actual Workbench/CaffeineOS `DEVS:`, and transfer the two `C/` executables to its `C:` via your existing Amiga file-transfer mechanism.

```
C:usbmass-z2-pi4
C:usbmass-info-pi4
```

- `usbmass-z2-pi4`: **does not require** OpenDevice. Reads expansion.library's FRF 6D73:0069 autoconfig board, shared mailbox, CPU1 xHCI slot, capacity/presence and media state. This separates firmware enumeration from the guest driver.
- `usbmass-info-pi4`: Opens `usbmass.device` unit0, asks `TD_GETGEOMETRY`, reads raw LBA0 and recognises RDB or MBR. No writes.
- Neither utility automatically mounts or modifies the USB drive. An Amiga-formatted RDB partition may still need an AmigaOS mount/boot-node integration; FAT32 needs a compatible Amiga filesystem such as fat95 with an accurate DOSDriver entry. A PC NTFS/GPT disk cannot be expected to appear as a Workbench icon merely because its USB sectors can be read.

## How to interpret R112 POST

| Display | Meaning |
|---|---|
| MSC SEEN=00 / CFG=00 | Device not enumerated as BOT. Check power/cable, direct Pi4 port vs powered hub, and whether hub sees the device |
| UAS greater than zero | USB bridge advertises UAS (protocol 62), not the supported BOT protocol 50 |
| MSC SEEN greater than zero / CFG=00 | USB class found, but xHCI bulk endpoint configuration failed |
| MSC CFG greater than zero / READY=00 | USB BOT endpoints present; SCSI READ CAPACITY not yet successful, check TRY and BOT STAGE |
| MSC READY greater than zero | ARM disk transport exposed; use Zorro-II diagnostic to verify guest bridge, then install guest driver and mount the filesystem |

BOT STAGE 1=CBW OUT failure, 2=DATA IN failure, 3=CSW IN failure, 4=bad CSW signature, 5=tag or residue mismatch, 6=SCSI CHECK CONDITION/nonzero status. Zero means the latest BOT transaction succeeded.

**Cold-boot test sequence:** first use a supported USB flash stick directly in a Pi4 port while your working mouse/hub remains attached. Then test the USB HDD with external power or a powered USB hub and the same hub/mouse setup. Confirm R112 POST, scanlines, keyboard+mouse, MSC counters and `C:usbmass-z2-pi4` results.

## Code and tests

- `tests/test_pi4_r112_topology.py`: verifies no premature 4-HID storage gate, 24 xHCI resource slots and 8 HID records.
- `tests/test_pi4_r112_mass.c`: mocks only USB bulk transport and executes the real Pi4 mass worker to test CBW/CSW, SCSI capacity/READ10, read bounds, write rejection, HDD not-ready/retry, and late Z2 mailbox status.
- `tests/test_pi4_r110_protocol.py`: confirms the previously-working Pi4 AGABOOT control decoder remains unchanged.
- ARM BE kernel cross-compiles with `-DFRF_PI4_XHCI_HID=ON`, `-DFRF_PI4_USB_MASS_RO=ON`, `-DFRF_VIRTUAL_AGA=ON`, `-DVARIANT=pistorm-classic`.

## Rollback

Restore only the Pi4 kernel line to `kernel=emu68-pi4-r110-agaboot-control-only-test.img` to get back to the hardware-confirmed scanline baseline. **R112 remains unverified on actual Pi4 hardware and should not replace the stable R110 release.**

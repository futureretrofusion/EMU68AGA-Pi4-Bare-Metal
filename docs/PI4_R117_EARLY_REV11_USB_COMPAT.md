# Future Retro Fusion EMU68AGA Pi4 R117 — early Pi4B rev1.1 USB compatibility diagnostic

**Experimental ARM-native Pi4B build; real rev1.1 USB input has NOT yet been confirmed.** The stable R115 package, R114 no-stall transport, R113 mouse diagnostic, R110 AGABOOT and scanline controls remain preserved. No support from Pi3's DWC2 is used.

## Target from user hardware report

- **Older 4GB Raspberry Pi 4 Model B, board revision 1.1:** EMU68AGA boots but USB keyboard/mouse are not detected.
- **Newer 1GB Pi 4 Model B, revision 1.5:** USB works under a particular bootloader.
- Both boards operate USB input with other systems, so the discrepancy should be traced in our Pi4 native USB / firmware initialization, **not treated as proof of broken hardware**.

Official Raspberry Pi EEPROM documentation confirms the early 4B revision 1.1/1.2 has a separately programmed VL805 firmware EEPROM; newer 4B revisions use firmware bundled with the bootloader EEPROM. A rev1.1 PCB also differs in power input behaviour from rev1.2. USB behaviour may depend on the combination of bootloader, installed VL805 firmware, and PCIe initialization. Do not flash VL805, change boot EEPROM, or force a Pi4C/CM4-specific bootloader setting as part of this kernel test.

## R117 changes

1. **Correct board model and PCB revision** from the firmware device tree's `/system/linux,revision` (if available). Decode 4B type 0x11, PCB nibble and RAM code. **Never decide legacy status solely from 4GB memory size**.
2. **Older EEPROM handling** only for Pi4B revision 1.1 or 1.2. R117 leaves working rev1.5 PCIe, USB host and xHCI initialization alone.
3. **Additional 150ms firmware settle** before xHCI rings are initialized, for old EEPROM-based Pi4B revisions only.
4. **Exactly one guarded firmware-notify recovery** only if the early board's xHCI capabilities fail validation **and** its PCI config firmware-version register is nonzero. If the R115 driver already attempted a zero-version firmware notification, R117 does not repeat it. This is a firmware `NOTIFY_XHCI_RESET` request, **not a flash or EEPROM write**. After success, it re-establishes endpoint BAR+PCI command bits, then rereads xHCI capabilities. No repeated global controller/PCIe reset.
5. **Slightly longer first hub scan budget:** 16 seconds for confirmed rev1.1/1.2 vs existing R115 12 seconds for newer boards; AmigaOS CPU0 boots independently while CPU1 scans.
6. **Live HDMI diagnostics on native AGA screen** for the older-board case: raw board revision, PCB revision, RAM code, PCIe link status, VL805 firmware before/after, recovery status, DMA-failure mask, xHCI stage, PCIe-capability registers, root PORTSC and USB HID/storage counts.
7. **No changes** to R115 MSC read-only storage transport, USB HID report mapping, R110 1280×720p50 HDMI, integer scaling, scanlines, AGABOOT, P96 mode switches or AmigaOS drives.

## R117 POST fields

| Field | Use |
|---|---|
| `BOARD:` | Full Pi firmware revision; e.g. `00C03111` identifies a 4GB Pi4B PCB rev1.1 |
| `PCB:1.x` | PCB revision printed from 4-bit revision nibble |
| `RAM:` | RAM code (4=4GB, 2=1GB), not a compatibility decision |
| `FW PRE / POST` | VL805 PCI config-space firmware version before and after any firmware notification |
| `PCIE` | PCIe root link status, for diagnosing dead or uninitialised links |
| `REC` | 00=not attempted (normal), 01=attempt in progress, 02=firmware notified and xHCI caps recovered, 03=recovery did not validate |
| `DMA` | xHCI inbound ring address validity; 0000 means no DMA range rejection reported |
| `XHCI STAGE` | Last CPU1 host stage; compare between old/new board |
| `ROOT` / `P1` | Root-port state and first port status if HID is absent |
| `FWRC` / `CAPS` | Firmware-notify return code and raw xHCI CAPBASE when no keyboard/mouse is present |
| `HOST`, `HID`, `KB`, `MS`, `MSC READY` | Functional readiness and USB enumeration outcomes |

If `BOARD=00000000`, the firmware device tree did not supply the standard board-revision property. **No legacy-specific recovery is attempted** in that case, so the build stays conservative.

The **firmware-version register being nonzero does not prove the xHCI controller is functional**: independent readback of xHCI CAPBASE and PCIe root status remains necessary.

## Hardware acceptance test (rev1.1 and rev1.5)

1. Boot the same backed-up R115 kernel on both Pi4B boards and record whether keyboard/mouse work. Do not change hardware/bootloader during comparison.
2. Test R117 on the **4GB Pi4B rev1.1** with a keyboard and mouse plugged directly into USB-A, without external hub. Cold power off between tests. Photograph the full R117 POST after Workbench has started.
3. Check `BOARD`, `FW PRE / POST`, `PCIE`, `REC`, `FWRC`, `CAPS`, `ROOT`, `P1`, `HID`, `KB`, `MS`, `HOST` values.
4. Test identical R117 SD card and USB devices on the **1GB Pi4B rev1.5**. Confirm no unnecessary recovery `REC=00`, and input continues to work. Compare firmware and root-port statuses.
5. Only after both boards have reliable HID input test a FAT32 mass drive; R115 reported `MSC READY=01` on the newer board, but guest FAT95 mounting is a separate unresolved issue.

**Rollback:** On verified Pi4 boot FAT partition restore `kernel=emu68-pi4-r115-early-msc-scan-budget.img`. Do not delete the R115 kernel, and do not modify `hdmi_group=1`, `hdmi_mode=19`, the AGABOOT/scanline controls, `dtoverlay=emmc,unit0=rw`, the Amiga RDB partition, or any unrelated PFS-III configuration.

## Validation performed on Studio PC

- Big-endian AArch64 Pi4 native kernel compiled.
- Standalone C regression tests assert early 4GB board rev1.1/1.2 handling, new 1GB board rev1.5 exclusion, unknown/misidentified board rejection.
- Python source checks for single guarded firmware recovery, native xHCI capability precondition, preserved framebuffer size and visible POST.
- R115 BOT/SCSI host-mock tests passed.
- R112 USB topology and R110 AGABOOT protocol regression tests passed.

**Not yet proven:** on-device older rev1.1 physical USB enumeration, firmware recovery effectiveness, and hotplug. R117 is a diagnostic/test candidate, not an established compatibility fix.

# Priority 1: Raspberry Pi 400 Wi-Fi failure — research and patch candidate

**Status: two reported failures; candidate identified, neither built nor hardware-tested.**

Two testers report that CaffeineOS 3.9 never completes Wi-Fi connection on the Pi 400 with the EMU68AGA bare-metal Pi4-family kernel. This research branch is based on the working Pi4 R107 USB code. Do **not** reset xHCI, modify working Pi4 USB HID or publish a new stable release on the strength of this patch alone.

## Source-level finding

- Raspberry Pi 400 advertises device-tree root `compatible = "raspberrypi,400", "brcm,bcm2711"`.
- Upstream `michalsc/WiFiPi.device` `src/init.c` selects the firmware descriptor from the **first** `compatible` string. Its `FirmwareMatrix` currently has Pi 3A+, 3B+, 4B and CM4 entries, **but no Pi 400**.
- The existing WiFiPi CM4 descriptor already contains the BCM43456 selection: chip ID `0x4345`, revision mask `0xffb0`, files `brcmfmac43456-sdio.bin`, `brcmfmac43456-sdio.clm_blob`, `brcmfmac43456-sdio.txt`.
- A one-line source patch adds `raspberrypi,400` pointing to this existing descriptor. Located at `patches/WiFiPi_Pi400_BCM43456_model_fix.patch`.
- Raspberry Pi 400 BCM43456 firmware differences are documented independently; do **not** spoof the Pi 400 device tree as a Pi 3A+ or Pi 4B.
- **The installed CaffeineOS 3.9 `wifipi.device` version and its firmware files have NOT been verified.** The upstream patch must first be checked against the exact bundled source/binary. A source patch is not an installed fix.

## Local FRF repository audit

Pi4 test build configuration has `FRF_WIFI_GUEST_COMPAT=OFF`, `FRF_WIFI_HW_PROBE=OFF`, `FRF_WIFI_SD1_ENUM=OFF`. Those optional modules are for the Pi 3A+, not a Pi 400 network driver. `FRF_NET_Z2_BRIDGE=ON` contains test shared-memory framing, **not** working wireless networking. The intended Wi-Fi driver for this investigation remains Amiga-side `wifipi.device`, using SDIO; never attempt simultaneous independent ARM ownership of the Wi-Fi SDIO host.

## Hardware validation sequence

1. Confirm physical board is Pi 400 and kernel boot reports the genuine board identity; capture `compatible` and `model` from device tree without altering them.
2. Inspect CaffeineOS version of `DEVS:Networks/wifipi.device` and check presence/identity of BCM43456 `.bin`, `.clm_blob` and `.txt` in `DEVS:Firmware`. Back up originals.
3. Inspect `T:wifipi.txt` (if produced) and radio firmware-load return/status, without collecting SSID or passwords. Verify firmware selection chip ID/revision.
4. Compile the corresponding WiFiPi source with Pi 400 firmware-matrix entry; test Wi-Fi scan and association independently of MiamiDx; then test TCP/IP/DHCP and CaffeineOS autoconnect.
5. If connection still stalls, investigate `WirelessManager`, `WaitUntilConnected`, `STARTNET` and MiamiDx online script separately, using bounded timeouts so the desktop remains responsive.
6. Verify both testers can connect, disconnect, reboot, and reconnect without regressing Pi4 R107 USB input or USB hubs.

## References

- Upstream WiFiPi source: https://github.com/michalsc/WiFiPi.device/blob/main/src/init.c
- Pi400 device tree: https://code.googlesource.com/linux/torvalds/linux/+/e2e13630f93d942d02f3b3f98660228a3545c60e/arch/arm/boot/dts/broadcom/bcm2711-rpi-400.dts
- Pi400 BCM43456 firmware evidence: https://github.com/AlmaLinux/raspberry-pi/issues/23

**Not a validated driver fix. No WiFiPi binary has been rebuilt or flashed as part of this research.**

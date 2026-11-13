# Pi400 WiFiPi R1 — compiled test artifact

Build date: **13 November 2026**

**Hardware status:** cross-build completed; Pi400 association/DHCP **not tested**.

## Results on Studio Main

- Local isolated upstream: `michalsc/WiFiPi.device` at commit `1752e4f`, version **0.4.0**
- AmigaOS cross-compiler: `/opt/amiga/bin/m68k-amigaos-gcc` 6.5.0b
- Patch: one `FirmwareMatrix` entry `raspberrypi,400 -> modelCM4Desc`
- Driver compiled with CMake target `wifipi.device`; `file` reports an AmigaOS loadseg-compatible binary
- Binary size: **49,528 bytes**
- SHA256: `1186428cba9f0a8595779bceb2e88b14faf752da5ff55320460d974be2e166d1`
- Binary strings include `raspberrypi,400` and all three `brcmfmac43456-sdio` filenames
- No EMU68AGA kernel, Pi4 R107 USB, or SD card modified

Packaged local artifact:
```
/home/fff2025/Downloads/PI400_WIFI_PATCH_R1_20261113/
    FRF_PI400_WIFIPI_0_4_R1_TEST.zip
```
SHA256 ZIP: `770531c555978767cbfe69309cd78675c7629a2b34cc3dedd0b6d437f9f43fc6`

The ZIP includes `Devs/Networks/wifipi.device`, `Devs/Firmware/brcmfmac43456-sdio.{bin,clm_blob,txt}`, `README_INSTALL.txt`, `WiFiPi_Pi400_source.patch`, and `SHA256SUMS.txt`. ZIP CRC integrity check passes.

## Reproducing the build

Apply [the one-line WiFiPi patch](../patches/WiFiPi_Pi400_BCM43456_model_fix.patch) to **WiFiPi 0.4.0** at commit `1752e4f`, with recursive submodules available. Configure in an isolated build directory:

```bash
cmake -S WiFiPi.device -B build-pi400 \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/emu68-driver-stack/cmake/toolchain.cmake \
  -DTOOLCHAIN_PATH=/opt/amiga -DCMAKE_BUILD_TYPE=Release
cmake --build build-pi400 --target wifipi.device -j6
file build-pi400/wifipi.device
strings build-pi400/wifipi.device | grep -F 'raspberrypi,400'
```

`cmake` defaulted to `/opt/m68k-amigaos` on the host; setting `TOOLCHAIN_PATH=/opt/amiga` was required.

## Tester procedure

1. Capture version/size of existing `DEVS:Networks/wifipi.device`; back up original driver and firmware.
2. Replace only `DEVS:Networks/wifipi.device` with patched binary; do not change SSID/password, MiamiDx configuration or USB kernel.
3. Check `DEVS:Firmware` has the BCM43456 files; restore originals if anything regresses.
4. Cold boot Pi400 and test **scan → association → DHCP → LAN ping → reconnect after reboot**.
5. If failing, record exact failure stage and firmware load error (redact Wi-Fi credentials). Restore saved driver to roll back.

**Caveat:** The Pi400 model entry is a strong source-level explanation for firmware selection failure, but two testers have not yet verified it. It does not independently implement WPA or TCP/IP.

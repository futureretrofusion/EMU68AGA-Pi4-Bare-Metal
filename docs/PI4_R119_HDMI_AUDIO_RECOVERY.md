# FRF EMU68AGA Pi4 R119 — HDMI audio recovery and AGABOOT Sound diagnostics

**Experimental test for missing HDMI audio on recent Pi4 builds, Saturday 14 November 2026. Hardware audio restoration is not yet confirmed.**

## Investigation result

R110's new AGABOOT trap handlers add picture controls. They do not delete or intentionally disable the existing standalone HDMI audio initialization. The AGA code still calls `aga_hdmi_boot()`, `aga_hdmi_paula(0)`, and `aga_hdmi_enable(1)` at boot in `machine/arm/aga_glue.c`.

There is nevertheless an existing Pi4 initialization defect in `machine/arm/aga_hdmi.c`: the `get_clock_rate_measured(HSM_CLOCK)` query can return zero even with a display showing a picture (previously documented in the source for a CM4). The old code set `H_NO_CLOCK` and returned without setting `ready`. Therefore the subsequent standalone `aga_hdmi_enable(1)` did nothing; `H_NO_CLOCK` on Pi4 is not retryable; and HDMI sound could never start. This is a source-proven failure path, **not proof that it caused this specific user's silent audio**.

An AGABOOT **Sound Off** word `$5820` can also turn audio off. A user-saved AGABOOT setting may have issued this word; the code has always accepted it. R119 distinguishes it from a missing-clock failure without changing the requested Sound On/Off semantics.

The current Pi4 audio implementation targets **HDMI0 only**. On a Raspberry Pi 4B, HDMI0 is the micro-HDMI socket nearest the USB-C power connector. HDMI1 is not implemented as an audio output by this driver.

## R119 changes

- Retain the proven fixed **108 MHz Pi4 MAI/DVP audio clock** when the firmware's HSM measurement returns zero. Initialize ring and DMA-channel bookkeeping instead of returning permanently silent.
- Enter `H_NO_LINK` and defer HDMI MAI register access until `guard_now()` or `guard_ok()` observes actual HDMI HVS picture motion. The audio service on CPU2 then retries. **Never write HDMI registers when the picture guard fails.**
- Pi3 HSM clock measurement, clock checks and existing startup path remain unchanged.
- Keep explicit `aga.nohdmiaudio` and `aga.hdmiforce` command-line controls; the former remains authoritative and the latter remains an explicit advanced override.
- Record AGABOOT Sound On/Off writes (`$5821` / `$5820`) without changing their effects.
- Record each accepted HDMI audio-feed batch and rejected batch to distinguish an uninitialized audio path from silence generated upstream.
- Add native AGA HDMI diagnostic overlay while retaining USB HOST/HID/MSC read-only storage, R113 mouse and R117 older-Pi4B compatibility code.
- No changes to AGABOOT scanlines, integer scaling, P96 handoff, native xHCI USB mass storage, Zorro FRUM interface, any AmigaOS device driver, or USB filesystem mounting. R117 is kept for immediate rollback.

## Reading R119 LIVE HDMI AUDIO panel

| Field | Meaning |
| --- | --- |
| `HDMI ST` | 00=off, **01=running**, 02=waiting for active HDMI picture, 03=no HSM clock, 04=packet slot busy, 05=no DMA, 07=disabled by command line, 08=another driver owns MAI |
| `W` | 01=HDMI audio currently requested, 00=sound switched off |
| `RD` | 01=HDMI audio initialization completed, 00=not initialized |
| `OFF` | 01=`aga.nohdmiaudio` was read from the kernel command line; this is different from AGABOOT Sound Off |
| `HSM kHz` | The boot firmware-measured HSM clock in kHz; zero by itself does **not** establish a dead HDMI picture on Pi4 |
| `FALL` | 01=R119's Pi4-only zero-HSM recovery path was exercised |
| `DMA` | DMA channel number + 1, or 00 when none has been selected |
| `SND CMD` | Number of AGABOOT sound On/Off commands seen by Pi4 trap handlers |
| `OFF` (row with SND CMD) | Number of AGABOOT Sound Off writes seen |
| `LAST` | Latest AGABOOT Sound setting: 0=off, 1=on, F=no command received |
| `HDMI FEED` | Accepted audio-feed batches from the virtual Paula/AHI path (should increase during actual sound output) |
| `REJ` | Audio-feed batches rejected before initialization or during unavailable audio |
| `HOST/KB/MS/MSC READY` | Existing Pi4 USB diagnostics remain, useful to confirm no USB regression |

**Interpretation:** `W=00`, `SND CMD>0`, `LAST=0` points to AGABOOT intentionally switching sound off; change the AGABOOT Sound setting to On. `HSM kHz=0`, `FALL=1`, `RD=1`, `W=1` confirms the new fallback ran and wants sound once the HDMI link is active. `HDMI ST=01` with `HDMI FEED` increasing but no audible sound points to HDMI0 MAI/DMA, monitor audio output or HDMI cabling, not the high-level Sound toggle. `HDMI ST=01` with no feeds while playing a game that produces Paula audio suggests the upstream audio producer is not running.

## Hardware test

1. Keep HDMI connected to **HDMI0 (nearest USB-C)** and use the same TV/monitor and audio-generating Amiga game or demo as before. Test with the same USB setup.
2. Install R119 to the verified **Pi4** FAT32 EMU68 boot partition only, backing up the original `config.txt`. Use `kernel=emu68-pi4-r119-hdmi-audio-recovery.img` in the active `[pi4]` section. Keep `R117` and the current AmigaOS RDB files untouched.
3. Cold-boot and compare boot speed, USB input and HDMI display, then try a game/demo with known Paula audio.
4. Photograph **R119 HDMI AUDIO** panel, particularly `HDMI ST`, `W`, `RD`, `OFF`, `HSM kHz`, `FALL`, `DMA`, `SND CMD`, `LAST`, `HDMI FEED`, `REJ`.
5. If `SND CMD` increments and `LAST=0`, open the existing AGABOOT Sound menu, select Sound On and retest. Don't permanently change other AGABOOT settings yet.
6. If `HDMI ST=01` and feeds increase but sound remains absent, compare HDMI0 vs monitor audio controls and inspect the DMA/MAI status before modifying registers further.

**Rollback:** restore `kernel=emu68-pi4-r117-legacy-rev11-usb-test.img` in the original Pi4 section of `config.txt`; keep all other options and kernel images.

### Software verification

Cross-compiled for **big-endian AArch64 Pi4** with `FRF_PI4_HDMI_AUDIO_DIAG=ON`, `FRF_PI4_LEGACY_REV_DIAG=ON`, `FRF_PI4_USB_MASS_RO=ON`, and existing POST/quiet mode. Static regression checks prove that zero-HSM recovery is Pi4-only, is guarded by HVS motion, and tracks both AGABOOT handlers, sound state, feed counters and picture layout. R115 BOT/SCSI, R117 legacy revision decode, R112 topology, and R110 AGABOOT protocol tests passed. **These tests do not establish physical HDMI audio output.**

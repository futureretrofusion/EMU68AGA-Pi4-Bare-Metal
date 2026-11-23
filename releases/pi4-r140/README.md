# Pi4 R140 — USB multi-slot card-reader LUN readiness (candidate)

**Source-only candidate. No R140 image has been released or written to an SD card.**

The reported PFS3 initialization failure is on an **external USB card reader attached to the Pi4**, not on the Pi4 boot SD's internal Amiga RDB partition. CaffeineOS already has its original Pi3 mount files; do not change them.

## Suspected Pi4-only failure mode

`frf_pi4_usb_mass_multi.c` probes a newly registered physical USB reader by sending SCSI READ CAPACITY(10) directly to LUN0. An empty first card-reader slot can reject that command without supplying its expected 8-byte data stage. The existing BOT handler treats the missing/short data stage as a failed physical endpoint and calls `kill_host()`, invalidating **all LUNs** on that reader before a populated LUN1/2/3 is examined.

The R140 candidate issues zero-data TEST UNIT READY (TUR) for each logical LUN **before** READ CAPACITY. A normal CHECK CONDITION on an empty slot does not kill the reader; other LUNs remain eligible for discovery. The existing fair retries continue for temporarily not-ready media. `frf_xhci_host.c` now logs the success or failure and returned value of GET_MAX_LUN.

## Validation already performed in Studio Main

- Cross-built big-endian Pi4 ARM64 image before final logging change: **PASS**.
- R140 host mock regression with LUN0 empty, LUN1 containing RDB, and a second physical USB disk: **PASS**.
- Normal occupied LUN0 + LUN1 scenario: **PASS** in the same mock.
- No disk writes were added; the RDB protection remains.
- **Not hardware-verified**: no Pi4 reader boot test, no confirmed CaffeineOS PFS3 mount, and no assertion that the real reader actually returns TUR CHECK CONDITION.
- Studio Main tunnel dropped during final rebuild/checks. Consequently R140 is not installed and no downloadable R140 kernel has been validated.

## Reproduce host regression

```sh
gcc -O2 -std=c11 -Wall -Wextra -Werror -DFRF_PI4_USB_MASS_RW=1 \
  -Itests/r111-include -Imachine/Emu68/include/frf_xhci \
  -Imachine/Emu68/src/frf_usb \
  tests/test_pi4_r140_multilun_reader.c \
  machine/Emu68/src/frf_xhci/frf_pi4_usb_mass_multi.c \
  -o /tmp/r140_reader_test
/tmp/r140_reader_test
```

## Read-only hardware diagnostic plan

1. Boot with the **external USB card reader and populated RDB card connected before power-on**. To isolate unit mapping, temporarily test with no other external USB storage.
2. Watch kernel logging for `[R140-MSC] ... GET_MAX_LUN ok= ... value=...`, `mapped unitN slotS LUNL`, `not ready TUR/BOT`, `READY`, and `Amiga RDB found`.
3. From the Amiga, run the existing R138 read-only RDB diagnostic to inspect the FRUM v2 mapping and each physical LUN. This requires a working installed AmigaOS `usbmass.device`, not merely a staged copy on FAT32.
4. If GET_MAX_LUN fails or reports 0 despite a multi-slot reader, investigate its USB control request and endpoint STALL recovery, **rather than** altering PFS3 or auto-mounting RDB.
5. If LUNs appear but reads fail, inspect BOT CBW/data/CSW stages and transfer-completion status; isolate the bad slot without killing all units.
6. Verify `DEVS:usbmass.device` version (staging a v0.8 binary on Pi boot FAT32 does not install it in AmigaOS). Leave original CaffeineOS USBR0..USBR4 mounts unchanged.

## Rollback and safety

Retain R138 boot kernel and its config.txt entry until an R140 binary is validated. Never format, initialize, repair, or write the user's RDB media. Mass storage hotplug remains unsupported. HID stack and display settings should stay unchanged.

Tracking: https://github.com/futureretrofusion/EMU68AGA-Pi4-Bare-Metal/issues/7

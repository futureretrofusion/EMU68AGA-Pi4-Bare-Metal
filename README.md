# EMU68AGA Bare Metal — Raspberry Pi 4

**Future Retro Fusion | Independent Raspberry Pi 4 development project**

A standalone bare-metal Amiga/AGA emulation system for Raspberry Pi 4B, integrating Emu68 and AGA-PiStorm chipset emulation with a Pi4-specific VL805/xHCI USB host. No physical Amiga motherboard or PiStorm adapter is required.

**Current release: Pi4 R101 Experimental Preview.** This repository is developed independently from the Raspberry Pi 3 version because the hardware, USB controller, startup path and optimisations differ.

## What is included

- AArch64 big-endian Emu68 CPU execution with virtual Amiga chipset integration.
- Native Raspberry Pi 4 VL805/xHCI USB HID support for keyboard, mouse and controllers.
- Original EMU68AGA BARE METAL circuit-board splash with compact boot-only POST showing live USB enumeration counts.
- Quiet release configuration, with persistent debug overlays and logging disabled.
- AGA and RTG/P96 integration.

## Build and installation

See [RELEASE_README.md](RELEASE_README.md) for the complete cross-compile command, required toolchain and installation instructions.

**Important:** This is an experimental Pi4-only build, not a Raspberry Pi 3 or Pi 5 kernel. Cascaded USB hubs, including Huasheng 214b:7260, are not yet reliable. Use direct USB connections for initial testing. R101's new POST has not been independently verified on a recorded cold boot.

The Pi4 boot configuration requires a compatible initramfs/ROM bundle already present on the SD card. No proprietary Amiga Kickstart ROMs, AmigaOS media or commercial games are included.

## Source licensing and credits

AGA-PiStorm chipset code is GPL-2.0-only; Emu68 is MPL-2.0. UAE/E-UAE/WinUAE copyright notices and upstream contributions remain attributed in the source. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), [machine/LICENSE](machine/LICENSE) and [machine/Emu68/LICENSE](machine/Emu68/LICENSE).

## Other platforms

The separate Raspberry Pi 3 project remains at [Future Retro Fusion / EMU68AGA-Bare-Metal](https://github.com/futureretrofusion/EMU68AGA-Bare-Metal). Pi4 development, issues and releases should be tracked in this repository.

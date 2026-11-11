EMU68AGA PI4B R100 Quiet Production / no debug displays or logging
Built from independent R99 tree; R96 proven USB gamepad + R98 USB hub
retries + R99 epoch-alternating HID TRB recovery all retained.

- FRF_RELEASE_QUIET=ON: suppress ALL kprintf/vkprintf/__printf_chk
  UART formatting, AGA ring log writes and boot progress squares.
- FRF_PI4_USB_HUD deliberately NOT defined.
- No Pi3-style on-screen USB POST, hex counters, or diagnostic overlay:
  aga_video_overlay_allow and enable force OFF even if boot config asks.
- R99 2-second root hub GET_STATUS debug monitoring disabled.
- AGA_DIAG and AGA_PROBES=OFF, NDEBUG=ON in CMake.
- Keeps real Amiga/HDMI output, audio, keyboard/mouse, Switch Pro,
  LSHIFT+CTRL+F11 ARM reboot, xHCI HID and hub code compiled in.
- Hub cascade still unreliable on real hardware; not claiming fix.
- Writes no new USB debug logs to FAT32; existing old files not deleted.
- No ROM, Amiga partition, or boot configuration changes until an
  explicit SD installation step is verified.
To re-enable debug on this branch build with FRF_RELEASE_QUIET=OFF,
FRF_PI4_USB_HUD=1 and optionally AGA_DIAG/AGA_PROBES=ON.

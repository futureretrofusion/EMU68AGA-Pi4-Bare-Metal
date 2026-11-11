R101 Original EMU68AGA BARE METAL Splash + small Boot POST
Keep R100 pristine (RGB565/RLE 480x270, 192 colors, exactly same header).
R101 overlays a separate compact 320x202 black/cyan Pi3-style POST
on the old firmware RGB565 splash framebuffer at boot only. Native
Topaz 8x16 text. Real status from CPU1 USB: ONLINE, keyboard/mouse,
gamepad/HID, number external hubs, current boot stage. Does NOT enable
the permanent on-screen AGA overlay/hex panel, UART logging, AGA trace,
or R99 periodic hub monitor. Does not modify active Amiga framebuffer.
Only updates when frf_boot_stage or USB enumerator reports status.
No fake success claims; USB hub instability remains known issue.
Compile -DFRF_RELEASE_QUIET=ON -DFRF_BOOT_POST=ON.
R101 not hardware tested until installed/booted on Pi4.

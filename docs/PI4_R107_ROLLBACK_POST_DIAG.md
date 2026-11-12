# Pi4 R107 — known-working R104 USB host + detailed non-invasive POST

**R107 is based directly on Pi4 R104 commit 7e835be.** It does not contain the R105/R106 USB initialization, xHCI Supported Protocol root filtering or idle-worker rescan patches. The entire USB host/HID source remains byte-for-byte identical to R104 (port selection, xHCI reset, HID polling, wireless composite HID and joystick handling unchanged).

## Why

R105/R106 failed physical USB boot tests on the user's Pi4, including a total absence of detected keyboard/mouse/gamepad. Earlier R104/R102 USB composite path had working wireless USB mouse. The Pi4B's physical blue USB3 sockets do not reconfigure the controller's software-visible port topology; modifying connectors could affect downstream electrical signals but is not the expected cause of complete USB2 HID failure.

## R107 changes (diagnostics only)

- POST header `FRF PI4 BARE METAL: R107` / `AGA-STORM PORT: v1.121`.
- Report `USB HOST: HID READY` only if actual input HID counts are nonzero; `NO HID` if none; `HOST ERROR` on nonzero host error.
- Show `ROOT` number, `XHCI ERR`, `HOST PHASE`, `PORTS`, raw first root `PORT1`, raw selected root PORTSC `SELECTED`, and full `SDLOG` stage word.
- Increase POST area height without changing the framebuffer initialization, xHCI register access, USB input data processing or native AGA scanline pipeline.
- Keep v1.121 AGABOOT SCANPCT controls from R104.
- The on-screen POST is transient; capture with phone video or a photo before AGA takes over.

## Safe test matrix

1. Keep R104 known working kernel backed up.
2. Boot R107 with one keyboard in black USB2 port; record the whole POST.
3. If USB fails: capture ROOT, XHCI ERR, HOST PHASE, PORT1, SELECTED, SDLOG as well as input counts.
4. If USB works, recheck wireless combo receiver and another black or blue USB-A socket.
5. This is not yet tested on physical Pi4; do not distribute until confirmed.

Caveat: R107 is **not a new attempt to rewrite USB**. It is an R104-based diagnostic/rollback build to isolate the unexpected USB regression.
